#include "NodeProgram.h"

#include <imgui/imgui.h>
#include "utilities/builders.h"
#include "utilities/widgets.h"

#include <ImGuiNode/imgui_node_editor.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_internal.h>

#include "../ImGuiCustom.h"

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <utility>

#include "Dymatic/Core/Base.h"
#include "Dymatic/Renderer/Texture.h"

#include <fstream>

#include "Dymatic/Math/Math.h"

#include "Dymatic/Core/Input.h"

static inline ImRect ImGui_GetItemRect()
{
    return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}

static inline ImRect ImRect_Expanded(const ImRect& rect, float x, float y)
{
    auto result = rect;
    result.Min.x -= x;
    result.Min.y -= y;
    result.Max.x += x;
    result.Max.y += y;
    return result;
}

namespace ed = ax::NodeEditor;
namespace util = ax::NodeEditor::Utilities;

using namespace ax;

using ax::Widgets::IconType;

static ed::EditorContext* m_Editor = nullptr;

//extern "C" __declspec(dllimport) short __stdcall GetAsyncKeyState(int vkey);
//extern "C" bool Debug_KeyPress(int vkey)
//{
//    static std::map<int, bool> state;
//    auto lastState = state[vkey];
//    state[vkey] = (GetAsyncKeyState(vkey) & 0x8000) != 0;
//    if (state[vkey] && !lastState)
//        return true;
//    else
//        return false;
//}

enum class PinType
{
    Flow,
    Bool,
    Int,
    Float,
    String,
    Object,
    Function,
    Delegate,
};

enum class PinKind
{
    Output,
    Input
};

enum class NodeFunction
{
    Node,
    Event,
    Function
};

enum class NodeType
{
    Blueprint,
    Simple,
    Tree,
    Comment,
    Houdini
};

struct TypeValue
{
    bool Bool = false;
    int Int = 0;
    float Float = 0.0f;
    std::string String = "";
};

struct Node;

struct Pin
{
	ed::PinId   ID;
	::Node* Node;
	std::string Name;
	PinType     Type;
	PinKind     Kind;
	TypeValue Value;

	Pin(int id, const char* name, PinType type) :
		ID(id), Node(nullptr), Name(name), Type(type), Kind(PinKind::Input)
	{
	}
};

struct Node
{
	ed::NodeId ID;
	std::string Name;
    std::string DisplayName = "";
	std::vector<Pin> Inputs;
	std::vector<Pin> Outputs;
	ImColor Color;
	NodeType Type;
	NodeFunction Function = NodeFunction::Event;
	ImVec2 Size;

	std::string State;
	std::string SavedState;

    bool AddOutputs = false;

	Node(int id, const char* name, ImColor color = ImColor(255, 255, 255)) :
		ID(id), Name(name), Color(color), Type(NodeType::Blueprint), Size(0, 0)
	{
	}
};

struct Link
{
    ed::LinkId ID;

    ed::PinId StartPinID;
    ed::PinId EndPinID;

    ImColor Color;

    Link(ed::LinkId id, ed::PinId startPinId, ed::PinId endPinId):
        ID(id), StartPinID(startPinId), EndPinID(endPinId), Color(255, 255, 255)
    {
    }
};

struct SpawnNodeStruct
{
    std::string variableName;
    PinType variableType;
};

struct Variable
{
    int ID;
    std::string Name;
    TypeValue Values;
    PinType Type;

    Variable(int id, std::string name, PinType type)
        : ID(id), Name(name), Type(type)
    {
    }
};

struct NodeIdLess
{
    bool operator()(const ed::NodeId& lhs, const ed::NodeId& rhs) const
    {
        return lhs.AsPointer() < rhs.AsPointer();
    }
};

class NodeEditorInternal;

struct SearchResultData
{
	std::string Name;
	Node* (NodeEditorInternal::*Function)(SpawnNodeStruct);
    SpawnNodeStruct spawnStruct = {};
};

struct SearchData
{
	std::string Name;
	std::vector<SearchData> LowerTree;
	std::vector<SearchResultData> Results;

	bool open = false;
	bool confirmed = true;

	SearchData* PushBackTree(std::string name)
	{
		SearchData newData;
		newData.Name = name;
		LowerTree.push_back(newData);
		return &LowerTree.back();
	}

    void PushBackResult(std::string name, Node* (NodeEditorInternal::* function)(SpawnNodeStruct), SpawnNodeStruct spawnStruct = {})
	{
		SearchResultData newData;
		newData.Name = name;
		newData.Function = function;
        newData.spawnStruct = spawnStruct;
		Results.push_back(newData);
	}

    void Clear()
    {
        LowerTree = {};
        Results = {};

		bool open = false;
		bool confirmed = true;
    }
};

struct ConversionReturn
{
	Node* (NodeEditorInternal::* function)(SpawnNodeStruct);
};

class NodeEditorInternal
{
public:
    //Touch
    int GetNextId();
    ed::LinkId GetNextLinkId();
    void TouchNode(ed::NodeId id);
    float GetTouchProgress(ed::NodeId id);
    void UpdateTouch();

    //Creation And Access
    Node* FindNode(ed::NodeId id);
    Link* FindLink(ed::LinkId id);
    std::vector<Link*> GetPinLinks(ed::PinId id);
    Pin* FindPin(ed::PinId id);
    bool IsPinLinked(ed::PinId id);
    bool CanCreateLink(Pin* a, Pin* b);
    void BuildNode(Node* node);

    //Nodes
    Node* SpawnOnCreateNode(SpawnNodeStruct in);
    Node* SpawnOnUpdateNode(SpawnNodeStruct in);
    //Make Literal Nodes
    Node* SpawnMakeLiteralBoolNode(SpawnNodeStruct in);
    Node* SpawnMakeLiteralIntNode(SpawnNodeStruct in);
    Node* SpawnMakeLiteralFloatNode(SpawnNodeStruct in);
    Node* SpawnMakeLiteralStringNode(SpawnNodeStruct in);

    Node* SpawnIntToBoolNode(SpawnNodeStruct in);
    Node* SpawnFloatToBoolNode(SpawnNodeStruct in);
    Node* SpawnStringToBoolNode(SpawnNodeStruct in);
    Node* SpawnBoolToFloatNode(SpawnNodeStruct in);
    Node* SpawnIntToFloatNode(SpawnNodeStruct in);
    Node* SpawnStringToFloatNode(SpawnNodeStruct in);
    Node* SpawnBoolToIntNode(SpawnNodeStruct in);
    Node* SpawnFloatToIntNode(SpawnNodeStruct in);
    Node* SpawnStringToIntNode(SpawnNodeStruct in);
    Node* SpawnBoolToStringNode(SpawnNodeStruct in);
    Node* SpawnIntToStringNode(SpawnNodeStruct in);
    Node* SpawnFloatToStringNode(SpawnNodeStruct in);

    Node* SpawnANDBooleanNode(SpawnNodeStruct in);
    Node* SpawnEqualBooleanNode(SpawnNodeStruct in);
    Node* SpawnNANDBooleanNode(SpawnNodeStruct in);
    Node* SpawnNORBooleanNode(SpawnNodeStruct in);
    Node* SpawnNOTBooleanNode(SpawnNodeStruct in);
    Node* SpawnNotEqualBooleanNode(SpawnNodeStruct in);
    Node* SpawnORBooleanNode(SpawnNodeStruct in);
    Node* SpawnXORBooleanNode(SpawnNodeStruct in);
    Node* SpawnModuloFloatNode(SpawnNodeStruct in);
    Node* SpawnAbsoluteFloatNode(SpawnNodeStruct in);
    Node* SpawnCeilNode(SpawnNodeStruct in);
    Node* SpawnClampFloatNode(SpawnNodeStruct in);
    Node* SpawnNormalizeAngleNode(SpawnNodeStruct in);
    Node* SpawnLerpAngleNode(SpawnNodeStruct in);
    Node* SpawnEqualFloatNode(SpawnNodeStruct in);
    Node* SpawnExpFloatNode(SpawnNodeStruct in);

    Node* SpawnInputActionNode(SpawnNodeStruct in);
    Node* SpawnBranchNode(SpawnNodeStruct in);
    Node* SpawnSequenceNode(SpawnNodeStruct in);
    Node* SpawnDoNNode(SpawnNodeStruct in);
    Node* SpawnOutputActionNode(SpawnNodeStruct in);
    Node* SpawnPrintStringNode(SpawnNodeStruct in);
    Node* SpawnMessageNode(SpawnNodeStruct in);
    Node* SpawnSetTimerNode(SpawnNodeStruct in);
    Node* SpawnLessNode(SpawnNodeStruct in);
    Node* SpawnWeirdNode(SpawnNodeStruct in);
    Node* SpawnTraceByChannelNode(SpawnNodeStruct in);
    Node* SpawnTreeSequenceNode(SpawnNodeStruct in);
    Node* SpawnTreeTaskNode(SpawnNodeStruct in);
    Node* SpawnTreeTask2Node(SpawnNodeStruct in);
    Node* SpawnComment(SpawnNodeStruct in);
    Node* SpawnHoudiniTransformNode(SpawnNodeStruct in);
    Node* SpawnHoudiniGroupNode(SpawnNodeStruct in);
    //User Custom Nodes
    Node* SpawnGetVariableNode(SpawnNodeStruct in);
    Node* SpawnSetVariableNode(SpawnNodeStruct in);

    ConversionReturn NodeEditorInternal::ConversionAvalible(Pin* A, Pin* B);
    void BuildNodes();
    const char* Application_GetName();
    ImColor GetIconColor(PinType type);
    void DrawPinIcon(const Pin& pin, bool connected, int alpha);

    void ShowStyleEditor(bool* show = nullptr);
    void ShowLeftPane(float paneWidth);

    void OnEvent(Dymatic::Event& e);
    bool OnKeyPressed(Dymatic::KeyPressedEvent& e);

    std::string ConvertPinTypeToString(PinType pinType, bool beginCaps = false);
    std::string ConvertPinValueToString(PinType type, TypeValue value);
    std::string UnderscoreSpaces(std::string inString);
    void CompileNodes();

    void CopyNodes();
    void PasteNodes();
    void DuplicateNodes();
    void DeleteNodes();

    std::string ToLower(std::string inString);
    bool DisplayResult(SearchResultData result, std::string searchBuffer);
    bool DisplaySubcatagories(SearchData searchData, std::string searchBuffer);

    void OpenSearchList(SearchData* searchData);
    void CloseSearchList(SearchData* searchData);
    Node* DisplaySearchData(SearchData* searchData, std::string searchBuffer, bool origin = true);

    void InitSearchData();
    void Application_Initialize();
    void Application_Finalize();
    void Application_Frame();


private:
	const int            m_PinIconSize = 24;
	std::vector<Node>    m_Nodes;
	std::vector<Link>    m_Links;
    std::vector<Variable> m_Variables;

    Dymatic::Ref<Dymatic::Texture2D> m_HeaderBackground = nullptr;
    ImTextureID          m_SampleImage = nullptr;
	Dymatic::Ref<Dymatic::Texture2D> m_SaveIcon = nullptr;
	Dymatic::Ref<Dymatic::Texture2D> m_RestoreIcon = nullptr;

	const float          m_TouchTime = 1.0f;
	std::map<ed::NodeId, float, NodeIdLess> m_NodeTouchTime;

    int m_NextId = 1;

    SearchData m_SearchData;

    bool ResetSearchArea = false;

    int m_SelectedVariable = -1;
};

static std::vector<NodeEditorInternal> EditorInternalStack;

int NodeEditorInternal::GetNextId()
{
    return m_NextId++;
}

ed::LinkId NodeEditorInternal::GetNextLinkId()
{
    return ed::LinkId(GetNextId());
}

void NodeEditorInternal::TouchNode(ed::NodeId id)
{
    m_NodeTouchTime[id] = m_TouchTime;
}

float NodeEditorInternal::GetTouchProgress(ed::NodeId id)
{
    auto it = m_NodeTouchTime.find(id);
    if (it != m_NodeTouchTime.end() && it->second > 0.0f)
        return (m_TouchTime - it->second) / m_TouchTime;
    else
        return 0.0f;
}

void NodeEditorInternal::UpdateTouch()
{
    const auto deltaTime = ImGui::GetIO().DeltaTime;
    for (auto& entry : m_NodeTouchTime)
    {
        if (entry.second > 0.0f)
            entry.second -= deltaTime;
    }
}

Node* NodeEditorInternal::FindNode(ed::NodeId id)
{
    for (auto& node : m_Nodes)
        if (node.ID == id)
            return &node;

    return nullptr;
}

Link* NodeEditorInternal::FindLink(ed::LinkId id)
{
    for (auto& link : m_Links)
        if (link.ID == id)
            return &link;

    return nullptr;
}

std::vector<Link*> NodeEditorInternal::GetPinLinks(ed::PinId id)
{
    std::vector<Link*> linksToReturn;
    for (auto& link : m_Links)
        if (link.StartPinID == id || link.EndPinID == id)
            linksToReturn.push_back(&link);

	return linksToReturn;
}

Pin* NodeEditorInternal::FindPin(ed::PinId id)
{
    if (!id)
        return nullptr;

    for (auto& node : m_Nodes)
    {
        for (auto& pin : node.Inputs)
            if (pin.ID == id)
                return &pin;

        for (auto& pin : node.Outputs)
            if (pin.ID == id)
                return &pin;
    }

    return nullptr;
}

bool NodeEditorInternal::IsPinLinked(ed::PinId id)
{
    if (!id)
        return false;

    for (auto& link : m_Links)
        if (link.StartPinID == id || link.EndPinID == id)
            return true;

    return false;
}

bool NodeEditorInternal::CanCreateLink(Pin* a, Pin* b)
{
    if (!a || !b || a == b || a->Kind == b->Kind || ((ConversionAvalible(a, b).function != nullptr) ? false : (a->Type != b->Type)) || a->Node == b->Node)
        return false;

    return true;
}

//static void DrawItemRect(ImColor color, float expand = 0.0f)
//{
//    ImGui::GetWindowDrawList()->AddRect(
//        ImGui::GetItemRectMin() - ImVec2(expand, expand),
//        ImGui::GetItemRectMax() + ImVec2(expand, expand),
//        color);
//};

