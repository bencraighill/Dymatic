#include "PopupsAndNotifications.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <glm/gtc/type_ptr.hpp>
#include "Dymatic/Math/Math.h"

#include "Settings/Preferences.h"

#include "Dymatic/Utils/PlatformUtils.h"

#include <iomanip>
#include <ctime>
#include <sstream>

#include "TextSymbols.h"

namespace Dymatic {

	static std::vector<Popup> s_PopupList;
	static std::vector<Notification> s_NotificationList;
	static Ref<Texture2D> s_DymaticLogo;
	static Ref<Texture2D> s_NotificationIcon;

	void Popup::Create(const std::string& title, const std::string& message, std::vector<ButtonData> buttons, Ref<Texture2D> icon, bool loading, std::function<void()> onRender, glm::vec2 onRenderSize)
	{
		s_PopupList.emplace_back(title, message, buttons, icon, loading, onRender, onRenderSize);
	}

	Popup::Popup(const std::string& title, const std::string& message, const std::vector<ButtonData>& buttons, Ref<Texture2D> icon, bool loading, std::function<void()> onRender, glm::vec2 onRenderSize)
		: Title(title), Message(message), Buttons(buttons), Icon(icon), Loading(loading), OnRender(onRender), OnRenderSize(onRenderSize) 
	{
		if (loading)
			Taskbar::SetLoading(true);
	}

	void Popup::RemoveTopmostPopup()
	{
		if (!s_PopupList.empty())
		{
			s_PopupList.erase(s_PopupList.begin());
			Taskbar::SetLoading(false);
		}
	}

	void Popup::OnImGuiRender(Timestep ts)
	{
		if (s_PopupList.empty())
			return;

		auto& style = ImGui::GetStyle();

		auto& popup = s_PopupList.front();

		auto id = (popup.Title + "##" + std::to_string(popup.ID));

		const ImVec2 window_pos = ImGui::GetWindowPos();
		const ImVec2 window_size = ImGui::GetWindowSize();

		const float barThickness = 10.0f;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(25.0f, 25.0f));
		ImGui::OpenPopup(id.c_str());
		if (ImGui::BeginPopupModal(id.c_str(), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			const float min_width = 300.0f;
			const float min_button_width = 50.0f;

			// Constants for this popup
			static float button_width = 0.0f;
			static float width = 0.0f;
			static float image_width = 0.0f;

			if (ImGui::IsWindowAppearing())
			{
				button_width = 0.0f;

				for (auto& button : popup.Buttons)
					if (ImGui::CalcTextSize(button.Name.c_str()).x > button_width)
						button_width = ImGui::CalcTextSize(button.Name.c_str()).x;
				button_width += style.FramePadding.x * 2.0f;
				button_width = std::max(button_width, min_button_width);
				width = style.WindowPadding.x * 2.0f + std::max(button_width * popup.Buttons.size(), popup.OnRenderSize.x);
				width = std::max(width, min_width);

				{
					std::string buf;
					for (auto& character : popup.Message)
						if (character == '\n')
						{
							if (ImGui::CalcTextSize(buf.c_str()).x + style.FramePadding.x * 2.0f > width)
								width = ImGui::CalcTextSize(buf.c_str()).x + style.FramePadding.x * 2.0f + style.WindowPadding.x * 2.0f;
							buf.clear();
						}
						else
							buf += character;
					if (ImGui::CalcTextSize(buf.c_str()).x + style.FramePadding.x * 2.0f > width)
						width = ImGui::CalcTextSize(buf.c_str()).x + style.FramePadding.x * 2.0f + style.WindowPadding.x * 2.0f;
				}

				button_width = (width - style.WindowPadding.x * 2.0f) / (float)popup.Buttons.size() - style.FramePadding.x * 2.0f;

				const float message_height = ImGui::CalcTextSize(popup.Message.c_str(), nullptr, false, width).y;
				const float height = popup.OnRenderSize.y + style.WindowPadding.y * 2.0f + message_height + style.FramePadding.y * 2.0f + 55.0f + (popup.Loading ? (barThickness + 2.0f * style.FramePadding.y + 10.0f) : 0.0f);
				
				if (popup.Icon)
					image_width = std::min(height - (ImGui::GetTextLineHeight() + style.FramePadding.y * 2.0f) - style.WindowPadding.y * 2.0f, (float)popup.Icon->GetHeight()) * ((float)popup.Icon->GetWidth() / (float)popup.Icon->GetHeight());
				else
					image_width = 0.0f;

				const ImVec2 size = { width + image_width,  height};

				ImGui::SetWindowSize(size);
				ImGui::SetWindowPos(window_pos + (window_size - size) * 0.5f);
			}

			if (popup.Icon)
			{
				ImGui::Image((ImTextureID)popup.Icon->GetRendererID(), ImVec2(image_width, image_width / ((float)popup.Icon->GetWidth() / (float)popup.Icon->GetHeight())), { 0, 1 }, { 1, 0 });
				ImGui::SameLine();
			}

			ImGui::BeginGroup();
			ImGui::PushTextWrapPos(ImGui::GetWindowContentRegionMax().x);
			ImGui::TextWrapped(popup.Message.c_str());
			ImGui::PopTextWrapPos();
			
			if (popup.OnRender)
				popup.OnRender();

			if (popup.Loading)
			{
				ImGui::Dummy(ImVec2{ 0, 10 });

				const float padding = 10.0f;
				const float size = 100.0f;

				float time = (ImGui::GetTime() - std::floor(ImGui::GetTime()));
				ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(ImGui::GetCursorScreenPos().x + padding - 1.0f, ImGui::GetCursorScreenPos().y - 1.0f), ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvailWidth() - padding + 1.0f, ImGui::GetCursorScreenPos().y + barThickness + 1.0f), ImGui::GetColorU32(ImGuiCol_ProgressBarBg));
				ImGui::GetWindowDrawList()->AddRect(ImVec2(ImGui::GetCursorScreenPos().x + padding - 1.0f, ImGui::GetCursorScreenPos().y - 1.0f), ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvailWidth() - padding + 1.0f, ImGui::GetCursorScreenPos().y + barThickness + 1.0f), ImGui::GetColorU32(ImGuiCol_ProgressBarBorder));

