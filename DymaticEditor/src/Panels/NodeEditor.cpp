#include "NodeEditor.h"
#include "ImGuiCustom.h"

namespace Dymatic {

	NodeEditor::NodeEditor()
	{

	}

	void NodeEditor::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);

		dispatcher.Dispatch<MouseButtonPressedEvent>(DY_BIND_EVENT_FN(NodeEditor::OnMouseButtonPressed));
		dispatcher.Dispatch<MouseScrolledEvent>(DY_BIND_EVENT_FN(NodeEditor::OnMouseScrolled));
	}

	bool NodeEditor::OnKeyPressed(KeyPressedEvent& e)
	{
		return false;
	}

	bool NodeEditor::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		if (e.GetMouseButton() == Mouse::ButtonLeft && m_WorkspaceWindowHovered) { m_SelectedNode = m_HoveredNode; m_SelectedPin = m_HoveredPin; }
		return false;
	}

	bool NodeEditor::OnMouseScrolled(MouseScrolledEvent& e)
	{
		float delta = e.GetYOffset() * 0.1f;
		MouseZoom(delta);
		return false;
	}

	void NodeEditor::MouseZoom(float delta)
	{
		m_Zoom -= delta;//* ZoomSpeed();
		if (m_Zoom < 1.0f)
		{
			m_Zoom = 1.0f;
		}
	}

	void NodeEditor::OnImGuiRender()
	{

		ImGui::Begin("Node Editor");

		static float variation1 = 0.0f;
		static float variation2 = 0.0f;

		float h = ImGui::GetContentRegionAvail().y;
		float minSize = 20.0f;
		float sz1 = ImGui::GetContentRegionAvail().x / 5 * 4 - variation1;
		float sz2 = ImGui::GetContentRegionAvail().x / 5 * 1 - variation2;

		ImGui::Custom::Splitter(true, 2.0f, &sz1, &sz2, minSize, minSize, h);
		variation1 = (ImGui::GetContentRegionAvail().x / 5 * 4) - sz1;
		variation2 = (ImGui::GetContentRegionAvail().x / 5 * 1) - sz2;

		ImVec2 totalWindowSize = ImGui::GetWindowSize();

		ImGui::BeginChild("##NodeEditorWorkspace", ImVec2{sz1, h});

		m_WorkspaceWindowHovered = ImGui::IsWindowHovered();

		//float lineDistance = 60.0f / m_Zoom;
		float lineDistance = Math::RoundUp(55.0f / m_Zoom, 0.01f);

		ImVec4 gridColor{0.1f, 0.1f, 0.1f, 1.0f};
		float gridThickness = 1.0f;

		ImVec4 bkColor{ 0.14f, 0.14f, 0.14f, 1.0f };

		ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos(), ImGui::GetWindowPos() + totalWindowSize, ImGui::ColorConvertFloat4ToU32(bkColor));

		int numberOfLines = 0;
		while (lineDistance * numberOfLines + lineDistance < totalWindowSize.x)
		{
			auto pos = Math::NormalizeAngle(ImGui::GetWindowPos().x + lineDistance * numberOfLines - m_ViewPos.x, ImGui::GetWindowPos().x, ImGui::GetWindowPos().x * 0 + totalWindowSize.x);
			ImGui::GetWindowDrawList()->AddLine(ImVec2{ pos, ImGui::GetWindowPos().y }, ImVec2{ pos, ImGui::GetWindowPos().y + totalWindowSize.y }, ImGui::ColorConvertFloat4ToU32(gridColor), gridThickness);
			numberOfLines++;
		}

		numberOfLines = 0;
		while (lineDistance * numberOfLines + lineDistance < totalWindowSize.y)
		{
			auto pos = Math::NormalizeAngle(ImGui::GetWindowPos().y + lineDistance * numberOfLines - m_ViewPos.y, ImGui::GetWindowPos().y, ImGui::GetWindowPos().y * 0 + totalWindowSize.y);
			ImGui::GetWindowDrawList()->AddLine(ImVec2{ ImGui::GetWindowPos().x, pos }, ImVec2{ ImGui::GetWindowPos().x + totalWindowSize.x, pos }, ImGui::ColorConvertFloat4ToU32(gridColor), gridThickness);
			numberOfLines++;
		}


		if (Input::IsMouseButtonPressed(Mouse::ButtonMiddle))
		{
			if (m_PreviousMousePos == glm::vec2{ -1.0f, -1.0f }) { m_PreviousMousePos = Input::GetMousePosition(); }
			auto delta = Input::GetMousePosition() - m_PreviousMousePos;
			m_PreviousMousePos = Input::GetMousePosition();
			m_ViewPos -= ImVec2{ delta.x, delta.y } *ImVec2{ m_Zoom, m_Zoom };
		}
		else
		{
			m_PreviousMousePos = glm::vec2{ -1.0f, -1.0f };
		}

		static bool nodeDrag = false;
		static bool pinDrag = false;
		if (!Input::IsMouseButtonPressed(Mouse::ButtonLeft)) { nodeDrag = false; JoinPin(m_SelectedPin, m_HoveredPin); m_SelectedPin = {}; }

		m_HoveredNode = -1;
		m_HoveredPin = {};

		for (int i = 0; i < m_NodeList.size(); i++)
		{
			static float padding = 10.0f;
			static float pinBlockYOffset = 100.0f;
			static float pinDistance = 40.0f;
			static float pinRadius = 10.0f;
			static float pinLineThickness = 2.5f;

			auto size = ImVec2{ 270, 600 };


			//ImGui::GetWindowDrawList()->AddRectFilled(ConvertPosition(data.position), ConvertPosition(data.position + size), ImGui::ColorConvertFloat4ToU32(ImVec4{ 0.5f, 0.5f, 0.5f, 0.5f }), 10.0f);
			auto& data = m_NodeList[i];

			const ImRect bb(ConvertPosition(data.position - ImVec2{ padding, padding }), ConvertPosition(data.position + size + ImVec2{ padding, padding }));

			//std::string label = "NodeEditorNode" + std::to_string(data.UniqueID);
			const ImGuiID id = ImGuiID(data.UniqueID);


			ImGui::RenderNavHighlight(bb, id);
			if (m_SelectedNode == data.UniqueID)
			{
				ImGui::PushStyleColor(ImGuiCol_Border, ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f });
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
			}
			////ImGui::GetWindowDrawList()->AddRectFilled(ConvertPosition(data.position + ImVec2{ 5.0f, 5.0f }), ConvertPosition(data.position + size + ImVec2{5.0f, 5.0f}), ImGui::ColorConvertFloat4ToU32(ImVec4{ 1.0f, 1.0f, 1.0f, 0.75f }), 10.0f);
			ImGui::RenderFrame(bb.Min, bb.Max, ImGui::ColorConvertFloat4ToU32(data.colorEnabled ? ImVec4{ data.color.x, data.color.y, data.color.z, 0.5f } : ImVec4{ 0.5f, 0.5f, 0.5f, 0.5f }), true, 10.0f / m_Zoom);
			if (m_SelectedNode == data.UniqueID)
			{
				ImGui::PopStyleVar();
				ImGui::PopStyleColor();
			}

			ImGui::GetWindowDrawList()->AddText(ImGui::GetIO().Fonts->Fonts[2], 40.0f * (1 / m_Zoom), ConvertPosition(data.position), ImGui::ColorConvertFloat4ToU32(ImVec4{1.0f, 1.0f, 1.0f, 1.0f}), (data.label == "" ? data.name : data.label).c_str());


			for (int i = 0; i < data.Inputs.size() + data.Outputs.size(); i++)
			{
				bool inOut = i < data.Inputs.size();
				int index = inOut ? i : i - (data.Inputs.size());

				auto& input = inOut ? data.Inputs[index] : data.Outputs[index];
				ImVec4 color = GetIconColor(input.Type);

				auto drawPos = ImVec2{ input.Kind == Input ? data.position.x + pinRadius : data.position.x + size.x - pinRadius, data.position.y + index * pinDistance + pinBlockYOffset };
				
				input.position = drawPos;

				switch (GetIconType(input.Type))
				{
				case IconType::FlowIcon:
				{
					float xExtension = pinLineThickness * 2 + pinRadius / 2;
					//if (input.ConnectedNode == -1) ImGui::GetWindowDrawList()->AddTriangle(ConvertPosition(data.position + ImVec2{ -pinRadius, pinRadius } + ImVec2{ pinRadius, i * pinDistance + pinBlockYOffset }), ConvertPosition(data.position + ImVec2{ pinRadius, 0 } + ImVec2{ pinRadius, i * pinDistance + pinBlockYOffset }), ConvertPosition(data.position - ImVec2{ pinRadius, pinRadius } + ImVec2{ pinRadius, i * pinDistance + pinBlockYOffset }), ImGui::ColorConvertFloat4ToU32(color), 2.0f / m_Zoom);
					ImVec2 points[5]{ ConvertPosition(drawPos + ImVec2{-pinRadius + xExtension * 0.5f, pinRadius}), ConvertPosition(drawPos + ImVec2{ 0 + xExtension * 0.5f, pinRadius}), ConvertPosition(drawPos + ImVec2{pinRadius + xExtension * 0.5f, 0}), ConvertPosition(drawPos + ImVec2{0 + xExtension * 0.5f, -pinRadius}), ConvertPosition(drawPos + ImVec2{-pinRadius + xExtension * 0.5f, -pinRadius})};
					if (/*input.ConnectedNode*/-1 == -1) ImGui::GetWindowDrawList()->AddPolyline(points, 5, ImGui::ColorConvertFloat4ToU32(color), true, pinLineThickness / m_Zoom);
					break;
				}
				case IconType::CircleIcon:
				{
					if (/*input.ConnectedNode*/-1 == -1) ImGui::GetWindowDrawList()->AddCircle(ConvertPosition(drawPos), pinRadius / m_Zoom, ImGui::ColorConvertFloat4ToU32(color), 0, pinLineThickness / m_Zoom);
					else ImGui::GetWindowDrawList()->AddCircleFilled(ConvertPosition(drawPos), pinRadius / m_Zoom, ImGui::ColorConvertFloat4ToU32(color));
					ImVec2 points[3]{ ConvertPosition(drawPos + ImVec2{ pinRadius + pinLineThickness * 2, -pinRadius / 2 }), ConvertPosition(drawPos + ImVec2{ pinRadius + pinLineThickness * 2 + pinRadius / 2, 0 }), ConvertPosition(drawPos + ImVec2{ pinRadius + pinLineThickness * 2, pinRadius / 2 }) };
					ImGui::GetWindowDrawList()->AddConvexPolyFilled(points, 3, ImGui::ColorConvertFloat4ToU32(color));
					break;
				}
				}
				const ImRect pin_bb(ConvertPosition(drawPos - ImVec2{ pinRadius, pinRadius }), ConvertPosition(drawPos + ImVec2{ pinRadius, pinRadius }));
				const ImGuiID pin_id = ImGuiID(input.ID);
				bool pinHovered, pinHeld;
				bool pinPressed = ImGui::ButtonBehavior(pin_bb, pin_id, &pinHovered, &pinHeld);

				if (!Input::IsMouseButtonPressed(Mouse::ButtonLeft)) { input.ConnectToCursor = false; }

				if (pinHovered)
				{
					m_HoveredNode = data.UniqueID;
					m_HoveredPin = &input;
				}

				if (!input.Links.empty())
				{
					for (int y = 0; y < input.Links.size(); y++)
					{
						DrawNodeLine(ConvertPosition(input.position), ConvertPosition(input.Links[y]->position));
					}
				}

				
			}

			if (m_SelectedPin != nullptr) { DrawNodeLine(ConvertPosition(m_SelectedPin->position), ImGui::GetMousePos()); }

			bool hovered, held;
			bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
			if (hovered)
			{
				m_HoveredNode = data.UniqueID;
			}

			if (held && m_SelectedNode)
			{
				nodeDrag = true;
				m_NodeClickOffset = (data.position) - ConvertPosition(ImGui::GetMousePos() - ImGui::GetWindowPos());
				m_NodeClickOffset = ImGui::GetMousePos() - ConvertPosition(data.position);
			}

			if (nodeDrag && m_SelectedNode == data.UniqueID) { data.position = UnconvertPosition(ImGui::GetMousePos() - m_NodeClickOffset); }
		}
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("##NodeEditorProperties", ImVec2{sz2, h});

		
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

		if (m_SelectedNode != -1)
		{
			auto& data = m_NodeList[GetNodeIndexByID(m_SelectedNode)];
			ImGui::TextDisabled(("ID: #" + std::to_string(data.UniqueID)).c_str());
			if (ImGui::TreeNodeEx("Node", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text("Name:");
				ImGui::SameLine();

				char nameBuffer[256];
				memset(nameBuffer, 0, sizeof(nameBuffer));
				std::strncpy(nameBuffer, data.name.c_str(), sizeof(nameBuffer));
				if (ImGui::InputText("##NodeName", nameBuffer, sizeof(nameBuffer)))
				{
					data.name = std::string(nameBuffer);
				}

				ImGui::Text("Label:");
				ImGui::SameLine();

				char labelBuffer[256];
				memset(labelBuffer, 0, sizeof(labelBuffer));
				std::strncpy(labelBuffer, data.label.c_str(), sizeof(labelBuffer));
				if (ImGui::InputText("##NodeLabel", labelBuffer, sizeof(labelBuffer)))
				{
					data.label = std::string(labelBuffer);
				}

				ImGui::Text("Position");
				ImGui::SameLine();
				ImGui::DragFloat2("##NodePositionVec2Edit", (float*)&(data.position), 0.1f, 0.0f, 0.0f, "%.3f");

				bool colorOpen = ImGui::TreeNodeEx("Color", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding);
				ImGui::SameLine();
				ImGui::Custom::Checkbox("##NodeColorEnabledCheckbox", &data.colorEnabled);
				if (colorOpen)
				{
					ImGui::ColorEdit3("##NodeColorDisplayColorPicker", (float*)&(data.color));
					ImGui::TreePop();
				}

				ImGui::TreePop();
			}
			if (ImGui::TreeNodeEx("Inputs", treeNodeFlags))
			{
				ImGui::TreePop();
			}
			if (ImGui::TreeNodeEx("Outputs", treeNodeFlags))
			{
				ImGui::TreePop();
			}
		}

		ImGui::EndChild();

		ImGui::End();
	}

	ImVec2 NodeEditor::ConvertPosition(ImVec2 position)
	{
		return ImGui::GetWindowPos() + (position - m_ViewPos) * ImVec2 { 1 / m_Zoom, 1 / m_Zoom };
	}

	ImVec2 NodeEditor::UnconvertPosition(ImVec2 position)
	{
		return (position - ImGui::GetWindowPos()) / ImVec2{ 1 / m_Zoom, 1 / m_Zoom } + m_ViewPos;
	}

	int NodeEditor::GetNodeIndexByID(int id)
	{
		for (int i = 0; i < m_NodeList.size(); i++)
		{
			if (m_NodeList[i].UniqueID == id) { return i; }
		}
		return -1;
	}

	int NodeEditor::GetPinIndexByID(int id)
	{
		for (int i = 0; i < m_NodeList.size(); i++)
		{
			for (int x = 0; x < m_NodeList[i].Inputs.size(); x++)
			{
				if (m_NodeList[i].Inputs[x].ID == id) { return x; }
			}
			for (int x = 0; x < m_NodeList[i].Outputs.size(); x++)
			{
				if (m_NodeList[i].Inputs[x].ID == id) { return x; }
			}
			
		}
		return -1;
	}

	ImVec4 NodeEditor::GetIconColor(PinType type)
	{
		switch (type)
		{
		default:
		case PinType::Flow:			return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		case PinType::Bool:			return ImVec4(0.56f, 0.03f, 0.0f, 1.0f);
		case PinType::Int:			return ImVec4(0.13f, 0.89f, 0.67f, 1.0f);
		case PinType::Float:		return ImVec4(0.62f, 1.0f, 0.25f, 1.0f);
		case PinType::Float2:		return ImVec4(0.62f, 1.0f, 0.25f, 1.0f);
		case PinType::Float3:		return ImVec4(0.62f, 1.0f, 0.25f, 1.0f);
		case PinType::Float4:		return ImVec4(0.62f, 1.0f, 0.25f, 1.0f);
		case PinType::Vector:		return ImVec4(1.0f, 0.78f, 0.17f, 1.0f);
		case PinType::Transform:	return ImVec4(1.0f, 0.47f, 0.0f, 1.0f);
		case PinType::String:		return ImVec4(0.96f, 0.02f, 0.8f, 1.0f);
		case PinType::Rotator:		return ImVec4(0.6f, 0.7f, 0.97f, 1.0f);
		case PinType::LinearColor:	return ImVec4(0.0f, 0.36f, 0.84f, 1.0f);
		}
	}

	IconType NodeEditor::GetIconType(PinType type)
	{
		switch (type)
		{
		case PinType::Flow:			return IconType::FlowIcon;
		case PinType::Bool:			return IconType::CircleIcon;
		case PinType::Int:			return IconType::CircleIcon;
		case PinType::Float:		return IconType::CircleIcon;
		case PinType::Float2:		return IconType::CircleIcon;
		case PinType::Float3:		return IconType::CircleIcon;
		case PinType::Float4:		return IconType::CircleIcon;
		case PinType::Vector:		return IconType::CircleIcon;
		case PinType::Transform:	return IconType::CircleIcon;
		case PinType::String:		return IconType::CircleIcon;
		case PinType::Rotator:		return IconType::CircleIcon;
		case PinType::LinearColor:	return IconType::CircleIcon;
		}
	}

	void NodeEditor::DrawNodeLine(ImVec2 start, ImVec2 end)
	{
		ImGui::GetWindowDrawList()->AddLine(start, end, ImGui::ColorConvertFloat4ToU32(ImVec4{1.0f, 1.0f, 1.0f, 1.0f}), 4.0f);
	}

	void NodeEditor::JoinPin(Pin* a, Pin* b)
	{
		if (a != nullptr && b != nullptr)
		{
			//if (!DoesLinkExist(a, b) && a->ID != b->ID && m_NodeList[GetNodeIndexByID(a->node)].UniqueID != m_NodeList[GetNodeIndexByID(b->node)].UniqueID)
			{
				if (a->Kind != b->Kind && a->Type == b->Type)
				{
					if (a->oneLink) { a->Links.clear(); }
					if (b->oneLink) { b->Links.clear(); }
					a->Links.erase(a->Links.begin(), a->Links.end());
					b->Links.clear();
					a->Links.push_back(b);
					b->Links.push_back(a);
					DY_CORE_INFO("Link Esstablished: {0} --> {1}", a->ID, b->ID);
					
				}
			}
		}
	}

	bool NodeEditor::DoesLinkExist(Pin* a, Pin* b)
	{
		int linked = 0;

		for (Pin* pin : a->Links)
		{
			if (pin->ID == b->ID) { linked++; }
		}
		for (Pin* pin : b->Links)
		{
			if (pin->ID == a->ID) { linked++; }
		}
		return (linked > 1);
	}

	float NodeEditor::ZoomSpeed() const
	{
		float zoom = (1 / m_Zoom) * 1.0f;
		zoom = std::max(zoom, 0.0f);
		float speed = zoom * zoom;
		speed = std::min(speed, 100.0f); // max speed = 100
		return speed;
	}

}