#include "PopupsAndNotifications.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <glm/gtc/type_ptr.hpp>
#include "Dymatic/Math/Math.h"

#include "../Preferences.h"

#include <string>
#include <sstream>
#include <ctime>

#include "../TextSymbols.h"

namespace Dymatic {

	PopupsAndNotifications::PopupsAndNotifications(Preferences* preferencesRef)
	{
		m_PreferencesReference = preferencesRef;
	}

	void PopupsAndNotifications::Popup(std::string title, std::string message, std::vector<ButtonData> buttons, bool loading)
	{
		PopupData data;
		data.title = title;
		data.message = message;
		if (data.message[data.message.size() - 1] != '\n') { data.message += '\n'; }
		data.buttons = buttons;

		data.id = GetNextNotificationId();
		data.loading = loading;

		m_PopupList.push_back(data);
	}

	void PopupsAndNotifications::RemoveTopmostPopup()
	{
		if (m_PopupList.size() > 0)
		{
			m_PopupList.erase(m_PopupList.begin());
		}
	}

	void PopupsAndNotifications::PopupUpdate()
	{
		//Popup Update Code

		if (!m_PopupList.empty())
		{
			auto data = m_PopupList[0];
			m_PopupOpen = true;
			m_PreviousPopupOpen = true;
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
			ImGui::OpenPopup(data.title.c_str());
			if (ImGui::BeginPopupModal(data.title.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				float totalWidth = 0;
				float additionalPadding = 20.0f;
				for (int i = 0; i < data.buttons.size(); i++)
				{
					totalWidth += ImGui::CalcTextSize(data.buttons[i].name.c_str()).x + additionalPadding;
				}
				float totalTextWidth = ImGui::CalcTextSize(data.message.c_str()).x;

				float popupWidth = 400;
				float popupHeight = 134.5;

				if (totalTextWidth > totalWidth && totalTextWidth > popupWidth)
					popupWidth = totalTextWidth;

				if (totalWidth > totalTextWidth && totalWidth > popupWidth)
					popupWidth = totalWidth;

				if (m_CurrentTitle != data.title)
				{
					m_CurrentTitle = data.title;
					ImGui::SetWindowPos(ImVec2((((io.DisplaySize.x * 0.5f) - (popupWidth / 2)) + dockspaceWindowPosition.x), (((io.DisplaySize.y * 0.5f) - (popupHeight / 2)) + dockspaceWindowPosition.y)));
				}

				ImGui::Dummy(ImVec2{ popupWidth, 10 });

				size_t numberOfLines = std::count(data.message.begin(), data.message.end(), '\n');

				std::stringstream test(data.message);
				std::string segment;
				std::vector<std::string> seglist;

				while (std::getline(test, segment, '\n'))
				{
					seglist.push_back(segment);
				}

				for (int i = 0; i < numberOfLines; i++)
				{
					ImGui::Spacing();
					const char* messageText = seglist[i].c_str();
					ImGui::SameLine((popupWidth / 2) - ImGui::CalcTextSize(messageText).x / 2);
					ImGui::Text(messageText);
				}

				ImGui::Dummy(ImVec2{ 0, 10 });

				if (data.loading)
				{
					static float barThickness = 10.0f;
					static float padding = 25.0f;
					static float size = 100.0f;
				
					double time = (ImGui::GetTime() - std::floor(ImGui::GetTime()));
					ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(ImGui::GetWindowPos().x + padding, ImGui::GetCursorScreenPos().y), ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - padding, ImGui::GetCursorScreenPos().y + barThickness), ImGui::GetColorU32(ImGuiCol_ProgressBarBg));
					ImGui::GetWindowDrawList()->AddRect(ImVec2(ImGui::GetWindowPos().x + padding, ImGui::GetCursorScreenPos().y), ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - padding, ImGui::GetCursorScreenPos().y + barThickness), ImGui::GetColorU32(ImGuiCol_ProgressBarBorder));
					
					double minX = ImGui::GetWindowPos().x - size + padding + (ImGui::GetWindowSize().x + size) * (time);
					double maxX = ImGui::GetWindowPos().x - size - padding + (ImGui::GetWindowSize().x + size) * (time) + size;

					if (minX > (ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - padding)) { minX = (ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - padding); }
					if (maxX > (ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - padding)) { maxX = (ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - padding); }

					if (minX < (ImGui::GetWindowPos().x + padding)) { minX = (ImGui::GetWindowPos().x + padding); }
					if (maxX < (ImGui::GetWindowPos().x + padding)) { maxX = (ImGui::GetWindowPos().x + padding); }

					ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(minX, ImGui::GetCursorScreenPos().y), ImVec2(maxX, ImGui::GetCursorScreenPos().y + barThickness), ImGui::GetColorU32(ImGuiCol_ProgressBarFill));
					
					ImGui::Dummy(ImVec2(0, barThickness));
				}