				double minX = ImGui::GetCursorScreenPos().x - size + padding + (ImGui::GetContentRegionAvailWidth() + size) * (time);
				double maxX = ImGui::GetCursorScreenPos().x - size - padding + (ImGui::GetContentRegionAvailWidth() + size) * (time)+size;

				if (minX > (ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvailWidth() - padding)) { minX = (ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvailWidth() - padding); }
				if (maxX > (ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvailWidth() - padding)) { maxX = (ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvailWidth() - padding); }

				if (minX < (ImGui::GetCursorScreenPos().x + padding)) { minX = (ImGui::GetCursorScreenPos().x + padding); }
				if (maxX < (ImGui::GetCursorScreenPos().x + padding)) { maxX = (ImGui::GetCursorScreenPos().x + padding); }

				ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(minX, ImGui::GetCursorScreenPos().y), ImVec2(maxX, ImGui::GetCursorScreenPos().y + barThickness), ImGui::GetColorU32(ImGuiCol_ProgressBarFill));

				ImGui::Dummy(ImVec2(0, barThickness));
			}

			ImGui::Dummy(ImVec2{ 0, 10 });

			size_t index = 0;
			ButtonData* clickedButton = nullptr;
			for (auto& button : popup.Buttons)
			{
				ImGui::PushID(index);
				if (ImGui::Button(button.Name.c_str(), ImVec2(button_width, 0)))
					clickedButton = &button;
				ImGui::SameLine();
				ImGui::PopID();

				index++;
			}

			if (clickedButton)
			{
				if (clickedButton->OnPressedFunction)
					clickedButton->OnPressedFunction();
				RemoveTopmostPopup();
			}
				
			
			ImGui::EndGroup();

			ImGui::EndPopup();
		}
		ImGui::PopStyleVar(3);
	}

	void Notification::Init()
	{
		s_DymaticLogo = Texture2D::Create("Resources/Icons/Branding/DymaticLogo.png");
		s_NotificationIcon = Texture2D::Create("Resources/Icons/General/NotificationIcon.png");
	}

	void Notification::Create(const std::string& title, const std::string& message, const std::vector<ButtonData>& buttons, float displayTime, bool loading)
	{
		s_NotificationList.emplace_back(title, message, buttons,  displayTime, loading);
		Taskbar::SetNotificationIcon(s_NotificationIcon, glm::vec3(0.32f, 0.66f, 0.89f));
	}

	Notification::Notification(const std::string& title, const std::string& message, const std::vector<ButtonData>& buttons, float displayTime, bool loading)
		: Title(title), Message(message), Buttons(buttons), DisplayTime(displayTime), Loading(loading)
	{
		// Get Time String
		auto t = std::time(nullptr);
		auto tm = *std::localtime(&t);

		std::ostringstream oss;
		oss << std::put_time(&tm, "%H-%M-%S");
		Timestamp = oss.str();

		Time = ImGui::GetTime();
	}

	void Notification::Clear()
	{
		for (auto& notification : s_NotificationList)
			notification._fadeOut = true;
		Taskbar::SetNotificationIcon(nullptr);
	}

	void Notification::OnImGuiRender(Timestep ts)
	{
		if (s_NotificationList.empty())
			return;

		bool visible = !ImGui::GetTopMostPopupModal();
		auto& style = ImGui::GetStyle();

		for (int i = s_NotificationList.size() - 1; i >= 0; i--)
		{
			auto& notification = s_NotificationList[i];

			const auto titleSize = ImGui::CalcTextSize(notification.Title.c_str());
			const auto messageSize = ImGui::CalcTextSize(notification.Message.c_str());
			float buttonSize = 100.0f;
			for (auto button : notification.Buttons)
				buttonSize += ImGui::CalcTextSize(button.Name.c_str()).x + style.FramePadding.x * 2.0f;
			auto maxTextWidth = (titleSize.x > messageSize.x) ? (titleSize.x) : (messageSize.x); if (buttonSize > maxTextWidth) { maxTextWidth = buttonSize; }
			auto WindowPosMax = ImVec2(ImGui::GetWindowPos().x + ImGui::GetContentRegionMax().x - 10.0f, ImGui::GetWindowPos().y + ImGui::GetContentRegionMax().y - 10.0f);
			auto yMin = ImGui::GetWindowPos().y + ImGui::GetContentRegionMax().y - messageSize.y - titleSize.y * 2.0f - 20.0f - (notification.Buttons.size() > 0 ? 40.0f : 0.0f);
			yMin = (WindowPosMax.y - std::max(WindowPosMax.y - yMin, 100.0f));
			auto WindowPosMin = ImVec2(ImGui::GetWindowPos().x + ImGui::GetContentRegionMax().x - 120.0f - maxTextWidth, yMin);

			notification._height = WindowPosMax.y - WindowPosMin.y;
			for (size_t n = i + 1; n < s_NotificationList.size(); n++)
				if (s_NotificationList[n].ID != notification.ID)
				{
					WindowPosMin = ImVec2(WindowPosMin.x, WindowPosMin.y - s_NotificationList[n]._height - 10.0f);
					WindowPosMax = ImVec2(WindowPosMax.x, WindowPosMax.y - s_NotificationList[n]._height - 10.0f);
				}

			//Lerping Between Positions
			s_NotificationList[i]._offsetMin = glm::lerp(s_NotificationList[i]._offsetMin, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y - WindowPosMin.y, 6.0f * ts);
			s_NotificationList[i]._offsetMax = s_NotificationList[i]._offsetMin - (WindowPosMax.y - WindowPosMin.y);

			WindowPosMin = ImVec2(WindowPosMin.x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y - s_NotificationList[i]._offsetMin);
			WindowPosMax = ImVec2(WindowPosMax.x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y - s_NotificationList[i]._offsetMax);

			//Setting Colors
			auto& OverallOpacity = notification._currentOpacity;
			if (notification._fadeIn) { OverallOpacity += /*0.0375*/ (2.25f * ts); if (OverallOpacity >= 1.0f) { notification._fadeIn = false; } }
			else if (notification._fadeOut) { OverallOpacity -= (2.25f * ts); }
			else
				OverallOpacity = glm::lerp(OverallOpacity, ((i > ((int)s_NotificationList.size()) - 6) ? 1.0f : 0.0f), (9.0f * ts));
			auto textCol = ImGui::GetStyleColorVec4(ImGuiCol_Text);
			auto popupBgCol = ImGui::GetStyleColorVec4(ImGuiCol_PopupBg);
			auto borderCol = ImGui::GetStyleColorVec4(ImGuiCol_Border);

			textCol.w *= OverallOpacity;
			popupBgCol.w *= OverallOpacity;
			borderCol.w *= OverallOpacity;

			auto popupOpacity = 1.0f - ImGui::GetStyleColorVec4(ImGuiCol_ModalWindowDimBg).w;
			if (!visible)
			{
				textCol.w *= popupOpacity;
				popupBgCol.w *= popupOpacity;
				borderCol.w *= popupOpacity;
			}

			//Drawing Elements
			{
				ImGui::GetForegroundDrawList()->AddRectFilled(WindowPosMin, WindowPosMax, ImGui::ColorConvertFloat4ToU32(popupBgCol), 5.0f);
				ImGui::GetForegroundDrawList()->AddRect(WindowPosMin, WindowPosMax, ImGui::ColorConvertFloat4ToU32(borderCol), 5.0f, 15, 3.0f);
				auto imageAndCirclePos = ImVec2((WindowPosMin.x + 50.0f), ((WindowPosMin.y + 30.0f) + (WindowPosMax.y - 30.0f)) / 2.0f);
				float imageWidth = 30.0f;
				ImGui::GetForegroundDrawList()->AddImage(reinterpret_cast<void*>(s_DymaticLogo->GetRendererID()), ImVec2(imageAndCirclePos.x - imageWidth, imageAndCirclePos.y - imageWidth), ImVec2(imageAndCirclePos.x + imageWidth, imageAndCirclePos.y + imageWidth), ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, ImGui::ColorConvertFloat4ToU32(textCol));
				float circleRadius = 40.0f * (((std::sin(ImGui::GetTime() * 5.0f) + 1.0f) / 2.0f) * 0.25f + 0.8f);
				ImGui::GetForegroundDrawList()->AddCircle(imageAndCirclePos, circleRadius, ImGui::ColorConvertFloat4ToU32(textCol), (int)circleRadius - 1);

				auto textPos = ImVec2(WindowPosMin.x + 100.0f, WindowPosMin.y + 10.0f);
				ImGui::GetForegroundDrawList()->AddText(ImGui::GetIO().Fonts->Fonts[0], 18, textPos, ImGui::ColorConvertFloat4ToU32(textCol), notification.Title.c_str());
				ImGui::GetForegroundDrawList()->AddText(ImVec2(textPos.x, textPos.y + 22.0f), ImGui::ColorConvertFloat4ToU32(textCol), notification.Message.c_str());

				ImGuiContext& g = *GImGui;
				bool windowHovered = true;
				// Filter by viewport
				if (ImGui::GetCurrentWindow()->Viewport != g.MouseViewport)
					if (g.MovingWindow == NULL || ImGui::GetCurrentWindow()->RootWindow != g.MovingWindow->RootWindow)
						windowHovered = false;

				float currentXPos = 100.0f;
				for (auto& button : notification.Buttons)
				{
					float textLength = ImGui::CalcTextSize(button.Name.c_str()).x + style.FramePadding.x * 2.0f;

					const ImRect bb = ImRect(ImVec2(WindowPosMin.x + currentXPos, WindowPosMax.y - 35.0f), ImVec2(WindowPosMin.x + currentXPos + textLength, WindowPosMax.y - 15.0f));
					currentXPos += textLength + 5.0f;

					auto offsetMousePos = ImGui::GetMousePos();
					bool hovered = windowHovered && ImGui::GetWindowViewport() && (offsetMousePos.x > bb.Min.x && offsetMousePos.y > bb.Min.y && offsetMousePos.x < bb.Max.x&& offsetMousePos.y < bb.Max.y);
					bool held = hovered && ImGui::IsMouseClicked(0);
					bool pressed = hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left);

					ImVec4 buttonColor = ImGui::GetStyleColorVec4((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
					buttonColor.w *= OverallOpacity;
					if (!visible) { buttonColor.w *= popupOpacity; }

					if (hovered)
						ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

					ImGui::GetForegroundDrawList()->AddRectFilled(bb.Min, bb.Max, ImGui::ColorConvertFloat4ToU32(buttonColor), 3.0f);
					ImGui::GetForegroundDrawList()->AddText(ImVec2(bb.Min.x + 2.5f, bb.Min.y), ImGui::ColorConvertFloat4ToU32(textCol), button.Name.c_str());
					if (pressed && visible)
					{
						notification._fadeOut = true;
						button.OnPressedFunction();
					}
				}
			}
		}

		size_t index = 0;
		for (auto& notification : s_NotificationList)
		{
			if (ImGui::GetTime() - notification.Time > notification.DisplayTime && notification.DisplayTime != 0.0f)
				notification._fadeOut = true;

			if (notification._fadeOut && notification._currentOpacity <= 0.1f)
			{
				s_NotificationList.erase(s_NotificationList.begin() + index);
				if (s_NotificationList.empty())
					Taskbar::SetNotificationIcon(nullptr);
			}
			else
				index++;
		}
	}

	void NotificationsPannel::OnImGuiRender(Timestep ts)
	{
		auto& notificationsVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Notifications);
		if (!notificationsVisible)
			return;

		auto& style = ImGui::GetStyle();

		ImGui::Begin(CHARACTER_ICON_NOTIFICATIONS " Notifications", &notificationsVisible);
		ImGui::Text("Number: %d", s_NotificationList.size());

		ImGui::SameLine(ImGui::GetContentRegionAvailWidth() - ImGui::CalcTextSize("Clear").x - style.FramePadding.x * 2.0f);
		if (ImGui::Button("Clear"))
			Notification::Clear();

		if (!s_NotificationList.empty())
		{
			for (int i = s_NotificationList.size() - 1; i >= 0; i--)
			{
				if (!s_NotificationList[i]._fadeOut)
				{
					auto& notification = s_NotificationList[i];

					ImGui::PushID(notification.ID);
					const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
					ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
					float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
					ImGui::Spacing();
					bool open = ImGui::TreeNodeEx("##NotificationTreeNode", treeNodeFlags, (notification.Timestamp + "  -  " + notification.Title).c_str());
					ImGui::PopStyleVar();

					ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
					if (ImGui::Button("X", ImVec2{ lineHeight, lineHeight }))
						notification._fadeOut = true;
					if (open)
					{
						ImGui::Dummy(ImVec2{ 0, 5 });
						ImGui::Text(notification.Message.c_str());
						ImGui::Dummy(ImVec2{ 0, 2 });
						size_t index = 0;
						for (auto& button : notification.Buttons)
						{
							ImGui::PushID(index);
							ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
							if (ImGui::Button(button.Name.c_str()))
							{
								notification._fadeOut = true;
								button.OnPressedFunction();
							}
							ImGui::SameLine();
							ImGui::PopStyleVar();
							ImGui::PopID();

							index++;
						}
						ImGui::Dummy(ImVec2{ 0, 5 });
						ImGui::TreePop();
					}
					ImGui::PopID();
				}
			}
		}
		ImGui::End();
	}
}