//static void FillItemRect(ImColor color, float expand = 0.0f, float rounding = 0.0f)
//{
//    ImGui::GetWindowDrawList()->AddRectFilled(
//        ImGui::GetItemRectMin() - ImVec2(expand, expand),
//        ImGui::GetItemRectMax() + ImVec2(expand, expand),
//        color, rounding);
//};

void NodeEditorInternal::BuildNode(Node* node)
{
    for (auto& input : node->Inputs)
    {
        input.Node = node;
        input.Kind = PinKind::Input;
    }

    for (auto& output : node->Outputs)
    {
        output.Node = node;
        output.Kind = PinKind::Output;
    }
}

Node* NodeEditorInternal::SpawnOnCreateNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "On Create", ImColor(255, 128, 128));
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnOnUpdateNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "On Update", ImColor(255, 128, 128));
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Delta Seconds", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

//Make Literal Nodes
Node* NodeEditorInternal::SpawnMakeLiteralBoolNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Make Literal Bool", ImColor(255, 128, 128));
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Value", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnMakeLiteralIntNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Make Literal Int", ImColor(255, 128, 128));
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Value", PinType::Int);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::Int);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnMakeLiteralFloatNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Make Literal Float", ImColor(255, 128, 128));
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Value", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnMakeLiteralStringNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Make Literal String", ImColor(255, 128, 128));
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Value", PinType::String);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::String);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

//ConversionNodes

Node* NodeEditorInternal::SpawnIntToBoolNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Int To Bool", ImColor(255, 128, 128));
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Int);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnFloatToBoolNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Float To Bool", ImColor(255, 128, 128));
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnStringToBoolNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "String To Bool", ImColor(255, 128, 128));
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::String);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnBoolToFloatNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Bool To Float", ImColor(255, 128, 128));
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnIntToFloatNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Int To Float", ImColor(255, 128, 128));
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Int);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnStringToFloatNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "String To Float", ImColor(255, 128, 128));
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::String);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnBoolToIntNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Bool To Int", ImColor(255, 128, 128));
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Int);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnFloatToIntNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Float To Int", ImColor(255, 128, 128));
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Int);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnStringToIntNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "String To Int", ImColor(255, 128, 128));
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::String);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Int);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnBoolToStringNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Bool To String", ImColor(255, 128, 128));
    m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Bool);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::String);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnIntToStringNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Int To String", ImColor(255, 128, 128));
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Int);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::String);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnFloatToStringNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Float To String", ImColor(255, 128, 128));
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::String);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

//Operator nodes

//Bool Operators
Node* NodeEditorInternal::SpawnANDBooleanNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "AND Boolean", ImColor(255, 128, 128));
    m_Nodes.back().DisplayName = "AND";
	m_Nodes.back().Type = NodeType::Simple;
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Bool);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnEqualBooleanNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Equal Boolean", ImColor(255, 128, 128));
	m_Nodes.back().DisplayName = "==";
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Bool);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnNANDBooleanNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "NAND Boolean", ImColor(255, 128, 128));
	m_Nodes.back().DisplayName = "NAND";
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Bool);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnNORBooleanNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "NOR Boolean", ImColor(255, 128, 128));
	m_Nodes.back().DisplayName = "NOR";
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Bool);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnNOTBooleanNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "NOT Boolean", ImColor(255, 128, 128));
	m_Nodes.back().DisplayName = "NOT";
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnNotEqualBooleanNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Not Equal Boolean", ImColor(255, 128, 128));
	m_Nodes.back().DisplayName = "!=";
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Bool);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnORBooleanNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "OR Boolean", ImColor(255, 128, 128));
	m_Nodes.back().DisplayName = "OR";
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Bool);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnXORBooleanNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "XOR Boolean", ImColor(255, 128, 128));
	m_Nodes.back().DisplayName = "XOR";
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Bool);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

//Float Operators
Node* NodeEditorInternal::SpawnModuloFloatNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Modulo Float", ImColor(255, 128, 128));
	m_Nodes.back().DisplayName = "%";
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnAbsoluteFloatNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Absolute Float", ImColor(255, 128, 128));
	m_Nodes.back().DisplayName = "ABS";
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnCeilNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Ceil", ImColor(255, 128, 128));
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::Int);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnClampFloatNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Clamp Float", ImColor(255, 128, 128));
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Value", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Min", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Max", PinType::Float);
    m_Nodes.back().Inputs.back().Value.Float = 1.0f;
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnNormalizeAngleNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Normalize Angle", ImColor(255, 128, 128));
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Value", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnLerpAngleNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Lerp Angle", ImColor(255, 128, 128));
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Float);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Float);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "Alpha", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnEqualFloatNode(SpawnNodeStruct in)
{
    m_Nodes.emplace_back(GetNextId(), "Equal Float", ImColor(255, 128, 128));
    m_Nodes.back().DisplayName = "==";
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Float);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Float);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

    BuildNode(&m_Nodes.back());

    return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnExpFloatNode(SpawnNodeStruct in)
{
    m_Nodes.emplace_back(GetNextId(), "Exp Float", ImColor(255, 128, 128));
    m_Nodes.back().DisplayName = "e";
    m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}


//division (whole and remainder), all basic opperators, floor fraction, hypotenuse, in range, int * float, lerp, map range clamped and unclamped, max, min, max array, min array, multiply by pi, nearly equal, normalize to range power, search float for more, make sure to add the math/random library

Node* NodeEditorInternal::SpawnInputActionNode(SpawnNodeStruct in)
{
    m_Nodes.emplace_back(GetNextId(), "InputAction Fire", ImColor(255, 128, 128));
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Delegate);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "Pressed", PinType::Flow);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "Released", PinType::Flow);

    BuildNode(&m_Nodes.back());

    return &m_Nodes.back(); 
}