				ImGui::Dummy(ImVec2{ 0, 10 });

				ImGui::Spacing();
				ImVec2 size{ 70, 23 };
				ImGui::SameLine((popupWidth / 2) - ((totalWidth) / 2));

				for (int i = 0; i < data.buttons.size(); i++)
				{
					ImGui::PushID(data.buttons[i].id);
					if (ImGui::Button(data.buttons[i].name.c_str(), ImVec2(ImGui::CalcTextSize(data.buttons[i].name.c_str()).x + additionalPadding, size.y)))
					{
						m_PopupList.erase(m_PopupList.begin());
						data.buttons[i].func();
						m_CurrentTitle = "";
					}
					ImGui::SameLine();
					ImGui::PopID();
				}
				ImGui::EndPopup();
				ImGui::PopStyleVar(2);
			}
		}
	}

	void PopupsAndNotifications::Notification(int notificationIndex, std::string title, std::string message, std::vector<ButtonData> buttons, bool loading, float displayTime)
	{
		if (m_PreferencesReference->m_PreferenceData.NotificationEnabled[notificationIndex])
		{
			NotificationData data;
			data.title = title;
			data.message = message;
			data.buttons = buttons;
			data.loading = loading;

			data.id = GetNextNotificationId();
			data.time = m_ProgramTime;
			data.displayTime = displayTime;

			//Calculate Time Executed
			time_t tt;
			time(&tt);
			tm TM = *localtime(&tt);

			int Hours = TM.tm_hour + 0;
			int Minutes = TM.tm_min + 0;
			int Seconds = TM.tm_sec + 0;

			std::string Hours_S = std::to_string(Hours);
			Hours_S = Hours_S.length() == 1 ? "0" + Hours_S : Hours_S;
			std::string Minutes_S = std::to_string(Minutes);
			Minutes_S = Minutes_S.length() == 1 ? "0" + Minutes_S : Minutes_S;
			std::string Seconds_S = std::to_string(Seconds);
			Seconds_S = Seconds_S.length() == 1 ? "0" + Seconds_S : Seconds_S;

			data.executedTime = ("{ " + Hours_S + " : " + Minutes_S + " : " + Seconds_S + " }");

			m_NotificationList.push_back(data);
		}
	}

	void PopupsAndNotifications::NotificationUpdate(Timestep ts, float programTime)
	{
		// Show Notification Popups
		bool visible = !ImGui::GetTopMostPopupModal();
		{
			for (int i = (int)m_NotificationList.size() - 1; i >= 0; i--)
			{
				auto titleSize = ImGui::CalcTextSize(m_NotificationList[i].title.c_str());
				auto messageSize = ImGui::CalcTextSize(m_NotificationList[i].message.c_str());
				auto buttonSize = 100.0f; for (auto button : m_NotificationList[i].buttons) { buttonSize += ImGui::CalcTextSize(button.name.c_str()).x + 5.0f; }
				auto maxTextWidth = (titleSize.x > messageSize.x) ? (titleSize.x) : (messageSize.x); if (buttonSize > maxTextWidth) { maxTextWidth = buttonSize; }
				auto WindowPosMax = ImVec2(ImGui::GetWindowPos().x + ImGui::GetContentRegionMax().x - 10.0f, ImGui::GetWindowPos().y + ImGui::GetContentRegionMax().y - 10);
				auto yMin = ImGui::GetWindowPos().y + ImGui::GetContentRegionMax().y - messageSize.y - titleSize.y * 2 - 20.0f - (m_NotificationList[i].buttons.size() > 0 ? 40.0f : 0.0f);
				yMin = (WindowPosMax.y - std::max(WindowPosMax.y - yMin, 100.0f));
				auto WindowPosMin = ImVec2(ImGui::GetWindowPos().x + ImGui::GetContentRegionMax().x - 120.0f -  maxTextWidth, yMin);

				m_NotificationList[i].height = WindowPosMax.y - WindowPosMin.y;
				for (int n = i + 1; n < m_NotificationList.size(); n++)
				{
					if (m_NotificationList[n].id != m_NotificationList[i].id) { WindowPosMin = ImVec2(WindowPosMin.x, WindowPosMin.y - m_NotificationList[n].height - 10.0f); WindowPosMax = ImVec2(WindowPosMax.x, WindowPosMax.y - m_NotificationList[n].height - 10.0f); }
				}

				//Lerping Between Positions
				m_NotificationList[i].offsetMin = glm::lerp(m_NotificationList[i].offsetMin, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y - WindowPosMin.y, /*0.1f*/ 6.0f * ts);
				m_NotificationList[i].offsetMax = m_NotificationList[i].offsetMin - (WindowPosMax.y - WindowPosMin.y);
				
				WindowPosMin = ImVec2(WindowPosMin.x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y - m_NotificationList[i].offsetMin);
				WindowPosMax = ImVec2(WindowPosMax.x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y - m_NotificationList[i].offsetMax);

				//Setting Colors
				auto& OverallOpacity = m_NotificationList[i].currentOpacity;
				if (m_NotificationList[i].fadeIn) { OverallOpacity += /*0.0375*/ (2.25f * ts); if (OverallOpacity >= 1.0f) { m_NotificationList[i].fadeIn = false; } }
				else if (m_NotificationList[i].fadeOut) { OverallOpacity -= /*0.0375*/ (2.25f * ts); }
				else { OverallOpacity = glm::lerp(OverallOpacity, ((i > (int)m_NotificationList.size() - 6)? 1.0f : 0.0f), /*0.15f*/(9.0f * ts)); }
				auto textCol = ImGui::GetStyleColorVec4(ImGuiCol_Text);
				auto popupBgCol = ImGui::GetStyleColorVec4(ImGuiCol_PopupBg);
				auto borderCol = ImGui::GetStyleColorVec4(ImGuiCol_Border);
				textCol.w *= OverallOpacity; popupBgCol.w *= OverallOpacity; borderCol.w *= OverallOpacity;
				auto popupOpacity = 1.0f - ImGui::GetStyleColorVec4(ImGuiCol_ModalWindowDimBg).w;
				if (!visible) { textCol.w *= popupOpacity; popupBgCol.w *= popupOpacity; borderCol.w *= popupOpacity; }

				//Drawing Elements
				{
					ImGui::GetForegroundDrawList()->AddRectFilled(WindowPosMin, WindowPosMax, ImGui::ColorConvertFloat4ToU32(popupBgCol), 5.0f);
					ImGui::GetForegroundDrawList()->AddRect(WindowPosMin, WindowPosMax, ImGui::ColorConvertFloat4ToU32(borderCol), 5.0f, 15, 3.0f);
					auto imageAndCirclePos = ImVec2((WindowPosMin.x + 50.0f), ((WindowPosMin.y + 30.0f) + (WindowPosMax.y - 30.0f)) / 2.0f);
					float imageWidth = 30.0f;
					ImGui::GetForegroundDrawList()->AddImage(reinterpret_cast<void*>(m_DymaticLogo->GetRendererID()), ImVec2(imageAndCirclePos.x - imageWidth, imageAndCirclePos.y - imageWidth), ImVec2(imageAndCirclePos.x + imageWidth, imageAndCirclePos.y + imageWidth), ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, ImGui::ColorConvertFloat4ToU32(textCol));
					float circleRadius = 40.0f * (((std::sin(ImGui::GetTime() * 5.0f) + 1.0f) / 2.0f) * 0.25f + 0.8f);
					ImGui::GetForegroundDrawList()->AddCircle(imageAndCirclePos, circleRadius, ImGui::ColorConvertFloat4ToU32(textCol), (int)circleRadius - 1);

					auto textPos = ImVec2(WindowPosMin.x + 100.0f, WindowPosMin.y + 10.0f);
					ImGui::GetForegroundDrawList()->AddText(ImGui::GetIO().Fonts->Fonts[0], 18, textPos, ImGui::ColorConvertFloat4ToU32(textCol), m_NotificationList[i].title.c_str());
					ImGui::GetForegroundDrawList()->AddText(ImVec2(textPos.x, textPos.y + 22.0f), ImGui::ColorConvertFloat4ToU32(textCol), m_NotificationList[i].message.c_str());

					
					ImGuiContext& g = *GImGui;
					bool windowHovered = true;
					// Filter by viewport
					if (ImGui::GetCurrentWindow()->Viewport != g.MouseViewport)
						if (g.MovingWindow == NULL || ImGui::GetCurrentWindow()->RootWindow != g.MovingWindow->RootWindow)
							windowHovered = false;

					float currentXPos = 100.0f;
					for (int b = 0; b < m_NotificationList[i].buttons.size(); b++)
					{
						auto& button = m_NotificationList[i].buttons[b];

						float textLength = ImGui::CalcTextSize(button.name.c_str()).x + 5.0f;

						const ImRect bb = ImRect(ImVec2(WindowPosMin.x + currentXPos, WindowPosMax.y - 35.0f), ImVec2(WindowPosMin.x + currentXPos + textLength, WindowPosMax.y - 15.0f));
						currentXPos += textLength + 5.0f;

						auto offsetMousePos = ImGui::GetMousePos();
						bool hovered = windowHovered && ImGui::GetWindowViewport() && (offsetMousePos.x > bb.Min.x && offsetMousePos.y > bb.Min.y && offsetMousePos.x < bb.Max.x&& offsetMousePos.y < bb.Max.y);
						bool held = hovered && Input::IsMouseButtonPressed(Mouse::ButtonLeft);
						bool pressed = hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left);

						ImVec4 buttonColor = ImGui::GetStyleColorVec4((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
						buttonColor.w *= OverallOpacity;
						if (!visible) { buttonColor.w *= popupOpacity; }

						ImGui::GetForegroundDrawList()->AddRectFilled(bb.Min, bb.Max, ImGui::ColorConvertFloat4ToU32(buttonColor), 3.0f);
						ImGui::GetForegroundDrawList()->AddText(ImVec2(bb.Min.x + 2.5f, bb.Min.y), ImGui::ColorConvertFloat4ToU32(textCol), button.name.c_str());
						if (pressed && visible)
						{
							// Button Pressed Code
							m_NotificationList[i].fadeOut = true;
							button.func();
						}
					}
				}
			}
		}

		//Update Popup Stuff
		if (m_PreviousPopupOpen == false)
			m_PopupOpen = false;
		m_PreviousPopupOpen = false;

		io = ImGui::GetIO();
		dockspaceWindowPosition = ImGui::GetWindowPos();
		m_ProgramTime = programTime;

		unsigned int count = 0;
		for (int i = 0; i < m_NotificationList.size(); i++)
		{
			if (((m_ProgramTime - m_NotificationList[i].time) > m_NotificationList[i].displayTime) && (m_NotificationList[i].displayTime != 0))
			{
				m_NotificationList[i].fadeOut = true;
			}
			if (m_NotificationList[i].fadeOut)
			{
				if (m_NotificationList[i].currentOpacity <= 0.1f)
				{
					m_NotificationList.erase(m_NotificationList.begin() + i);
					i--;
				}
			}
			else { count++; }
		}

		if (m_NotificationsVisible)
		{
			ImGui::Begin((std::string(CHARACTER_WINDOW_ICON_NOTIFICATIONS) + " Notifications").c_str(), &m_NotificationsVisible);
			ImGui::Text("Number: %d", count);


			ImGui::SameLine(ImGui::GetContentRegionAvailWidth() - ImGui::CalcTextSize("Clear").x - 5);
			if (ImGui::Button("Clear"))
			{
				ClearNotificationList();
			}

			for (int i = m_NotificationList.size() - 1; i >= 0; i = i - 1)
			{
				if (!m_NotificationList[i].fadeOut)
				{
					ImGui::PushID(m_NotificationList[i].id);
					const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
					ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
					float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
					ImGui::Spacing();
					bool open = ImGui::TreeNodeEx((void*)(m_NotificationList[i].id), treeNodeFlags, (m_NotificationList[i].executedTime + "  -  " + m_NotificationList[i].title).c_str());
					ImGui::PopStyleVar();

					ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
					if (ImGui::Button("X", ImVec2{ lineHeight, lineHeight })/* || removeNotification*/)
					{
						m_NotificationList[i].fadeOut = true;
					}
					if (open)
					{
						ImGui::Dummy(ImVec2{ 0, 5 });
						ImGui::Text(m_NotificationList[i].message.c_str());
						ImGui::Dummy(ImVec2{ 0, 2 });
						for (int b = 0; b < m_NotificationList[i].buttons.size(); b++)
						{
							ImGui::PushID(m_NotificationList[i].buttons[b].id);
							ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
							if (ImGui::Button(m_NotificationList[i].buttons[b].name.c_str()))
							{
								// Button Pressed Code
								m_NotificationList[i].fadeOut = true;
								m_NotificationList[i].buttons[b].func();
							}
							ImGui::SameLine();
							ImGui::PopStyleVar();
							ImGui::PopID();
						}
						ImGui::Dummy(ImVec2{ 0, 5 });
						ImGui::TreePop();
					}
					ImGui::PopID();
				}
			}
			ImGui::End();
		}
	}

	void PopupsAndNotifications::ClearNotificationList()
	{
		for (auto& notification : m_NotificationList) { notification.fadeOut = true; }
	}

}