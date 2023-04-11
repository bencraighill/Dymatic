#include "ImageEditor.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "Dymatic/Math/Math.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>

#include "Settings/Preferences.h"
#include "TextSymbols.h"

#include "blur_uchar_rgb.h"

//Image Exports
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../Dymatic/vendor/stb_image/stb_image_write.h"

namespace Dymatic {


	ImageEditor::ImageEditor()
	{
		//int dataSize = m_DocumentWidth * m_DocumentHeight * 4;
		//unsigned char* data = new unsigned char[dataSize];
		//for (int i = 0; i < dataSize; i++)
		//{
		//	data[i] = 1.0f;
		//}
		//m_Layers[0].Texture->SetData(data, dataSize);

		//unsigned char* dataStroke = new unsigned char[dataSize];
		//int a = 1;
		//for (int i = 0; i < dataSize; i++)
		//{
		//	dataStroke[i] = 0.0f;
		//	if (a == 4) { dataStroke[i] = 255; a = 0; }
		//	a++;
		//}
		//m_StrokeTextureBuffer->SetData(dataStroke, dataSize);

		//delete[] dataStroke;
		//delete[] data;

		auto dataSize = m_DocumentWidth * m_DocumentHeight;
		m_StrokeBuffer = new unsigned char[dataSize];
		for (int i = 0; i < dataSize; i++) { m_StrokeBuffer[i] = 0.0f; }

		auto brushDataSize = 100 * 100 * 4;
		m_BrushData = new unsigned char[brushDataSize];
		m_Brush->GetData(m_BrushData, brushDataSize);
	}

	void ImageEditor::OnEvent(Event& e)
	{
		if (Preferences::GetEditorWindowVisible(Preferences::EditorWindow::ImageEditor))
		{
			EventDispatcher dispatcher(e);

			dispatcher.Dispatch<KeyPressedEvent>(DY_BIND_EVENT_FN(ImageEditor::OnKeyPressed));
			dispatcher.Dispatch<MouseButtonPressedEvent>(DY_BIND_EVENT_FN(ImageEditor::OnMouseButtonPressed));
			dispatcher.Dispatch<MouseButtonReleasedEvent>(DY_BIND_EVENT_FN(ImageEditor::OnMouseButtonReleased));
			dispatcher.Dispatch<MouseScrolledEvent>(DY_BIND_EVENT_FN(ImageEditor::OnMouseScrolled));
		}
	}

	bool ImageEditor::OnKeyPressed(KeyPressedEvent& e)
	{
		return false;
	}

