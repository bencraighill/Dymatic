#include "CurveEditor.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h" 
#include "imgui/imgui_internal.h"

#include "Dymatic/Math/Math.h"
#include "Dymatic/Math/StringUtils.h"

#include "Settings/Preferences.h"
#include "TextSymbols.h"

#include <iostream>

namespace Dymatic {

	static int getPt(int n1, int n2, float perc)
	{
		int diff = n2 - n1;

		return n1 + (diff * perc);
	}

	CurveEditor::CurveEditor()
	{
		m_Channels = { Channel(GetNextCurveID(), "X Location", glm::vec3(0.85f, 0.1f, 0.2f), {CurvePoint(GetNextCurveID(), glm::vec2(200.0f, 50.0f), glm::vec2(10.0f, 40.0f), glm::vec2(90.0f, 30.0f)), CurvePoint(GetNextCurveID(), glm::vec2(80.0f, 50.0f), glm::vec2(10.0f, 40.0f), glm::vec2(90.0f, 30.0f)), CurvePoint(GetNextCurveID(), glm::vec2(80.0f, 50.0f), glm::vec2(10.0f, 40.0f), glm::vec2(90.0f, 30.0f)), CurvePoint(GetNextCurveID(), glm::vec2(80.0f, 50.0f), glm::vec2(10.0f, 40.0f), glm::vec2(90.0f, 30.0f))}), Channel(GetNextCurveID(), "Y Location", glm::vec3(0.2f, 0.8f, 0.2f), {CurvePoint(GetNextCurveID(), glm::vec2(80.0f, 50.0f), glm::vec2(10.0f, 40.0f), glm::vec2(90.0f, 30.0f)), CurvePoint(GetNextCurveID(), glm::vec2(80.0f, 50.0f), glm::vec2(10.0f, 40.0f), glm::vec2(90.0f, 30.0f)), CurvePoint(GetNextCurveID(), glm::vec2(80.0f, 50.0f), glm::vec2(10.0f, 40.0f), glm::vec2(90.0f, 30.0f))}) };
	}