Node* NodeEditorInternal::SpawnBranchNode(SpawnNodeStruct in)
{
    m_Nodes.emplace_back(GetNextId(), "Branch");
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "Condition", PinType::Bool);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "True", PinType::Flow);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "False", PinType::Flow);

    BuildNode(&m_Nodes.back());

    return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnSequenceNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), "Sequence");
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Size", PinType::Int);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "0", PinType::Flow);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "1", PinType::Flow);
    m_Nodes.back().AddOutputs = true;

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnDoNNode(SpawnNodeStruct in)
{
    m_Nodes.emplace_back(GetNextId(), "Do N");
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "Enter", PinType::Flow);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "N", PinType::Int);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "Reset", PinType::Flow);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "Exit", PinType::Flow);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "Counter", PinType::Int);

    BuildNode(&m_Nodes.back());

    return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnOutputActionNode(SpawnNodeStruct in)
{
    m_Nodes.emplace_back(GetNextId(), "OutputAction");
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "Sample", PinType::Float);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "Condition", PinType::Bool);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "Event", PinType::Delegate);

    BuildNode(&m_Nodes.back());

    return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnPrintStringNode(SpawnNodeStruct in)
{
    m_Nodes.emplace_back(GetNextId(), "Print String");
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "In String", PinType::String);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

    BuildNode(&m_Nodes.back());

    return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnMessageNode(SpawnNodeStruct in)
{
    m_Nodes.emplace_back(GetNextId(), "", ImColor(128, 195, 248));
    m_Nodes.back().Type = NodeType::Simple;
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "Message", PinType::String);

    BuildNode(&m_Nodes.back());

    return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnSetTimerNode(SpawnNodeStruct in)
{
    m_Nodes.emplace_back(GetNextId(), "Set Timer", ImColor(128, 195, 248));
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "Object", PinType::Object);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "Function Name", PinType::Function);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "Time", PinType::Float);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "Looping", PinType::Bool);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

    BuildNode(&m_Nodes.back());

    return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnLessNode(SpawnNodeStruct in)
{
    m_Nodes.emplace_back(GetNextId(), "<", ImColor(128, 195, 248));
    m_Nodes.back().Type = NodeType::Simple;
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

    BuildNode(&m_Nodes.back());

    return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnWeirdNode(SpawnNodeStruct in)
{
    m_Nodes.emplace_back(GetNextId(), "o.O", ImColor(128, 195, 248));
    m_Nodes.back().Type = NodeType::Simple;
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

    BuildNode(&m_Nodes.back());

    return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnTraceByChannelNode(SpawnNodeStruct in)
{
    m_Nodes.emplace_back(GetNextId(), "Single Line Trace by Channel", ImColor(255, 128, 64));
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "Start", PinType::Flow);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "End", PinType::Int);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "Trace Channel", PinType::Float);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "Trace Complex", PinType::Bool);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "Actors to Ignore", PinType::Int);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "Draw Debug Type", PinType::Bool);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "Ignore Self", PinType::Bool);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "Out Hit", PinType::Float);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::Bool);

    BuildNode(&m_Nodes.back());

    return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnTreeSequenceNode(SpawnNodeStruct in)
{
    m_Nodes.emplace_back(GetNextId(), "Sequence");
    m_Nodes.back().Type = NodeType::Tree;
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

    BuildNode(&m_Nodes.back());

    return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnTreeTaskNode(SpawnNodeStruct in)
{
    m_Nodes.emplace_back(GetNextId(), "Move To");
    m_Nodes.back().Type = NodeType::Tree;
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);

    BuildNode(&m_Nodes.back());

    return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnTreeTask2Node(SpawnNodeStruct in)
{
    m_Nodes.emplace_back(GetNextId(), "Random Wait");
    m_Nodes.back().Type = NodeType::Tree;
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);

    BuildNode(&m_Nodes.back());

    return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnComment(SpawnNodeStruct in)
{
    m_Nodes.emplace_back(GetNextId(), "Test Comment");
    m_Nodes.back().Type = NodeType::Comment;
    m_Nodes.back().Size = ImVec2(300, 200);

    return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnHoudiniTransformNode(SpawnNodeStruct in)
{
    m_Nodes.emplace_back(GetNextId(), "Transform");
    m_Nodes.back().Type = NodeType::Houdini;
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

    BuildNode(&m_Nodes.back());

    return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnHoudiniGroupNode(SpawnNodeStruct in)
{
    m_Nodes.emplace_back(GetNextId(), "Group");
    m_Nodes.back().Type = NodeType::Houdini;
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

    BuildNode(&m_Nodes.back());

    return &m_Nodes.back();
}

//User Custom Nodes
Node* NodeEditorInternal::SpawnGetVariableNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), ("DymaticVariable_Get_" + in.variableName).c_str());
	m_Nodes.back().DisplayName = ("Get " + in.variableName).c_str();
    m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", in.variableType);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnSetVariableNode(SpawnNodeStruct in)
{
	m_Nodes.emplace_back(GetNextId(), ("DymaticVariable_Set_" + in.variableName).c_str());
    m_Nodes.back().DisplayName = ("Set " + in.variableName).c_str();
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", in.variableType);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", in.variableType);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

ConversionReturn NodeEditorInternal::ConversionAvalible(Pin* A, Pin* B)
{
    ConversionReturn returnValue = {};
	if (A->Kind == PinKind::Input)
	{
		std::swap(A, B);
	}

	auto a = A->Type;
	auto b = B->Type;

	if (a == PinType::Int && b == PinType::Bool) { returnValue.function = &NodeEditorInternal::SpawnIntToBoolNode; }
	else if (a == PinType::Float && b == PinType::Bool) { returnValue.function = &NodeEditorInternal::SpawnFloatToBoolNode; }
	else if (a == PinType::String && b == PinType::Bool) { returnValue.function = &NodeEditorInternal::SpawnStringToBoolNode; }
	else if (a == PinType::Bool && b == PinType::Int) { returnValue.function = &NodeEditorInternal::SpawnBoolToIntNode; }
    else if (a == PinType::Float && b == PinType::Int) { returnValue.function = &NodeEditorInternal::SpawnFloatToIntNode; }
    else if (a == PinType::String && b == PinType::Int) { returnValue.function = &NodeEditorInternal::SpawnStringToIntNode; }
	else if (a == PinType::Bool && b == PinType::Float) { returnValue.function = &NodeEditorInternal::SpawnBoolToFloatNode; }
	else if (a == PinType::Int && b == PinType::Float) { returnValue.function = &NodeEditorInternal::SpawnIntToFloatNode; }
	else if (a == PinType::String && b == PinType::Float) { returnValue.function = &NodeEditorInternal::SpawnStringToFloatNode; }
    else if (a == PinType::Bool && b == PinType::String) { returnValue.function = &NodeEditorInternal::SpawnBoolToStringNode; }
    else if (a == PinType::Int && b == PinType::String) { returnValue.function = &NodeEditorInternal::SpawnIntToStringNode; }
	else if (a == PinType::Float && b == PinType::String) { returnValue.function = &NodeEditorInternal::SpawnFloatToStringNode; }

	return returnValue;
}

void NodeEditorInternal::BuildNodes()
{
    for (auto& node : m_Nodes)
        BuildNode(&node);
}

const char* NodeEditorInternal::Application_GetName()
{
    return "Blueprints";
}

ImColor NodeEditorInternal::GetIconColor(PinType type)
{
    switch (type)
    {
        default:
        case PinType::Flow:     return ImColor(255, 255, 255);
        case PinType::Bool:     return ImColor(220,  48,  48);
        case PinType::Int:      return ImColor( 68, 201, 156);
        case PinType::Float:    return ImColor(147, 226,  74);
        case PinType::String:   return ImColor(218, 0, 183);
        case PinType::Object:   return ImColor( 51, 150, 215);
        case PinType::Function: return ImColor(218,   0, 183);
        case PinType::Delegate: return ImColor(255,  48,  48);
    }
};

void NodeEditorInternal::DrawPinIcon(const Pin& pin, bool connected, int alpha)
{
    IconType iconType;
    ImColor  color = GetIconColor(pin.Type);
    color.Value.w = alpha / 255.0f;
    switch (pin.Type)
    {
        case PinType::Flow:     iconType = IconType::Flow;   break;
        case PinType::Bool:     iconType = IconType::Circle; break;
        case PinType::Int:      iconType = IconType::Circle; break;
        case PinType::Float:    iconType = IconType::Circle; break;
        case PinType::String:   iconType = IconType::Circle; break;
        case PinType::Object:   iconType = IconType::Circle; break;
        case PinType::Function: iconType = IconType::Circle; break;
        case PinType::Delegate: iconType = IconType::Square; break;
        default:
            return;
    }

    ax::Widgets::Icon(ImVec2(m_PinIconSize, m_PinIconSize), iconType, connected, color, ImColor(32, 32, 32, alpha));
};

void NodeEditorInternal::ShowStyleEditor(bool* show)
{
    if (!ImGui::Begin("Style", show))
    {
        ImGui::End();
        return;
    }

    auto paneWidth = ImGui::GetContentRegionAvailWidth();

    auto& editorStyle = ed::GetStyle();
    ImGui::BeginHorizontal("Style buttons", ImVec2(paneWidth, 0), 1.0f);
    ImGui::TextUnformatted("Values");
    ImGui::Spring();
    if (ImGui::Button("Reset to defaults"))
        editorStyle = ed::Style();
    ImGui::EndHorizontal();
    ImGui::Spacing();
    ImGui::DragFloat4("Node Padding", &editorStyle.NodePadding.x, 0.1f, 0.0f, 40.0f);
    ImGui::DragFloat("Node Rounding", &editorStyle.NodeRounding, 0.1f, 0.0f, 40.0f);
    ImGui::DragFloat("Node Border Width", &editorStyle.NodeBorderWidth, 0.1f, 0.0f, 15.0f);
    ImGui::DragFloat("Hovered Node Border Width", &editorStyle.HoveredNodeBorderWidth, 0.1f, 0.0f, 15.0f);
    ImGui::DragFloat("Selected Node Border Width", &editorStyle.SelectedNodeBorderWidth, 0.1f, 0.0f, 15.0f);
    ImGui::DragFloat("Pin Rounding", &editorStyle.PinRounding, 0.1f, 0.0f, 40.0f);
    ImGui::DragFloat("Pin Border Width", &editorStyle.PinBorderWidth, 0.1f, 0.0f, 15.0f);
    ImGui::DragFloat("Link Strength", &editorStyle.LinkStrength, 1.0f, 0.0f, 500.0f);
    //ImVec2  SourceDirection;
    //ImVec2  TargetDirection;
    ImGui::DragFloat("Scroll Duration", &editorStyle.ScrollDuration, 0.001f, 0.0f, 2.0f);
    ImGui::DragFloat("Flow Marker Distance", &editorStyle.FlowMarkerDistance, 1.0f, 1.0f, 200.0f);
    ImGui::DragFloat("Flow Speed", &editorStyle.FlowSpeed, 1.0f, 1.0f, 2000.0f);
    ImGui::DragFloat("Flow Duration", &editorStyle.FlowDuration, 0.001f, 0.0f, 5.0f);
    //ImVec2  PivotAlignment;
    //ImVec2  PivotSize;
    //ImVec2  PivotScale;
    //float   PinCorners;
    //float   PinRadius;
    //float   PinArrowSize;
    //float   PinArrowWidth;
    ImGui::DragFloat("Group Rounding", &editorStyle.GroupRounding, 0.1f, 0.0f, 40.0f);
    ImGui::DragFloat("Group Border Width", &editorStyle.GroupBorderWidth, 0.1f, 0.0f, 15.0f);

    ImGui::Separator();

    static ImGuiColorEditFlags edit_mode = ImGuiColorEditFlags_RGB;
    ImGui::BeginHorizontal("Color Mode", ImVec2(paneWidth, 0), 1.0f);
    ImGui::TextUnformatted("Filter Colors");
    ImGui::Spring();
    ImGui::RadioButton("RGB", &edit_mode, ImGuiColorEditFlags_RGB);
    ImGui::Spring(0);
    ImGui::RadioButton("HSV", &edit_mode, ImGuiColorEditFlags_HSV);
    ImGui::Spring(0);
    ImGui::RadioButton("HEX", &edit_mode, ImGuiColorEditFlags_HEX);
    ImGui::EndHorizontal();

    static ImGuiTextFilter filter;
    filter.Draw("", paneWidth);

    ImGui::Spacing();

    ImGui::PushItemWidth(-160);
    for (int i = 0; i < ed::StyleColor_Count; ++i)
    {
        auto name = ed::GetStyleColorName((ed::StyleColor)i);
        if (!filter.PassFilter(name))
            continue;

        ImGui::ColorEdit4(name, &editorStyle.Colors[i].x, edit_mode);
    }
    ImGui::PopItemWidth();

    ImGui::End();
}

void NodeEditorInternal::ShowLeftPane(float paneWidth)
{
    auto& io = ImGui::GetIO();

    ImGui::BeginChild("Selection", ImVec2(paneWidth, 0));

    paneWidth = ImGui::GetContentRegionAvailWidth();

    static bool showStyleEditor = false;
    ImGui::BeginHorizontal("Style Editor", ImVec2(paneWidth, 0));
    ImGui::Spring(0.0f, 0.0f);
    if (ImGui::Button("Zoom to Content"))
        ed::NavigateToContent();
    ImGui::Spring(0.0f);
    if (ImGui::Button("Show Flow"))
    {
        for (auto& link : m_Links)
            ed::Flow(link.ID);
    }
    ImGui::Spring();
    if (ImGui::Button("Edit Style"))
        showStyleEditor = true;
    ImGui::EndHorizontal();

    if (showStyleEditor)
        ShowStyleEditor(&showStyleEditor);

    std::vector<ed::NodeId> selectedNodes;
    std::vector<ed::LinkId> selectedLinks;
    selectedNodes.resize(ed::GetSelectedObjectCount());
    selectedLinks.resize(ed::GetSelectedObjectCount());

    int nodeCount = ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
    int linkCount = ed::GetSelectedLinks(selectedLinks.data(), static_cast<int>(selectedLinks.size()));

    selectedNodes.resize(nodeCount);
    selectedLinks.resize(linkCount);

    int saveIconWidth     = m_SaveIcon->GetWidth();
    int saveIconHeight    = m_SaveIcon->GetHeight();
    int restoreIconWidth = m_RestoreIcon->GetWidth();
    int restoreIconHeight = m_RestoreIcon->GetHeight();

    ImGui::GetWindowDrawList()->AddRectFilled(
        ImGui::GetCursorScreenPos(),
        ImGui::GetCursorScreenPos() + ImVec2(paneWidth, ImGui::GetTextLineHeight()),
        ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), ImGui::GetTextLineHeight() * 0.25f);
    ImGui::Spacing(); ImGui::SameLine();
    ImGui::TextUnformatted("Nodes");
    ImGui::Indent();
    for (auto& node : m_Nodes)
    {
        ImGui::PushID(node.ID.AsPointer());
        auto start = ImGui::GetCursorScreenPos();

        if (const auto progress = GetTouchProgress(node.ID))
        {
            ImGui::GetWindowDrawList()->AddLine(
                start + ImVec2(-8, 0),
                start + ImVec2(-8, ImGui::GetTextLineHeight()),
                IM_COL32(255, 0, 0, 255 - (int)(255 * progress)), 4.0f);
        }

        bool isSelected = std::find(selectedNodes.begin(), selectedNodes.end(), node.ID) != selectedNodes.end();
        if (ImGui::Selectable((node.Name + "##" + std::to_string(reinterpret_cast<uintptr_t>(node.ID.AsPointer()))).c_str(), &isSelected))
        {
            if (io.KeyCtrl)
            {
                if (isSelected)
                    ed::SelectNode(node.ID, true);
                else
                    ed::DeselectNode(node.ID);
            }
            else
                ed::SelectNode(node.ID, false);

            ed::NavigateToSelection();
        }
        if (ImGui::IsItemHovered() && !node.State.empty())
            ImGui::SetTooltip("State: %s", node.State.c_str());

        auto id = std::string("(") + std::to_string(reinterpret_cast<uintptr_t>(node.ID.AsPointer())) + ")";
        auto textSize = ImGui::CalcTextSize(id.c_str(), nullptr);
        auto iconPanelPos = start + ImVec2(
            paneWidth - ImGui::GetStyle().FramePadding.x - ImGui::GetStyle().IndentSpacing - saveIconWidth - restoreIconWidth - ImGui::GetStyle().ItemInnerSpacing.x * 1,
            (ImGui::GetTextLineHeight() - saveIconHeight) / 2);
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(iconPanelPos.x - textSize.x - ImGui::GetStyle().ItemInnerSpacing.x, start.y),
            IM_COL32(255, 255, 255, 255), id.c_str(), nullptr);

        auto drawList = ImGui::GetWindowDrawList();
        ImGui::SetCursorScreenPos(iconPanelPos);
        ImGui::SetItemAllowOverlap();
        if (node.SavedState.empty())
        {
            if (ImGui::InvisibleButton("save", ImVec2((float)saveIconWidth, (float)saveIconHeight)))
                node.SavedState = node.State;

            if (ImGui::IsItemActive())
                drawList->AddImage(reinterpret_cast<void*>(m_SaveIcon->GetRendererID()), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
            else if (ImGui::IsItemHovered())
                drawList->AddImage(reinterpret_cast<void*>(m_SaveIcon->GetRendererID()), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
            else
                drawList->AddImage(reinterpret_cast<void*>(m_SaveIcon->GetRendererID()), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
        }
        else
        {
            ImGui::Dummy(ImVec2((float)saveIconWidth, (float)saveIconHeight));
            drawList->AddImage(reinterpret_cast<void*>(m_SaveIcon->GetRendererID()), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
        }

        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::SetItemAllowOverlap();
        if (!node.SavedState.empty())
        {
            if (ImGui::InvisibleButton("restore", ImVec2((float)restoreIconWidth, (float)restoreIconHeight)))
            {
                node.State = node.SavedState;
                ed::RestoreNodeState(node.ID);
                node.SavedState.clear();
            }

            if (ImGui::IsItemActive())
                drawList->AddImage(reinterpret_cast<void*>(m_RestoreIcon->GetRendererID()), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
            else if (ImGui::IsItemHovered())
                drawList->AddImage(reinterpret_cast<void*>(m_RestoreIcon->GetRendererID()), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
            else
                drawList->AddImage(reinterpret_cast<void*>(m_RestoreIcon->GetRendererID()), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
        }
        else
        {
            ImGui::Dummy(ImVec2((float)restoreIconWidth, (float)restoreIconHeight));
            drawList->AddImage(reinterpret_cast<void*>(m_RestoreIcon->GetRendererID()), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
        }

        ImGui::SameLine(0, 0);
        ImGui::SetItemAllowOverlap();
        ImGui::Dummy(ImVec2(0, (float)restoreIconHeight));

        ImGui::PopID();
    }
    ImGui::Unindent();

    static int changeCount = 0;

    ImGui::GetWindowDrawList()->AddRectFilled(
        ImGui::GetCursorScreenPos(),
        ImGui::GetCursorScreenPos() + ImVec2(paneWidth, ImGui::GetTextLineHeight()),
        ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), ImGui::GetTextLineHeight() * 0.25f);
    ImGui::Spacing(); ImGui::SameLine();
    ImGui::TextUnformatted("Selection");

    ImGui::BeginHorizontal("Selection Stats", ImVec2(paneWidth, 0));
    ImGui::Text("Changed %d time%s", changeCount, changeCount > 1 ? "s" : "");
    ImGui::Spring();
    if (ImGui::Button("Deselect All"))
        ed::ClearSelection();
    ImGui::EndHorizontal();
    ImGui::Indent();
    for (int i = 0; i < nodeCount; ++i) ImGui::Text("Node (%p)", selectedNodes[i].AsPointer());
    for (int i = 0; i < linkCount; ++i) ImGui::Text("Link (%p)", selectedLinks[i].AsPointer());
    ImGui::Unindent();

    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z)))
        for (auto& link : m_Links)
            ed::Flow(link.ID);

    if (ed::HasSelectionChanged())
        ++changeCount;

    ImGui::EndChild();
}

void NodeEditorInternal::OnEvent(Dymatic::Event& e)
{
    Dymatic::EventDispatcher dispatcher(e);

	dispatcher.Dispatch<Dymatic::KeyPressedEvent>(DY_BIND_EVENT_FN(NodeEditorInternal::OnKeyPressed));
}

bool NodeEditorInternal::OnKeyPressed(Dymatic::KeyPressedEvent& e)
{
    return false;
}

std::string NodeEditorInternal::ConvertPinTypeToString(PinType pinType, bool beginCaps)
{
    switch (pinType)
    {
    case PinType::Bool:     return beginCaps ? "Bool" : "bool";
    case PinType::Int:      return beginCaps ? "Int" : "int";
    case PinType::Float:    return beginCaps ? "Float" : "float";
    case PinType::String:   return beginCaps ? "String" : "string";
    }
    DY_CORE_ASSERT("No node type conversion to string found");
}

std::string NodeEditorInternal::ConvertPinValueToString(PinType type, TypeValue value)
{
	switch (type)
	{
	case PinType::Bool: {return value.Bool ? "true" : "false";}
	case PinType::Int: {return std::to_string(value.Int); }
	case PinType::Float: {return (std::to_string(value.Float) + "f"); }
	case PinType::String: {return "\"" + value.String + "\""; }
	}
    DY_CORE_ASSERT("No node value conversion to string found");
}

std::string NodeEditorInternal::UnderscoreSpaces(std::string inString)
{   
    std::string String = inString;
    std::string outString = "";
    for (int i = 0; i < String.size(); i++)
    {
        std::string character = String.substr(i, 1);

		if (character == " ") { outString += "_"; }
		else if (character == "_") { outString += "_"; }
		else if (character == ".") { outString += "_Full_Stop_46"; }
		else if (character == "!") { outString += "_Exclamation_Mark_33_"; }
		else if (character == "@") { outString += "_At_Sign_64_"; }
		else if (character == "#") { outString += "_Hashtag_35_"; }
		else if (character == "$") { outString += "_Dollar_Sign_36_"; }
		else if (character == "%") { outString += "_Percent_37_"; }
		else if (character == "^") { outString += "_Circumflex_Accent_94_"; }
		else if (character == "&") { outString += "_Ampersand_38_"; }
		else if (character == "*") { outString += "_Asterisk_42_"; }
		else if (character == "(") { outString += "_Left_Parenthesis_40_"; }
		else if (character == ")") { outString += "_Right_Parenthesis_41_"; }
		else if (character == "-") { outString += "_Hyphen_Minus_55_"; }
		else if (character == "+") { outString += "_Plus_Sign_43_"; }
		else if (character == "=") { outString += "Equal_Sign_61_"; }
		else if (character == "/") { outString += "_Slash_47_"; }
		else if (character == "\\") { outString += "_Backslash_92_"; }
		else if (character == "?") { outString += "_Question_Mark_63_"; }
		else if (character == "\"") { outString += "_Quotation_Mark_34_"; }
		else if (character == "[") { outString += "_Left_Square_Bracket_91_"; }
		else if (character == "]") { outString += "_Right_Square_Bracket_93_"; }
		else if (character == "{") { outString += "_Left_Curly_Bracket_123_"; }
		else if (character == "}") { outString += "_Right_Curly_Bracket_125_"; }
		else if (character == ":") { outString += "_Colon_58_"; }
		else if (character == ";") { outString += "_Semicolon_59_"; }
		else if (character == "\'") { outString += "_Apostrophe_39_"; }
		else if (character == "<") { outString += "_Less_Than_Sign_60_"; }
		else if (character == ">") { outString += "_Greater_Than_Sign_62_"; }
		else if (character == ",") { outString += "_Comma_44_"; }
		else if (character == "~") { outString += "_Tilde_126_"; }
		else if (character == "`") { outString += "_Grave_Accent_96_"; }
		else if (character == "|") { outString += "_Vertical_Bar_124_"; }
        
        else if (!isdigit(character[0]) && !isalpha(character[0])) { outString += "_UnknownSpecialCharacter_00_"; }

        else { outString += character; }
    }
    return outString;
}

void NodeEditorInternal::CompileNodes()
{
	for (auto& link : m_Links)
	{
		ed::Flow(link.ID);
	}

    std::string NodeScriptName = "BlueprintClass";

    std::string out = "";

    out += "#pragma once\r";
    out += "//Dymatic C++ Include Script\r\r";
   
    out += "#include \"DymaticNodeLibrary.h\"\r";
    out += "#include \"Dymatic/Scene/ScriptableEntity.h\"\r\r";

    out += "class " + NodeScriptName +" : public ScriptableEntity\r{\r";

    out += "public:\r";

    out += "//Variables\r";
    for (int i = 0; i < m_Variables.size(); i++)
    {
        out += ConvertPinTypeToString(m_Variables[i].Type) + " DymaticNodeVariable_" + UnderscoreSpaces(m_Variables[i].Name) + " = " + ConvertPinValueToString(m_Variables[i].Type, m_Variables[i].Values) + ";\r";
    }

    out += "\r//Custom Node Library Additions\r";
    for (int i = 0; i < m_Variables.size(); i++)
    {
		out += "FunctionReturn DymaticVariable_Get_" + UnderscoreSpaces(m_Variables[i].Name) + "(FunctionReturn functionIn)\r{";
		out += "FunctionReturn returnVal;\r";
		out += "returnVal.PinValues.push_back({});\r";
		out += "returnVal.PinValues[0]." + ConvertPinTypeToString(m_Variables[i].Type, true) + " = DymaticNodeVariable_" + UnderscoreSpaces(m_Variables[i].Name) + ";\r";
		out += "return returnVal;\r";
		out += "}\r\r";

        out += "FunctionReturn DymaticVariable_Set_" + UnderscoreSpaces(m_Variables[i].Name) + "(FunctionReturn functionIn)\r{";
        out += "FunctionReturn returnVal;\r";
        out += "returnVal.PinValues.push_back({});\r";
        out += "returnVal.PinValues.push_back({});\r";
        out += "DymaticNodeVariable_" + UnderscoreSpaces(m_Variables[i].Name) + " = functionIn.PinValues[1]." + ConvertPinTypeToString(m_Variables[i].Type, true) + ";\r";
        out += "returnVal.PinValues[1]." + ConvertPinTypeToString(m_Variables[i].Type, true) + " = DymaticNodeVariable_" + UnderscoreSpaces(m_Variables[i].Name) + ";\r";
        out += "return returnVal;\r";
        out += "}\r\r";
    }

    out += "\r//Function Save States\r";

    for (int i = 0; i < m_Nodes.size(); i++)
    {
        out += "FunctionReturn " + UnderscoreSpaces("s_NodeEditorFunctionSave_" + m_Nodes[i].Name + "_" + std::to_string(m_Nodes[i].ID.Get())) + ";\r";
    }

    for (int i = 0; i < m_Nodes.size(); i++)
    {
        out += "FunctionReturn NodeEvent_" + UnderscoreSpaces(m_Nodes[i].Name + "_" + std::to_string(m_Nodes[i].ID.Get())) + " ();\r\r";
    }

    out += "virtual void OnCreate() override\r{\r";
    for (int i = 0; i < m_Nodes.size(); i++)
    {
        if (m_Nodes[i].Name == "On Create")
        {
            out += "NodeEvent_" + UnderscoreSpaces(m_Nodes[i].Name + "_" + std::to_string(m_Nodes[i].ID.Get())) + "();\r";
        }
    }
    out += "}\r\r";

	out += "virtual void OnUpdate(Timestep ts) override\r{\r";
	for (int i = 0; i < m_Nodes.size(); i++)
	{
		if (m_Nodes[i].Name == "On Update")
		{
			out += "NodeEvent_" + UnderscoreSpaces(m_Nodes[i].Name + "_" + std::to_string(m_Nodes[i].ID.Get())) + "();\r";
		}
	}
	out += "}\r\r";

    out += "};";

    out += "\r\r";

    for (int i = 0; i < m_Nodes.size(); i++)
    {
        out += "FunctionReturn " + NodeScriptName + "::NodeEvent_" + UnderscoreSpaces(m_Nodes[i].Name + "_" + std::to_string(m_Nodes[i].ID.Get())) + " ()\r{\r";

        Node* node = &m_Nodes[i];

		std::vector<Pin*> ExecutablePins;
		for (int x = 0; x < node->Outputs.size(); x++)
		{
			if (node->Outputs[x].Type == PinType::Flow)
			{
				ExecutablePins.push_back(&node->Outputs[x]);
			}
		}

		//int* o = reinterpret_cast<int*>(node);
		//NodeCompilerLoopBackOutput CompilerLoopOutput = CreateLoopBackVariables(o);
		//out += CompilerLoopOutput.Declarations + CompilerLoopOutput.Definitions + "\r\r\r\r";

		for (int i = 0; i < node->Inputs.size(); i++)
		{
			out += UnderscoreSpaces(/*"NodeInputReturnCalculationVariable_"*/"s_NodeEditorFunctionSave_" + node->Name + "_" + std::to_string(node->ID.Get())) + ".PinValues.push_back({});\r";
		}

        //Node Defs

		for (int y = 0; y < node->Inputs.size(); y++)
		{
			if (node->Inputs[y].Type != PinType::Flow)
			{
                std::string PinValueName = ConvertPinTypeToString(node->Inputs[y].Type, true);
                

				if (!IsPinLinked(node->Inputs[y].ID))
				{
					std::string OutDefaultValue = ConvertPinValueToString(node->Inputs[y].Type, node->Inputs[y].Value);

					out += UnderscoreSpaces(/*"NodeInputReturnCalculationVariable_"*/"s_NodeEditorFunctionSave_" + node->Name + "_" + std::to_string(node->ID.Get())) + ".PinValues[" + std::to_string(y) + "]." + PinValueName + " = " + OutDefaultValue + ";\r";
				}

				else
				{
					Pin* tempPin = FindPin(GetPinLinks(node->Inputs[y].ID)[0]->StartPinID);
					Node* tempNode = tempPin->Node;
					int PinIndex = -1;
					for (int x = 0; x < tempNode->Outputs.size(); x++)
					{
						if (tempNode->Outputs[x].ID == tempPin->ID)
						{
							PinIndex = x;
						}
					}

					int execCount = 0;
					for (Pin pin : tempNode->Inputs)
					{
						if (pin.Type == PinType::Flow) { execCount++; }
					}
					for (Pin pin : tempNode->Outputs)
					{
						if (pin.Type == PinType::Flow) { execCount++; }
					}

					if (execCount < 1)
					{
						//out += UnderscoreSpaces(/*"NodeInputReturnCalculationVariable_"*/"s_NodeEditorFunctionSave_" + node->Name + "_" + std::to_string(node->ID.Get())) + ".PinValues[" + std::to_string(i) + "]." + PinValueName + " = " + UnderscoreSpaces(tempNode->Name) + "(" + UnderscoreSpaces(/*"NodeInputReturnCalculationVariable_"*/"s_NodeEditorFunctionSave_" + tempNode->Name + "_" + std::to_string(tempNode->ID.Get())) + ").PinValues[" + std::to_string(PinIndex) + "]" + "." + PinValueName + ";\r";
						out += UnderscoreSpaces("s_NodeEditorFunctionSave_" + node->Name + "_" + std::to_string(node->ID.Get())) + ".PinValues[" + std::to_string(y) + "]." + PinValueName + " = " + "NodeEvent_" + UnderscoreSpaces(tempNode->Name + "_" + std::to_string(tempNode->ID.Get())) + "().PinValues[" + std::to_string(PinIndex) + "]" + "." + PinValueName + ";\r";
					}
					else
					{
						out += UnderscoreSpaces(/*"NodeInputReturnCalculationVariable_"*/"s_NodeEditorFunctionSave_" + node->Name + "_" + std::to_string(node->ID.Get())) + ".PinValues[" + std::to_string(y) + "]." + PinValueName + " = " + UnderscoreSpaces("s_NodeEditorFunctionSave_" + tempNode->Name + "_" + std::to_string(tempNode->ID.Get())) + ".PinValues[" + std::to_string(PinIndex) + "]" + "." + PinValueName + ";\r";
					}
				}
			}
		}

		int execCount = 0;
		for (Pin pin : node->Inputs)
		{
			if (pin.Type == PinType::Flow) { execCount++; }
		}
		for (Pin pin : node->Outputs)
		{
			if (pin.Type == PinType::Flow) { execCount++; }
		}

		{
			if (execCount < 1)
			{
				out += UnderscoreSpaces("s_NodeEditorFunctionSave_" + node->Name + "_" + std::to_string(node->ID.Get())) + " = " + UnderscoreSpaces(node->Name) + "(" + UnderscoreSpaces(/*"NodeInputReturnCalculationVariable_"*/"s_NodeEditorFunctionSave_" + node->Name + "_" + std::to_string(node->ID.Get())) + ");\r";
			}
			else
			{
				//out += "switch ((" + UnderscoreSpaces("s_NodeEditorFunctionSave_" + node->Name + "_" + std::to_string(node->ID.Get())) + " = " + UnderscoreSpaces(node->Name) + "(" + UnderscoreSpaces(/*"NodeInputReturnCalculationVariable_"*/"s_NodeEditorFunctionSave_" + node->Name + "_" + std::to_string(node->ID.Get())) + ")).Executable){\r";
                out += "for (int index : (" + UnderscoreSpaces("s_NodeEditorFunctionSave_" + node->Name + "_" + std::to_string(node->ID.Get())) + " = " + UnderscoreSpaces(node->Name) + "(" + UnderscoreSpaces(/*"NodeInputReturnCalculationVariable_"*/"s_NodeEditorFunctionSave_" + node->Name + "_" + std::to_string(node->ID.Get())) + ")).Executable){";
                out += "switch (index){\r";
			}
			//out += "switch (" + UnderscoreSpaces("s_NodeEditorFunctionSave_" + node->Name + "_" + std::to_string(node->ID.Get())) + " = " + UnderscoreSpaces(node->Name) + "(funcUpload).Executable)\r{\r";

			for (int x = 0; x < ExecutablePins.size(); x++)
			{
				if (IsPinLinked(ExecutablePins[x]->ID))
				{
					for (int y = 0; y < GetPinLinks(ExecutablePins[x]->ID).size(); y++)
					{
                        Node* o = FindPin(GetPinLinks(ExecutablePins[x]->ID)[y]->EndPinID)->Node;
						out += "case " + std::to_string(x) + ": { " + "NodeEvent_" + UnderscoreSpaces(o->Name + "_" + std::to_string(o->ID.Get())) + "(); break; }\r";
					}
				}
			}

			if (!(execCount < 1))
			{
				out += "};\r};\r\r";
			}

            out += "return " + UnderscoreSpaces("s_NodeEditorFunctionSave_" + node->Name + "_" + std::to_string(node->ID.Get())) + ";\r";
		}

        out += "\r}\r";
    }

	//for (int i = 0; i < m_Nodes.size(); i++)
	//{
	//    auto data = m_Nodes[i];
	//
	//    if (data.Function == NodeFunction::Function)
	//    {
	//        out += "FunctionReturn " + UnderscoreSpaces(data.Name) + "(";
	//
	//        for (int x = 0; x < data.Inputs.size(); x++)
	//        {
	//            auto input = data.Inputs[x];
	//            if (input.Type == PinType::Bool || input.Type == PinType::Int || input.Type == PinType::Float || input.Type == PinType::String)
	//            {
	//                if (input.Type != PinType::Flow)
	//                {
	//                    out += ConvertPinTypeToString(input.Type) + " " + UnderscoreSpaces(input.Name);
	//                    if (x != data.Inputs.size() - 1) { out += ", "; }
	//                }
	//            }
	//        }
	//        
	//        out += ");\r\r";
	//    }
	//
	//    if (data.Name == "On Create")
	//    {
	//	    out += "virtual void OnCreate() override\r{\r";
	//        int* o = reinterpret_cast<int*>(&m_Nodes[i]);
	//        out += CallNode(o);
	//        out += "}";
	//    }
	//}

	std::ofstream fout("src/BlueprintCode.h");
    fout << out.c_str();
}

void NodeEditorInternal::CopyNodes()
{

}

void NodeEditorInternal::PasteNodes()
{

}

void NodeEditorInternal::DuplicateNodes()
{
	std::vector<ed::NodeId> selectedNodes;
	std::vector<ed::LinkId> selectedLinks;
	selectedNodes.resize(ed::GetSelectedObjectCount());
	selectedLinks.resize(ed::GetSelectedObjectCount());

	int nodeCount = ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
	int linkCount = ed::GetSelectedLinks(selectedLinks.data(), static_cast<int>(selectedLinks.size()));

	selectedNodes.resize(nodeCount);
	selectedLinks.resize(linkCount);

    ed::ClearSelection();

    std::vector<ed::PinId> originalPins;
    std::vector<ed::PinId> newPins;

	for (int i = 0; i < selectedNodes.size(); i++)
	{
		for (int x = 0; x < m_Nodes.size(); x++)
		{
			if (selectedNodes[i] == m_Nodes[x].ID)
			{
				m_Nodes.emplace_back(m_Nodes[x]);

				m_Nodes.back().ID = GetNextId();
				for (int y = 0; y < m_Nodes.back().Inputs.size(); y++)
				{
                    m_Nodes.back().Inputs[y].ID = GetNextId();
                    m_Nodes.back().Inputs[y].Node = &m_Nodes.back();

					originalPins.emplace_back(m_Nodes[x].Inputs[y].ID);
					newPins.emplace_back(m_Nodes.back().Inputs[y].ID);
				}
                for (int y = 0; y < m_Nodes.back().Outputs.size(); y++)
				{
                    m_Nodes.back().Outputs[y].ID = GetNextId();
                    m_Nodes.back().Outputs[y].Node = &m_Nodes.back();

                    originalPins.emplace_back(m_Nodes[x].Outputs[y].ID);
                    newPins.emplace_back(m_Nodes.back().Outputs[y].ID);
				}

				ed::SetNodePosition(m_Nodes.back().ID, ed::GetNodePosition(m_Nodes[x].ID) + ImVec2(50, 50));

				BuildNode(&m_Nodes[x]);
				BuildNode(&m_Nodes.back());

                ed::SelectNode(m_Nodes.back().ID, true);
			}
		}
	}

    for (int i = 0; i < m_Links.size(); i++)
    {
        int selectSuccess = 0;
        for (int x = 0; x < selectedNodes.size(); x++)
        {
            auto selectedNodeTemp = FindNode(selectedNodes[x]);
            for (int y = 0; y < selectedNodeTemp->Inputs.size(); y++)
            {
                if (selectedNodeTemp->Inputs[y].ID == m_Links[i].StartPinID || selectedNodeTemp->Inputs[y].ID == m_Links[i].EndPinID)
                {
                    selectSuccess++;
                }
            }
            for (int y = 0; y < selectedNodeTemp->Outputs.size(); y++)
            {
				if (selectedNodeTemp->Outputs[y].ID == m_Links[i].StartPinID || selectedNodeTemp->Outputs[y].ID == m_Links[i].EndPinID)
				{
					selectSuccess++;
				}
            }
        }
        if (selectSuccess > 1)
        {
            ed::PinId newStartPin;
            ed::PinId newEndPin;

            for (int x = 0; x < originalPins.size(); x++)
            {
                if (originalPins[x] == m_Links[i].StartPinID)
                {
                    newStartPin = newPins[x];
                }
				if (originalPins[x] == m_Links[i].EndPinID)
				{
					newEndPin = newPins[x];
				}
            }

            m_Links.emplace_back(GetNextLinkId(), newStartPin, newEndPin);
            m_Links.back().Color = m_Links[i].Color;
        }
    }
}

void NodeEditorInternal::DeleteNodes()
{

}

std::string NodeEditorInternal::ToLower(std::string inString)
{
	transform(inString.begin(), inString.end(), inString.begin(), ::tolower);
	return inString;
}

bool NodeEditorInternal::DisplayResult(SearchResultData result, std::string searchBuffer)
{
    bool returnBool = false;

    std::vector<std::string> searchWords;

	std::string searchBufferTemp = ToLower(searchBuffer);
	std::string nameTemp = ToLower(result.Name);

	for (int x = 0; x < searchBufferTemp.length(); x++)
	{
		if (searchBufferTemp.substr(x, 2) == "  ") { searchBufferTemp = searchBufferTemp.erase(x, 1); x--; }
	}

	while (searchBufferTemp.find_first_of(" ") != std::string::npos)
	{
		searchWords.push_back(searchBufferTemp.substr(0, searchBufferTemp.find_first_of(" ")));
		searchBufferTemp = searchBufferTemp.substr(searchBufferTemp.find_first_of(" ") + 1, searchBufferTemp.size() - 1 - searchBufferTemp.find_first_of(" "));
	}
	searchWords.push_back(searchBufferTemp);
	for (int x = 0; x < searchWords.size(); x++)
	{
		if (searchWords[x] == " " || searchWords[x] == "") { searchWords.erase(searchWords.begin() + x); }
	}

	bool visible = true;
	if (searchBuffer != "")
	{
		for (int x = 0; x < searchWords.size(); x++)
		{
			if (nameTemp.find(searchWords[x]) == std::string::npos) { visible = false; }
		}
	}

	if (visible) { returnBool = true; }

    return returnBool;
}

bool NodeEditorInternal::DisplaySubcatagories(SearchData searchData, std::string searchBuffer)
{
	bool returnBool = false;
	for (int i = 0; i < searchData.LowerTree.size(); i++)
	{
        bool result = DisplaySubcatagories(searchData.LowerTree[i], searchBuffer);

        if (result) returnBool = result;
	}
	for (int i = 0; i < searchData.Results.size(); i++)
	{
        std::vector<std::string> searchWords;

        std::string searchBufferTemp = ToLower(searchBuffer);
        std::string nameTemp = ToLower(searchData.Results[i].Name);

        for (int x = 0; x < searchBufferTemp.length(); x++)
        {
            if (searchBufferTemp.substr(x, 2) == "  ") { searchBufferTemp = searchBufferTemp.erase(x, 1); x--; }
        }

        while (searchBufferTemp.find_first_of(" ") != std::string::npos) 
        {
            searchWords.push_back(searchBufferTemp.substr(0, searchBufferTemp.find_first_of(" ")));
            searchBufferTemp = searchBufferTemp.substr(searchBufferTemp.find_first_of(" ") +1, searchBufferTemp.size() - 1 - searchBufferTemp.find_first_of(" "));
        }
        searchWords.push_back(searchBufferTemp);
        for (int x = 0; x < searchWords.size(); x++)
        {
            if (searchWords[x] == " " || searchWords[x] == "") { searchWords.erase(searchWords.begin() + x); }
        }

        bool visible = true;
        if (searchBuffer != "")
        {
            for (int x = 0; x < searchWords.size(); x++)
            {
                if (nameTemp.find(searchWords[x]) == std::string::npos) { visible = false; }
            }
        }

        if (visible) { returnBool = true; }


		//if (ToLower(searchData.Results[i].Name).find(ToLower(searchBuffer)) != std::string::npos) { returnBool = true; }
	}

    return returnBool;
}

void NodeEditorInternal::OpenSearchList(SearchData* searchData)
{
	searchData->open = true;
	searchData->confirmed = false;
	for (int i = 0; i < searchData->LowerTree.size(); i++)
	{
		OpenSearchList(&searchData->LowerTree[i]);
	}
}

void NodeEditorInternal::CloseSearchList(SearchData* searchData)
{
	searchData->open = false;
	searchData->confirmed = false;
	for (int i = 0; i < searchData->LowerTree.size(); i++)
	{
		CloseSearchList(&searchData->LowerTree[i]);
	}
}



Node* NodeEditorInternal::DisplaySearchData(SearchData* searchData, std::string searchBuffer, bool origin)
{
    static int index = 0;

    Node* returnNode = {};

    if (origin) { index = 0; }

    if (DisplaySubcatagories(*searchData, searchBuffer))
    {
        if (searchData->confirmed == false)
        {
            ImGui::SetNextTreeNodeOpen(searchData->open);
            searchData->confirmed = true;
        }
        if (origin ? true : ImGui::TreeNodeEx(searchData->Name.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth))
		{
            for (int i = 0; i < searchData->LowerTree.size(); i++)
            {
                auto ab = DisplaySearchData(&searchData->LowerTree[i], searchBuffer, false);

                if (ab) { returnNode = ab; }
            }
            for (int i = 0; i < searchData->Results.size(); i++)
            {   
                if (DisplayResult(searchData->Results[i], searchBuffer))
                {
                    if (ImGui::MenuItem(searchData->Results[i].Name.c_str())) { returnNode = (this->*searchData->Results[i].Function)(searchData->Results[i].spawnStruct); }
                    index++;
                }
            }
            if (!origin)
                ImGui::TreePop();
        }
    }

    return returnNode;
}

void NodeEditorInternal::InitSearchData()
{
    m_SearchData.Clear();

    m_SearchData.Name = "Dymatic Nodes";
    auto Event = m_SearchData.PushBackTree("Events");
        Event->PushBackResult("On Create", &NodeEditorInternal::SpawnOnCreateNode);
        Event->PushBackResult("On Update", &NodeEditorInternal::SpawnOnUpdateNode);
        auto CustomEvent = Event->PushBackTree("Custom Events");
    auto Variable = m_SearchData.PushBackTree("Variables");
        for (int i = 0; i < m_Variables.size(); i++)
        {
            SpawnNodeStruct newStruct = {};
            newStruct.variableName = m_Variables[i].Name;
            newStruct.variableType = m_Variables[i].Type;
            Variable->PushBackResult("Get " + m_Variables[i].Name, &NodeEditorInternal::SpawnGetVariableNode, newStruct);
            Variable->PushBackResult("Set " + m_Variables[i].Name, &NodeEditorInternal::SpawnSetVariableNode, newStruct);
        }
    auto Math = m_SearchData.PushBackTree("Math");
        auto Boolean = Math->PushBackTree("Boolean");
            Boolean->PushBackResult("Make Literal Bool", &NodeEditorInternal::SpawnMakeLiteralBoolNode);
            Boolean->PushBackResult("AND Boolean", &NodeEditorInternal::SpawnANDBooleanNode);
            Boolean->PushBackResult("Equal Boolean", &NodeEditorInternal::SpawnEqualBooleanNode);
            Boolean->PushBackResult("NAND Boolean", &NodeEditorInternal::SpawnNANDBooleanNode);
            Boolean->PushBackResult("NOR Boolean", &NodeEditorInternal::SpawnNORBooleanNode);
            Boolean->PushBackResult("NOT Boolean", &NodeEditorInternal::SpawnNOTBooleanNode);
            Boolean->PushBackResult("Not Equal Boolean", &NodeEditorInternal::                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             SpawnNotEqualBooleanNode);
            Boolean->PushBackResult("OR Boolean", &NodeEditorInternal::SpawnORBooleanNode);
            Boolean->PushBackResult("XOR Boolean", &NodeEditorInternal::SpawnXORBooleanNode);
        auto Int = Math->PushBackTree("Int");
            Int->PushBackResult("Make Literal Int", &NodeEditorInternal::SpawnMakeLiteralIntNode);
        auto Float = Math->PushBackTree("Float");
            Float->PushBackResult("Make Literal Float", &NodeEditorInternal::SpawnMakeLiteralFloatNode);
            Float->PushBackResult("Modulo (Float)", &NodeEditorInternal::SpawnModuloFloatNode);
            Float->PushBackResult("Absolute (Float)", &NodeEditorInternal::SpawnAbsoluteFloatNode);
            Float->PushBackResult("Ceil", &NodeEditorInternal::SpawnCeilNode);
            Float->PushBackResult("Clamp Float", &NodeEditorInternal::SpawnClampFloatNode);
            Float->PushBackResult("Normalize Angle", &NodeEditorInternal::SpawnNormalizeAngleNode);
            Float->PushBackResult("Lerp Angle", &NodeEditorInternal::SpawnLerpAngleNode);
            Float->PushBackResult("Equal Float", &NodeEditorInternal::SpawnEqualFloatNode);
            Float->PushBackResult("Exp Float", &NodeEditorInternal::SpawnExpFloatNode);
        auto Conversions = Math->PushBackTree("Conversions");
		Conversions->PushBackResult("Int To Bool", &NodeEditorInternal::SpawnIntToBoolNode);
		Conversions->PushBackResult("Float To Bool", &NodeEditorInternal::SpawnFloatToBoolNode);
		Conversions->PushBackResult("String To Bool", &NodeEditorInternal::SpawnStringToBoolNode);
        Conversions->PushBackResult("Bool To Int", &NodeEditorInternal::SpawnBoolToIntNode);
        Conversions->PushBackResult("Float To Int", &NodeEditorInternal::SpawnFloatToIntNode);
        Conversions->PushBackResult("String To Int", &NodeEditorInternal::SpawnStringToIntNode);
        Conversions->PushBackResult("Bool To Float", &NodeEditorInternal::SpawnBoolToFloatNode);
        Conversions->PushBackResult("Int To Float", &NodeEditorInternal::SpawnIntToFloatNode);
        Conversions->PushBackResult("String To Float", &NodeEditorInternal::SpawnStringToFloatNode);
    auto Utilities = m_SearchData.PushBackTree("Utilities");
        auto String = Utilities->PushBackTree("String");
            String->PushBackResult("Make Literal String", &NodeEditorInternal::SpawnMakeLiteralStringNode);
            String->PushBackResult("Bool To String", &NodeEditorInternal::SpawnBoolToStringNode);
            String->PushBackResult("Int To String", &NodeEditorInternal::SpawnIntToStringNode);
            String->PushBackResult("Float To String", &NodeEditorInternal::SpawnFloatToStringNode);
            String->PushBackResult("Print String", &NodeEditorInternal::SpawnPrintStringNode);
        auto FlowControl = Utilities->PushBackTree("Flow Control");
            FlowControl->PushBackResult("Branch", &NodeEditorInternal::SpawnBranchNode);
            FlowControl->PushBackResult("Sequence", &NodeEditorInternal::SpawnSequenceNode);
}

void NodeEditorInternal::Application_Initialize()
{

    //ed::EnableShortcuts(true);

	ed::Config config;

	//config.SettingsFile = "Blueprints.json";

	//config.LoadNodeSettings = [](ed::NodeId nodeId, char* data, void* userPointer) -> size_t
	//{
	//	auto node = FindNode(nodeId);
	//	if (!node)
	//		return 0;
	//
	//	if (data != nullptr)
	//		memcpy(data, node->State.data(), node->State.size());
	//	return node->State.size();
	//};
	//
	//config.SaveNodeSettings = [](ed::NodeId nodeId, const char* data, size_t size, ed::SaveReasonFlags reason, void* userPointer) -> bool
	//{
	//	auto node = FindNode(nodeId);
	//	if (!node)
	//		return false;
	//
	//	node->State.assign(data, size);
	//
	//	TouchNode(nodeId);
	//
	//	return true;
	//};

	m_Editor = ed::CreateEditor(&config);
	ed::SetCurrentEditor(m_Editor);

	Node* node;
    node = SpawnInputActionNode({});      ed::SetNodePosition(node->ID, ImVec2(-252, 220));
    node = SpawnBranchNode({});           ed::SetNodePosition(node->ID, ImVec2(-300, 351));
    node = SpawnDoNNode({});              ed::SetNodePosition(node->ID, ImVec2(-238, 504));
    node = SpawnOutputActionNode({});     ed::SetNodePosition(node->ID, ImVec2(71, 80));
    node = SpawnSetTimerNode({});         ed::SetNodePosition(node->ID, ImVec2(168, 316));
    
    node = SpawnTreeSequenceNode({});     ed::SetNodePosition(node->ID, ImVec2(1028, 329));
    node = SpawnTreeTaskNode({});         ed::SetNodePosition(node->ID, ImVec2(1204, 458));
    node = SpawnTreeTask2Node({});        ed::SetNodePosition(node->ID, ImVec2(868, 538));
    
	node = SpawnComment({});              ed::SetNodePosition(node->ID, ImVec2(112, 576));
	node = SpawnComment({});              ed::SetNodePosition(node->ID, ImVec2(800, 224));
    
    node = SpawnLessNode({});             ed::SetNodePosition(node->ID, ImVec2(366, 652));
    node = SpawnWeirdNode({});            ed::SetNodePosition(node->ID, ImVec2(144, 652));
    node = SpawnMessageNode({});          ed::SetNodePosition(node->ID, ImVec2(-348, 698));
    node = SpawnPrintStringNode({});      ed::SetNodePosition(node->ID, ImVec2(-69, 652));
    
    node = SpawnHoudiniTransformNode({}); ed::SetNodePosition(node->ID, ImVec2(500, -70));
    node = SpawnHoudiniGroupNode({});     ed::SetNodePosition(node->ID, ImVec2(500, 42));
    
	ed::NavigateToContent();
    
	BuildNodes();
    
	m_Links.push_back(Link(GetNextLinkId(), m_Nodes[5].Outputs[0].ID, m_Nodes[6].Inputs[0].ID));
	m_Links.push_back(Link(GetNextLinkId(), m_Nodes[5].Outputs[0].ID, m_Nodes[7].Inputs[0].ID));
    
	m_Links.push_back(Link(GetNextLinkId(), m_Nodes[14].Outputs[0].ID, m_Nodes[15].Inputs[0].ID));

    m_Variables.emplace_back(Variable(GetNextId(), "Test Bool", PinType::Bool));
    m_Variables.back().Values.Bool = true;
    m_Variables.emplace_back(Variable(GetNextId(), "Test Float", PinType::Float));
    m_Variables.back().Values.Float = 5.423f;
    m_Variables.emplace_back(Variable(GetNextId(), "Test Int", PinType::Int));
    m_Variables.back().Values.Int = 12;

	m_HeaderBackground = Dymatic::Texture2D::Create("src/Panels/NodeEditor/data/BlueprintBackground.png");
	m_SaveIcon = Dymatic::Texture2D::Create("src/Panels/NodeEditor/data/ic_save_white_24dp.png");
	m_RestoreIcon = Dymatic::Texture2D::Create("src/Panels/NodeEditor/data/ic_restore_white_24dp.png");


	//auto& io = ImGui::GetIO();
}

Dymatic::NodeEditorPannel::NodeEditorPannel()
{
    EditorInternalStack.push_back({});
    this->stackIndex = EditorInternalStack.size() - 1;
}

void Dymatic::NodeEditorPannel::OnEvent(Event& e)
{
    EditorInternalStack[this->stackIndex].OnEvent(e);
}

void Dymatic::NodeEditorPannel::Application_Initialize()
{
    EditorInternalStack[this->stackIndex].Application_Initialize();
}

void Dymatic::NodeEditorPannel::Application_Finalize()
{
    EditorInternalStack[this->stackIndex].Application_Finalize();
}

void Dymatic::NodeEditorPannel::CompileNodes()
{
    EditorInternalStack[this->stackIndex].CompileNodes();
}

void Dymatic::NodeEditorPannel::CopyNodes()
{
    EditorInternalStack[this->stackIndex].CopyNodes();
}

void Dymatic::NodeEditorPannel::PasteNodes()
{
    EditorInternalStack[this->stackIndex].PasteNodes();
}

void Dymatic::NodeEditorPannel::DuplicateNodes()
{
    EditorInternalStack[this->stackIndex].DuplicateNodes();
}

void Dymatic::NodeEditorPannel::DeleteNodes()
{
    EditorInternalStack[this->stackIndex].DeleteNodes();
}

void Dymatic::NodeEditorPannel::Application_Frame()
{
    EditorInternalStack[this->stackIndex].Application_Frame();
}

void NodeEditorInternal::Application_Finalize()
{
	auto releaseTexture = [](ImTextureID& id)
	{
		if (id)
		{
			//TODO: destroy texture
			id = nullptr;
		}
	};

	//TODO add releasing of textures
	if (m_Editor)
	{
		ed::DestroyEditor(m_Editor);
		m_Editor = nullptr;
	}
}

void NodeEditorInternal::Application_Frame()
{
    UpdateTouch();

	std::vector<ed::NodeId> selectedNodes;
	std::vector<ed::LinkId> selectedLinks;
	selectedNodes.resize(ed::GetSelectedObjectCount());
	selectedLinks.resize(ed::GetSelectedObjectCount());

	int nodeCount = ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
	int linkCount = ed::GetSelectedLinks(selectedLinks.data(), static_cast<int>(selectedLinks.size()));

	selectedNodes.resize(nodeCount);
	selectedLinks.resize(linkCount);

    auto& io = ImGui::GetIO();

    ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
    if (ImGui::Button("Compile"))
    {
        CompileNodes();
    }

    ed::SetCurrentEditor(m_Editor);

    //auto& style = ImGui::GetStyle();

# if 0
    {
        for (auto x = -io.DisplaySize.y; x < io.DisplaySize.x; x += 10.0f)
        {
            ImGui::GetWindowDrawList()->AddLine(ImVec2(x, 0), ImVec2(x + io.DisplaySize.y, io.DisplaySize.y),
                IM_COL32(255, 255, 0, 255));
        }
    }
# endif

    static ed::NodeId contextNodeId      = 0;
    static ed::LinkId contextLinkId      = 0;
    static ed::PinId  contextPinId       = 0;
    static bool createNewNode  = false;
    static Pin* newNodeLinkPin = nullptr;
    static Pin* newLinkPin     = nullptr;

    static float panelA = ImGui::GetContentRegionAvail().x / 12 * 2;
    static float panelB = ImGui::GetContentRegionAvail().x / 12 * 8;
    static float panelC = ImGui::GetContentRegionAvail().x / 12 * 2;

	ImGui::Splitter("##NodeOutlinerSplitEditor", true, 2.0f, &panelA, &panelB, 50.0f, 50.0f);

	ImGui::BeginChild("Nodes Outliner", ImVec2(panelA - 10.0f, 0));
    //ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize() - ImVec2(0, 0), ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 1.0f, 0.0f, 1.0f)));
    for (int i = 0; i < m_Variables.size(); i++)
    {
		ImGuiTreeNodeFlags flags = ((m_Variables[i].ID == m_SelectedVariable) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;

		char buffer[256];
		memset(buffer, 0, sizeof(buffer));
		std::strncpy(buffer, m_Variables[i].Name.c_str(), sizeof(buffer));
        auto variableTreeNode = ImGui::TreeNodeInputEx(("##" + std::to_string(m_Variables[i].ID)).c_str(), m_Variables[i].Name.c_str(), flags, buffer, sizeof(buffer));
        if (variableTreeNode.object) { ImGui::TreePop(); }
        if (variableTreeNode.input) {  m_Variables[i].Name = std::string(buffer); }

		if (ImGui::BeginPopupContextItem())
		{
            ImGuiID id = ImGui::GetCurrentWindow()->GetID("##VariableRename");
            if (ImGui::MenuItem("Duplicate"))
            { 
                m_Variables.push_back(m_Variables[i]);
                m_Variables.back().ID = GetNextId();

                int index = 1;
                std::string newName;
                bool looking = true;
                while (looking)
                {
                    std::string modName = m_Variables[i].Name;
                    if (modName.find_last_of("_") != std::string::npos) { if (true) { modName = modName.substr(0, modName.find_last_of("_")); } }
                    newName = modName + "_" + (index < 10 ? "0" : "") + std::to_string(index);
                    looking = false;
                    for (int y = 0; y < m_Variables.size(); y++)
                    {
                        if (m_Variables[y].Name == newName) { looking = true; }
                    }
                    index++;
                }

                m_Variables.back().Name = newName;
            }
            if (ImGui::MenuItem("Delete")) { m_Variables.erase(m_Variables.begin() + i); }

			ImGui::EndPopup();
		}

    }

    ImGui::Separator();

    for (int i = 0; i < m_Nodes.size(); i++)
    {
        bool selected = false;
        for (int x = 0; x < selectedNodes.size(); x++)
        {
            if (selectedNodes[x] == m_Nodes[i].ID) { selected = true; }
        }

		ImGuiTreeNodeFlags flags = ((selected) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (ImGui::TreeNodeEx(std::to_string(m_Nodes[i].ID.Get()).c_str(), flags, m_Nodes[i].Name.c_str())) { ImGui::TreePop(); }

        if (ImGui::IsItemClicked())
        {
            if (Dymatic::Input::IsKeyPressed(Dymatic::Key::LeftShift) || Dymatic::Input::IsKeyPressed(Dymatic::Key::LeftControl) || Dymatic::Input::IsKeyPressed(Dymatic::Key::RightShift) || Dymatic::Input::IsKeyPressed(Dymatic::Key::RightControl))
            {
                if (selected) { ed::DeselectNode(m_Nodes[i].ID); }
                else { ed::SelectNode(m_Nodes[i].ID, true);}
                
            }
            else
            {
                ed::SelectNode(m_Nodes[i].ID, false);
            }
        }
    }

	ImGui::EndChild();

    ImGui::SameLine(0.0f, 12.0f);

	ImGui::Splitter("##NodeEditorSplitInfo", true, 2.0f, &panelB, &panelC, 50.0f, 50.0f);

    ed::Begin("Node Editor Panel", ImVec2(panelB, 0));
    {
        auto cursorTopLeft = ImGui::GetCursorScreenPos();

        util::BlueprintNodeBuilder builder(reinterpret_cast<void*>(m_HeaderBackground->GetRendererID()), m_HeaderBackground->GetWidth(), m_HeaderBackground->GetHeight());

        for (auto& node : m_Nodes)
        {
            if (node.Type != NodeType::Blueprint && node.Type != NodeType::Simple)
                continue;

            const auto isSimple = node.Type == NodeType::Simple;

            bool hasOutputDelegates = false;
            for (auto& output : node.Outputs)
                if (output.Type == PinType::Delegate)
                    hasOutputDelegates = true;

            builder.Begin(node.ID);
                if (!isSimple)
                {
                    builder.Header(node.Color);
                        ImGui::Spring(0);
                        ImGui::TextUnformatted((node.DisplayName == "" ? node.Name : node.DisplayName).c_str());
                        ImGui::Spring(1);
                        ImGui::Dummy(ImVec2(0, 28));
                        if (hasOutputDelegates)
                        {
                            ImGui::BeginVertical("delegates", ImVec2(0, 28));
                            ImGui::Spring(1, 0);
                            for (auto& output : node.Outputs)
                            {
                                if (output.Type != PinType::Delegate)
                                    continue;

                                auto alpha = ImGui::GetStyle().Alpha;
                                if (newLinkPin && !CanCreateLink(newLinkPin, &output) && &output != newLinkPin)
                                    alpha = alpha * (48.0f / 255.0f);

                                ed::BeginPin(output.ID, ed::PinKind::Output);
                                ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
                                ed::PinPivotSize(ImVec2(0, 0));
                                ImGui::BeginHorizontal(output.ID.AsPointer());
                                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                                if (!output.Name.empty())
                                {
                                    ImGui::TextUnformatted(output.Name.c_str());
                                    ImGui::Spring(0);
                                }
                                DrawPinIcon(output, IsPinLinked(output.ID), (int)(alpha * 255));
                                ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
                                ImGui::EndHorizontal();
                                ImGui::PopStyleVar();
                                ed::EndPin();

                                //DrawItemRect(ImColor(255, 0, 0));
                            }
                            ImGui::Spring(1, 0);
                            ImGui::EndVertical();
                            ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
                        }
                        else
                            ImGui::Spring(0);
                    builder.EndHeader();
                }

                for (auto& input : node.Inputs)
                {
                    auto alpha = ImGui::GetStyle().Alpha;
                    if (newLinkPin && !CanCreateLink(newLinkPin, &input) && &input != newLinkPin)
                        alpha = alpha * (48.0f / 255.0f);

                    builder.Input(input.ID);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                    DrawPinIcon(input, IsPinLinked(input.ID), (int)(alpha * 255));
                    ImGui::Spring(0);
                    if (!input.Name.empty())
                    {
                        ImGui::TextUnformatted(input.Name.c_str());
                        ImGui::Spring(0);
                    }
                    if (!IsPinLinked(input.ID))
                    {
                        if (input.Type == PinType::Bool)
                        {
                            ImGui::Custom::Checkbox(("##NodeEditorBoolCheckbox_" + std::to_string(input.ID.Get())).c_str(), &input.Value.Bool);
                            ImGui::Spring(0);
                        }
                        if (input.Type == PinType::String)
                        {
                            char buffer[512];
                            memset(buffer, 0, sizeof(buffer));
                            std::strncpy(buffer, input.Value.String.c_str(), sizeof(buffer));
                            ImGui::SetNextItemWidth(std::clamp(ImGui::CalcTextSize((input.Value.String).c_str()).x + 20, 30.0f, 500.0f));
                            if (ImGui::InputText(("##NodeEditorStringInputText_" + std::to_string(input.ID.Get())).c_str(), buffer, sizeof(buffer)))
                            {
                                input.Value.String = std::string(buffer);
                            }

                            ImGui::Spring(0);
                        }
                        if (input.Type == PinType::Int)
                        {
                            ImGui::SetNextItemWidth(std::clamp(ImGui::CalcTextSize((std::to_string(input.Value.Int)).c_str()).x + 20, 50.0f, 300.0f));
                            ImGui::DragInt(("##NodeEditorIntSlider_" + std::to_string(input.ID.Get())).c_str(), &input.Value.Int, 0.1f, 0, 0);
                        }
                        if (input.Type == PinType::Float)
                        {
                            ImGui::SetNextItemWidth(std::clamp(ImGui::CalcTextSize((std::to_string(input.Value.Float)).c_str()).x - 10, 50.0f, 300.0f));
                            ImGui::DragFloat(("##NodeEditorFloatSlider_" + std::to_string(input.ID.Get())).c_str(), &input.Value.Float, 0.1f, 0, 0, "%.3f");
                        }
                    }
                    ImGui::PopStyleVar();
                    builder.EndInput();
                }

                if (isSimple)
                {
                    builder.Middle();

                    ImGui::Spring(1, 0);
                    ImGui::TextUnformatted((node.DisplayName == "" ? node.Name : node.DisplayName).c_str());
                    ImGui::Spring(1, 0);
                }

                for (auto& output : node.Outputs)
                {
                    if (!isSimple && output.Type == PinType::Delegate)
                        continue;

                    auto alpha = ImGui::GetStyle().Alpha;
                    if (newLinkPin && !CanCreateLink(newLinkPin, &output) && &output != newLinkPin)
                        alpha = alpha * (48.0f / 255.0f);

                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                    builder.Output(output.ID);
                    if (!output.Name.empty())
                    {
                        ImGui::Spring(0);
                        ImGui::TextUnformatted(output.Name.c_str());
                    }
                    ImGui::Spring(0);
                    DrawPinIcon(output, IsPinLinked(output.ID), (int)(alpha * 255));
                    ImGui::PopStyleVar();
                    builder.EndOutput();
                }

				if (node.AddOutputs)
				{
                    ImGui::Spring(0);

					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
					bool newPin = ImGui::Button("Add Pin +");
					ImGui::PopStyleColor(3);
                    
                    if (newPin)
                    {
                        node.Outputs.emplace_back(GetNextId(), std::to_string((int)std::stof(node.Outputs[node.Outputs.size() - 1].Name) + 1).c_str(), PinType::Flow);
                        node.Outputs.back().Kind = PinKind::Output;
                    }
				}

            builder.End();
        }

        for (auto& node : m_Nodes)
        {
            if (node.Type != NodeType::Tree)
                continue;

            const float rounding = 5.0f;
            const float padding  = 12.0f;

            const auto pinBackground = ed::GetStyle().Colors[ed::StyleColor_NodeBg];

            ed::PushStyleColor(ed::StyleColor_NodeBg,        ImColor(128, 128, 128, 200));
            ed::PushStyleColor(ed::StyleColor_NodeBorder,    ImColor( 32,  32,  32, 200));
            ed::PushStyleColor(ed::StyleColor_PinRect,       ImColor( 60, 180, 255, 150));
            ed::PushStyleColor(ed::StyleColor_PinRectBorder, ImColor( 60, 180, 255, 150));

            ed::PushStyleVar(ed::StyleVar_NodePadding,  ImVec4(0, 0, 0, 0));
            ed::PushStyleVar(ed::StyleVar_NodeRounding, rounding);
            ed::PushStyleVar(ed::StyleVar_SourceDirection, ImVec2(0.0f,  1.0f));
            ed::PushStyleVar(ed::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
            ed::PushStyleVar(ed::StyleVar_LinkStrength, 0.0f);
            ed::PushStyleVar(ed::StyleVar_PinBorderWidth, 1.0f);
            ed::PushStyleVar(ed::StyleVar_PinRadius, 5.0f);
            ed::BeginNode(node.ID);

            ImGui::BeginVertical(node.ID.AsPointer());
            ImGui::BeginHorizontal("inputs");
            ImGui::Spring(0, padding * 2);

            ImRect inputsRect;
            int inputAlpha = 200;
            if (!node.Inputs.empty())
            {
                    auto& pin = node.Inputs[0];
                    ImGui::Dummy(ImVec2(0, padding));
                    ImGui::Spring(1, 0);
                    inputsRect = ImGui_GetItemRect();

                    ed::PushStyleVar(ed::StyleVar_PinArrowSize, 10.0f);
                    ed::PushStyleVar(ed::StyleVar_PinArrowWidth, 10.0f);
                    ed::PushStyleVar(ed::StyleVar_PinCorners, 12);
                    ed::BeginPin(pin.ID, ed::PinKind::Input);
                    ed::PinPivotRect(inputsRect.GetTL(), inputsRect.GetBR());
                    ed::PinRect(inputsRect.GetTL(), inputsRect.GetBR());
                    ed::EndPin();
                    ed::PopStyleVar(3);

                    if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
                        inputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
            }
            else
                ImGui::Dummy(ImVec2(0, padding));

            ImGui::Spring(0, padding * 2);
            ImGui::EndHorizontal();

            ImGui::BeginHorizontal("content_frame");
            ImGui::Spring(1, padding);

            ImGui::BeginVertical("content", ImVec2(0.0f, 0.0f));
            ImGui::Dummy(ImVec2(160, 0));
            ImGui::Spring(1);
            ImGui::TextUnformatted(node.Name.c_str());
            ImGui::Spring(1);
            ImGui::EndVertical();
            auto contentRect = ImGui_GetItemRect();

            ImGui::Spring(1, padding);
            ImGui::EndHorizontal();

            ImGui::BeginHorizontal("outputs");
            ImGui::Spring(0, padding * 2);

            ImRect outputsRect;
            int outputAlpha = 200;
            if (!node.Outputs.empty())
            {
                auto& pin = node.Outputs[0];
                ImGui::Dummy(ImVec2(0, padding));
                ImGui::Spring(1, 0);
                outputsRect = ImGui_GetItemRect();

                ed::PushStyleVar(ed::StyleVar_PinCorners, 3);
                ed::BeginPin(pin.ID, ed::PinKind::Output);
                ed::PinPivotRect(outputsRect.GetTL(), outputsRect.GetBR());
                ed::PinRect(outputsRect.GetTL(), outputsRect.GetBR());
                ed::EndPin();
                ed::PopStyleVar();

                if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
                    outputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
            }
            else
                ImGui::Dummy(ImVec2(0, padding));

            ImGui::Spring(0, padding * 2);
            ImGui::EndHorizontal();

            ImGui::EndVertical();

            ed::EndNode();
            ed::PopStyleVar(7);
            ed::PopStyleColor(4);

            auto drawList = ed::GetNodeBackgroundDrawList(node.ID);

            //const auto fringeScale = ImGui::GetStyle().AntiAliasFringeScale;
            //const auto unitSize    = 1.0f / fringeScale;

            //const auto ImDrawList_AddRect = [](ImDrawList* drawList, const ImVec2& a, const ImVec2& b, ImU32 col, float rounding, int rounding_corners, float thickness)
            //{
            //    if ((col >> 24) == 0)
            //        return;
            //    drawList->PathRect(a, b, rounding, rounding_corners);
            //    drawList->PathStroke(col, true, thickness);
            //};

            drawList->AddRectFilled(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
                IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 12);
            //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
            drawList->AddRect(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
                IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 12);
            //ImGui::PopStyleVar();
            drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
                IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 3);
            //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
            drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
                IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 3);
            //ImGui::PopStyleVar();
            drawList->AddRectFilled(contentRect.GetTL(), contentRect.GetBR(), IM_COL32(24, 64, 128, 200), 0.0f);
            //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
            drawList->AddRect(
                contentRect.GetTL(),
                contentRect.GetBR(),
                IM_COL32(48, 128, 255, 100), 0.0f);
            //ImGui::PopStyleVar();
        }

        for (auto& node : m_Nodes)
        {
            if (node.Type != NodeType::Houdini)
                continue;

            const float rounding = 10.0f;
            const float padding  = 12.0f;


            ed::PushStyleColor(ed::StyleColor_NodeBg,        ImColor(229, 229, 229, 200));
            ed::PushStyleColor(ed::StyleColor_NodeBorder,    ImColor(125, 125, 125, 200));
            ed::PushStyleColor(ed::StyleColor_PinRect,       ImColor(229, 229, 229, 60));
            ed::PushStyleColor(ed::StyleColor_PinRectBorder, ImColor(125, 125, 125, 60));

            const auto pinBackground = ed::GetStyle().Colors[ed::StyleColor_NodeBg];

            ed::PushStyleVar(ed::StyleVar_NodePadding,  ImVec4(0, 0, 0, 0));
            ed::PushStyleVar(ed::StyleVar_NodeRounding, rounding);
            ed::PushStyleVar(ed::StyleVar_SourceDirection, ImVec2(0.0f,  1.0f));
            ed::PushStyleVar(ed::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
            ed::PushStyleVar(ed::StyleVar_LinkStrength, 0.0f);
            ed::PushStyleVar(ed::StyleVar_PinBorderWidth, 1.0f);
            ed::PushStyleVar(ed::StyleVar_PinRadius, 6.0f);
            ed::BeginNode(node.ID);

            ImGui::BeginVertical(node.ID.AsPointer());
            if (!node.Inputs.empty())
            {
                ImGui::BeginHorizontal("inputs");
                ImGui::Spring(1, 0);

                ImRect inputsRect;
                int inputAlpha = 200;
                for (auto& pin : node.Inputs)
                {
                    ImGui::Dummy(ImVec2(padding, padding));
                    inputsRect = ImGui_GetItemRect();
                    ImGui::Spring(1, 0);
                    inputsRect.Min.y -= padding;
                    inputsRect.Max.y -= padding;

                    //ed::PushStyleVar(ed::StyleVar_PinArrowSize, 10.0f);
                    //ed::PushStyleVar(ed::StyleVar_PinArrowWidth, 10.0f);
                    ed::PushStyleVar(ed::StyleVar_PinCorners, 15);
                    ed::BeginPin(pin.ID, ed::PinKind::Input);
                    ed::PinPivotRect(inputsRect.GetCenter(), inputsRect.GetCenter());
                    ed::PinRect(inputsRect.GetTL(), inputsRect.GetBR());
                    ed::EndPin();
                    //ed::PopStyleVar(3);
                    ed::PopStyleVar(1);

                    auto drawList = ImGui::GetWindowDrawList();
                    drawList->AddRectFilled(inputsRect.GetTL(), inputsRect.GetBR(),
                        IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 15);
                    drawList->AddRect(inputsRect.GetTL(), inputsRect.GetBR(),
                        IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 15);

                    if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
                        inputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
                }

                //ImGui::Spring(1, 0);
                ImGui::EndHorizontal();
            }

            ImGui::BeginHorizontal("content_frame");
            ImGui::Spring(1, padding);

            ImGui::BeginVertical("content", ImVec2(0.0f, 0.0f));
            ImGui::Dummy(ImVec2(160, 0));
            ImGui::Spring(1);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            ImGui::TextUnformatted(node.Name.c_str());
            ImGui::PopStyleColor();
            ImGui::Spring(1);
            ImGui::EndVertical();
            auto contentRect = ImGui_GetItemRect();

            ImGui::Spring(1, padding);
            ImGui::EndHorizontal();

            if (!node.Outputs.empty())
            {
                ImGui::BeginHorizontal("outputs");
                ImGui::Spring(1, 0);

                ImRect outputsRect;
                int outputAlpha = 200;
                for (auto& pin : node.Outputs)
                {
                    ImGui::Dummy(ImVec2(padding, padding));
                    outputsRect = ImGui_GetItemRect();
                    ImGui::Spring(1, 0);
                    outputsRect.Min.y += padding;
                    outputsRect.Max.y += padding;

                    ed::PushStyleVar(ed::StyleVar_PinCorners, 3);
                    ed::BeginPin(pin.ID, ed::PinKind::Output);
                    ed::PinPivotRect(outputsRect.GetCenter(), outputsRect.GetCenter());
                    ed::PinRect(outputsRect.GetTL(), outputsRect.GetBR());
                    ed::EndPin();
                    ed::PopStyleVar();

                    auto drawList = ImGui::GetWindowDrawList();
                    drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR(),
                        IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 15);
                    drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR(),
                        IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 15);


                    if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
                        outputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
                }

                ImGui::EndHorizontal();
            }

            ImGui::EndVertical();

            ed::EndNode();
            ed::PopStyleVar(7);
            ed::PopStyleColor(4);

            auto drawList = ed::GetNodeBackgroundDrawList(node.ID);

            //const auto fringeScale = ImGui::GetStyle().AntiAliasFringeScale;
            //const auto unitSize    = 1.0f / fringeScale;

            //const auto ImDrawList_AddRect = [](ImDrawList* drawList, const ImVec2& a, const ImVec2& b, ImU32 col, float rounding, int rounding_corners, float thickness)
            //{
            //    if ((col >> 24) == 0)
            //        return;
            //    drawList->PathRect(a, b, rounding, rounding_corners);
            //    drawList->PathStroke(col, true, thickness);
            //};

            //drawList->AddRectFilled(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
            //    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 12);
            //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
            //drawList->AddRect(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
            //    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 12);
            //ImGui::PopStyleVar();
            //drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
            //    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 3);
            ////ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
            //drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
            //    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 3);
            ////ImGui::PopStyleVar();
            //drawList->AddRectFilled(contentRect.GetTL(), contentRect.GetBR(), IM_COL32(24, 64, 128, 200), 0.0f);
            //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
            //drawList->AddRect(
            //    contentRect.GetTL(),
            //    contentRect.GetBR(),
            //    IM_COL32(48, 128, 255, 100), 0.0f);
            //ImGui::PopStyleVar();
        }

        for (auto& node : m_Nodes)
        {
            if (node.Type != NodeType::Comment)
                continue;

            const float commentAlpha = 0.75f;

            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, commentAlpha);
            ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(255, 255, 255, 64));
            ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(255, 255, 255, 64));
            ed::BeginNode(node.ID);
            ImGui::PushID(node.ID.AsPointer());
            ImGui::BeginVertical("content");
            ImGui::BeginHorizontal("horizontal");
            ImGui::Spring(1);
            ImGui::TextUnformatted(node.Name.c_str());
            ImGui::Spring(1);
            ImGui::EndHorizontal();
            ed::Group(node.Size);
            ImGui::EndVertical();
            ImGui::PopID();
            ed::EndNode();
            ed::PopStyleColor(2);
            ImGui::PopStyleVar();

            if (ed::BeginGroupHint(node.ID))
            {
                //auto alpha   = static_cast<int>(commentAlpha * ImGui::GetStyle().Alpha * 255);
                auto bgAlpha = static_cast<int>(ImGui::GetStyle().Alpha * 255);

                //ImGui::PushStyleVar(ImGuiStyleVar_Alpha, commentAlpha * ImGui::GetStyle().Alpha);

                auto min = ed::GetGroupMin();
                //auto max = ed::GetGroupMax();

                ImGui::SetCursorScreenPos(min - ImVec2(-8, ImGui::GetTextLineHeightWithSpacing() + 4));
                ImGui::BeginGroup();
                ImGui::TextUnformatted(node.Name.c_str());
                ImGui::EndGroup();

                auto drawList = ed::GetHintBackgroundDrawList();

                auto hintBounds      = ImGui_GetItemRect();
                auto hintFrameBounds = ImRect_Expanded(hintBounds, 8, 4);

                drawList->AddRectFilled(
                    hintFrameBounds.GetTL(),
                    hintFrameBounds.GetBR(),
                    IM_COL32(255, 255, 255, 64 * bgAlpha / 255), 4.0f);

                drawList->AddRect(
                    hintFrameBounds.GetTL(),
                    hintFrameBounds.GetBR(),
                    IM_COL32(255, 255, 255, 128 * bgAlpha / 255), 4.0f);

                //ImGui::PopStyleVar();
            }
            ed::EndGroupHint();
        }

        for (auto& link : m_Links)
            ed::Link(link.ID, link.StartPinID, link.EndPinID, link.Color, 2.0f);

        if (!createNewNode)
        {
            if (ed::BeginCreate(ImColor(255, 255, 255), 2.0f))
            {
                auto showLabel = [](const char* label, ImColor color)
                {
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
                    auto size = ImGui::CalcTextSize(label);

                    auto padding = ImGui::GetStyle().FramePadding;
                    auto spacing = ImGui::GetStyle().ItemSpacing;

                    ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

                    auto rectMin = ImGui::GetCursorScreenPos() - padding;
                    auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

                    auto drawList = ImGui::GetWindowDrawList();
                    drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
                    ImGui::TextUnformatted(label);
                };

                ed::PinId startPinId = 0, endPinId = 0;
                if (ed::QueryNewLink(&startPinId, &endPinId))
                {
                    auto startPin = FindPin(startPinId);
                    auto endPin   = FindPin(endPinId);

                    newLinkPin = startPin ? startPin : endPin;

                    if (startPin->Kind == PinKind::Input)
                    {
                        std::swap(startPin, endPin);
                        std::swap(startPinId, endPinId);
                    }

                    if (startPin && endPin)
                    {
                        if (endPin == startPin)
                        {
                            ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                        }
                        else if (endPin->Kind == startPin->Kind)
                        {
                            showLabel("x Incompatible Pin Kind", ImColor(45, 32, 32, 180));
                            ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                        }
                        else if (endPin->Node == startPin->Node)
                        {
                            showLabel("x Cannot connect to self", ImColor(45, 32, 32, 180));
                            ed::RejectNewItem(ImColor(255, 0, 0), 1.0f);
                        }
                        else if (endPin->Type != startPin->Type)
                        {
                            Node* (NodeEditorInternal::*function)(SpawnNodeStruct) = ConversionAvalible(startPin, endPin).function;
                            if (function != nullptr)
                            {
								showLabel("+ Create Conversion", ImColor(32, 45, 32, 180));
                                if (ed::AcceptNewItem(ImColor(128, 206, 244), 4.0f))
                                {
                                    Node* node = (this->*function)({});
                                    
                                    Node* startNode = startPin->Node;
                                    Node* endNode = endPin->Node;
                                    ed::SetNodePosition(node->ID, (ed::GetNodePosition(startNode->ID) + ed::GetNodePosition(endNode->ID)) / 2);

									m_Links.emplace_back(Link(GetNextId(), startPin->ID, node->Inputs[0].ID));
									m_Links.back().Color = GetIconColor(startPin->Type);

									m_Links.emplace_back(Link(GetNextId(), node->Outputs[0].ID, endPin->ID));
									m_Links.back().Color = GetIconColor(endPin->Type);

									//ed::QueryNewLink(&startPin->ID, &node->Inputs[0].ID);
									//ed::AcceptNewItem();
                                }
                            }
                            else
                            {
								showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
								ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
                            }
                        }
                        else
                        {
                            showLabel("+ Create Link", ImColor(32, 45, 32, 180));
                            if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
                            {
                                
                                for (int x = 0; x < m_Links.size(); x++)
                                {
                                    if (m_Links[x].EndPinID == endPinId && endPin->Type != PinType::Flow)
                                    {
                                        ed::DeleteLink(m_Links[x].ID);
                                    }
                                    else if (m_Links[x].StartPinID == startPinId && endPin->Type == PinType::Flow)
                                        ed::DeleteLink(m_Links[x].ID);
                                }
                                

                                m_Links.emplace_back(Link(GetNextId(), startPinId, endPinId));
                                m_Links.back().Color = GetIconColor(startPin->Type);
                            }
                        }
                    }
                }

                ed::PinId pinId = 0;
                if (ed::QueryNewNode(&pinId))
                {
                    newLinkPin = FindPin(pinId);
                    if (newLinkPin)
                        showLabel("+ Create Node", ImColor(32, 45, 32, 180));

                    if (ed::AcceptNewItem())
                    {
                        createNewNode  = true;
                        newNodeLinkPin = FindPin(pinId);
                        newLinkPin = nullptr;
                        ed::Suspend();
                        ImGui::OpenPopup("Create New Node");
                        ResetSearchArea = true;
                        ed::Resume();
                    }
                }
            }
            else
                newLinkPin = nullptr;

            ed::EndCreate();

            if (ed::BeginDelete())
            {
                ed::LinkId linkId = 0;
                while (ed::QueryDeletedLink(&linkId))
                {
                    if (ed::AcceptDeletedItem())
                    {
                        auto id = std::find_if(m_Links.begin(), m_Links.end(), [linkId](auto& link) { return link.ID == linkId; });
                        if (id != m_Links.end())
                            m_Links.erase(id);
                    }
                }

                ed::NodeId nodeId = 0;
                while (ed::QueryDeletedNode(&nodeId))
                {
                    if (ed::AcceptDeletedItem())
                    {
                        auto id = std::find_if(m_Nodes.begin(), m_Nodes.end(), [nodeId](auto& node) { return node.ID == nodeId; });
                        if (id != m_Nodes.end())
                            m_Nodes.erase(id);
                    }
                }
            }
            ed::EndDelete();
        }

        ImGui::SetCursorScreenPos(cursorTopLeft);
    }

# if 1
    auto openPopupPosition = ImGui::GetMousePos();
    ed::Suspend();
    if (ed::ShowNodeContextMenu(&contextNodeId))
        ImGui::OpenPopup("Node Context Menu");
    else if (ed::ShowPinContextMenu(&contextPinId))
        ImGui::OpenPopup("Pin Context Menu");
    else if (ed::ShowLinkContextMenu(&contextLinkId))
        ImGui::OpenPopup("Link Context Menu");
    else if (ed::ShowBackgroundContextMenu())
    {
        ImGui::OpenPopup("Create New Node");
        ResetSearchArea = true;
        newNodeLinkPin = nullptr;
    }
    ed::Resume();

    ed::Suspend();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    if (ImGui::BeginPopup("Node Context Menu"))
    {
        auto node = FindNode(contextNodeId);

        ImGui::TextUnformatted("Node Context Menu");
        ImGui::Separator();
        if (node)
        {
            ImGui::Text("ID: %p", node->ID.AsPointer());
            ImGui::Text("Type: %s", node->Type == NodeType::Blueprint ? "Blueprint" : (node->Type == NodeType::Tree ? "Tree" : "Comment"));
            ImGui::Text("Inputs: %d", (int)node->Inputs.size());
            ImGui::Text("Outputs: %d", (int)node->Outputs.size());
        }
        else
            ImGui::Text("Unknown node: %p", contextNodeId.AsPointer());
        ImGui::Separator();
        if (ImGui::MenuItem("Delete"))
            ed::DeleteNode(contextNodeId);
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Pin Context Menu"))
    {
        auto pin = FindPin(contextPinId);

        ImGui::TextUnformatted("Pin Context Menu");
        ImGui::Separator();
        if (pin)
        {
            ImGui::Text("ID: %p", pin->ID.AsPointer());
            if (pin->Node)
                ImGui::Text("Node: %p", pin->Node->ID.AsPointer());
            else
                ImGui::Text("Node: %s", "<none>");
        }
        else
            ImGui::Text("Unknown pin: %p", contextPinId.AsPointer());

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Link Context Menu"))
    {
        auto link = FindLink(contextLinkId);

        ImGui::TextUnformatted("Link Context Menu");
        ImGui::Separator();
        if (link)
        {
            ImGui::Text("ID: %p", link->ID.AsPointer());
            ImGui::Text("From: %p", link->StartPinID.AsPointer());
            ImGui::Text("To: %p", link->EndPinID.AsPointer());
        }
        else
            ImGui::Text("Unknown link: %p", contextLinkId.AsPointer());
        ImGui::Separator();
        if (ImGui::MenuItem("Delete"))
            ed::DeleteLink(contextLinkId);
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Create New Node"))
    {
        InitSearchData();

        auto newNodePostion = openPopupPosition;
        //ImGui::SetCursorScreenPos(ImGui::GetMousePosOnOpeningCurrentPopup());

        //auto drawList = ImGui::GetWindowDrawList();
        //drawList->AddCircleFilled(ImGui::GetMousePosOnOpeningCurrentPopup(), 10.0f, 0xFFFF00FF);

        Node* node = nullptr;
        static bool ContextSensitive = false;

        static std::string SearchBuffer;
        static std::string PreviousSearchBuffer;

        if (ResetSearchArea) { SearchBuffer = ""; PreviousSearchBuffer = ""; }

        ImGui::Text("All Actions for this File");
        ImGui::SameLine();
        ImGui::Custom::Checkbox("##NodeSearchPopupContextSensitiveCheckbox", &ContextSensitive);
        ImGui::SameLine();
        ImGui::Text("Context Sensitive");


        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        std::strncpy(buffer, SearchBuffer.c_str(), sizeof(buffer));

        static bool SetKeyboardFocus = false;

        if (SetKeyboardFocus)
        {
            ImGui::SetKeyboardFocusHere();
            SetKeyboardFocus = false;
        }

        if (ResetSearchArea)
        {
            SetKeyboardFocus = true;
        }

        

        if (ImGui::InputTextWithHint("##NodeEditorSearchBar", "Search:", buffer, sizeof(buffer)))
        {
            SearchBuffer = std::string(buffer);

        }
		

		bool Searching = SearchBuffer != "";

		//if ((SearchBuffer != PreviousSearchBuffer) || ResetSearchArea)
		//{
		//	std::vector<std::string> treeNodes = { "Math", "Float" };
        //
		//	for (int i = 0; i < treeNodes.size(); i++)
		//	{
		//		auto id = ImGui::GetID(treeNodes[i].c_str());
		//		if (ImGui::GetStateStorage()->GetInt(id) == Searching ? 0 : 1)
		//		{
		//			ImGui::GetStateStorage()->SetInt(id, Searching ? 1 : 0);
		//		}
		//	}
		//}



        if (ResetSearchArea)
        {
            
        }

        //if ((SearchBuffer != PreviousSearchBuffer) || ResetSearchArea) { CloseAllTreeNodes(searchData, SearchBuffer); }
        //CloseSearchList(searchData);

        if (((SearchBuffer != PreviousSearchBuffer) || ResetSearchArea))
        {
            if (SearchBuffer == "") CloseSearchList(&m_SearchData);
            else OpenSearchList(&m_SearchData);
        }

        Node* returnNode = DisplaySearchData(&m_SearchData, SearchBuffer);



        node = returnNode;

        ResetSearchArea = false;
        PreviousSearchBuffer = SearchBuffer;

        if (node)
        {
            BuildNodes();

            createNewNode = false;

            ed::SetNodePosition(node->ID, newNodePostion);

            if (auto startPin = newNodeLinkPin)
            {
                auto& pins = startPin->Kind == PinKind::Input ? node->Outputs : node->Inputs;

                for (auto& pin : pins)
                {
                    if (CanCreateLink(startPin, &pin))
                    {
                        auto endPin = &pin;
                        if (startPin->Kind == PinKind::Input)
                            std::swap(startPin, endPin);

                        m_Links.emplace_back(Link(GetNextId(), startPin->ID, endPin->ID));
                        m_Links.back().Color = GetIconColor(startPin->Type);

                        break;
                    }
                }
            }
        }

        ImGui::EndPopup();
    }
    else
        createNewNode = false;
    ImGui::PopStyleVar();
    ed::Resume();
# endif

/*
    cubic_bezier_t c;
    c.p0 = pointf(100, 600);
    c.p1 = pointf(300, 1200);
    c.p2 = pointf(500, 100);
    c.p3 = pointf(900, 600);

    auto drawList = ImGui::GetWindowDrawList();
    auto offset_radius = 15.0f;
    auto acceptPoint = [drawList, offset_radius](const bezier_subdivide_result_t& r)
    {
        drawList->AddCircle(to_imvec(r.point), 4.0f, IM_COL32(255, 0, 255, 255));

        auto nt = r.tangent.normalized();
        nt = pointf(-nt.y, nt.x);

        drawList->AddLine(to_imvec(r.point), to_imvec(r.point + nt * offset_radius), IM_COL32(255, 0, 0, 255), 1.0f);
    };

    drawList->AddBezierCurve(to_imvec(c.p0), to_imvec(c.p1), to_imvec(c.p2), to_imvec(c.p3), IM_COL32(255, 255, 255, 255), 1.0f);
    cubic_bezier_subdivide(acceptPoint, c);
*/

    ed::End();

	ImGui::SameLine(0.0f, 12.0f);


	ImGui::BeginChild("Nodes Info", ImVec2(panelC, 0));
    //ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.0f, 0.0f, 1.0f)));
	ImGui::EndChild();


    //ImGui::ShowTestWindow();
    //ImGui::ShowMetricsWindow();
}

