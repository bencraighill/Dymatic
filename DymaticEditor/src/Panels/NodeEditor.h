#pragma once

#include "Dymatic.h"
#include "Dymatic/Core/Base.h"

#include "Dymatic/Math/Math.h"

//TODO: Maybe try and remove this include or put it in the src file. Could replace ImVec2 with glm::vec2
#include <imgui/imgui.h>

namespace Dymatic {

	enum PinType
	{
		Flow,
		Bool,
		Int,
		Float,
		Float2,
		Float3,
		Float4,
		Vector,
		Transform,
		String,
		LinearColor,
		Rotator
	};

	enum PinKind
	{
		Input,
		Output
	};

	enum NodeType
	{
		Program,
		Material
	};

	enum IconType
	{
		FlowIcon,
		CircleIcon
	};



	struct Node;

	struct Pin
	{
		ImVec2 position = ImVec2{ 0, 0 };

		int ID = -1;
		Node* node;
		std::string Name;
		PinType Type;
		PinKind Kind;

		bool ConnectToCursor = false;

		std::vector<Pin*> Links;

		bool oneLink;

		Pin(int id, std::string name, PinType type, Node* node, bool oneLink = false)
			: ID(id), node(node), Name(name), Type(type), Kind(PinKind::Input), oneLink(oneLink)
		{
		}
	};

	struct Node
	{
		std::string name = "Unknown Node";
		std::string label = "";
		int UniqueID = -1;
		ImVec2 position{ 0.0f, 0.0f };
		bool colorEnabled = false;
		ImVec4 color{ 1.0f, 1.0f, 1.0f, 1.0f };

		NodeType nodeType = NodeType::Program;

		std::vector<Pin> Inputs;
		std::vector<Pin> Outputs;
	};


	class NodeEditor
	{
	public:
		NodeEditor();
		void OnEvent(Event& e);

		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		bool OnMouseScrolled(MouseScrolledEvent& e);

		void MouseZoom(float delta);

		void OnImGuiRender();

		ImVec2 ConvertPosition(ImVec2 position);
		ImVec2 UnconvertPosition(ImVec2 position);

		int GetNodeIndexByID(int id);
		int GetPinIndexByID(int id);

		ImVec4 GetIconColor(PinType type);
		IconType GetIconType(PinType type);

		void DrawNodeLine(ImVec2 start, ImVec2 end);

		void JoinPin(Pin* a, Pin* b);

		bool DoesLinkExist(Pin* a, Pin* b);
	private:
		float ZoomSpeed() const;
	private:
		std::vector<Node> m_NodeList;
		ImVec2 m_ViewPos;
		ImVec2 m_NodeClickOffset;
		glm::vec2 m_PreviousMousePos;
		float m_Zoom = 1.0f;

		int m_SelectedNode = -1;
		int m_HoveredNode = -1;

		Pin* m_SelectedPin{};
		Pin* m_HoveredPin{};

		bool m_WorkspaceWindowHovered = false;
	};

}