	void CurveEditor::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);

		dispatcher.Dispatch<KeyPressedEvent>(DY_BIND_EVENT_FN(CurveEditor::OnKeyPressed));
		dispatcher.Dispatch<MouseButtonPressedEvent>(DY_BIND_EVENT_FN(CurveEditor::OnMouseButtonPressed));
		dispatcher.Dispatch<MouseScrolledEvent>(DY_BIND_EVENT_FN(CurveEditor::OnMouseScrolled));
	}

	bool CurveEditor::OnKeyPressed(KeyPressedEvent& e)
	{
		return false;
	}

	bool CurveEditor::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		switch (e.GetMouseButton())
		{
		case Mouse::ButtonLeft:{ m_Clicked = true; break; }
		}
		return false;
	}

	bool CurveEditor::OnMouseScrolled(MouseScrolledEvent& e)
	{
		float delta = e.GetYOffset() * 0.1f;

		glm::vec2 distance = m_Scale * 0.025f;
		distance = glm::vec2(std::max(distance.x, 0.0f), std::max(distance.y, 0.0f));
		glm::vec2 speed = distance * distance;
		speed = glm::vec2(std::min(speed.x, 100.0f), std::min(speed.y, 100.0f)); // max speed = 100
		
		delta* speed;

		m_Scale.x = (m_Scale.x - delta);
		m_Scale.y = (m_Scale.y - delta);
		m_Scale.x = (m_Scale.x < 0.01f ? 0.01f : m_Scale.x);
		m_Scale.y = (m_Scale.y < 0.01f ? 0.01f : m_Scale.y);
		return false;
	}
	void CurveEditor::OnImGuiRender()
	{
		auto& curveEditorVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::CurveEditor);
		if (curveEditorVisible)
		{
			if (Input::IsKeyPressed(Key::D1))
			{
				m_Scale.x = m_Scale.x /= 1.25f;
				m_Scale.x = m_Scale.x < 0.1f ? 0.1f : m_Scale.x;
				m_Scale.y = m_Scale.y < 0.1f ? 0.1f : m_Scale.y;
			}

			if (Input::IsKeyPressed(Key::D2))
			{
				m_Scale.x = m_Scale.x *= 1.25f;
				m_Scale.x = m_Scale.x < 0.1f ? 0.1f : m_Scale.x;
				m_Scale.y = m_Scale.y < 0.1f ? 0.1f : m_Scale.y;
			}

			if (Input::IsKeyPressed(Key::D3))
			{
				m_Scale.y = m_Scale.y /= 1.25f;
				m_Scale.x = m_Scale.x < 0.1f ? 0.1f : m_Scale.x;
				m_Scale.y = m_Scale.y < 0.1f ? 0.1f : m_Scale.y;
			}

			if (Input::IsKeyPressed(Key::D4))
			{
				m_Scale.y = m_Scale.y *= 1.25f;
				m_Scale.x = m_Scale.x < 0.1f ? 0.1f : m_Scale.x;
				m_Scale.y = m_Scale.y < 0.1f ? 0.1f : m_Scale.y;
			}

			if (Input::IsMouseButtonPressed(Mouse::ButtonMiddle))
			{
				m_XLocation -= previousMouseX - Input::GetMouseX();
				m_YLocation -= previousMouseY - Input::GetMouseY();
			}

			previousMouseX = Input::GetMouseX();
			previousMouseY = Input::GetMouseY();
			ImGui::Begin(CHARACTER_ICON_CURVE " Curve Editor", &curveEditorVisible);

			static float panelA = ImGui::GetContentRegionAvail().x / 10 * 1.25f;
			static float panelB = ImGui::GetContentRegionAvail().x / 10 * 6.5f;
			static float panelC = ImGui::GetContentRegionAvail().x / 10 * 2.25f;

			ImGui::Splitter("##MainGraphSplitEditor", true, 2.0f, &panelA, &panelB, 50.0f, 50.0f);

			ImGui::BeginChild("##GraphHeirachy", ImVec2(panelA, 0));
			for (int i = 0; i < m_Channels.size(); i++)
			{
				if (ImGui::TreeNodeEx((m_Channels[i].m_ChannelName + "##" + std::to_string(m_Channels[i].ChannelID)).c_str()))
				{
					for (int x = 0; x < m_Channels[i].m_Points.size(); x++)
					{
						if (ImGui::TreeNodeEx(("Point_" + std::to_string(m_Channels[i].m_Points[x].CurveID)).c_str(), ImGuiTreeNodeFlags_Leaf))
						{
							ImGui::TreePop();
						}
					}
					ImGui::TreePop();
				}
			}
			ImGui::EndChild();

			ImGui::SameLine();
			ImGui::Splitter("##SideGraphSplitEditor", true, 2.0f, &panelB, &panelC, 50.0f, 50.0f);

			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			ImGui::BeginChild("##MainGraph", ImVec2(panelB, 0));
			ImGui::PopStyleColor();

			if (ImGui::BeginPopupContextWindow(0, 1, false))
			{
				if (ImGui::MenuItem("Always Show Handles", m_AlwaysShowHandles ? "(Enabled)" : "(Disabled)")) { m_AlwaysShowHandles = !m_AlwaysShowHandles; }

				if (ImGui::MenuItem("Handles Relative", m_HandlesRelative ? "(Enabled)" : "(Disabled)")) { m_HandlesRelative = !m_HandlesRelative; }

				if (!m_SelectedPoints.empty())
				{
					if (ImGui::MenuItem("Delete"))
					{
						for (int x = 0; x < m_Channels.size(); x++)
						{
							for (int y = 0; y < m_Channels[x].m_Points.size(); y++)
							{
								bool success = false;
								for (int i = 0; i < m_SelectedPoints.size(); i++)
								{
									if (m_SelectedPoints[i] == m_Channels[x].m_Points[y].CurveID) { success = true; m_SelectedPoints.erase(m_SelectedPoints.begin() + i); i--; }
								}
								if (success)
								{
									m_Channels[x].m_Points.erase(m_Channels[x].m_Points.begin() + y);
									y--;
								}
							}
						}

					}

					if (ImGui::MenuItem("Subdivide"))
					{
						for (int x = 0; x < m_Channels.size(); x++)
						{
							int checkIndex = -1;
							for (int y = 0; y < m_Channels[x].m_Points.size(); y++)
							{
								bool success = false;
								for (int i = 0; i < m_SelectedPoints.size(); i++)
								{
									if (m_SelectedPoints[i] == m_Channels[x].m_Points[y].CurveID) { success = true; }
								}
								if (success)
								{
									if (checkIndex == -1)
									{
										checkIndex = y;
									}
									else
									{
										glm::vec2 location = (m_Channels[x].m_Points[y].Position + m_Channels[x].m_Points[checkIndex].Position) / 2.0f;
										int middleIndex = (checkIndex + y) / 2.0f;

										auto uploadPoint = CurvePoint(GetNextCurveID(), location, location - glm::vec2(100, 0), location + glm::vec2(100, 0));
										m_Channels[x].m_Points.insert(m_Channels[x].m_Points.begin() + middleIndex + 1, uploadPoint);
										//m_Channels[x].m_Points.push_back(CurvePoint(GetNextCurveID(), location, location + glm::vec2(100, 0), location - glm::vec2(100, 0)));
										checkIndex = -1;
										//y--;
									}
								}
							}
						}

					}

					CurveType selectedType = Curve_None;

					if (ImGui::BeginMenu("Select Curve Type"))
					{
						for (int i = 1; i < CURVE_TYPE_SIZE; i++)
						{
							if (ImGui::MenuItem(ConvertCurveTypeToString((CurveType)i).c_str()))
							{
								selectedType = (CurveType)i;
							}
						}
						ImGui::EndMenu();
					}

					if (selectedType != None)
					{
						for (int x = 0; x < m_Channels.size(); x++)
						{
							int checkIndex = -1;
							for (int y = 0; y < m_Channels[x].m_Points.size(); y++)
							{
								bool success = false;
								for (int i = 0; i < m_SelectedPoints.size(); i++)
								{
									if (m_SelectedPoints[i] == m_Channels[x].m_Points[y].CurveID) { success = true; }
								}
								if (success)
								{
									m_Channels[x].m_Points[y].curveType = selectedType;
								}
							}
						}

					}
				}

				ImGui::EndPopup();
			}

			auto drawList = ImGui::GetWindowDrawList();
			static float radius = 3.0f;
			static ImVec2 buttonPadding = ImVec2(2.0f, 2.0f);

			//Draw Grid
			ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
			ImVec2 canvas_sz = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
			if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
			if (canvas_sz.y < 50.0f) canvas_sz.y = 50.0f;
			ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

			drawList->PushClipRect(canvas_p0, canvas_p1, true);
			{
				static float repeatThreashold = 0.1f;
				const float GRID_STEP = 10.0f;
				for (float x = fmodf(m_XLocation, GRID_STEP / glm::mod(m_Scale.x, GRID_STEP * repeatThreashold)); x < canvas_sz.x; x += GRID_STEP / glm::mod(m_Scale.x, GRID_STEP * repeatThreashold))
				{
					float index = (int)((x - m_XLocation) / (GRID_STEP / glm::mod(m_Scale.x, GRID_STEP * repeatThreashold)));
					bool locX = glm::mod(index, 10.0f) == 1;
					drawList->AddLine(ImVec2(canvas_p0.x + x, canvas_p0.y), ImVec2(canvas_p0.x + x, canvas_p1.y), IM_COL32(25, 25, 25, 100), locX ? 3.0f : 1.0f);
					if (locX)
					{
						std::string text = String::FloatToString((int)((canvas_p0.x + x - ImGui::GetWindowPos().x - m_XLocation) * m_Scale.x));
						drawList->AddText(ImVec2(canvas_p0.x + x - ImGui::CalcTextSize(text.c_str()).x / 2, canvas_p0.y), IM_COL32(255, 255, 255, 255), (text).c_str());
					}
				}
				for (float y = fmodf(m_YLocation, GRID_STEP / glm::mod(m_Scale.y, GRID_STEP * repeatThreashold)); y < canvas_sz.y; y += GRID_STEP / glm::mod(m_Scale.y, GRID_STEP * repeatThreashold))
				{
					float index = (int)((y - m_YLocation) / (GRID_STEP / glm::mod(m_Scale.y, GRID_STEP * repeatThreashold)));
					bool locY = glm::mod(index, 10.0f) == 1;
					drawList->AddLine(ImVec2(canvas_p0.x, canvas_p0.y + y), ImVec2(canvas_p1.x, canvas_p0.y + y), IM_COL32(25, 25, 25, 100), locY ? 3.0f : 1.0f);
					if (locY)
					{
						std::string text = String::FloatToString((int)((canvas_p0.y + y - ImGui::GetWindowPos().y - m_YLocation) * m_Scale.y));
						drawList->AddText(ImVec2(canvas_p0.x, canvas_p0.y + y - ImGui::CalcTextSize(text.c_str()).y / 2), IM_COL32(255, 255, 255, 255), (text).c_str());
					}
				}
			}
			drawList->PopClipRect();
			//---------------//

			if (!Input::IsMouseButtonPressed(Mouse::ButtonLeft))
			{
				m_HeldPointID = -1;
				m_HeldHandle = -1;
			}


			bool anyClicked = false;
			bool graphEditorHovered = ImGui::IsWindowHovered();
			for (int x = 0; x < m_Channels.size(); x++)
			{
				auto& m_Points = m_Channels[x].m_Points;

				int index = -1;
				for (int i = 0; i < m_Points.size(); i++)
				{
					if (m_Points[i].CurveID == m_HeldPointID) { index = i; }
				}

				if (m_HeldHandle == 0 && index != -1)
				{

					for (int z = 0; z < m_Points.size(); z++)
					{
						bool selected = false;
						for (int y = 0; y < m_SelectedPoints.size(); y++)
						{
							if (m_SelectedPoints[y] == m_Points[z].CurveID) { selected = true; }
						}

						if (z != index && selected)
						{
							glm::vec2 offsetPos = m_Points[z].Position - m_Points[index].Position;
							glm::vec2 offsetL = m_Points[z].HandleL - m_Points[z].Position;
							glm::vec2 offsetR = m_Points[z].HandleR - m_Points[z].Position;
							m_Points[z].Position = glm::vec2((ImGui::GetMousePos().x - ImGui::GetWindowPos().x - m_XLocation) * m_Scale.x, (ImGui::GetMousePos().y - ImGui::GetWindowPos().y - m_YLocation) * m_Scale.y) + offsetPos;
							if (m_HandlesRelative)
							{
								m_Points[z].HandleL = m_Points[z].Position + offsetL;
								m_Points[z].HandleR = m_Points[z].Position + offsetR;
							}
						}
					}

					bool selected = false;
					for (int y = 0; y < m_SelectedPoints.size(); y++)
					{
						if (m_SelectedPoints[y] == m_Points[index].CurveID) { selected = true; }
					}

					if (selected)
					{
						glm::vec2 offsetL = m_Points[index].HandleL - m_Points[index].Position;
						glm::vec2 offsetR = m_Points[index].HandleR - m_Points[index].Position;
						m_Points[index].Position = glm::vec2((ImGui::GetMousePos().x - ImGui::GetWindowPos().x - m_XLocation) * m_Scale.x, (ImGui::GetMousePos().y - ImGui::GetWindowPos().y - m_YLocation) * m_Scale.y);
						if (m_HandlesRelative)
						{
							m_Points[index].HandleL = m_Points[index].Position + offsetL;
							m_Points[index].HandleR = m_Points[index].Position + offsetR;
						}
					}
				}

				if (m_HeldHandle == 1 && index != -1)
				{

					m_Points[index].HandleL = glm::vec2((ImGui::GetMousePos().x - ImGui::GetWindowPos().x - m_XLocation) * m_Scale.x, (ImGui::GetMousePos().y - ImGui::GetWindowPos().y - m_YLocation) * m_Scale.y);

					glm::vec2 offsetL = m_Points[index].HandleL - m_Points[index].Position;
					float distanceL = std::sqrt(std::pow(m_Points[index].Position.x - m_Points[index].HandleL.x, 2) + std::pow(m_Points[index].Position.y - m_Points[index].HandleL.y, 2));
					float distanceR = std::sqrt(std::pow(m_Points[index].Position.x - m_Points[index].HandleR.x, 2) + std::pow(m_Points[index].Position.y - m_Points[index].HandleR.y, 2));

					if (m_Points[index].curveType == CurveType::Automatic)
					{
						m_Points[index].HandleR = glm::vec2((ImGui::GetMousePos().x - ImGui::GetWindowPos().x - m_XLocation) * m_Scale.x, (ImGui::GetMousePos().y - ImGui::GetWindowPos().y - m_YLocation) * m_Scale.y) - (offsetL * (1.0f + (distanceR / distanceL)));
					}

				}

				if (m_HeldHandle == 2 && index != -1)
				{
					m_Points[index].HandleR = glm::vec2((ImGui::GetMousePos().x - ImGui::GetWindowPos().x - m_XLocation) * m_Scale.x, (ImGui::GetMousePos().y - ImGui::GetWindowPos().y - m_YLocation) * m_Scale.y);

					glm::vec2 offsetR = m_Points[index].HandleR - m_Points[index].Position;
					float distanceL = std::sqrt(std::pow(m_Points[index].Position.x - m_Points[index].HandleL.x, 2) + std::pow(m_Points[index].Position.y - m_Points[index].HandleL.y, 2));
					float distanceR = std::sqrt(std::pow(m_Points[index].Position.x - m_Points[index].HandleR.x, 2) + std::pow(m_Points[index].Position.y - m_Points[index].HandleR.y, 2));

					if (m_Points[index].curveType == CurveType::Automatic)
					{
						m_Points[index].HandleL = glm::vec2((ImGui::GetMousePos().x - ImGui::GetWindowPos().x - m_XLocation) * m_Scale.x, (ImGui::GetMousePos().y - ImGui::GetWindowPos().y - m_YLocation) * m_Scale.y) - (offsetR * (1.0f + (distanceL / distanceR)));
					}
				}

				if (!m_Points.empty())
				{
					ImVec2 startPos = ImGui::GetWindowPos() + ImVec2(m_Points[0].Position.x / m_Scale.x, m_Points[0].Position.y / m_Scale.y) + ImVec2(m_XLocation, m_YLocation);
					drawList->AddLine(startPos, ImVec2(ImGui::GetWindowPos().x, startPos.y), ImGui::ColorConvertFloat4ToU32(ImVec4(m_Channels[x].m_Color.x, m_Channels[x].m_Color.y, m_Channels[x].m_Color.z, 1.0f)), 2.0f);

					ImVec2 endPos = ImGui::GetWindowPos() + ImVec2(m_Points[m_Points.size() - 1].Position.x / m_Scale.x, m_Points[m_Points.size() - 1].Position.y / m_Scale.y) + ImVec2(m_XLocation, m_YLocation);
					drawList->AddLine(endPos, ImVec2((ImGui::GetWindowPos() + ImGui::GetWindowSize()).x, endPos.y), ImGui::ColorConvertFloat4ToU32(ImVec4(m_Channels[x].m_Color.x, m_Channels[x].m_Color.y, m_Channels[x].m_Color.z, 1.0f)), 2.0f);
				}

				for (int i = 0; i < m_Points.size(); i++)
				{

					int selectedIndex = -1;
					for (int x = 0; x < m_SelectedPoints.size(); x++)
					{
						if (m_Points[i].CurveID == m_SelectedPoints[x])
						{
							selectedIndex = x;
						}
					}
					bool selected = selectedIndex != -1;

					ImVec2 mainPos = ImGui::GetWindowPos() + ImVec2(m_Points[i].Position.x / m_Scale.x, m_Points[i].Position.y / m_Scale.y) + ImVec2(m_XLocation, m_YLocation);
					ImVec2 mainHandle = ImGui::GetWindowPos() + ImVec2(m_Points[i].HandleR.x / m_Scale.x, m_Points[i].HandleR.y / m_Scale.y) + ImVec2(m_XLocation, m_YLocation);
					ImVec2 offHandle = ImGui::GetWindowPos() + ImVec2(m_Points[i].HandleL.x / m_Scale.x, m_Points[i].HandleL.y / m_Scale.y) + ImVec2(m_XLocation, m_YLocation);

					ImVec2 linkPos;
					ImVec2 linkHandle;
					bool linked = false;

					if (i < m_Points.size() - 1)
					{
						linked = true;
						linkPos = ImGui::GetWindowPos() + ImVec2(m_Points[i + 1].Position.x / m_Scale.x, m_Points[i + 1].Position.y / m_Scale.y) + ImVec2(m_XLocation, m_YLocation);
						linkHandle = ImGui::GetWindowPos() + ImVec2(m_Points[i + 1].HandleL.x / m_Scale.x, m_Points[i + 1].HandleL.y / m_Scale.y) + ImVec2(m_XLocation, m_YLocation);
						if (m_Points[i + 1].curveType == Linear || m_Points[i + 1].curveType == Constant)
						{
							linkHandle = linkPos;
						}
					}
					if (m_Points[i].curveType == Linear || m_Points[i].curveType == Constant)
					{
						mainHandle = mainPos;
						offHandle = mainPos;
					}

					//Main Position Grab
					const ImGuiID id = ImGui::GetCurrentWindow()->GetID(("MainCurvePosGrab" + std::to_string(m_Points[i].CurveID)).c_str());
					bool hovered, held;
					bool pressed = ImGui::ButtonBehavior(ImRect(mainPos - ImVec2(radius, radius) - buttonPadding, mainPos + ImVec2(radius, radius) + buttonPadding), id, &hovered, &held);
					if (m_Clicked && hovered)
					{
						anyClicked = true;
						if (Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift))
						{
							if (selected)
							{
								m_SelectedPoints.erase(m_SelectedPoints.begin() + selectedIndex);
							}
							else
							{
								m_SelectedPoints.push_back(m_Points[i].CurveID);
							}
						}
						else
						{
							if (!selected)
							{
								m_SelectedPoints.clear();
								m_SelectedPoints.push_back(m_Points[i].CurveID);
							}
						}

						m_HeldPointID = m_Points[i].CurveID;
						m_HeldHandle = 0;
					}

					drawList->AddCircle(mainPos, radius, ImGui::ColorConvertFloat4ToU32(selected ? ((m_Points[i].CurveID == m_SelectedPoints.back()) ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.9f, 0.52f, 0.25f, 1.0f)) : ImVec4(0.0f, 0.0f, 0.0f, 1.0f)), 0, 2.0f);

					if ((selected || m_AlwaysShowHandles) && (m_Points[i].curveType != CurveType::Linear) && (m_Points[i].curveType != CurveType::Constant))
					{
						//Main Handle Grab
						const ImGuiID handleId = ImGui::GetCurrentWindow()->GetID(("MainCurveHandleGrab" + std::to_string(m_Points[i].CurveID)).c_str());
						bool handleHovered, handleHeld;
						bool handlePressed = ImGui::ButtonBehavior(ImRect(mainHandle - ImVec2(radius, radius) - buttonPadding, mainHandle + ImVec2(radius, radius) + buttonPadding), handleId, &handleHovered, &handleHeld);
						if (handleHeld)
						{
							anyClicked = true;
							m_HeldPointID = m_Points[i].CurveID;
							m_HeldHandle = 2;
						}
						//drawList->AddCircle(mainHandle, radius, ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.2f, 0.85f, 1.0f)), 0, 2.0f);
						drawList->AddRect(mainHandle - ImVec2(radius, radius), mainHandle + ImVec2(radius, radius), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.67f, 0.46f, 1.0f)), 0, 2.0f);
						drawList->AddLine(mainPos, mainHandle, ImGui::ColorConvertFloat4ToU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f)));

						//Off Left Handle Grab
						const ImGuiID offhandleId = ImGui::GetCurrentWindow()->GetID(("OffCurveHandleGrab" + std::to_string(m_Points[i].CurveID)).c_str());
						bool offhandleHovered, offhandleHeld;
						bool offhandlePressed = ImGui::ButtonBehavior(ImRect(offHandle - ImVec2(radius, radius) - buttonPadding, offHandle + ImVec2(radius, radius) + buttonPadding), offhandleId, &offhandleHovered, &offhandleHeld);
						if (offhandleHeld)
						{
							anyClicked = true;
							m_HeldPointID = m_Points[i].CurveID;
							m_HeldHandle = 1;
						}
						//drawList->AddCircle(mainHandle, radius, ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.2f, 0.85f, 1.0f)), 0, 2.0f);
						drawList->AddRect(offHandle - ImVec2(radius, radius), offHandle + ImVec2(radius, radius), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.67f, 0.46f, 1.0f)), 0, 2.0f);
						drawList->AddLine(mainPos, offHandle, ImGui::ColorConvertFloat4ToU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f)));
					}



					if (linked)
					{
						static float offset = 0.01f;

						for (float t = 0; t < 1.0f; t += offset)
						{
							//ImVec2 p = mainPos + (linkPos - mainPos) * t;
							//ImVec2 p1 = mainPos + (linkPos - mainPos) * (t + offset);

							//ImVec2 p = ImVec2((std::pow(1 - t, 3) * mainPos.x + 3 * t * std::pow(1 - t, 2) * mainHandle.x + 3 * std::pow(t, 2) * (1 - t) * linkHandle.x + std::pow(t, 3) * linkPos.x), (std::pow(1 - t, 3) * mainPos.y + 3 * t * std::pow(1 - t, 2) * mainHandle.y + 3 * std::pow(t, 2) * (1 - t) * linkHandle.y + std::pow(t, 3) * linkPos.y));
							//ImVec2 p1 = ImVec2((std::pow(1 - (t + offset), 3) * mainPos.x + 3 * (t + offset) * std::pow(1 - (t + offset), 2) * mainHandle.x + 3 * std::pow((t + offset), 2) * (1 - (t + offset)) * linkHandle.x + std::pow((t + offset), 3) * linkPos.x), (std::pow(1 - (t + offset), 3) * mainPos.y + 3 * (t + offset) * std::pow(1 - (t + offset), 2) * mainHandle.y + 3 * std::pow((t + offset), 2) * (1 - (t + offset)) * linkHandle.y + std::pow((t + offset), 3) * linkPos.y));

							ImVec2 p = ImVec2((std::pow(1 - t, 3) * mainPos.x + 3 * t * std::pow(1 - t, 2) * mainHandle.x + 3 * std::pow(t, 2) * (1 - t) * linkHandle.x + std::pow(t, 3) * linkPos.x), (std::pow(1 - t, 3) * mainPos.y + 3 * t * std::pow(1 - t, 2) * mainHandle.y + 3 * std::pow(t, 2) * (1 - t) * linkHandle.y + std::pow(t, 3) * linkPos.y));
							ImVec2 p1 = ImVec2((std::pow(1 - (t + offset), 3) * mainPos.x + 3 * (t + offset) * std::pow(1 - (t + offset), 2) * mainHandle.x + 3 * std::pow((t + offset), 2) * (1 - (t + offset)) * linkHandle.x + std::pow((t + offset), 3) * linkPos.x), (std::pow(1 - (t + offset), 3) * mainPos.y + 3 * (t + offset) * std::pow(1 - (t + offset), 2) * mainHandle.y + 3 * std::pow((t + offset), 2) * (1 - (t + offset)) * linkHandle.y + std::pow((t + offset), 3) * linkPos.y));

							if (m_Points[i].curveType == Constant)
							{
								p = ImVec2((std::pow(1 - t, 3) * mainPos.x + 3 * t * std::pow(1 - t, 2) * mainHandle.x + 3 * std::pow(t, 2) * (1 - t) * linkHandle.x + std::pow(t, 3) * linkPos.x), ((t) >= 1.0f ? linkPos.y : mainPos.y));
								p1 = ImVec2((std::pow(1 - (t + offset), 3) * mainPos.x + 3 * (t + offset) * std::pow(1 - (t + offset), 2) * mainHandle.x + 3 * std::pow((t + offset), 2) * (1 - (t + offset)) * linkHandle.x + std::pow((t + offset), 3) * linkPos.x), ((t + offset) >= 1.0f ? linkPos.y : mainPos.y));
							}

							drawList->AddLine(p, p1, ImGui::ColorConvertFloat4ToU32(ImVec4(m_Channels[x].m_Color.x, m_Channels[x].m_Color.y, m_Channels[x].m_Color.z, 1.0f)), 2.0f);
						}
					}
				}
			}

			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("##GraphEditorProperties", ImVec2(panelC, 0));
			if (!m_SelectedPoints.empty())
			{
				for (int x = 0; x < m_Channels.size(); x++)
				{
					auto& m_Points = m_Channels[x].m_Points;
					for (int i = 0; i < m_Points.size(); i++)
					{
						if (m_Points[i].CurveID == m_SelectedPoints.back())
						{
							ImGui::Spacing();

							ImGui::Text(("Point_" + std::to_string(m_Points[i].CurveID)).c_str());
							ImGui::Text("Location X"); ImGui::SameLine();
							ImGui::DragFloat("##X", &m_Points[i].Position.x, 0.1f, 0.0f, 0.0f, "%.2f");
							ImGui::Text("Location Y"); ImGui::SameLine();
							ImGui::DragFloat("##Y", &m_Points[i].Position.y, 0.1f, 0.0f, 0.0f, "%.2f");


							ImGui::Text("Curve Type");
							ImGui::SameLine();

							const char* items[4] = { "Linear", "Constant", "Free", "Automatic" };

							int index = m_Points[i].curveType - 1;
							if (ImGui::Combo("##Curve Type", &index, items, 4, 20))
							{
								m_Points[i].curveType = (CurveType)(index + 1);
							}
						}
					}
				}
			}
			ImGui::EndChild();

			ImGui::End();

			if (m_Clicked && !anyClicked && !Input::IsKeyPressed(Key::LeftShift) && !Input::IsKeyPressed(Key::RightShift) && graphEditorHovered)
			{
				m_SelectedPoints.clear();
			}

			m_Clicked = false;
		}
	}

	std::string CurveEditor::ConvertCurveTypeToString(CurveType type)
	{
		if (type == None) { return "None"; }
		if (type == Linear) { return "Linear"; }
		if (type == Constant) { return "Constant"; }
		if (type == Free) { return "Free"; }
		if (type == Automatic) { return "Automatic"; }

		DY_CORE_ASSERT(false);
	}


}