	bool ImageEditor::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		if (Preferences::GetEditorWindowVisible(Preferences::EditorWindow::ImageEditor))
		{
			switch (e.GetMouseButton())
			{
			case (Mouse::ButtonLeft): {
				auto size = m_DocumentWidth * m_DocumentHeight;
				if (m_SelectedLayer != nullptr)
				{
					if (m_LayerBuffer != NULL) {
						delete[] m_LayerBuffer;
					}
					
					m_LayerBuffer = new unsigned char[size * 4];
					m_SelectedLayer->Texture->GetData(m_LayerBuffer, size * 4);

				}
				for (int i = 0; i < size; i++) { m_StrokeBuffer[i] = 0.0f; }
			}
			}
			return false;
		}
	}

	bool ImageEditor::OnMouseButtonReleased(MouseButtonReleasedEvent& e)
	{
		if (Preferences::GetEditorWindowVisible(Preferences::EditorWindow::ImageEditor))
		{
			switch (e.GetMouseButton())
			{
			case (Mouse::ButtonLeft): {
				//auto size = 300 * 300 * 4;
				//unsigned char* strokeData = new unsigned char[size];
				//for (int i = 0; i < size; i++)
				//{
				//	strokeData[i] = 0.0f;
				//}
				//m_StrokeTextureBuffer->SetData(strokeData, size);
				//delete[] strokeData;
			}
			}
			return false;
		}
	}

	bool ImageEditor::OnMouseScrolled(MouseScrolledEvent& e)
	{
		auto delta = e.GetYOffset() * 0.1f;
		//static int step = 50;
		//int maxSteps = 200;
		//
		//step = std::clamp(step + (int)e.GetYOffset(), 0, maxSteps);
		//
		//float minZoom = 0.1f;
		//float maxZoom = 200.0f;
		//float logMinZoom = std::log(minZoom);
		//float logMaxZoom = std::log(maxZoom);
		//
		//float logZoom = logMinZoom + (logMaxZoom - logMinZoom) * step / maxSteps;
		//
		//m_Zoom = std::exp(logZoom);
		//m_Position -= m_MousePosition * e.GetYOffset();//std::powf(m_Zoom, -1);

		m_Zoom += delta;
		m_Position -= m_MousePosition * delta;

		return false;
	}

	void ImageEditor::OnImGuiRender()
	{
		if (auto& imageEditorVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::ImageEditor))
		{
			ImGui::Begin(CHARACTER_ICON_IMAGE " Image Editor", &imageEditorVisible, ImGuiWindowFlags_MenuBar);

			// Menu Bar

			ImGui::BeginMenuBar();
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Quick Export")) { ExportImage("test_file.bmp"); }
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Filter"))
			{
				static float blurAmount = 1.0f;
				ImGui::InputFloat("Blur Amount", &blurAmount, 1.0f, 0.1f, "%.4f");
				if (ImGui::MenuItem("Blur")) {
					if (m_SelectedLayer != NULL)
					{
						auto size = m_DocumentWidth * m_DocumentHeight * 4;
						unsigned char* data = new unsigned char[size];
						unsigned char* outData = new unsigned char[size];
						m_SelectedLayer->Texture->GetData(data, size);
						fast_gaussian_blur_rgb(data, outData, m_DocumentWidth, m_DocumentHeight, 4, blurAmount);
						m_SelectedLayer->Texture->SetData(data, size);
						delete[] data;
						delete[] outData;
					}
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();

			// End Menu Bar

			ImGui::Dummy(ImGui::GetContentRegionAvail());
			// Drag Drop Target
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					const char* path = (const char*)payload->Data;
					auto id = GetNextLayerID();

					m_Layers.push_back({ id, "Image Layer", m_DocumentWidth, m_DocumentHeight });
					m_SelectedLayer = GetLayerById(id);

					auto& loadedTexture = Texture2D::Create(path);
					auto loadedTextureSize = loadedTexture->GetWidth() * loadedTexture->GetHeight() * 4;
					unsigned char* loadedData = new unsigned char[loadedTextureSize];
					loadedTexture->GetData(loadedData, loadedTextureSize);
					
					auto& newTexture = Texture2D::Create(m_DocumentWidth, m_DocumentHeight);
					auto newTextureSize = newTexture->GetWidth() * newTexture->GetHeight() * 4;
					unsigned char* newData = new unsigned char[newTextureSize];

					int yIndex = -((100 / 2) + ((m_DocumentWidth * (100)) / 2));
					for (int y = 0; y < (100 * 100); y++)
					{
						if ((0)+(yIndex) < newTextureSize / 4 && (0)+(yIndex) > 0)
						{
							auto uIndex = (0 * 4) + (yIndex * 4);

							newData[uIndex + 0] = (unsigned int)loadedData[y * 4 + 3];
							newData[uIndex + 1] = (unsigned int)loadedData[y * 4 + 3];
							newData[uIndex + 2] = (unsigned int)loadedData[y * 4 + 3];
							newData[uIndex + 3] = 255.0f;

							yIndex++;
							if (std::fmod(y + 2, 100) == 1)
							{
								yIndex += (m_DocumentWidth - 100);
							}
						}
					}

					newTexture->SetData(newData, newTextureSize);

					m_SelectedLayer->Texture = newTexture;

					delete[] newData;
					delete[] loadedData;
				}
				ImGui::EndDragDropTarget();
			}
			// End Drag Drop Target

			m_MousePosition = glm::vec2(ImGui::GetMousePos().x - ImGui::GetWindowPos().x - (ImGui::GetWindowSize().x / 2) + m_DocumentWidth / 2, ImGui::GetMousePos().y - ImGui::GetWindowPos().y - (ImGui::GetWindowSize().y / 2) + m_DocumentWidth / 2);

			auto drawList = ImGui::GetWindowDrawList();
			m_HoveredPixel = glm::vec2((ImGui::GetMousePos().x - ImGui::GetWindowPos().x - m_Position.x) / m_Zoom, (m_DocumentHeight - (ImGui::GetMousePos().y - ImGui::GetWindowPos().y - m_Position.y) / m_Zoom));

			auto startPos = ImGui::GetWindowPos() + ImVec2(m_Position.x, m_Position.y);
			auto endPos = ImGui::GetWindowPos() + ImVec2(m_DocumentWidth, m_DocumentHeight) * m_Zoom + ImVec2(m_Position.x, m_Position.y);

			drawList->AddImage((ImTextureID)m_CheckerboardTexture->GetRendererID(), startPos, endPos, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
			for (int i = 0; i < m_Layers.size(); i++)
			{
				if (m_Layers[i].Visible)
				{
					auto& texture = m_Layers[i].Texture;
					//auto& texture = m_StrokeTextureBuffer;
					drawList->AddImage((ImTextureID)texture->GetRendererID(), startPos, endPos, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, m_Layers[i].Opacity / 100.0f)));

					auto size = m_DocumentWidth * m_DocumentHeight * 4;
					unsigned char* data = new unsigned char[size];

					texture->GetData(data, size);

					glm::vec2 min, max;
					FindBoundingBox(data, size, &min, &max);
					delete[] data;
					drawList->AddRect(ImGui::GetWindowPos() + ImVec2(min.x, m_DocumentWidth - min.y), ImGui::GetWindowPos() + ImVec2(max.x, m_DocumentWidth - max.y), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.0f, 0.0f, 1.0f)), 0.0f, NULL, 2.0f);
				}
			}

			if ((Input::IsMouseButtonPressed(Mouse::ButtonMiddle) || (Input::IsMouseButtonPressed(Mouse::ButtonLeft) && m_Tool == Hand)) && ImGui::IsWindowHovered())
			{
				Application::Get().GetWindow().SetCursor(3);
				m_Position += Input::GetMousePosition() - m_PreviousMousePos;
			}
			m_PreviousMousePos = Input::GetMousePosition();

			if (Input::IsMouseButtonPressed(Mouse::ButtonLeft) && ImGui::IsWindowHovered())
			{
				if (m_Tool == Move)
				{
					// Select Layer Code

					//bool found = false;
					//for (int i = m_Layers.size() - 1; i >= 0; i--)
					//{
					//	auto size = m_DocumentWidth * m_DocumentHeight * 4;
					//	unsigned char* data = new unsigned char[size];
					//	m_Layers[i].Texture->GetData(data, size);
					//	auto index = ConvertVectorToIndex(m_HoveredPixel, m_DocumentWidth) * 4 + 3;
					//	if (index < size && index > 0)
					//	{
					//		if (data[index] != 0)
					//		{
					//			found = true;
					//			m_SelectedLayer = &m_Layers[i];
					//			delete[] data;
					//			break;
					//		}
					//	}
					//	delete[] data;
					//}
					//if (!found) { m_SelectedLayer = nullptr; }

					//m_SelectedLayer
				}
				else if (m_Tool == Brush)
				{
					if (m_SelectedLayer != nullptr)
					{
						int dataSize = m_DocumentWidth * m_DocumentHeight;
						DrawLine(m_StrokeBuffer, dataSize, m_PreviouslyHoveredPixel, m_HoveredPixel);
						dataSize *= 4;

						// Draws Line To Stroke Buffer

						//Paint Code

						if (m_LayerBuffer != NULL)
						{
							unsigned char* data = new unsigned char[dataSize];
							//unsigned char* strokeData = new unsigned char[dataSize];
							m_SelectedLayer->Texture->GetData(data, dataSize);
							//m_StrokeTextureBuffer->GetData(strokeData, dataSize);
							for (int i = 0; i < dataSize; i += 4)
							{
								data[i + 0] = std::clamp(glm::lerp((float)m_LayerBuffer[i + 0], m_BrushColor.x * 255.0f, ((float)m_StrokeBuffer[i / 4] / 255.0f) * (m_BrushOpacity / 100.0f)), 0.0f, 255.0f);
								data[i + 1] = std::clamp(glm::lerp((float)m_LayerBuffer[i + 1], m_BrushColor.y * 255.0f, ((float)m_StrokeBuffer[i / 4] / 255.0f) * (m_BrushOpacity / 100.0f)), 0.0f, 255.0f);
								data[i + 2] = std::clamp(glm::lerp((float)m_LayerBuffer[i + 2], m_BrushColor.z * 255.0f, ((float)m_StrokeBuffer[i / 4] / 255.0f) * (m_BrushOpacity / 100.0f)), 0.0f, 255.0f);
								data[i + 3] = std::clamp(glm::lerp((float)m_LayerBuffer[i + 3], m_BrushColor.w * 255.0f, ((float)m_StrokeBuffer[i / 4] / 255.0f) * (m_BrushOpacity / 100.0f)), 0.0f, 255.0f);

								//data[i + 0] = std::clamp(glm::lerp((float)m_LayerBuffer[i + 0], m_BrushColor.x * 255.0f, ((float)m_StrokeBuffer[i] / 255.0f) * (m_BrushOpacity / 100.0f)), 0.0f, 255.0f);
								//data[i + 1] = std::clamp(glm::lerp((float)m_LayerBuffer[i + 1], m_BrushColor.y * 255.0f, ((float)m_StrokeBuffer[i] / 255.0f) * (m_BrushOpacity / 100.0f)), 0.0f, 255.0f);
								//data[i + 2] = std::clamp(glm::lerp((float)m_LayerBuffer[i + 2], m_BrushColor.z * 255.0f, ((float)m_StrokeBuffer[i] / 255.0f) * (m_BrushOpacity / 100.0f)), 0.0f, 255.0f);
								//data[i + 3] = std::clamp(glm::lerp((float)m_LayerBuffer[i + 3], m_BrushColor.w * 255.0f, ((float)m_StrokeBuffer[i] / 255.0f) * (m_BrushOpacity / 100.0f)), 0.0f, 255.0f);

								//data[i + 0] = std::clamp(glm::lerp((float)m_LayerBuffer[i + 0], m_BrushColor.x * 255.0f, ((float)(glm::lerp(255.0f, (float)m_StrokeBuffer[i], glm::lerp(m_LayerBuffer[i + 3] / 255.0f, 1.0f, (float)m_StrokeBuffer[i]))) / 255.0f) * (m_BrushOpacity / 100.0f)), 0.0f, 255.0f);
								//data[i + 1] = std::clamp(glm::lerp((float)m_LayerBuffer[i + 1], m_BrushColor.y * 255.0f, ((float)(glm::lerp(255.0f, (float)m_StrokeBuffer[i], glm::lerp(m_LayerBuffer[i + 3] / 255.0f, 1.0f, (float)m_StrokeBuffer[i]))) / 255.0f) * (m_BrushOpacity / 100.0f)), 0.0f, 255.0f);
								//data[i + 2] = std::clamp(glm::lerp((float)m_LayerBuffer[i + 2], m_BrushColor.z * 255.0f, ((float)(glm::lerp(255.0f, (float)m_StrokeBuffer[i], glm::lerp(m_LayerBuffer[i + 3] / 255.0f, 1.0f, (float)m_StrokeBuffer[i]))) / 255.0f) * (m_BrushOpacity / 100.0f)), 0.0f, 255.0f);
								//data[i + 3] = std::clamp(glm::lerp((float)m_LayerBuffer[i + 3], m_BrushColor.w * 255.0f, ((float)m_StrokeBuffer[i] / 255.0f) * (m_BrushOpacity / 100.0f)), 0.0f, 255.0f);
							}

							m_SelectedLayer->Texture->SetData(data, dataSize);

							delete[] data;
							//delete[] strokeData;
						}

						// --- End Paint Code
					}
				}
			}

			m_PreviouslyHoveredPixel = m_HoveredPixel;
			ImGui::End();

			ImGui::Begin("Layers");

			ImGui::BeginChild("##UpperLayerProperties", ImVec2(0, 25.0f));

			if (m_SelectedLayer != NULL)
			{
				ImGui::SetNextItemWidth(200);
				if (ImGui::BeginCombo("##ThemeSelectCombo", "Normal"))
				{
					ImGui::Selectable("Normal");
					ImGui::Selectable("Dissolve");
					ImGui::Separator();
					ImGui::Selectable("Darken");
					ImGui::Selectable("Multiply");
					ImGui::Separator();
					ImGui::Selectable("Lighten");
					ImGui::Selectable("Screen");
					ImGui::Separator();
					ImGui::Selectable("Hue");
					ImGui::Selectable("Saturation");
					ImGui::Selectable("Color");
					ImGui::Selectable("Luminosity");
					ImGui::EndCombo();
				}

				ImGui::SameLine();

				ImGui::Text("Opacity");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(-1);
				if (ImGui::InputInt("##OpacityInput", &m_SelectedLayer->Opacity, 0))
				{
					m_SelectedLayer->Opacity = std::clamp(m_SelectedLayer->Opacity, 0, 100);
				}
			}
			ImGui::EndChild();

			ImGui::BeginChild("##LayerArea", ImGui::GetContentRegionAvail() - ImVec2(0, 27.5f));

			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
				m_SelectedLayer = nullptr;

			unsigned int idToMove = 0;
			int idToMoveTarget = 0;

			for (int i = m_Layers.size() - 1; i >= 0; i--)
			{
				ImGui::PushID(m_Layers[i].ID);

				bool layerDeleted = false;

				ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, 7.5f));
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("IMAGE_EDITOR_LAYER"))
					{
						idToMove = *(unsigned int*)payload->Data;
						idToMoveTarget = i;
					}
					ImGui::EndDragDropTarget();
				}
				ImGui::SetCursorPos(ImGui::GetCursorPos() - ImVec2(0, 8.0f));
				ImGui::Separator();

				bool selected = false;
				if (m_SelectedLayer != NULL)
				{
					selected = m_Layers[i].ID == m_SelectedLayer->ID;
				}
				if (selected) ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_ButtonToggled));
				ImGui::BeginChild("##LayerItemWindow", ImVec2(0, 30));
				if (selected) ImGui::PopStyleColor();

				// Drag Drop Source Interaction

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("IMAGE_EDITOR_LAYER", &m_Layers[i].ID, sizeof(unsigned int), ImGuiCond_Once);

					auto size = ImVec2(m_Layers[i].Texture->GetWidth() / (m_Layers[i].Texture->GetHeight() / 30), 30);
					auto cursorPos = ImGui::GetCursorPos();
					ImGui::Image((ImTextureID)m_CheckerboardTexture->GetRendererID(), size, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
					ImGui::SetCursorPos(cursorPos);
					ImGui::Image((ImTextureID)m_Layers[i].Texture->GetRendererID(), size, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

					ImGui::SameLine();

					ImGui::Text(m_Layers[i].Name.c_str());
					ImGui::EndDragDropSource();
				}

				// Right Click Context

				if (ImGui::BeginPopupContextWindow())
				{
					if (ImGui::MenuItem((std::string(CHARACTER_ICON_DELETE) + " Delete").c_str()))
						layerDeleted = true;
					if (ImGui::MenuItem((std::string(CHARACTER_ICON_DUPLICATE) + " Duplicate").c_str()))
					{
						bool selected = m_SelectedLayer != NULL;
						unsigned int id;
						if (selected) { id = m_SelectedLayer->ID; }

						ImageLayer newLayer = ImageLayer(GetNextLayerID(), m_Layers[i].Name, m_DocumentWidth, m_DocumentHeight);
						newLayer.Opacity = m_Layers[i].Opacity;

						auto dataSize = m_DocumentWidth * m_DocumentHeight * 4;
						unsigned char* data = new unsigned char[dataSize];
						m_Layers[i].Texture->GetData(data, dataSize);
						newLayer.Texture->SetData(data, dataSize);
						delete[] data;

						m_Layers.insert(m_Layers.begin() + i, newLayer);

						if (selected) { m_SelectedLayer = GetLayerById(id); }
					}

					ImGui::EndPopup();
				}

				ImGui::Dummy(ImVec2(0.5f, 0.0f));
				ImGui::SameLine();

				ImGui::BeginGroup();
				ImGui::Dummy(ImVec2(0, 1.0f));
				bool visibilityButton = ImGui::ImageButton((ImTextureID)((m_Layers[i].Visible ? m_VisibleIcon : m_HiddenIcon)->GetRendererID()), ImVec2(22.5f, 22.5f), { 0, 1 }, { 1, 0 });
				if (visibilityButton)
					m_Layers[i].Visible = !m_Layers[i].Visible;
				ImGui::EndGroup();
				ImGui::SameLine();

				auto& texture = m_Layers[i].Texture;
				auto size = ImVec2(texture->GetWidth() / (texture->GetHeight() / 30), 30);
				auto cursorPos = ImGui::GetCursorPos();
				ImGui::Image((ImTextureID)m_CheckerboardTexture->GetRendererID(), size, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
				ImGui::SetCursorPos(cursorPos);
				ImGui::Image((ImTextureID)texture->GetRendererID(), size, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

				ImGui::SameLine();

				// Editable Text Box
				ImGui::BeginGroup();
				ImGui::Dummy(ImVec2(0.0f, 3.75f));

				char buf[256];
				auto buf_size = sizeof(buf);
				memset(buf, 0, buf_size);
				std::strncpy(buf, m_Layers[i].Name.c_str(), sizeof(buf));

				auto flags = ImGuiSelectableFlags_None;
				ImGuiContext& g = *GImGui;
				ImGuiWindow* window = g.CurrentWindow;
				ImVec2 pos_before = window->DC.CursorPos;

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(g.Style.ItemSpacing.x, g.Style.FramePadding.y * 2.0f));
				bool ret = ImGui::Selectable("##Selectable", false, flags | ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_AllowItemOverlap);
				ImGui::PopStyleVar();

				ImGuiID id = window->GetID("##Input");
				bool temp_input_is_active = ImGui::TempInputIsActive(id);
				bool temp_input_start = ret ? ImGui::IsMouseDoubleClicked(0) : false;

				if (temp_input_start)
					ImGui::SetActiveID(id, window);

				if (temp_input_is_active || temp_input_start)
				{
					ImVec2 pos_after = window->DC.CursorPos;
					window->DC.CursorPos = pos_before;
					ret = ImGui::TempInputText(g.LastItemData.Rect, id, "##Input", buf, (int)buf_size, ImGuiInputTextFlags_None);
					window->DC.CursorPos = pos_after;
				}
				else
				{
					window->DrawList->AddText(pos_before, ImGui::GetColorU32(ImGuiCol_Text), buf);
				}

				if (ret)
				{
					m_Layers[i].Name = buf;
				}
				ImGui::EndGroup();
				// End Editable Text Box

				if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !visibilityButton)
					m_SelectedLayer = &m_Layers[i];

				ImGui::EndChild();
				ImGui::PopID();

				if (layerDeleted)
				{
					if (m_SelectedLayer != nullptr)
					{
						if (m_Layers[i].ID == m_SelectedLayer->ID)
							m_SelectedLayer = nullptr;
					}
					m_Layers.erase(m_Layers.begin() + i);
				}
			}

			// End Layer Loop

			// Final Nav Line(Identical Code To That inside loop)
			ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, 7.5f));
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("IMAGE_EDITOR_LAYER"))
				{
					idToMove = *(unsigned int*)payload->Data;
					idToMoveTarget = -1;
				}
				ImGui::EndDragDropTarget();
			}
			ImGui::SetCursorPos(ImGui::GetCursorPos() - ImVec2(0, 8.0f));
			ImGui::Separator();			

			if (idToMove != 0)
			{
				auto index = GetLayerIndexById(idToMove);
				auto targetIndex = idToMoveTarget + (index > idToMoveTarget ? 1 : 0);
				auto content = m_Layers[index];
				m_Layers.erase(m_Layers.begin() + index);
				m_Layers.insert(m_Layers.begin() + targetIndex, content);

				m_SelectedLayer = GetLayerById(idToMove);
			}

			ImGui::EndChild();
			if (ImGui::Button("+"))
			{
				auto id = GetNextLayerID();
				m_Layers.push_back({id, "Untitled Layer", m_DocumentWidth, m_DocumentHeight});
				m_SelectedLayer = GetLayerById(id);
			}
			ImGui::End();

			ImGui::Begin("Color");
			if (ImGui::InputInt("Opacity", &m_BrushOpacity, 0)) { m_BrushOpacity = std::clamp(m_BrushOpacity, 0, 100); }
			ImGui::ColorPicker3("Brush Color", glm::value_ptr(m_BrushColor));
			ImGui::End();

			ImGui::Begin("Image Toolbar");
			const char* items[] = { "Move", "Brush", "Hand" };
			ImGui::ButtonStackEx("##ImageEditorTools", items, 3, (int*)&m_Tool, ImVec2(50.0f, 150.0f), 5.0f);
			ImGui::End();
		}
	}

	//void ImageEditor::DrawLine(unsigned char* data, int dataSize, glm::vec2 p0, glm::vec2 p1)
	//{
	//	int dx = p1.x - p0.x;
	//	int dy = p1.y - p0.y;
	//
	//	int dLong = abs(dx);
	//	int dShort = abs(dy);
	//
	//	int offsetLong = dx > 0 ? 1 : -1;
	//	int offsetShort = dy > 0 ? m_DocumentWidth : -m_DocumentWidth; // Could be height
	//
	//	if (dLong < dShort)
	//	{
	//		std::swap(dShort, dLong);
	//		std::swap(offsetShort, offsetLong);
	//	}
	//
	//	int error = dLong / 2;
	//	int index = p0.y * m_DocumentWidth + p0.x; // Could be height
	//	const int offset[] = { offsetLong, offsetLong + offsetShort };
	//	const int abs_d[] = { dShort, dShort - dLong };
	//	for (int i = 0; i <= dLong; ++i)
	//	{
	//		if (index >= 0 && index < dataSize / 4 && (ConvertIndexToVector(index, m_DocumentWidth).x < m_DocumentWidth)) {
	//			//buffer[index] = 255;  // or a call to your painting method
	//
	//			int yIndex = -((100 / 2) + ((m_DocumentWidth * (100)) / 2));
	//			for (int y = 0; y < (100 * 100); y++)
	//			{
	//				if ((index)+(yIndex) < dataSize / 4 && (index)+(yIndex) > 0)
	//				{
	//					if (m_BrushData[y * 4 + 3] > data[(index * 4) + (yIndex * 4) + 0])
	//					{
	//						auto uIndex = (index * 4) + (yIndex * 4);
	//
	//							data[uIndex + 0] = (unsigned int)m_BrushData[y * 4 + 3];
	//							data[uIndex + 1] = (unsigned int)m_BrushData[y * 4 + 3];
	//							data[uIndex + 2] = (unsigned int)m_BrushData[y * 4 + 3];
	//							data[uIndex + 3] = 255.0f;
	//					}
	//
	//					yIndex++;
	//					if (std::fmod(y + 2, 100) == 1)
	//					{
	//						yIndex += (m_DocumentWidth - 100);
	//					}
	//				}
	//			}
	//		}
	//		const int errorIsTooBig = error >= dLong;
	//		index += offset[errorIsTooBig];
	//		error += abs_d[errorIsTooBig];
	//	}
	//}

	void ImageEditor::PutPixel(unsigned char* data, int& dataSize, glm::vec2 point)
	{
		auto startPoint = point - (m_BrushSize / 2.0f);
		glm::vec2 brushPoint = startPoint;
		for (int y = 0; y < (m_BrushSize.x * m_BrushSize.y); y++)
		{
			auto uIndex = ConvertVectorToIndex(brushPoint, m_DocumentWidth);
			if (brushPoint.x >= 0 && brushPoint.x < m_DocumentWidth && brushPoint.y >= 0 && brushPoint.y < m_DocumentHeight)
			{
				if (m_BrushData[y * 4 + 3] > data[uIndex + 0])
				{
					data[uIndex + 0] = (unsigned int)m_BrushData[y * 4 + 3];
				}
			}
		
			brushPoint.x++;
			if (std::fmod(y + 2, m_BrushSize.x) == 1)
			{
				brushPoint.y++;
				brushPoint.x = startPoint.x;
			}
		}
	}

	void ImageEditor::DrawLine(unsigned char* data, int& dataSize, glm::vec2 p1, glm::vec2 p2)
	{
		int x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;
		dx = p2.x - p1.x;
		dy = p2.y - p1.y;
		dx1 = fabs(dx);
		dy1 = fabs(dy);
		px = 2 * dy1 - dx1;
		py = 2 * dx1 - dy1;
		if (dy1 <= dx1)
		{
			if (dx >= 0)
			{
				x = p1.x;
				y = p1.y;
				xe = p2.x;
			}
			else
			{
				x = p2.x;
				y = p2.y;
				xe = p1.x;
			}
			PutPixel(data, dataSize, { x, y });
			for (i = 0; x < xe; i++)
			{
				x = x + 1;
				if (px < 0)
				{
					px = px + 2 * dy1;
				}
				else
				{
					if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
					{
						y = y + 1;
					}
					else
					{
						y = y - 1;
					}
					px = px + 2 * (dy1 - dx1);
				}
				PutPixel(data, dataSize, { x, y });
			}
		}
		else
		{
			if (dy >= 0)
			{
				x = p1.x;
				y = p1.y;
				ye = p2.y;
			}
			else
			{
				x = p2.x;
				y = p2.y;
				ye = p1.y;
			}
			PutPixel(data, dataSize, { x, y });
			for (i = 0; y < ye; i++)
			{
				y = y + 1;
				if (py <= 0)
				{
					py = py + 2 * dx1;
				}
				else
				{
					if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
					{
						x = x + 1;
					}
					else
					{
						x = x - 1;
					}
					py = py + 2 * (dx1 - dy1);
				}
				PutPixel(data, dataSize, { x, y });
			}
		}
	}

	ImageLayer* ImageEditor::GetLayerById(unsigned int id)
	{
		for (auto& layer : m_Layers)
		{
			if (layer.ID == id)
			{
				return &layer;
			}
		}
		return nullptr;
	}

	int ImageEditor::GetLayerIndexById(unsigned int id)
	{
		for (int i = 0; i < m_Layers.size(); i++)
		{
			if (m_Layers[i].ID == id)
			{
				return i;
			}
		}
		return -1;
	}

	void ImageEditor::FindBoundingBox(unsigned char* data, uint32_t size, glm::vec2* min, glm::vec2* max)
	{
		min->x = -1; min->y = -1; max->x = -1; max->y = -1;
		for (int i = 3; i < size; i+=4)
		{
			if (data[i] != 0.0f)
			{
				auto pos = ConvertIndexToVector((i - 3) / 4, m_DocumentWidth);
				if (min->y == -1) { min->y = pos.y; max->y = pos.y; }
				if (min->x == -1 || pos.x < min->x) { min->x = pos.x; }
				if (pos.y > max->y) { max->y = pos.y; }
				if (pos.x > max->x) { max->x = pos.x; }
			}
		}
	}

	void ImageEditor::ExportImage(std::string)
	{
		auto size = m_DocumentWidth * m_DocumentHeight * 4;
		unsigned char* data = new unsigned char[size];
		m_Layers[0].Texture->GetData(data, size);
		stbi_flip_vertically_on_write(1);
		stbi_write_bmp("exported_image.bmp", m_DocumentWidth, m_DocumentHeight, 4, data);
		delete[] data;
	}

}