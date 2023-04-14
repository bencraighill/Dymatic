#include "ScriptEditor.h"

#include "Settings/Preferences.h"
#include "TextSymbols.h"

#include <imgui/imgui.h>
#include "NodeUtilities/builders.h"
#include "NodeUtilities/widgets.h"
#include <ImGuiNode/imgui_node_editor.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_internal.h>

#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <utility>

#include "Dymatic/Core/Base.h"
#include "Dymatic/Renderer/Texture.h"

#include <fstream>

#include "Dymatic/Math/Math.h"
#include "Dymatic/Math/StringUtils.h"

#include "Dymatic/Scripting/ScriptEngine.h"

#include "Dymatic/Core/Input.h"

//Opening Files
#include "Dymatic/Utils/PlatformUtils.h"

namespace Dymatic {

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

enum class PinKind
{
    Output,
    Input
};

enum class NodeFunction
{
    Node,

    EventOverride,
    Function,

    Variable,

    EventImplementation,
    EventCall,

    Macro,
    MacroBlock
};

enum class NodeType
{
    Blueprint,
    Point,
    Simple,
    Tree,
    Comment,
    Houdini
};

struct Node;

struct Pin
{
	ed::PinId   ID;
	Node* Node;
	std::string Name;
	std::string DisplayName;
    ParameterDeclaration Data;
	PinKind Kind;
    bool Deletable = false;

    // Compile Time Data
    bool Written = false;
    std::string InternalName;

    Pin() = default;
	Pin(int id, std::string name, std::string type) :
		ID(id), Node(nullptr), Name(name), Kind(PinKind::Input)
	{
        Data.Type = type;
	}

	Pin(ParameterDeclaration data, PinKind kind)
		: Data(data), Kind(kind)
	{
	}

	std::string& GetName()
	{
		return DisplayName.empty() ? Name : DisplayName;
	}
};

struct Node
{
	ed::NodeId ID;
	std::string Name;
    std::string DisplayName;

    Ref<Texture2D> Icon;
    ImColor IconColor = ImColor(255, 255, 255);

	std::vector<Pin> Inputs;
	std::vector<Pin> Outputs;
	ImColor Color;
	NodeType Type;
    NodeFunction Function;

    ImVec2 Position = {};
	ImVec2 Size;
    ImRect GroupBounds;

    bool Internal = false;

	std::string State;
	std::string SavedState;

    // Node Comment Data
    bool CommentEnabled = false;
    bool CommentPinned = false;
    std::string Comment = "";

    bool AddInputs = false;
    bool AddOutputs = false;
    bool Deletable = true;
    
    std::string InternalName;
    bool Pure = false;

    int ReferenceId = 0;

    // Compile Time Data
    int UbergraphID = -1;
    bool Written = false;
    bool Error = false;

    Node() = default;

	Node(int id, std::string name, ImColor color = ImColor(255, 255, 255), NodeFunction Function = NodeFunction::Node) :
		ID(id), Name(name), Color(color), Type(NodeType::Blueprint), Size(0, 0), Function(Function)
	{
	}
};

struct Link
{
    ed::LinkId ID;

    ed::PinId StartPinID;
    ed::PinId EndPinID;

    ImColor Color;

    Link() = default;

    Link(ed::LinkId id, ed::PinId startPinId, ed::PinId endPinId):
        ID(id), StartPinID(startPinId), EndPinID(endPinId), Color(255, 255, 255)
    {
    }
};

struct Variable
{
    int ID;
    std::string Name;
    std::string Tooltip;
    bool Public = false;

    ParameterDeclaration Data;

    Variable() = default;

	Variable(int id, std::string name, std::string type)
		: ID(id), Name(name)
	{
        Data.Type = type;
	}
};

struct NodeIdLess
{
    bool operator()(const ed::NodeId& lhs, const ed::NodeId& rhs) const
    {
        return lhs.AsPointer() < rhs.AsPointer();
    }
};

enum class CompilerResultType
{
    Info,
    Compile,
    Warning,
    Error
};

struct CompilerResult
{
    CompilerResultType Type;
    std::string Message;
    ed::NodeId ID = 0;
};

struct SearchResultData
{
	SearchResultData() = default;
	SearchResultData(const std::string& name, std::function<Node*()> callback)
		: Name(name), Callback(callback)
	{
	}

	std::string Name;
    std::function<Node*()> Callback;
};

struct SearchData
{
	std::string Name;
	std::vector<SearchData> LowerTrees;
	std::vector<SearchResultData> Results;

	bool IsOpen = false;
	bool IsConfirmed = true;

    void Open(bool open)
    {
        IsOpen = open;
        IsConfirmed = false;
        
        for (auto& lowerTree : LowerTrees)
            lowerTree.Open(open);
    }

	SearchData& AddTree(const std::string& name)
	{
		int pos = 0;
		for (auto& tree : LowerTrees)
			if (name > tree.Name) pos++;
			else break;

		LowerTrees.insert(LowerTrees.begin() + pos, { name });
		return LowerTrees[pos];
	}

    void AddResult(const std::string& name, std::function<Node*()> callback)
	{
		int pos = 0;
		for (auto& result : Results)
			if (name > result.Name) pos++;
			else break;

        Results.insert(Results.begin() + pos, { name, callback });
	}

    void Clear()
    {
        LowerTrees = {};
        Results = {};
    }
};

struct FindResultsData
{
    std::string Name;
    ImColor Color;
    int ID;
    Ref<Texture2D> Icon = nullptr;
    std::vector<FindResultsData> SubData;

    FindResultsData(const std::string& name, const ImColor& color, int id)
        : Name(name), Color(color), ID(id)
    {
    }
};

struct Ubergraph
{
    std::vector<ed::NodeId> Nodes;
    int ID;
    bool Linear = true;
    Ubergraph(int id)
        : ID(id)
    {
    }
};

class NodeCompiler;
class NodeGraph;
class GraphWindow;

class ScriptEditorInternal
{
public:
    ScriptEditorInternal()
    {
        Init();
    }

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
    
    // Events
	Node* SpawnOnCreateNode();
	Node* SpawnOnUpdateNode();
	Node* SpawnOnDestroyNode();

    // Events
	Node* SpawnEventImplementationNode();
    Node* SpawnEventOverrideNode(const std::string& name);
	Node* SpawnCallEventNode(int nodeID);

    // Macros
    Node* SpawnMacroNode(int macroID);

    // Utilities -> Flow Control
    Node* SpawnBranchNode();
    Node* SpawnSequenceNode();
    Node* SpawnRerouteNode();

	// Variables
	Node* SpawnGetVariableNode(const Variable& variable);
    Node* SpawnSetVariableNode(const Variable& variable);

    // No Category
    Node* SpawnComment();

	Node* SpawnNodeWithInternalName(const std::string& name);

    void BuildNodes();
    ImColor GetIconColor(const std::string& type);
    void DrawPinIcon(const ParameterDeclaration& data, const PinKind& kind, bool connected, int alpha);
    void DrawTypeIcon(const std::string& type);

    void ShowStyleEditor(bool* show = nullptr);
    void ShowLeftPane(float paneWidth);

    void OnEvent(Dymatic::Event& e);
    bool OnKeyPressed(Dymatic::KeyPressedEvent& e);

    // Node Compilation
	void CompileNodes();
    void RecursivePinWrite(Pin& pin, NodeCompiler& source, NodeCompiler& ubergraphHeader, NodeCompiler& ubergraphBody, std::string& line, std::vector<ed::NodeId>& node_errors, std::vector<ed::NodeId>& pureNodeList, std::vector<ed::NodeId>& localNodeList);
    void RecursiveNodeWrite(Node& node, NodeCompiler& source, NodeCompiler& ubergraphHeader, NodeCompiler& ubergraphBody, std::vector<ed::NodeId>& node_errors, std::vector<ed::NodeId>& pureNodeList, std::vector<ed::NodeId>& localNodeList);
    bool IsPinGlobal(Pin& pin, int ubergraphID);
    void RecursiveUbergraphOrganisation(Node& node, std::vector<Ubergraph>& ubergraphs, int& currentId);
    Node& GetNextExecNode(Pin& node);

    static std::string ConvertNodeFunctionToString(const NodeFunction& nodeFunction);
    static NodeFunction ConvertStringToNodeFunction(const std::string& string);
    static std::string ConvertNodeTypeToString(const NodeType& nodeType);
    static NodeType ConvertStringToNodeType(const std::string& string);
	static std::string ConvertPinKindToString(const PinKind& pinKind);
	static PinKind ConvertStringToPinKind(const std::string& string);

    void CopyNodes();
    void PasteNodes();
    void DuplicateNodes();
    void DeleteNodes();

    void ShowFlow();

    // Graph Management
    inline void SetCurrentGraph(int id) { m_CurrentGraphID = id; }
	inline NodeGraph* GetCurrentGraph() { return FindGraphByID(m_CurrentGraphID); }
	NodeGraph* FindGraphByID(int id);

    inline GraphWindow* GetCurrentWindow() { return FindWindowByID(m_CurrentWindowID); }
    GraphWindow* FindWindowByID(int id);

	GraphWindow* OpenGraph(int id);
	GraphWindow* OpenGraphInNewTab(int id);

    void ClearSelection();
    void SetNodePosition(ed::NodeId id, ImVec2 position);
    void NavigateToContent();
    void NavigateToNode(ed::NodeId id);

    void DefaultValueInput(ParameterDeclaration& data, bool spring = false);
    bool ValueTypeSelection(ParameterDeclaration& data, bool flow = false);

    void DrawGraphOption(NodeGraph& graph);
    
    void DeleteAllPinLinkAttachments(Pin* pin);
    void CheckLinkSafety(Pin* startPin, Pin* endPin);
    void CreateLink(NodeGraph& graph, Pin* a, Pin* b);

    void UpdateSearchData();
    void AddSearchData(const std::string& name, const std::string& category, const std::string& keywords, const std::vector<ParameterDeclaration>& parameters, const std::string& returnType, bool pure, std::function<Node*()> callback);
    Node* DisplaySearchData(SearchData& searchData, bool origin = true);

    void ImplicitExecuteableLinks(NodeGraph& graph, Pin* startPin, Node* node);

    void Init();
    void OnImGuiRender();

    // Create and update functions for script contents
    Variable& CreateVariable(const std::string& name = "", const std::string& type = "System.Bool");
	void SetVariableType(Variable* variable, const std::string& type);
    void SetVariableName(Variable* variable, const std::string& name);

    void CreateEventGraph();

    void CreateMacro();
    void SetMacroName(int macroId, const std::string& name);
    void AddMacroPin(int macroId, bool input);
    void UpdateMacroPin(int macroId, Pin* pin);
    void RemoveMacroPin(int macroId, Pin* pin);

    void SetCustomEventName(Node* event, const std::string& name);
    void AddCustomEventInput(Node* event);
    void UpdateCustomEventInput(Pin* input);

    inline void ResetSearchArea() 
    { 
        m_ResetSearchArea = true;
        m_SearchBuffer = "";
        m_PreviousSearchBuffer = "";
        m_NewNodeLinkPin = nullptr;
    }

    void FindResultsSearch(const std::string& search);
    
private:
    std::string m_ScriptClass;

    ImColor m_CurrentLinkColor = ImColor(255, 255, 255);

	const int m_PinIconSize = 24;
    int m_CurrentGraphID = 0;
    int m_CurrentWindowID = 0;
    std::vector<GraphWindow> m_Windows;
    std::vector<NodeGraph> m_Graphs;

    int m_SelectedDetailsReference = 0;
    NodeFunction m_SelectedDetailsFunction = NodeFunction::Node;

    std::map<int, Node> m_CustomEvents;

    int m_SelectedVariable = 0;
    std::map<int, Variable> m_Variables;
    std::string m_RecentPinType = "System.Boolean";

    bool m_HideUnconnected = false;

    Ref<Texture2D> m_HeaderBackground;
	Ref<Texture2D> m_SaveIcon;
	Ref<Texture2D> m_RestoreIcon;
	Ref<Texture2D> m_CommentIcon;
	Ref<Texture2D> m_PinIcon;
	Ref<Texture2D> m_PinnedIcon;

	Ref<Texture2D> m_RemoveIcon;

	Ref<Texture2D> m_GraphIcon;
	Ref<Texture2D> m_MacroIcon;

    // Node Icons
	Ref<Texture2D> m_EventIcon;
	Ref<Texture2D> m_CustomEventIcon;
	Ref<Texture2D> m_FunctionIcon;
	Ref<Texture2D> m_SequenceIcon;
	Ref<Texture2D> m_BranchIcon;
	Ref<Texture2D> m_SwitchIcon;

	const float m_TouchTime = 1.0f;
	std::map<ed::NodeId, float, NodeIdLess> m_NodeTouchTime;

    int m_NextId = 1;

    // Node Search Members
    SearchData m_SearchData;
	bool m_ContextSensitive = false;
	std::string m_SearchBuffer;
	std::string m_PreviousSearchBuffer;

    // Compiler and Results
    std::vector<CompilerResult> m_CompilerResults;
    std::string m_FindResultsSearchBar = "";
    std::vector<FindResultsData> m_FindResultsData;

    // Node Context
	ed::NodeId m_ContextNodeId = 0;
	ed::LinkId m_ContextLinkId = 0;
	ed::PinId  m_ContextPinId = 0;
	bool m_CreateNewNode = false;
	Pin* m_NewNodeLinkPin = nullptr;
	Pin* m_NewLinkPin = nullptr;
    ImVec2 m_NewNodePosition = {};
    float* m_DefaultColorPickerValue = nullptr;
    ImVec2 m_DefaultColorPickerPosition = { FLT_MAX, FLT_MAX };
    bool m_ResetSearchArea = false;
};

enum class NodeGraphType
{
    Event,
    Macro
};

class NodeGraph
{
public:
    NodeGraph(int id, std::string name, const NodeGraphType& type = NodeGraphType::Event)
        : ID(id), Name(name), Type(type)
    {
    }
public:
    NodeGraphType Type;

    bool Editable = true;
    std::vector<Node> Nodes;
    std::vector<Link> Links;
    int ID;
    std::string Name;
};

class GraphWindow
{
public:
    GraphWindow(int id, int graphId)
        : ID(id), GraphID(graphId)
    {
		ed::Config config;
		config.SettingsFile = "";

		InternalEditor = ed::CreateEditor(&config);
        ed::SetCurrentEditor(InternalEditor);
    }

	//~GraphWindow()
	//{
	//	ed::DestroyEditor(InternalEditor);
	//	InternalEditor = nullptr;
	//}
public:
    int ID;
    int GraphID;
    bool Initialized = false;
    bool Focused = false;

    // Comment Data
    float CommentOpacity = -2.5f;
    ed::NodeId HoveredCommentID = 0;

    ed::EditorContext* InternalEditor = nullptr;
};

class NodeCompiler
{
public:
    NodeCompiler() = default;
    std::string GenerateFunctionSignature(Node& node);
    std::string GenerateFunctionName(Node& node);

    void NewLine();
    void Write(const std::string& text);
    void WriteLine(const std::string& line);
    
    std::string UnderscoreSpaces(std::string inString);

    inline void Indent(int level = 1) { m_IndentLevel += level; }
    inline void Unindent(int level = 1) { m_IndentLevel -= level; }
    inline int& GetIndentation() { return m_IndentLevel; }

    inline void OpenScope() { WriteLine("{"); Indent(); }
    inline void CloseScope(bool semicolon = false) { Unindent(); WriteLine(semicolon ? "};" : "}"); }

    void OutputToFile(const std::filesystem::path& path);

    inline std::string& Contents() { return m_Buffer; }
private:
    std::string m_Buffer;
    int m_IndentLevel = 0;
    bool m_IndentedCurrentLine = false;
};

static std::string ConvertToDisplayString(std::string String)
{
    if (!String.empty())
    {
        String::ReplaceAll(String, "_", " ");
        const size_t length = String.length();
        for (size_t i = 0; i < length; i++)
        {
            bool first = i == 0;
            if (i != length - 1)
            if (islower(String[i]) && isupper(String[i + 1]))
            {
                String.insert(String.begin() + (i + 1), ' ');
                i++;
            }
			if (first)
				String[0] = toupper(String[0]);
            else if (String[i - 1] == ' ')
                String[i] = toupper(String[i]);
        }
    }
    return String;
}

int ScriptEditorInternal::GetNextId()
{
    return m_NextId++;
}

ed::LinkId ScriptEditorInternal::GetNextLinkId()
{
    return ed::LinkId(GetNextId());
}

void ScriptEditorInternal::TouchNode(ed::NodeId id)
{
    m_NodeTouchTime[id] = m_TouchTime;
}

float ScriptEditorInternal::GetTouchProgress(ed::NodeId id)
{
    auto it = m_NodeTouchTime.find(id);
    if (it != m_NodeTouchTime.end() && it->second > 0.0f)
        return (m_TouchTime - it->second) / m_TouchTime;
    else
        return 0.0f;
}

void ScriptEditorInternal::UpdateTouch()
{
    const auto deltaTime = ImGui::GetIO().DeltaTime;
    for (auto& entry : m_NodeTouchTime)
    {
        if (entry.second > 0.0f)
            entry.second -= deltaTime;
    }
}

Node* ScriptEditorInternal::FindNode(ed::NodeId id)
{
    for (auto& graph : m_Graphs)
		for (auto& node : graph.Nodes)
			if (node.ID == id)
				return &node;

    return nullptr;
}

Link* ScriptEditorInternal::FindLink(ed::LinkId id)
{
    for (auto& graph : m_Graphs)
		for (auto& link : graph.Links)
			if (link.ID == id)
				return &link;

    return nullptr;
}

std::vector<Link*> ScriptEditorInternal::GetPinLinks(ed::PinId id)
{
    std::vector<Link*> linksToReturn;
    for (auto& graph : m_Graphs)
		for (auto& link : graph.Links)
			if (link.StartPinID == id || link.EndPinID == id)
				linksToReturn.push_back(&link);

	return linksToReturn;
}

Pin* ScriptEditorInternal::FindPin(ed::PinId id)
{
    if (!id)
        return nullptr;

    for (auto& graph : m_Graphs)
		for (auto& node : graph.Nodes)
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

bool ScriptEditorInternal::IsPinLinked(ed::PinId id)
{
    if (!id)
        return false;

    for (auto& graph : m_Graphs)
		for (auto& link : graph.Links)
			if (link.StartPinID == id || link.EndPinID == id)
				return true;

    return false;
}

bool ScriptEditorInternal::CanCreateLink(Pin* a, Pin* b)
{
    if (a->Kind == PinKind::Input)
        std::swap(a, b);

    if (!a || !b || a == b || a->Kind == b->Kind || ((ScriptEngine::IsConversionAvalible(a->Data.Type, b->Data.Type)) ? false : (a->Data.Type != b->Data.Type)) || a->Node == b->Node)
        return false;

    return true;
}

void ScriptEditorInternal::BuildNode(Node* node)
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

// Events
Node* ScriptEditorInternal::SpawnOnCreateNode()
{
    auto& node = GetCurrentGraph()->Nodes.emplace_back(GetNextId(), "On Create", ImColor(255, 128, 128), NodeFunction::EventOverride);
	node.Outputs.emplace_back(GetNextId(), "", "Flow");
    node.Icon = m_EventIcon;

	BuildNode(&node);

	return &node;
}

Node* ScriptEditorInternal::SpawnOnUpdateNode()
{
    auto& node = GetCurrentGraph()->Nodes.emplace_back(GetNextId(), "On Update", ImColor(255, 128, 128), NodeFunction::EventOverride);
	node.Outputs.emplace_back(GetNextId(), "", "Flow");
	node.Outputs.emplace_back(GetNextId(), "Delta Seconds", "System.Single");
    node.Icon = m_EventIcon;

	BuildNode(&node);

	return &node;
}

Node* ScriptEditorInternal::SpawnOnDestroyNode()
{
    auto& node = GetCurrentGraph()->Nodes.emplace_back(GetNextId(), "On Destroy", ImColor(255, 128, 128), NodeFunction::EventOverride);
	node.Outputs.emplace_back(GetNextId(), "", "Flow");
	node.Icon = m_EventIcon;

	BuildNode(&node);

	return &node;
}

// Events
Node* ScriptEditorInternal::SpawnEventImplementationNode()
{
	auto& node = GetCurrentGraph()->Nodes.emplace_back(GetNextId(), "New Event", ImColor(255, 128, 128), NodeFunction::EventImplementation);
    node.DisplayName = "New Event";
	node.Outputs.emplace_back(GetNextId(), "", "Flow");
	node.Icon = m_CustomEventIcon;
    node.ReferenceId = node.ID.Get();

	BuildNode(&node);
	return &node;
}

Node* ScriptEditorInternal::SpawnEventOverrideNode(const std::string& name)
{
    auto& classes = ScriptEngine::GetEntityClassOverridableMethods();
    for (auto& [klass, methods] : classes)
    {
        if (klass == m_ScriptClass)
        {
            for (auto& method : methods)
            {
                if (method.FullName == name)
                {
                    auto& newNode = GetCurrentGraph()->Nodes.emplace_back(GetNextId(), method.Name, ImColor(255, 128, 128), NodeFunction::EventOverride);

					newNode.DisplayName = method.DisplayName;
					newNode.Icon = m_EventIcon;
					newNode.IconColor = method.IsPure ? ImColor(170, 242, 172) : ImColor(128, 195, 248);
					newNode.InternalName = method.FullName;
					newNode.Pure = method.IsPure;

					if (method.IsCompactNode)
						newNode.Type = NodeType::Simple;

					if (!method.IsPure)
						newNode.Outputs.emplace_back(GetNextId(), "", "Flow");

					if (method.ReturnType != "System.Void")
					{
						newNode.Outputs.emplace_back(GetNextId(), method.NoPinLabels ? "" : "Return Value", method.ReturnType);
						newNode.Outputs.back().InternalName = "return";
					}

					for (auto& param : method.Parameters)
					{
						auto& pinList = param.IsOut ? newNode.Outputs : newNode.Inputs;
						auto& pin = pinList.emplace_back(GetNextId(), (method.NoPinLabels || param.NoPinLabel) ? "" : param.Name, param.Type);
						pin.InternalName = param.Name;
						pin.Data = param;
					}

					BuildNode(&newNode);

					return &newNode;
                }
            }
        }
    }

    return nullptr;
}

Node* ScriptEditorInternal::SpawnCallEventNode(int nodeID)
{
    auto event = FindNode(nodeID);
    if (event != nullptr)
	{
		const std::string name = event->Name;

        auto& node = GetCurrentGraph()->Nodes.emplace_back(GetNextId(), name, ImColor(128, 195, 248), NodeFunction::EventCall);

        // Find again after reallocation
        event = FindNode(nodeID);

        node.DisplayName = name;
        node.Inputs.emplace_back(GetNextId(), "", "Flow");

		for (auto& input : event->Outputs)
			if (input.Data.Type != "Flow")
				node.Inputs.emplace_back(GetNextId(), input.Name, input.Data.Type);

        node.Outputs.emplace_back(GetNextId(), "", "Flow");
        node.Icon = m_CustomEventIcon;
        node.ReferenceId = nodeID;

        BuildNode(&node);
        return &node;
    }
    return nullptr;
}

// Macros
Node* ScriptEditorInternal::SpawnMacroNode(int macroID)
{
	if (auto macro = FindGraphByID(macroID))
	{
		const std::string name = macro->Name;
		auto& node = GetCurrentGraph()->Nodes.emplace_back(GetNextId(), name, ImColor(255, 255, 255), NodeFunction::Macro);

		// Find again after reallocation
        macro = FindGraphByID(macroID);

		node.DisplayName = name;

        // Find macro inputs node and copy inputs
        for (auto& macro_node : macro->Nodes)
            if (macro_node.Function == NodeFunction::MacroBlock && macro_node.Name == "Inputs")
            {
                for (auto& input : macro_node.Outputs)
                    node.Inputs.emplace_back(GetNextId(), input.Name, input.Data.Type);
                break;
            }

		// Find macro outputs node and copy output
		for (auto& macro_node : macro->Nodes)
			if (macro_node.Function == NodeFunction::MacroBlock && macro_node.Name == "Outputs")
			{
				for (auto& output : macro_node.Inputs)
					node.Outputs.emplace_back(GetNextId(), output.Name, output.Data.Type);
				break;
			}

		node.Icon = m_MacroIcon;
		node.ReferenceId = macroID;

		BuildNode(&node);
		return &node;
	}
	return nullptr;
}

// Variables
Node* ScriptEditorInternal::SpawnGetVariableNode(const Variable& variable)
{
    auto& node = GetCurrentGraph()->Nodes.emplace_back(GetNextId(), "Get " + variable.Name, ImColor(170, 242, 172), NodeFunction::Variable);
    node.DisplayName = "Get " + variable.Name;
	node.Type = NodeType::Simple;
	node.Outputs.emplace_back(GetNextId(), "", variable.Data.Type);
	node.ReferenceId = variable.ID;
          
	BuildNode(&node);
          
	return &node;
}

Node* ScriptEditorInternal::SpawnSetVariableNode(const Variable& variable)
{
    auto& node = GetCurrentGraph()->Nodes.emplace_back(GetNextId(), "Set " + variable.Name, ImColor(128, 195, 248), NodeFunction::Variable);
	node.DisplayName = "Set " + variable.Name;
	node.Inputs.emplace_back(GetNextId(), "", "Flow");
	node.Inputs.emplace_back(GetNextId(), "", variable.Data.Type);
	node.Outputs.emplace_back(GetNextId(), "", "Flow");
	node.Outputs.emplace_back(GetNextId(), "", variable.Data.Type);
	node.ReferenceId = variable.ID;

	BuildNode(&node);

	return &node;
}

// Utilities -> Flow Control
Node* ScriptEditorInternal::SpawnBranchNode()
{
    auto& node = GetCurrentGraph()->Nodes.emplace_back(GetNextId(), "Branch");
    node.Inputs.emplace_back(GetNextId(), "", "Flow");
    node.Inputs.emplace_back(GetNextId(), "Condition", "System.Boolean");
    node.Outputs.emplace_back(GetNextId(), "True", "Flow");
    node.Outputs.emplace_back(GetNextId(), "False", "Flow");
    node.Icon = m_BranchIcon;
    node.Internal = true;

    BuildNode(&node);

    return &node;
}

Node* ScriptEditorInternal::SpawnSequenceNode()
{
    auto& node = GetCurrentGraph()->Nodes.emplace_back(GetNextId(), "Sequence");
	node.Inputs.emplace_back(GetNextId(), "", "Flow");
	node.Outputs.emplace_back(GetNextId(), "0", "Flow");
	node.Outputs.emplace_back(GetNextId(), "1", "Flow");
    node.AddOutputs = true;
    node.Icon = m_SequenceIcon;
    node.Internal = true;

	BuildNode(&node);

	return &node;
}

Node* ScriptEditorInternal::SpawnRerouteNode()
{
	auto& node = GetCurrentGraph()->Nodes.emplace_back(GetNextId(), "Reroute");
	node.Type = NodeType::Point;
	node.Inputs.emplace_back(GetNextId(), "", "System.Boolean");
	node.Outputs.emplace_back(GetNextId(), "", "System.Boolean");

	BuildNode(&node);

	return &node;
}

// No Category
Node* ScriptEditorInternal::SpawnComment()
{
    auto& node = GetCurrentGraph()->Nodes.emplace_back(GetNextId(), "Comment");
    node.Type = NodeType::Comment;

    if (ed::GetSelectedObjectCount() > 0)
    {
        float padding = 20.0f;
        ImVec2 min, max;
        ed::GetSelectionBounds(min, max);

        SetNodePosition(node.ID, ImVec2(min.x - padding, min.y - padding * 2.0f));
        node.Size = ImVec2(max.x - min.x + padding, max.y - min.y + padding);
    }
    else
        node.Size = ImVec2(300, 100);


    return &node;
}

Node* ScriptEditorInternal::SpawnNodeWithInternalName(const std::string& name)
{
    for (const auto& [methodName, method] : ScriptEngine::GetStaticMethodDeclarations())
    {
        if (method.FullName == name)
        {
            auto& newNode = GetCurrentGraph()->Nodes.emplace_back(GetNextId(), method.Name.c_str(), method.IsPure ? ImColor(170, 242, 172) : ImColor(128, 195, 248), NodeFunction::Node);
			
            newNode.DisplayName = method.DisplayName;
			newNode.Icon = m_FunctionIcon;
			newNode.IconColor = method.IsPure ? ImColor(170, 242, 172) : ImColor(128, 195, 248);
            newNode.InternalName = method.FullName;
			newNode.Pure = method.IsPure;
			
			if (method.IsCompactNode)
                newNode.Type = NodeType::Simple;
			
			if (!method.IsPure)
			{
				newNode.Inputs.emplace_back(GetNextId(), "", "Flow");
				newNode.Outputs.emplace_back(GetNextId(), "", "Flow");
			}
			
			if (method.ReturnType != "System.Void")
			{
				newNode.Outputs.emplace_back(GetNextId(), method.NoPinLabels ? "" : "Return Value", method.ReturnType);
				newNode.Outputs.back().InternalName = "return";
			}
			
            for (auto& param : method.Parameters)
            {
                auto& pinList = param.IsOut ? newNode.Outputs : newNode.Inputs;
                auto& pin = pinList.emplace_back(GetNextId(), (method.NoPinLabels || param.NoPinLabel) ? "" : param.Name, param.Type);
                pin.InternalName = param.Name;
                pin.Data = param;
            }
			
			BuildNode(&newNode);
			
			return &newNode;
        }
    }

    return nullptr;
}

void ScriptEditorInternal::BuildNodes()
{
    for (auto& graph : m_Graphs)
		for (auto& node : graph.Nodes)
			BuildNode(&node);
}

ImColor ScriptEditorInternal::GetIconColor(const std::string& type)
{
	if (type == "Flow") return ImColor(255, 255, 255);
	if (type == "System.Boolean") return ImColor(220, 48, 48);
	if (type == "System.Int32") return ImColor(68, 201, 156);
	if (type == "System.Single") return ImColor(147, 226, 74);
	if (type == "System.String") return ImColor(218, 0, 183);
	if (type == "Dymatic.Vector3") return ImColor(242, 192, 33);
	if (type == "Dymatic.Color") return ImColor(0, 87, 200);
	if (type == "System.Byte") return ImColor(6, 100, 92);
	if (type == "Delegate") return ImColor(218, 0, 183);
    
    return ImColor(51, 150, 215);
};

void ScriptEditorInternal::DrawPinIcon(const ParameterDeclaration& data, const PinKind& kind, bool connected, int alpha)
{
    IconType iconType;
    ImColor  color = GetIconColor(data.Type);
    color.Value.w = alpha / 255.0f;

    if (data.Type == "Flow") iconType = IconType::Flow;
    else if (data.Type == "Delegate") iconType = IconType::Square;
    else iconType = IconType::Circle;

    if (data.ByRef && !data.IsOut)
        iconType = IconType::Diamond;

    ax::Widgets::Icon(ImVec2(m_PinIconSize, m_PinIconSize), iconType, connected, color, ImColor(32, 32, 32, alpha));
};

void ScriptEditorInternal::DrawTypeIcon(const std::string& type)
{
    ImColor  color = GetIconColor(type);
    auto& cursorPos = ImGui::GetCursorScreenPos();
    ax::Drawing::DrawIcon(ImGui::GetWindowDrawList(), cursorPos, cursorPos + ImVec2(m_PinIconSize, m_PinIconSize), IconType::Capsule, true, color, ImColor(32, 32, 32));
}

void ScriptEditorInternal::ShowStyleEditor(bool* show)
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

void ScriptEditorInternal::ShowLeftPane(float paneWidth)
{
    if (ed::GetCurrentEditor() != nullptr)
    {
        auto& io = ImGui::GetIO();

        ImGui::BeginChild("Selection", ImVec2(paneWidth, 0));

        paneWidth = ImGui::GetContentRegionAvailWidth();

        static bool showStyleEditor = false;
        ImGui::BeginHorizontal("Style Editor", ImVec2(paneWidth, 0));
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

        int saveIconWidth = m_SaveIcon->GetWidth();
        int saveIconHeight = m_SaveIcon->GetHeight();
        int restoreIconWidth = m_RestoreIcon->GetWidth();
        int restoreIconHeight = m_RestoreIcon->GetHeight();

        ImGui::GetWindowDrawList()->AddRectFilled(
            ImGui::GetCursorScreenPos(),
            ImGui::GetCursorScreenPos() + ImVec2(paneWidth, ImGui::GetTextLineHeight()),
            ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), ImGui::GetTextLineHeight() * 0.25f);
        ImGui::Spacing(); ImGui::SameLine();
        ImGui::TextUnformatted("NODES");
        ImGui::Indent();
        for (auto& graph : m_Graphs)
        {
            for (auto& node : graph.Nodes)
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
                        drawList->AddImage(reinterpret_cast<void*>(m_SaveIcon->GetRendererID()), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 1), ImVec2(1, 0), IM_COL32(255, 255, 255, 96));
                    else if (ImGui::IsItemHovered())
                        drawList->AddImage(reinterpret_cast<void*>(m_SaveIcon->GetRendererID()), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 1), ImVec2(1, 0), IM_COL32(255, 255, 255, 255));
                    else
                        drawList->AddImage(reinterpret_cast<void*>(m_SaveIcon->GetRendererID()), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 1), ImVec2(1, 0), IM_COL32(255, 255, 255, 160));
                }
                else
                {
                    ImGui::Dummy(ImVec2((float)saveIconWidth, (float)saveIconHeight));
                    drawList->AddImage(reinterpret_cast<void*>(m_SaveIcon->GetRendererID()), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 1), ImVec2(1, 0), IM_COL32(255, 255, 255, 32));
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
                        drawList->AddImage(reinterpret_cast<void*>(m_RestoreIcon->GetRendererID()), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 1), ImVec2(1, 0), IM_COL32(255, 255, 255, 96));
                    else if (ImGui::IsItemHovered())
                        drawList->AddImage(reinterpret_cast<void*>(m_RestoreIcon->GetRendererID()), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 1), ImVec2(1, 0), IM_COL32(255, 255, 255, 255));
                    else
                        drawList->AddImage(reinterpret_cast<void*>(m_RestoreIcon->GetRendererID()), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 1), ImVec2(1, 0), IM_COL32(255, 255, 255, 160));
                }
                else
                {
                    ImGui::Dummy(ImVec2((float)restoreIconWidth, (float)restoreIconHeight));
                    drawList->AddImage(reinterpret_cast<void*>(m_RestoreIcon->GetRendererID()), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 1), ImVec2(1, 0), IM_COL32(255, 255, 255, 32));
                }

                ImGui::SameLine(0, 0);
                ImGui::SetItemAllowOverlap();
                ImGui::Dummy(ImVec2(0, (float)restoreIconHeight));

                ImGui::PopID();
            }
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
            ClearSelection();
        ImGui::EndHorizontal();
        ImGui::Indent();
        for (int i = 0; i < nodeCount; ++i) ImGui::Text("Node (%p)", selectedNodes[i].AsPointer());
        for (int i = 0; i < linkCount; ++i) ImGui::Text("Link (%p)", selectedLinks[i].AsPointer());
        ImGui::Unindent();

        if (ed::HasSelectionChanged())
            ++changeCount;

        ImGui::EndChild();
    }
}

void ScriptEditorInternal::OnEvent(Dymatic::Event& e)
{
    Dymatic::EventDispatcher dispatcher(e);

	dispatcher.Dispatch<Dymatic::KeyPressedEvent>(DY_BIND_EVENT_FN(ScriptEditorInternal::OnKeyPressed));
}

bool ScriptEditorInternal::OnKeyPressed(Dymatic::KeyPressedEvent& e)
{
    return false;
}

std::string NodeCompiler::UnderscoreSpaces(std::string inString)
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

void ScriptEditorInternal::CompileNodes()
{
	// Clear Previous Compilation
	m_CompilerResults.clear();
	for (auto& graph : m_Graphs)
		for (auto& node : graph.Nodes)
		{
			node.Error = false;
			node.UbergraphID = -1;
		}

    // Make copy of node graph to roll back to after compilation is complete.
	std::vector<NodeGraph> graphs_backup = m_Graphs;
	std::vector<ed::NodeId> node_errors;

    BuildNodes();

    int compileTimeNextId = m_NextId + 1;

	std::string NodeScriptName = "NodeScript";
    NodeCompiler source;

    ShowFlow();

	// Set Window Focus
	ImGui::SetWindowFocus("Compiler Results");

	m_CompilerResults.push_back({ CompilerResultType::Info, "Build started [C#] - Dymatic Script Nodes Version " DY_VERSION_STRING " (" + NodeScriptName + ")" });

	// Pre Compile Checks //

	// Invalid Variable Usage
	m_CompilerResults.push_back({ CompilerResultType::Info, "Initializing Pre Compile Variable Checks" });
	for (auto& graph : m_Graphs)
		for (auto& node : graph.Nodes)
		{
            if (node.Function == NodeFunction::Variable)
                if (m_Variables.find(node.ReferenceId) == m_Variables.end())
                {
                    m_CompilerResults.push_back({ CompilerResultType::Error, "Variable in node '" + node.DisplayName +"' does not exist.", node.ID });
					node_errors.emplace_back(node.ID);
                }
		}

	// Invalid Pin Links
	m_CompilerResults.push_back({ CompilerResultType::Info, "Initializing Pre Compile Link Checks" });
    for (auto& graph : m_Graphs)
		for (size_t linkIndex = 0; linkIndex < graph.Links.size(); linkIndex++)
		{
            auto& link = graph.Links[linkIndex];

			auto startPin = FindPin(link.StartPinID);
			auto endPin = FindPin(link.EndPinID);
            if (startPin != nullptr && endPin != nullptr)
            {
                if (startPin->Data.Type != endPin->Data.Type)
                {
                    m_CompilerResults.push_back({ CompilerResultType::Error, "Can't connect pins : Data type is not the same.", startPin->Node->ID });
                    node_errors.emplace_back(startPin->Node->ID);
                    node_errors.emplace_back(endPin->Node->ID);
                }
            }
            else
            {
                graph.Links.erase(graph.Links.begin() + linkIndex);
                linkIndex--;
            }
		}

    // Pre Compile Node Checks
    m_CompilerResults.push_back({ CompilerResultType::Info, "Initializing Pre Compile Node Checks" });
    for (auto& graph : m_Graphs)
    {
        for (int nodeIndex = 0; nodeIndex < graph.Nodes.size(); nodeIndex++)
        {
            auto& node = graph.Nodes[nodeIndex];
            for (int pinIndex = 0; pinIndex < node.Inputs.size(); pinIndex++)
            {
                {
                    auto& node = graph.Nodes[nodeIndex];
                    auto& pin = node.Inputs[pinIndex];
                    if (pin.Data.ByRef)
                        if (!IsPinLinked(pin.ID))
                        {
                            m_CompilerResults.push_back({ CompilerResultType::Error, "The pin '" + pin.Name + "' in node '" + node.Name + "' is a reference and expects a linked input.", node.ID });
                            node_errors.emplace_back(node.ID);
                        }
                }
            }
        }
    }

    // Rebuild nodes after alterations have been made by the precompiler
    BuildNodes();

	source.WriteLine("//Dymatic C# Node Script - Version " DY_VERSION_STRING);
    source.NewLine();
	source.WriteLine("using System;");
    source.WriteLine("using Dymatic;");
    source.NewLine();

    // Open namespace and begin class
	source.WriteLine("namespace Sandbox");
    source.OpenScope();
    source.WriteLine("public class " + NodeScriptName + " : " + m_ScriptClass);
    source.OpenScope();

    // Write all Variables
    for (auto& [id, variable] : m_Variables)
    {
		if (!variable.Tooltip.empty())
			source.WriteLine("// Tooltip: " + variable.Tooltip);
        source.WriteLine((variable.Public ? "public " : "private ") + variable.Data.Type + " " + source.UnderscoreSpaces("bpv__" + variable.Name + "_" + std::to_string(variable.ID)) + "__pf" + (variable.Data.Value.empty() ? ";" : (" = " + variable.Data.Value + ";")));
    }

    source.NewLine();

    source.WriteLine("public override void OnCreate()");
    source.OpenScope();
    for (auto& graph : m_Graphs)
		for (auto& node : graph.Nodes)
			if (node.Function == NodeFunction::EventOverride)
				if (node.Name == "On Create")
					source.WriteLine(source.GenerateFunctionName(node) + "();");
    source.CloseScope();
    source.NewLine();

	source.WriteLine("public override void OnDestroy()");
	source.OpenScope();
    for (auto& graph : m_Graphs)
		for (auto& node : graph.Nodes)
            if (node.Function == NodeFunction::EventOverride)
				if (node.Name == "On Destroy")
					source.WriteLine(source.GenerateFunctionName(node) + "();");
	source.CloseScope();
	source.NewLine();

    source.WriteLine("public override void OnUpdate(float ts)");
    source.OpenScope();
    for (auto& graph : m_Graphs)
		for (auto& node : graph.Nodes)
            if (node.Function == NodeFunction::EventOverride)
				if (node.Name == "On Update")
					source.WriteLine(source.GenerateFunctionName(node) + "(ts);");
    source.CloseScope();
    source.NewLine();

    for (auto& graph : m_Graphs)
    {
        // Generate Ubergraphs
        std::vector<Ubergraph> ubergraphs;
        for (auto& node : graph.Nodes)
            if (node.Function == NodeFunction::EventOverride || node.Function == NodeFunction::EventImplementation)
            {
                int id = compileTimeNextId++;
                ubergraphs.push_back(id);
                RecursiveUbergraphOrganisation(node, ubergraphs, id);
            }

        // Store ubergraph ID in each node for ease of access.
        for (auto& ubergraph : ubergraphs)
            for (auto& node : ubergraph.Nodes)
                FindNode(node)->UbergraphID = ubergraph.ID;

        // Generate code for each ubergraph
        for (auto& ubergraph : ubergraphs)
        {
            for (auto& node : graph.Nodes)
            {
                node.Written = false;
                for (auto& input : node.Inputs)
                    input.Written = false;
                for (auto& output : node.Outputs)
                    output.Written = false;
            }

            NodeCompiler ubergraphHeader;
            ubergraphHeader.GetIndentation() = source.GetIndentation();
            ubergraphHeader.NewLine();
            
            ubergraphHeader.WriteLine("public void bpf__ExecuteUbergraph_" + ubergraphHeader.UnderscoreSpaces(NodeScriptName) + "__pf_" + std::to_string(ubergraph.ID) + "(System.Int32 bpp__EntryPoint__pf)");
            ubergraphHeader.OpenScope();

            NodeCompiler ubergraphBody;
            ubergraphBody.GetIndentation() = ubergraphHeader.GetIndentation();
            ubergraphBody.NewLine();

            if (ubergraph.Nodes.size() == 1)
            {
                ubergraphBody.WriteLine("Dymatic.Core.Assert(bpp__EntryPoint__pf == " + std::to_string(ubergraph.Nodes[0].Get()) + ");");
                ubergraphBody.WriteLine("return; // Termination end of function");
            }
            else
            {
                // Node checks for internal compilation optimization
                bool Sequences = false;
                bool nonLinearUbergraph = !ubergraph.Linear;
                for (auto& nodeID : ubergraph.Nodes)
                {
                    auto node = FindNode(nodeID.Get());
                    if (node->Internal)
                    {
                        if (node->Name == "Sequence")
                        {
                            bool found = false;
                            for (auto& pin : node->Outputs)
                                if (IsPinLinked(pin.ID))
                                {
                                    if (found)
                                    {
                                        Sequences = true;
                                        nonLinearUbergraph = true;
                                        break;
                                    }
                                    found = true;
                                }
                        }
                        else if (node->Name == "Branch")
                        {
                            if (IsPinLinked(node->Outputs[1].ID))
                            {
                                nonLinearUbergraph = true;
                            }
                        }
                    }
                }


                if (Sequences)
                    ubergraphBody.WriteLine("Stack<System.Int32> __StateStack = new Stack<System.Int32>();");

                if (nonLinearUbergraph)
                {
                    ubergraphBody.WriteLine("System.Int32 __CurrentState = bpp__EntryPoint__pf;");
                    ubergraphBody.WriteLine("do");
                    ubergraphBody.OpenScope();
                    ubergraphBody.WriteLine("switch( __CurrentState )");
                    ubergraphBody.OpenScope();
                }
                else
                    ubergraphBody.WriteLine("Dymatic.Core.Assert(bpp__EntryPoint__pf == " + std::to_string(ubergraph.Nodes[0].Get()) + ");");

                for (int i = 0; i < ubergraph.Nodes.size(); i++)
                {
                    auto& nodeId = ubergraph.Nodes[i];

                    auto& node = *FindNode(nodeId);
                    ubergraphBody.WriteLine("// Node_" + ubergraphBody.GenerateFunctionName(node));
                    if (node.CommentEnabled)
                        ubergraphBody.WriteLine("// Comment: " + node.Comment);

                    // Branch Specifies
                    if (node.Internal)
                    {
                        if (node.Name == "Branch")
                        {
                            if (nonLinearUbergraph)
                            {
                                ubergraphBody.WriteLine("case " + std::to_string(node.ID.Get()) + ":");
                                ubergraphBody.OpenScope();
                            }

                            // if false
                            // check for line termination
                            // set state
                            // break

                            std::string line;
                            std::vector<ed::NodeId> pureList;
                            std::vector<ed::NodeId> localList;
                            RecursivePinWrite(node.Inputs[1], source, ubergraphHeader, ubergraphBody, line, node_errors, pureList, localList);

                            ubergraphBody.WriteLine("if (!" + line + ")");
                            ubergraphBody.OpenScope();

                            if (IsPinLinked(node.Outputs[1].ID))
                            {
                                ubergraphBody.WriteLine("__CurrentState = " + std::to_string(GetNextExecNode(node.Outputs[1]).ID.Get()) + ";");
                            }
                            else
                                ubergraphBody.WriteLine(Sequences ? "__CurrentState = (__StateStack.Count > 0) ? __StateStack.Pop() : -1;" : (nonLinearUbergraph ? "__CurrentState = -1;" : "return; // Termination end of function"));
                            if (nonLinearUbergraph)
                                ubergraphBody.WriteLine("break;");

                            ubergraphBody.CloseScope();

                            // if true
                            // check for line termination
                            // set state and break if next state is not next index;

                            if (IsPinLinked(node.Outputs[0].ID))
                            {
                                if (nonLinearUbergraph)
                                {
                                    auto& nextNode = GetNextExecNode(node.Outputs[0]);

                                    bool write = true;
                                    if (i + 1 < ubergraph.Nodes.size())
                                        if (nextNode.ID == ubergraph.Nodes[i + 1])
                                            write = false;
                                    if (write)
                                    {
                                        ubergraphBody.WriteLine("__CurrentState = " + std::to_string(nextNode.ID.Get()) + ";");
                                        ubergraphBody.WriteLine("break;");
                                    }
                                    else
										ubergraphBody.WriteLine("goto case " + std::to_string(nextNode.ID.Get()) + ";");
                                }
                            }
                            else
                            {
                                ubergraphBody.WriteLine(Sequences ? "__CurrentState = (__StateStack.Count > 0) ? __StateStack.Pop() : -1;" : (nonLinearUbergraph ? "__CurrentState = -1;" : "return; // Termination end of function"));
                                if (nonLinearUbergraph)
                                    ubergraphBody.WriteLine("break;");
                            }

                            if (nonLinearUbergraph)
                                ubergraphBody.CloseScope();
                        }
                        // Sequence Specifics
                        else if (node.Name == "Sequence" && Sequences)
                        {
                            // Find total number of connected pins
                            int totalConnectedPins = 0;
                            for (auto& output : node.Outputs)
                                if (IsPinLinked(output.ID))
                                    totalConnectedPins++;

                            // Check to see if any pins generated and write termination case if so.
                            if (totalConnectedPins == 0)
                            {
                                ubergraphBody.WriteLine("case " + std::to_string(node.ID.Get()) + ":");
                                ubergraphBody.OpenScope();
                                ubergraphBody.WriteLine("__CurrentState = (__StateStack.Count > 0) ? __StateStack.Pop() : -1;");
                                ubergraphBody.WriteLine("break;");
                                ubergraphBody.CloseScope();
                            }
                            else
                            {
                                int currentConnectPins = 0;
                                int blockId = node.ID.Get();
                                for (auto& output : node.Outputs)
                                {
                                    // Only look at connected pins
                                    if (IsPinLinked(output.ID))
                                    {
                                        currentConnectPins++;

                                        ubergraphBody.WriteLine("case " + std::to_string(blockId) + ":");
                                        ubergraphBody.OpenScope();

                                        auto& nextNode = GetNextExecNode(output);

                                        // Check if reached last executable index
                                        if (currentConnectPins == totalConnectedPins)
                                        {
                                            // If next node is next index, leave case blank
                                            // Else set state and break

                                            bool write = true;
                                            if (i + 1 < ubergraph.Nodes.size())
                                                if (nextNode.ID == ubergraph.Nodes[i + 1])
                                                    write = false;
                                            if (write)
                                            {
                                                ubergraphBody.WriteLine("__CurrentState = " + std::to_string(nextNode.ID.Get()) + ";");
                                                ubergraphBody.WriteLine("break;");
                                            }
                                            else
                                                ubergraphBody.WriteLine("goto case " + std::to_string(nextNode.ID.Get()) + ";");
                                        }
                                        // push next sequence node, set state and break if not last index
                                        else
                                        {
                                            blockId = compileTimeNextId++;
                                            ubergraphBody.WriteLine("__StateStack.Push(" + std::to_string(blockId) + ");");
                                            ubergraphBody.WriteLine("__CurrentState = " + std::to_string(nextNode.ID.Get()) + ";");
                                            ubergraphBody.WriteLine("break;");
                                        }

                                        ubergraphBody.CloseScope();
                                    }
                                }
                            }
                        }
                    }
                    else if (node.Function == NodeFunction::Variable)
                    {
                        if (nonLinearUbergraph)
                        {
                            ubergraphBody.WriteLine("case " + std::to_string(node.ID.Get()) + ":");
                            ubergraphBody.OpenScope();
                        }

                        if (m_Variables.find(node.ReferenceId) != m_Variables.end())
                        {
                            auto& variable = m_Variables[node.ReferenceId];

                            std::string line;
                            std::vector<ed::NodeId> pureList;
                            std::vector<ed::NodeId> localList;
                            RecursivePinWrite(node.Inputs[1], source, ubergraphHeader, ubergraphBody, line, node_errors, pureList, localList);

                            ubergraphBody.WriteLine(ubergraphBody.UnderscoreSpaces("bpv__" + variable.Name + "_" + std::to_string(variable.ID) + "__pf") + " = " + line + ";");
                        }
                        else
                            m_CompilerResults.push_back({ CompilerResultType::Error, "Couldn't find associated variable for node '" + node.Name + "'", node.ID });

                        if (nonLinearUbergraph)
                        {
							auto& nextNode = GetNextExecNode(node.Outputs[0]);

							bool write = true;
							if (i + 1 < ubergraph.Nodes.size())
								if (nextNode.ID == ubergraph.Nodes[i + 1])
									write = false;
							if (write)
							{
								ubergraphBody.WriteLine("__CurrentState = " + std::to_string(nextNode.ID.Get()) + ";");
								ubergraphBody.WriteLine("break;");
							}
							else
								ubergraphBody.WriteLine("goto case " + std::to_string(nextNode.ID.Get()) + ";");

                            ubergraphBody.CloseScope();
                        }
                    }
                    // Event Specifics
                    else if (node.Function == NodeFunction::EventOverride || node.Function == NodeFunction::EventImplementation)
                    {
                        if (nonLinearUbergraph)
                        {
                            if (IsPinLinked(node.Outputs[0].ID))
                            {
                                ubergraphBody.WriteLine("case " + std::to_string(node.ID.Get()) + ":");

                                auto& execOutput = node.Outputs[0];
                                if (IsPinLinked(execOutput.ID))
                                {
                                    auto& nextNode = GetNextExecNode(execOutput);

                                    bool write = true;
                                    if (i + 1 < ubergraph.Nodes.size())
                                        if (nextNode.ID == ubergraph.Nodes[i + 1])
                                            write = false;
                                    if (write)
                                    {
                                        ubergraphBody.OpenScope();
                                        
                                        ubergraphBody.WriteLine("__CurrentState = " + std::to_string(nextNode.ID.Get()) + ";");
                                        ubergraphBody.WriteLine("break;");

										if (nonLinearUbergraph)
											ubergraphBody.CloseScope();
                                    }

                                    // If write is false we don't open/close scopes, effectively leaving us with an empty case statement (executing the next block)
                                }
                                else
                                {
                                    ubergraphBody.OpenScope();

                                    ubergraphBody.WriteLine("__CurrentState = -1;");
                                    ubergraphBody.WriteLine("break;");

                                    if (nonLinearUbergraph)
                                        ubergraphBody.CloseScope();
                                }
                            }
                        }
                    }
                    // Call Custom Event Specifics
                    else if (node.Function == NodeFunction::EventCall)
                    {
                        if (nonLinearUbergraph)
                        {
                            ubergraphBody.WriteLine("case " + std::to_string(node.ID.Get()) + ":");
                            ubergraphBody.OpenScope();
                        }

                        bool found = false;
                        for (auto& graph : m_Graphs)
                            for (auto& event : graph.Nodes)
                                if (event.Function == NodeFunction::EventImplementation)
                                    if (event.ID.Get() == node.ReferenceId)
                                    {
                                        found = true;

										std::string line;
										std::vector<ed::NodeId> pureList;
										std::vector<ed::NodeId> localList;
										//RecursivePinWrite(node.Inputs[1], source, header, ubergraphBody, line, node_errors, pureList, localList);

                                        ubergraphBody.WriteLine(ubergraphBody.GenerateFunctionName(event) + "(" + line + ");");
                                        break;
                                    }

                        if (!found)
                        {
                            m_CompilerResults.push_back({ CompilerResultType::Error, "Couldn't find custom event referred to for node '" + node.Name + "'", node.ID });
                            node_errors.emplace_back(node.ID);
                        }

                        if (nonLinearUbergraph)
                            ubergraphBody.CloseScope();
                    }
                    // Normal Node Pass
                    else
                    {
                        if (nonLinearUbergraph)
                        {
                            ubergraphBody.WriteLine("case " + std::to_string(node.ID.Get()) + ":");
                            ubergraphBody.OpenScope();
                        }

                        // Node Execution Code
                        std::vector<ed::NodeId> pureList;
                        std::vector<ed::NodeId> localList;
                        RecursiveNodeWrite(node, source, ubergraphHeader, ubergraphBody, node_errors, pureList, localList);

                        if (IsPinLinked(node.Outputs[0].ID))
                        {
                            if (nonLinearUbergraph)
                            {
                                auto& nextNode = GetNextExecNode(node.Outputs[0]);

                                bool write = true;
                                if (i + 1 < ubergraph.Nodes.size())
                                    if (nextNode.ID == ubergraph.Nodes[i + 1])
                                        write = false;
                                if (write)
                                {
                                    ubergraphBody.WriteLine("__CurrentState = " + std::to_string(nextNode.ID.Get()) + ";");
                                    ubergraphBody.WriteLine("break;");
                                }
                                else
                                    ubergraphBody.WriteLine("goto case " + std::to_string(nextNode.ID.Get()) + ";");
                            }
                        }
                        else
                        {
                            // Terminate if end of line
                            ubergraphBody.WriteLine(Sequences ? "__CurrentState = (__StateStack.Count > 0) ? __StateStack.Pop() : -1;" : (nonLinearUbergraph ? "__CurrentState = -1;" : "return; // Termination end of function"));
                            if (nonLinearUbergraph)
                                ubergraphBody.WriteLine("break;");
                        }
                        if (nonLinearUbergraph)
                            ubergraphBody.CloseScope();
                    }
                }

                // Default Case for Safety
                if (nonLinearUbergraph)
                {
                    ubergraphBody.WriteLine("default:");
                    ubergraphBody.Indent();
                    ubergraphBody.WriteLine("Dymatic.Core.Assert(false); // Invalid state");
                    ubergraphBody.WriteLine("break;");
                    ubergraphBody.Unindent();

                    ubergraphBody.CloseScope();
                    ubergraphBody.CloseScope();
                    ubergraphBody.WriteLine("while( __CurrentState != -1 );");
                }
            }

            ubergraphHeader.WriteLine(ubergraphBody.Contents());
            ubergraphHeader.CloseScope();

            source.WriteLine(ubergraphHeader.Contents());
        }

		for (auto& node : graph.Nodes)
		{
			if (node.Function == NodeFunction::EventOverride || node.Function == NodeFunction::EventImplementation)
			{
				for (auto& output : node.Outputs)
					if (output.Data.Type != "Flow")
						source.WriteLine(output.Data.Type + " b0l__NodeEvent_" + source.GenerateFunctionName(node) + "_" + source.UnderscoreSpaces(output.Name) + "__pf = default;");
				source.WriteLine((node.Function == NodeFunction::EventOverride ? "public override " : "public virtual ") + source.GenerateFunctionSignature(node));
				source.OpenScope();

                for (auto& output : node.Outputs)
                    if (output.Data.Type != "Flow")
                        source.WriteLine("b0l__NodeEvent_" + source.GenerateFunctionName(node) + "_" + source.UnderscoreSpaces(output.Name) + "__pf = " + source.UnderscoreSpaces(output.Name) + ";");

				for (auto& ubergraph : ubergraphs)
					for (auto& nodeId : ubergraph.Nodes)
						if (nodeId == node.ID)
						{
							source.WriteLine("bpf__ExecuteUbergraph_" + source.UnderscoreSpaces(NodeScriptName) + "__pf_" + std::to_string(ubergraph.ID) + "(" + std::to_string(node.ID.Get()) + ");");
							break;
						}
				source.CloseScope();
				source.NewLine();
			}
		}
    }

    // Close classes and namespaces
    source.CloseScope();
    source.CloseScope();

    // Restore changes to editor that took place during compilation for simplicity
    m_Graphs = graphs_backup;
    BuildNodes();

    // Apply all node errors
    for (auto& nodeId : node_errors)
    {
        auto p_node = FindNode(nodeId);
        if (p_node != nullptr)
            p_node->Error = true;
    }

	std::filesystem::path directory = "C:/dev/DymaticNodeAssembly";
    std::filesystem::path filepath = directory / NodeScriptName;

    // Create output directory if it doesn't exist
    if (!std::filesystem::exists(directory))
        std::filesystem::create_directory(directory);

    // Check error count and output message.
	int warningCount = 0;
	int errorCount = 0;
	for (auto& result : m_CompilerResults)
	{
		if (result.Type == CompilerResultType::Error)
			errorCount++;
		else if (result.Type == CompilerResultType::Warning)
			warningCount++;
	}

	if (errorCount == 0)
	{
		source.OutputToFile(filepath.string() + ".cs");
		m_CompilerResults.push_back({ CompilerResultType::Info, "Compile of " + NodeScriptName + " completed [C#] - Dymatic Nodes Version " DY_VERSION_STRING " (" + filepath.string() + ")" });
	}
	else
		m_CompilerResults.push_back({ CompilerResultType::Info, "Compile of " + NodeScriptName + " failed [C#] - " + std::to_string(errorCount) + " Error(s) " + std::to_string(warningCount) + " Warnings(s) - Dymatic Nodes Version " DY_VERSION_STRING " (" + filepath.string() + ")" });
}

void ScriptEditorInternal::RecursivePinWrite(Pin& pin, NodeCompiler& source, NodeCompiler& ubergraphHeader, NodeCompiler& ubergraphBody, std::string& line, std::vector<ed::NodeId>& node_errors, std::vector<ed::NodeId>& pureNodeList, std::vector<ed::NodeId>& localNodeList)
{
    if (IsPinLinked(pin.ID))
    {
        Link* link = GetPinLinks(pin.ID)[0];
        Pin* otherPin = FindPin(link->StartPinID == pin.ID ? link->EndPinID : link->StartPinID);
        Node* otherNode = otherPin->Node;

        if (pin.Data.ByRef)
            line += "ref ";

        if (otherNode->Function == NodeFunction::Variable)
        {
            auto& variable = m_Variables[otherNode->ReferenceId];
            line += ubergraphBody.UnderscoreSpaces(source.UnderscoreSpaces("bpv__" + variable.Name + "_" + std::to_string(variable.ID) + "__pf"));
        }
        else if (otherNode->Function == NodeFunction::EventOverride || otherNode->Function == NodeFunction::EventImplementation)
        {
            line += "b0l__NodeEvent_" + source.GenerateFunctionName(*otherNode) + "_" + source.UnderscoreSpaces(otherPin->Name) + "__pf";
        }
        else
        {
            if (otherNode->Pure)
            {
                std::vector<ed::NodeId> newList = pureNodeList;
                newList.push_back(pin.Node->ID);
                RecursiveNodeWrite(*otherNode, source, ubergraphHeader, ubergraphBody, node_errors, newList, localNodeList);
            }

            // Lookup function of node
            bool found = false;
            for (auto& [name, method] : ScriptEngine::GetStaticMethodDeclarations())
				if (method.FullName == otherNode->InternalName)
				{
					found = true;

					bool found = false;
					if (otherPin->InternalName == "return")
					{
						found = true;
						line += ubergraphBody.UnderscoreSpaces("b0l__CallFunc_" + method.FullName + "_" + std::to_string(otherNode->ID.Get()) + "_" + "return" + "__pf");
						break;
					}
					else
					{
						for (auto& param :method.Parameters)
							if (param.Name == otherPin->InternalName)
							{
								found = true;
								line += ubergraphBody.UnderscoreSpaces("b0l__CallFunc_" + method.FullName + "_" + std::to_string(otherNode->ID.Get()) + "_" + param.Name + "__pf");
								break;
							}
					}

					if (!found)
					{
						m_CompilerResults.push_back({ CompilerResultType::Error, "Couldn't find associated argument for pin '" + otherPin->Name + "' of node '" + otherNode->Name + "'", otherNode->ID });
						node_errors.emplace_back(otherNode->ID);
					}
				}
            if (!found)
            {
                m_CompilerResults.push_back({ CompilerResultType::Error, "Couldn't find associated function for '" + otherNode->Name + "'", otherNode->ID });
                node_errors.emplace_back(otherNode->ID);
            }
        }
    }
    else
    {
        line += (pin.Data.Value.empty() ? "default" : pin.Data.Value);
    }
}

void ScriptEditorInternal::RecursiveNodeWrite(Node& node, NodeCompiler& source, NodeCompiler& ubergraphHeader, NodeCompiler& ubergraphBody, std::vector<ed::NodeId>& node_errors, std::vector<ed::NodeId>& pureNodeList, std::vector<ed::NodeId>& localNodeList)
{
    std::string line;

    // Look for dependency cycles and call error if detected.
	bool found = false;
	for (auto& nodeId : pureNodeList)
		if (nodeId == node.ID)
			found = true;
	if (found)
		for (auto& nodeId : pureNodeList)
		{
			auto node = FindNode(nodeId);
			m_CompilerResults.push_back({ CompilerResultType::Error, "Dependency cycle detected, preventing node '" + node->Name + "' from being scheduled.", nodeId });
            node_errors.emplace_back(nodeId);
		}

    // Check that pure nodes are not being called more than once per non pure function
	bool localWritten = false;
	for (auto& local : localNodeList)
		if (local == node.ID)
		{
			localWritten = true;
			break;
		}
	localNodeList.push_back(node.ID);

    // Otherwise write node
	if (!localWritten)
    {
        bool found = false;
        for (auto& [name, method] : ScriptEngine::GetStaticMethodDeclarations())
        {
            if (method.FullName == node.InternalName)
            {
                if (method.ReturnType != "System.Void")
                {
                    bool found = false;
                    for (auto& output : node.Outputs)
                        if (output.InternalName == "return")
                        {
                            auto name = source.UnderscoreSpaces("b0l__CallFunc_" + method.FullName + "_" + std::to_string(node.ID.Get()) + "_return__pf");
                            if (!node.Written)
                            {
                                node.Written = true;
                                ((IsPinGlobal(output, node.UbergraphID) && !method.IsPure) ? source : ubergraphHeader).WriteLine(method.ReturnType + " " + name + " = default;");
                            }
                            line += name + " = ";

                            found = true;
                            break;
                        }
                    if (!found) m_CompilerResults.push_back({ CompilerResultType::Error, "Couldn't find associated pin for argument for return value of of function '" + method.FullName + "'", node.ID });
                }

                line += method.FullName + "(";

                bool first = true;
                for (auto& param : method.Parameters)
                {
                    Pin* paramPin = nullptr;
                    for (auto& pin : node.Inputs)
                        if (pin.InternalName == param.Name)
                        {
                            paramPin = &pin;
                            break;
                        }
                    for (auto& pin : node.Outputs)
                        if (pin.InternalName == param.Name)
                        {
                            paramPin = &pin;
                            break;
                        }
                    if (paramPin != nullptr)
                    {
                        auto& pin = *paramPin;

                        if (first) first = false;
                        else line += ", ";

                        if (pin.Kind == PinKind::Input)
                            RecursivePinWrite(pin, source, ubergraphHeader, ubergraphBody, line, node_errors, pureNodeList, localNodeList);
                        else
                        {
                            auto name = "b0l__CallFunc_" + method.FullName + "_" + std::to_string(node.ID.Get()) + "_" + param.Name + "__pf";
                            if (!pin.Written)
                            {
                                pin.Written = true;
                                ((IsPinGlobal(pin, node.UbergraphID) && !method.IsPure) ? source : ubergraphHeader).WriteLine(param.Type + " " + source.UnderscoreSpaces(name) + " = default;");
                            }
                            line += "out " + ubergraphBody.UnderscoreSpaces(name);
                        }
                    }
                    else
                    {
                        m_CompilerResults.push_back({ CompilerResultType::Error, "Couldn't find associated pin for argument '" + param.Name + "' of function '" + method.FullName + "'", node.ID });
                        node_errors.emplace_back(node.ID);
                    }
                }
                line += ");";

                ubergraphBody.WriteLine(line);
                found = true;
                break;
            }
        }
        if (!found)
        {
            m_CompilerResults.push_back({ CompilerResultType::Error, "Couldn't find associated function for '" + node.Name + "'", node.ID });
            node_errors.emplace_back(node.ID);
        }
    }
}

bool ScriptEditorInternal::IsPinGlobal(Pin& pin, int ubergraphID)
{
	auto& links = GetPinLinks(pin.ID);
	for (auto& link : links)
	{
		auto& node = *FindPin(link->StartPinID == pin.ID ? link->EndPinID : link->StartPinID)->Node;
		if (node.Pure)
			for (auto& output : node.Outputs)
			{
				if (IsPinGlobal(output, ubergraphID)) return true;
			}
		else
		{
			if (node.UbergraphID != -1 && node.UbergraphID != ubergraphID)
				return true;
		}
	}
    return false;
}

void ScriptEditorInternal::RecursiveUbergraphOrganisation(Node& node, std::vector<Ubergraph>& ubergraphs, int& currentId)
{
    // Search all ubergraphs for existing node definition and merge graphs if found.
    bool found = false;
    bool foundAll = false;
    for (auto& ubergraph : ubergraphs)
    {
        for (auto& nodeId : ubergraph.Nodes)
        {
            if (nodeId == node.ID)
            {
                for (int i = 0; i < ubergraphs.size(); i++)
                {
                    auto& currentUbergraph = ubergraphs[i];
                    if (currentUbergraph.ID == currentId)
                    {
                        if (ubergraph.ID != currentId)
                        {
                            ubergraph.Nodes.insert(ubergraph.Nodes.end(), currentUbergraph.Nodes.begin(), currentUbergraph.Nodes.end());
                            ubergraphs.erase(ubergraphs.begin() + i);
                            currentId = ubergraph.ID;
                            found = true;

                        }
                        ubergraph.Linear = false;
                        foundAll = true;

                    }
                }
            }
        }
    }

    // If not found add to current ubergraph
    if (!found)
        for (auto& ubergraph : ubergraphs)
            if (ubergraph.ID == currentId)
            {
                bool found = false;
                for (auto& nodeId : ubergraph.Nodes)
                    if (nodeId == node.ID)
                        found = true;
                if (!found)
                    ubergraph.Nodes.push_back(node.ID);
            }

    if (!foundAll)
    {
        // Recusive Node Search via all attached links
        for (auto& output : node.Outputs)
            if (output.Data.Type == "Flow")
                if (IsPinLinked(output.ID))
                {
                    auto link = GetPinLinks(output.ID)[0];
                    auto& id = output.ID.Get() == link->StartPinID.Get() ? link->EndPinID : link->StartPinID;
                    auto& nextNode = *FindPin(id)->Node;

                    RecursiveUbergraphOrganisation(nextNode, ubergraphs, currentId);
                }
    }
}

Node& ScriptEditorInternal::GetNextExecNode(Pin& pin)
{
	auto link = GetPinLinks(pin.ID)[0];
	auto& id = pin.ID.Get() == link->StartPinID.Get() ? link->EndPinID : link->StartPinID;
	return *FindPin(id)->Node;
}

std::string NodeCompiler::GenerateFunctionSignature(Node& node)
{
    std::string output = "void " + GenerateFunctionName(node) + "(";

    bool first = true;
    for (auto& pin : node.Inputs)
    {
        if (pin.Data.Type != "Flow")
        {
            if (first) first = false;
            else output += ", ";
            output += "const " + pin.Data.Type + " " + UnderscoreSpaces(pin.Name);
        }
    }

    for (auto& pin : node.Outputs)
    {
        if (pin.Data.Type != "Flow")
        {
            if (first) first = false;
            else output += ", ";
            output += pin.Data.Type + " " + UnderscoreSpaces(pin.Name);
        }
    }

    output += ")";
    return output;
}

std::string NodeCompiler::GenerateFunctionName(Node& node)
{
    return "bpf__" + UnderscoreSpaces(node.Name + "_" + std::to_string(node.ID.Get())) + "__pf";
}

void NodeCompiler::NewLine()
{
	m_Buffer += '\n';
    m_IndentedCurrentLine = false;
}

void NodeCompiler::Write(const std::string& text)
{
    if (!m_IndentedCurrentLine)
    {
        m_IndentedCurrentLine = true;
        for (int i = 0; i < m_IndentLevel; i++)
            m_Buffer += '\t';
    }
	m_Buffer += text;
}

void NodeCompiler::WriteLine(const std::string& line)
{
    Write(line);
    NewLine();
}

void NodeCompiler::OutputToFile(const std::filesystem::path& path)
{
	std::ofstream fout(path);
    fout << m_Buffer;
}

std::string ScriptEditorInternal::ConvertNodeFunctionToString(const NodeFunction& nodeFunction)
{
	switch (nodeFunction)
	{
    case NodeFunction::Node:        return "Node";
    case NodeFunction::EventOverride :      return "Event";
    case NodeFunction::Function :   return "Function";
    case NodeFunction::Variable :   return "Variable";
	}
	DY_ASSERT(false, "Node function conversion to string not found");
}

NodeFunction ScriptEditorInternal::ConvertStringToNodeFunction(const std::string& string)
{
	if (string == "Node")           return NodeFunction::Node;
	else if (string == "Event")     return NodeFunction::EventOverride;
	else if (string == "Function")  return NodeFunction::Function;
	else if (string == "Variable")  return NodeFunction::Variable;
	else
		DY_ASSERT(false, "String conversion to node function not found");
}

std::string ScriptEditorInternal::ConvertNodeTypeToString(const NodeType& nodeType)
{
	switch (nodeType)
	{
	case NodeType::Blueprint:       return "Blueprint";
	case NodeType::Simple:          return "Simple";
    case NodeType::Tree :           return "Tree";
    case NodeType::Comment :        return "Comment";
    case NodeType::Houdini :        return "Houdini";
	}
	DY_ASSERT(false, "Node type conversion to string not found");
}

NodeType ScriptEditorInternal::ConvertStringToNodeType(const std::string& string)
{
	     if (string == "Blueprint") return NodeType::Blueprint;
	else if (string == "Simple")    return NodeType::Simple;
	else if (string == "Tree")      return NodeType::Tree;
	else if (string == "Comment")   return NodeType::Comment;
	else if (string == "Houdini")   return NodeType::Houdini;
	else
		DY_ASSERT(false, "String conversion to node type not found");
}

std::string ScriptEditorInternal::ConvertPinKindToString(const PinKind& pinKind)
{
    switch (pinKind)
    {
    case PinKind::Input: return "Input";
    case PinKind::Output: return "Output";
    }
	DY_ASSERT(false, "Pin kind conversion to string not found");
}

PinKind ScriptEditorInternal::ConvertStringToPinKind(const std::string& string)
{
	     if (string == "Input")     return PinKind::Input;
	else if (string == "Output")    return PinKind::Output;
	else
		DY_ASSERT(false, "String conversion to pin kind not found");
}

void ScriptEditorInternal::CopyNodes()
{

}

void ScriptEditorInternal::PasteNodes()
{

}

void ScriptEditorInternal::DuplicateNodes()
{
    for (auto& graph : m_Graphs)
    {
        std::vector<ed::NodeId> selectedNodes;
        std::vector<ed::LinkId> selectedLinks;
        selectedNodes.resize(ed::GetSelectedObjectCount());
        selectedLinks.resize(ed::GetSelectedObjectCount());

        int nodeCount = ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
        int linkCount = ed::GetSelectedLinks(selectedLinks.data(), static_cast<int>(selectedLinks.size()));

        selectedNodes.resize(nodeCount);
        selectedLinks.resize(linkCount);

        ClearSelection();

        std::vector<ed::PinId> originalPins;
        std::vector<ed::PinId> newPins;

        for (auto& selectedNode : selectedNodes)
        {
            for (auto& node : graph.Nodes)
            {
                if (selectedNode == node.ID)
                {
                    graph.Nodes.emplace_back(node);

                    auto& newNode = graph.Nodes.back();
                    newNode.ID = GetNextId();
                    for (int i = 0; i < newNode.Inputs.size(); i++)
                    {
                        auto& input = newNode.Inputs[i];
                        input.ID = GetNextId();

                        originalPins.emplace_back(node.Inputs[i].ID);
                        newPins.emplace_back(input.ID);
                    }
                    for (int i = 0; i < newNode.Outputs.size(); i++)
                    {
                        auto& output = newNode.Outputs[i];
                        output.ID = GetNextId();

                        originalPins.emplace_back(node.Outputs[i].ID);
                        newPins.emplace_back(output.ID);
                    }

                    SetNodePosition(newNode.ID, ed::GetNodePosition(node.ID) + ImVec2(50, 50));

                    BuildNode(&node);
                    BuildNode(&newNode);

                    ed::SelectNode(newNode.ID, true);
                }
            }
        }

        for (auto& link : graph.Links)
        {
            int selectSuccess = 0;
            for (int x = 0; x < selectedNodes.size(); x++)
            {
                auto selectedNodeTemp = FindNode(selectedNodes[x]);
                for (int y = 0; y < selectedNodeTemp->Inputs.size(); y++)
                {
                    if (selectedNodeTemp->Inputs[y].ID == link.StartPinID || selectedNodeTemp->Inputs[y].ID == link.EndPinID)
                    {
                        selectSuccess++;
                    }
                }
                for (int y = 0; y < selectedNodeTemp->Outputs.size(); y++)
                {
                    if (selectedNodeTemp->Outputs[y].ID == link.StartPinID || selectedNodeTemp->Outputs[y].ID == link.EndPinID)
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
                    if (originalPins[x] == link.StartPinID)
                    {
                        newStartPin = newPins[x];
                    }
                    if (originalPins[x] == link.EndPinID)
                    {
                        newEndPin = newPins[x];
                    }
                }

                graph.Links.emplace_back(GetNextLinkId(), newStartPin, newEndPin);
                graph.Links.back().Color = link.Color;
            }
        }

    }
    BuildNodes();
}

void ScriptEditorInternal::DeleteNodes()
{

}

void ScriptEditorInternal::ShowFlow()
{
    auto editor = ed::GetCurrentEditor();
    for (auto& window : m_Windows)
    {
        ed::SetCurrentEditor(window.InternalEditor);
        auto p_graph = FindGraphByID(window.GraphID);
        if (p_graph != nullptr)
            for (auto& link : p_graph->Links)
                ed::Flow(link.ID);
    }
    ed::SetCurrentEditor(editor);
}

NodeGraph* ScriptEditorInternal::FindGraphByID(int id)
{
    if (id)
        for (auto& graph : m_Graphs)
            if (graph.ID == id)
                return &graph;
    return nullptr;
}

GraphWindow* ScriptEditorInternal::FindWindowByID(int id)
{
	if (id)
		for (auto& window : m_Windows)
			if (window.ID == id)
				return &window;
	return nullptr;
}

GraphWindow* ScriptEditorInternal::OpenGraph(int id)
{
    auto p_window = GetCurrentWindow();
    if (p_window != nullptr)
        if (p_window->GraphID == id)
            return p_window;

    for (auto& window : m_Windows)
        if (window.GraphID == id)
        {
            window.Focused = true;
            return p_window;
        }
    return OpenGraphInNewTab(id);
}

GraphWindow* ScriptEditorInternal::OpenGraphInNewTab(int id)
{
    auto& window = m_Windows.emplace_back(GetNextId(), id);

    auto editor = ed::GetCurrentEditor();
    ed::SetCurrentEditor(window.InternalEditor);
    auto& graph = *FindGraphByID(window.GraphID);
    for (auto& node : graph.Nodes)
		ed::SetNodePosition(node.ID, node.Position);
    ed::SetCurrentEditor(editor);

    return &window;
}

void ScriptEditorInternal::ClearSelection()
{
    auto editor = ed::GetCurrentEditor();
    for (auto& window : m_Windows)
    {
        ed::SetCurrentEditor(window.InternalEditor);
        ed::ClearSelection();
    }
    ed::SetCurrentEditor(editor);
}

void ScriptEditorInternal::SetNodePosition(ed::NodeId id, ImVec2 position)
{
    auto p_node = FindNode(id);
    if (p_node != nullptr)
    {
        p_node->Position = position;
        auto editor = ed::GetCurrentEditor();
        for (auto& window : m_Windows)
        {
            ed::SetCurrentEditor(window.InternalEditor);
            ed::SetNodePosition(id, position);
        }
        ed::SetCurrentEditor(editor);
    }
}

void ScriptEditorInternal::NavigateToContent()
{
    auto p_window = GetCurrentWindow();
    if (p_window != nullptr)
    {
        auto editor = ed::GetCurrentEditor();
        ed::SetCurrentEditor(p_window->InternalEditor);
        ed::NavigateToContent();
        ed::SetCurrentEditor(editor);
    }
}

void ScriptEditorInternal::NavigateToNode(ed::NodeId id)
{
    for (auto& graph : m_Graphs)
    {
        for (auto& node : graph.Nodes)
        {
            if (node.ID == id)
            {
                auto window = OpenGraph(graph.ID);
                if (window != nullptr)
                {
                    auto editor = ed::GetCurrentEditor();
                    ed::SetCurrentEditor(window->InternalEditor);
                    ed::SelectNode(node.ID);
                    ed::NavigateToSelection();
                    ed::SetCurrentEditor(editor);
                }
            }
        }
    }
}

void ScriptEditorInternal::DefaultValueInput(ParameterDeclaration& data, bool spring)
{
    if (data.Type == "Flow" || data.ByRef)
    {
		if (!data.Value.empty())
			data.Value.clear();
        ImGui::Dummy({});
    }
    else if (data.Type == "System.Boolean")
    {
        bool value = data.Value == "true" ? true : false;
        if (ImGui::Checkbox("##NodeEditorBoolCheckbox", &value))
            data.Value = value ? "true" : "false";

        if (spring) ImGui::Spring(0);
    }
    else if (data.Type == "System.Int32")
    {
        int value;

        try { value = std::stoi(data.Value); }
        catch (const std::invalid_argument&) { data.Value = "0", value = 0; }
        catch (const std::out_of_range&) { data.Value = "0", value = 0; }

        ImGui::SetNextItemWidth(std::clamp(ImGui::CalcTextSize(data.Value.c_str()).x + 20, 50.0f, 300.0f));
        if (ImGui::DragInt("##NodeEditorDragInt", &value, 0.1f, 0, 0))
            data.Value = std::to_string(value);

        if (spring) ImGui::Spring(0);
    }
    else if (data.Type == "System.Single")
    {
        float value;

		try { value = std::stof(data.Value); }
		catch (const std::invalid_argument&) { data.Value = "0.0f", value = 0.0f; }
		catch (const std::out_of_range&) { data.Value = "0.0f", value = 0.0f; }

        ImGui::SetNextItemWidth(std::clamp(ImGui::CalcTextSize((data.Value).c_str()).x - 10, 50.0f, 300.0f));
        if (ImGui::DragFloat("##NodeEditorDragFloat", &value, 0.1f, 0, 0, "%.3f"))
            data.Value = std::to_string(value) + "f";

        if (spring) ImGui::Spring(0);
    }
    else if (data.Type == "System.String")
    {
        std::string valueString = data.Value;

        if (valueString.size() >= 2)
        {
            if (valueString[0] == '\"')
                valueString.erase(valueString.begin());
            if (valueString[valueString.size() - 1] == '\"')
                valueString.erase(valueString.end() - 1);
        }

        char value[512];
        memset(value, 0, sizeof(value));
        std::strncpy(value, valueString.c_str(), sizeof(value));
        const float size = ImGui::CalcTextSize(value).x + 10.0f;
        ImGui::SetNextItemWidth(std::clamp(size, 25.0f, 500.0f));
        if (ImGui::InputText("##NodeEditorStringInputText", value, sizeof(value), size > 500.0f ? 0 : ImGuiInputTextFlags_NoHorizontalScroll))
        {
            data.Value = "\"" + std::string(value) + '\"';
        }

        if (spring) ImGui::Spring(0);
    }
    else
    {
        if (!data.Value.empty())
            data.Value.clear();
        ImGui::Dummy({});
    }
}

bool ScriptEditorInternal::ValueTypeSelection(ParameterDeclaration& data, bool flow)
{
    bool return_value = false;

	auto& cpos1 = ImGui::GetCursorPos();
	if (ImGui::BeginCombo("##VariableType", ("       " + data.Type).c_str()))
	{
        if (flow)
            if (ImGui::MenuItem("    Flow")){ data.Type = "Flow";  return_value = true; }
        if (ImGui::MenuItem("    Boolean")) {data.Type = "System.Boolean";   return_value = true; }
		if (ImGui::MenuItem("    Integer")) {data.Type = "System.Int32";    return_value = true; }
		if (ImGui::MenuItem("    Float"))   {data.Type = "System.Single";  return_value = true; }
		if (ImGui::MenuItem("    String"))  {data.Type = "System.String"; return_value = true; }
		ImGui::EndCombo();
	}
	auto& cpos2 = ImGui::GetCursorPos(); ImGui::SetCursorPos(cpos1); DrawTypeIcon(data.Type); ImGui::SetCursorPos(cpos2);
	ImGui::SameLine();
	DrawTypeIcon(data.Type);
	ImGui::Dummy(ImVec2(m_PinIconSize, m_PinIconSize));
	if (ImGui::IsItemClicked())
		ImGui::OpenPopup("##UpdateVariableContainer");

    if (return_value)
        m_RecentPinType = data.Type;

    return return_value;
}

void ScriptEditorInternal::DrawGraphOption(NodeGraph& graph)
{
	ImGui::PushID(graph.ID);

	ImGui::Image((ImTextureID)(graph.Type == NodeGraphType::Event ? m_GraphIcon : m_MacroIcon)->GetRendererID(), ImVec2(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()), { 0, 1 }, { 1, 0 }, ImGui::GetStyleColorVec4(ImGuiCol_Text));
	ImGui::SameLine();

	if (graph.Editable)
	{
		char buffer[256];
		memset(buffer, 0, sizeof(buffer));
		std::strncpy(buffer, graph.Name.c_str(), sizeof(buffer));

		bool input, clicked = ImGui::SelectableInput("##GraphTree", ImGui::GetContentRegionAvail().x, false, ImGuiSelectableFlags_None, buffer, sizeof(buffer), input);
        
        if ((GImGui->TempInputId != 0 && GImGui->TempInputId == GImGui->ActiveIdPreviousFrame && GImGui->TempInputId != GImGui->ActiveId))
        {
            switch (graph.Type)
            {
            case NodeGraphType::Event: graph.Name = buffer; break;
            case NodeGraphType::Macro: SetMacroName(graph.ID, buffer); break;
            }
        }
	}
	else
		ImGui::Selectable(graph.Name.c_str());
	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::MenuItem("Open Graph")) OpenGraph(graph.ID);
		if (ImGui::MenuItem("Open Graph In New Tab")) OpenGraphInNewTab(graph.ID);

		ImGui::EndPopup();
	}

	ImGui::PopID();
}

Node* ScriptEditorInternal::DisplaySearchData(SearchData& searchData, bool origin)
{
    Node* node = nullptr;

	if (!searchData.IsConfirmed)
	{
		ImGui::SetNextTreeNodeOpen(searchData.IsOpen);
		searchData.IsConfirmed = true;
	}
	bool searchEmpty = !m_SearchBuffer.empty();
    
	if (searchEmpty) 
        ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
	bool open = origin ? true : ImGui::TreeNodeEx(searchData.Name.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth);
	if (searchEmpty) 
        ImGui::PopItemFlag();
	
    if (open)
	{
        for (auto& tree : searchData.LowerTrees)
            if (Node* newNode = DisplaySearchData(tree, false))
                node = newNode;
        for (auto& result : searchData.Results)
            if (ImGui::MenuItem(result.Name.c_str()))
                if (result.Callback)
                    node = result.Callback();
        
		if (!origin)
			ImGui::TreePop();
	}

    return node;
}

void ScriptEditorInternal::DeleteAllPinLinkAttachments(Pin* pin)
{
    for (auto& graph : m_Graphs)
        for (auto& link : graph.Links)
            if (link.StartPinID == pin->ID || link.EndPinID == pin->ID)
                ed::DeleteLink(link.ID);
}

void ScriptEditorInternal::CheckLinkSafety(Pin* startPin, Pin* endPin)
{
	if (startPin != nullptr)
	{
        if (startPin->Kind == PinKind::Input)
            std::swap(startPin, endPin);
    }
    else if (endPin != nullptr)
    {
		if (endPin->Kind == PinKind::Output)
			std::swap(startPin, endPin);
    }

    for (auto& graph : m_Graphs)
		for (auto& link : graph.Links)
		{
			if (endPin != nullptr)
				if ((link.EndPinID == endPin->ID || link.StartPinID == endPin->ID) && endPin->Data.Type != "Flow")
				    ed::DeleteLink(link.ID);
			if (startPin != nullptr)
				if ((link.EndPinID == startPin->ID || link.StartPinID == startPin->ID) && startPin->Data.Type == "Flow")
				    ed::DeleteLink(link.ID);
		}
}

void ScriptEditorInternal::CreateLink(NodeGraph& graph, Pin* a, Pin* b)
{
	if (a->Kind == PinKind::Input)
		std::swap(a, b);

    CheckLinkSafety(a, b);

	graph.Links.emplace_back(GetNextLinkId(), a->ID, b->ID);
	graph.Links.back().Color = GetIconColor(a->Data.Type);
}

void ScriptEditorInternal::UpdateSearchData()
{
    m_SearchData.Clear();
    m_SearchData.Name = "Dymatic Nodes";

    for (auto& [name, method] : ScriptEngine::GetStaticMethodDeclarations())
        AddSearchData(method.Name, method.Category, method.Keywords, method.Parameters, method.ReturnType, method.IsPure, [=]() { return SpawnNodeWithInternalName(method.FullName); });
    

    AddSearchData("Branch", "Utilities|Flow Control", "if", { {"Condition", "System.Boolean"} }, "System.Void", false, [=](){ return SpawnBranchNode(); });
    AddSearchData("Sequence", "Utilities|Flow Control", "series", {}, {}, false, [=]() { return SpawnSequenceNode(); });
    AddSearchData("Add reroute node...", "", "", {}, {}, true, [=]() { return SpawnRerouteNode(); });

    if (m_NewNodeLinkPin == nullptr || !m_ContextSensitive) 
        AddSearchData(ed::GetSelectedObjectCount() > 0 ? "Add Comment to Selection" : "Add Comment...", "", "label", {}, {}, true, [=]() { return SpawnComment(); });

    AddSearchData("Add Event...", "Add Event", {}, {}, {}, false, [=]() { return SpawnEventImplementationNode(); });
    AddSearchData("On Create", "Add Event", "begin", {}, {}, false, [=]() { return SpawnOnCreateNode(); });
    AddSearchData("On Update", "Add Event", "tick frame", {}, "System.Single", false, [=]() { return SpawnOnUpdateNode(); });
    AddSearchData("On Destroy", "Add Event", "end", {}, {}, false, [=]() { return SpawnOnDestroyNode(); });

    for (auto& [klass, methods] : ScriptEngine::GetEntityClassOverridableMethods())
        if (klass == m_ScriptClass)
            for (auto& method : methods)
                AddSearchData(method.Name, "Add Event", {}, method.Parameters, method.ReturnType, method.IsPure, [=]() { return SpawnEventOverrideNode(method.FullName); });

    for (auto& graph : m_Graphs)
        for (auto& node : graph.Nodes)
            if (node.Function == NodeFunction::EventImplementation)
                AddSearchData(node.DisplayName, "Call Event", {}, {}, {}, false, [=]() { return SpawnCallEventNode(node.ID.Get()); });

    for (auto& graph : m_Graphs)
        if (graph.Type == NodeGraphType::Macro)
            AddSearchData(graph.Name, "Macros", {}, {}, {}, false, [=]() { return SpawnMacroNode(graph.ID); });

    for (auto& [id, variable] : m_Variables)
    {
        AddSearchData("Get " + variable.Name, "Variables", {}, {}, variable.Data.Type, true, [=]() { return SpawnGetVariableNode(variable); });
		AddSearchData("Set " + variable.Name, "Variables", {}, { variable.Data }, variable.Data.Type, false, [=]() { return SpawnSetVariableNode(variable); });
    }
}

void ScriptEditorInternal::AddSearchData(const std::string& name, const std::string& category, const std::string& keywords, const std::vector<ParameterDeclaration>& parameters, const std::string& returnType, bool pure, std::function<Node*()> callback)
{
	bool visible = true;
	if (!m_SearchBuffer.empty())
	{
		std::vector<std::string> searchWords;
		String::SplitStringByDelimiter(m_SearchBuffer, searchWords, ' ');

        std::string keywordString = String::ToLower(keywords);
        
        for (auto& search : searchWords)
        {
            if (!search.empty())
            {
                String::TransformLower(search);
                bool found = false;
                if (keywordString.find(search) != std::string::npos)
                    found = true;

                std::string funcName = name;
                String::TransformLower(funcName);
                if (funcName.find(search) != std::string::npos)
                    found = true;
                if (!found)
                    visible = false;
            }
        }
	}

	if (m_ContextSensitive && m_NewNodeLinkPin != nullptr)
	{
        if (m_NewNodeLinkPin->Data.Type == "Flow")
        {
            if (pure)
                visible = false;
        }
        else
        {
            bool found = false;
            for (auto& param : parameters)
            {
                if ((param.Type == m_NewNodeLinkPin->Data.Type || (m_NewNodeLinkPin->Kind == PinKind::Input ? ScriptEngine::IsConversionAvalible(param.Type, m_NewNodeLinkPin->Data.Type) : ScriptEngine::IsConversionAvalible(m_NewNodeLinkPin->Data.Type, param.Type))) && ((param.IsOut ? PinKind::Input : PinKind::Output) == m_NewNodeLinkPin->Kind))
                    found = true;
            }
            if (!found)
                if (returnType == "System.Void")
                    visible = false;
                else if ((returnType != m_NewNodeLinkPin->Data.Type && !ScriptEngine::IsConversionAvalible(returnType, m_NewNodeLinkPin->Data.Type)) || m_NewNodeLinkPin->Kind == PinKind::Output)
                        visible = false;
        }
	}

	if (visible)
	{
		// Split Category Into Segments
		std::vector<std::string> seglist;
		String::SplitStringByDelimiter(category, seglist, '|');

		// Find Corresponding Category
		SearchData* searchData = &m_SearchData;
		for (auto& segment : seglist)
		{
			// Remove Empty Categories
			if (segment.empty())
				continue;

			// Check if segment exists in current search data scope
			bool found = false;
			for (auto& branch : searchData->LowerTrees)
				if (branch.Name == segment)
				{
					searchData = &branch;
					found = true;
				}
			// If function doesn't exist create new data scope and update to current one
			if (!found)
			{
				searchData = &searchData->AddTree(segment);
			}
		}

		// Add Function To Corresponding Category
        searchData->AddResult(name, callback);
	}
}

void ScriptEditorInternal::ImplicitExecuteableLinks(NodeGraph& graph, Pin* startPin, Node* node)
{
    if (startPin->Data.Type != "Flow")
		for (auto& pin : (startPin->Kind == PinKind::Input ? startPin->Node->Inputs : startPin->Node->Outputs))
			if (pin.Data.Type == "Flow" && !IsPinLinked(pin.ID))
			{
				for (auto& otherPin : startPin->Kind == PinKind::Input ? node->Outputs : node->Inputs)
					if (otherPin.Data.Type == "Flow" && !IsPinLinked(otherPin.ID))
					{
                        CreateLink(graph, &pin, &otherPin);
						break;
					}
                break;
			}
}

void ScriptEditorInternal::Init()
{
    m_Graphs.emplace_back(GetNextId(), "Event Graph").Editable = false;
    m_CurrentGraphID = m_Graphs.back().ID;
    m_Graphs.emplace_back(GetNextId(), "Construction Script");

    OpenGraph(m_CurrentGraphID);
    OpenGraph(m_Graphs.back().ID);

	ed::Config config;
    config.SettingsFile = "";

	m_HeaderBackground = Dymatic::Texture2D::Create("Resources/Icons/NodeEditor/BlueprintBackground.png");
	m_SaveIcon = Dymatic::Texture2D::Create("Resources/Icons/NodeEditor/ic_save_white_24dp.png");
	m_RestoreIcon = Dymatic::Texture2D::Create("Resources/Icons/NodeEditor/ic_restore_white_24dp.png");
	m_CommentIcon = Dymatic::Texture2D::Create("Resources/Icons/NodeEditor/ic_comment_white_24dp.png");
	m_PinIcon = Dymatic::Texture2D::Create("Resources/Icons/NodeEditor/ic_pin_white_24dp.png");
	m_PinnedIcon = Dymatic::Texture2D::Create("Resources/Icons/NodeEditor/ic_pinned_white_24dp.png");

    m_RemoveIcon = Dymatic::Texture2D::Create("Resources/Icons/SceneHierarchy/ClearIcon.png");

    m_GraphIcon = Dymatic::Texture2D::Create("Resources/Icons/NodeEditor/GraphIcon.png");
    m_MacroIcon = Dymatic::Texture2D::Create("Resources/Icons/NodeEditor/MacroIcon.png");

    // Node Icons
	m_EventIcon = Dymatic::Texture2D::Create("Resources/Icons/NodeEditor/NodeEvent.png");
	m_CustomEventIcon = Dymatic::Texture2D::Create("Resources/Icons/NodeEditor/NodeCustomEvent.png");
	m_FunctionIcon = Dymatic::Texture2D::Create("Resources/Icons/NodeEditor/NodeFunction.png");
	m_SequenceIcon = Dymatic::Texture2D::Create("Resources/Icons/NodeEditor/NodeSequence.png");
	m_BranchIcon = Dymatic::Texture2D::Create("Resources/Icons/NodeEditor/NodeBranch.png");
	m_SwitchIcon = Dymatic::Texture2D::Create("Resources/Icons/NodeEditor/NodeSwitch.png");

    // Initial Node Loading
	{
		Node* node;
    
        node = SpawnOnCreateNode(); 
        SetNodePosition(node->ID, { 0.0f, -100.0f }); 
        node->CommentEnabled = true; 
        node->CommentPinned = true; 
        node->Comment = "Executed when script starts.";
    
		node = SpawnOnUpdateNode(); 
        SetNodePosition(node->ID, { 0.0f,  100.0f });
        node->CommentEnabled = true;
        node->CommentPinned = true;
        node->Comment = "Executed every frame and provides delta time.";
	}
    
	BuildNodes();

    NavigateToContent();
}

void ScriptEditorInternal::OnImGuiRender()
{
    if (auto& nodeEditorVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::ScriptEditor))
    {
        auto& io = ImGui::GetIO();
    
        ImGuiWindowClass windowClass;
        windowClass.ClassId = ImGui::GetID("##Node DockSpace Class");
        windowClass.DockingAllowUnclassed = false;
    
    	// DockSpace
    	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
        ImGui::Begin(CHARACTER_ICON_NODES " Script Editor", &nodeEditorVisible, ImGuiWindowFlags_MenuBar);
        ImGui::PopStyleVar();
    
    
    	ImGuiStyle& style = ImGui::GetStyle();
    	float minWinSizeX = style.WindowMinSize.x;
    	style.WindowMinSize.x = 150.0f;
    	ImGuiID dockspace_id = ImGui::GetID("Node DockSpace");
    	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    	{
    		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags, &windowClass);
    	}
        style.WindowMinSize.x = minWinSizeX;

    	ImGui::BeginMenuBar();
    	if (ImGui::BeginMenu("File"))
    	{
    		ImGui::EndMenu();
    	}
        if (ImGui::MenuItem("Compile")) CompileNodes();

		if (ImGui::BeginMenu("View"))
		{
            ImGui::MenuItem("Hide Unconnected Pins", "", &m_HideUnconnected);
            ImGui::Separator();
            if (ImGui::MenuItem("Zoom to Content"))
                NavigateToContent();
            if (ImGui::MenuItem("Show Flow"))
                ShowFlow();
			ImGui::EndMenu();
		}
    	ImGui::EndMenuBar();
        ImGui::End();
        
        for (size_t i = 0; i < m_Windows.size(); i++)
        {
            auto& window = m_Windows[i];
            auto p_graph = FindGraphByID(window.GraphID);
            if (p_graph != nullptr)
            {
                auto& graph = *p_graph;

                ImGui::PushID(window.ID);

				// Update Internal Editor for Each node graph
				ed::SetCurrentEditor(window.InternalEditor);

                if (!window.Initialized)
                {
                    window.Initialized = true;

                    auto center_id = ImGui::DockBuilderGetCentralNode(dockspace_id)->ID;
                    ImGui::DockBuilderDockWindow((graph.Name + "###GraphWindow" + std::to_string(window.ID)).c_str(), center_id);
                }

    	        ImGui::SetNextWindowClass(&windowClass);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
                bool open = true;
                if (window.Focused)
                {
                    ImGui::SetNextWindowFocus();
                    window.Focused = false;
                }
    	        bool windowVisible = ImGui::Begin((graph.Name + "###GraphWindow" + std::to_string(window.ID)).c_str(), &open, ImGuiWindowFlags_NoNavFocus);
                ImGui::PopStyleVar();
    	        UpdateTouch();

                if (ImGui::IsWindowFocused())
                {
                    m_CurrentGraphID = graph.ID;
                    m_CurrentWindowID = window.ID;
                }
    
# if 0
    	        {
    	            for (auto x = -io.DisplaySize.y; x < io.DisplaySize.x; x += 10.0f)
    	            {
    	                ImGui::GetWindowDrawList()->AddLine(ImVec2(x, 0), ImVec2(x + io.DisplaySize.y, io.DisplaySize.y),
    	                    IM_COL32(255, 255, 0, 255));
    	            }
    	        }
# endif

				
				//{
				//	const ImGuiID id = ImGui::GetID("##AddVariablePopup");
				//	if (m_OpenVarPopup) { ImGui::OpenPopupEx(id, ImGuiPopupFlags_None); m_OpenVarPopup = false; }
				//
				//	if (ImGui::BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings))
				//	{
				//		ImGui::SetWindowPos(ImVec2(Dymatic::Input::GetMousePosition().x, Dymatic::Input::GetMousePosition().y), ImGuiCond_Appearing);
				//		ImGui::TextDisabled(var->Name.c_str());
				//		ImGui::Separator();
				//        if (ImGui::MenuItem(("Get " + var->Name).c_str(), "Ctrl")) 
				//        { SpawnNodeData spawnData = { var->ID }; spawnData.GraphID = window.GraphID; SetNodePosition(NodeEditorInternal::SpawnGetVariableNode(spawnData)->ID, ed::ScreenToCanvas(ImGui::GetMousePos())); }
				//        else if (ImGui::MenuItem(("Set " + var->Name).c_str(), "Alt")) 
				//        { SpawnNodeData spawnData = { var->ID }; spawnData.GraphID = window.GraphID; SetNodePosition(NodeEditorInternal::SpawnSetVariableNode(spawnData)->ID, ed::ScreenToCanvas(ImGui::GetMousePos())); }
				//		ImGui::EndPopup();
				//	}
				//	else m_OpenVarPopup = false;
				//}

                if (windowVisible)
                {
                    ed::Begin(("Node Editor Panel" + std::to_string(window.ID)).c_str());
                    {
                        auto cursorTopLeft = ImGui::GetCursorScreenPos();
                    
                        util::BlueprintNodeBuilder builder(reinterpret_cast<void*>(m_HeaderBackground->GetRendererID()), m_HeaderBackground->GetWidth(), m_HeaderBackground->GetHeight());

                        // Drag Drop Target
                        const ImGuiID id = ImGui::GetID("##AddVariablePopup");
		            	if (ImGui::BeginDragDropTarget())
		            	{
		            		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("NODE_EDITOR_VARIABLE"))
		            		{
		            			int index = *(int*)payload->Data;
                                m_SelectedVariable = m_Variables[index].ID;

		            			if (io.KeyCtrl)
		            			{
		            				ed::SetNodePosition(SpawnGetVariableNode(m_Variables[index])->ID, ImGui::GetMousePos());
                                    BuildNodes();
		            			}
		            			else if (io.KeyAlt)
		            			{
		            				ed::SetNodePosition(SpawnSetVariableNode(m_Variables[index])->ID, ImGui::GetMousePos());
                                    BuildNodes();
		            			}
                                else
                                {
                                    ed::Suspend();
                                    ImGui::OpenPopupEx(id, ImGuiPopupFlags_None);
                                    m_NewNodePosition = ImGui::GetMousePos();
                                    ed::Resume();
                                }
		            		}
		            		ImGui::EndDragDropTarget();
		            	}

                        ed::Suspend();
                        if (ImGui::BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings))
                        {
				    		auto& variable = m_Variables[m_SelectedVariable];

                            ImGui::SetWindowPos(ImVec2(Dymatic::Input::GetMousePosition().x, Dymatic::Input::GetMousePosition().y), ImGuiCond_Appearing);
                            ImGui::TextDisabled(variable.Name.c_str());
                            ImGui::Separator();
                            if (ImGui::MenuItem(("Get " + variable.Name).c_str(), "Ctrl"))
                            {
                                SetCurrentGraph(window.GraphID);
                                SetNodePosition(ScriptEditorInternal::SpawnGetVariableNode(variable)->ID, ed::ScreenToCanvas(m_NewNodePosition));
                            }
                            else if (ImGui::MenuItem(("Set " + variable.Name).c_str(), "Alt"))
                            {
                                SetCurrentGraph(window.GraphID);
                                SetNodePosition(ScriptEditorInternal::SpawnSetVariableNode(variable)->ID, ed::ScreenToCanvas(m_NewNodePosition));
                            }
                            ImGui::EndPopup();
                        }
                        ed::Resume();

                        for (auto& node : graph.Nodes)
                        {
                            if (node.Type != NodeType::Point)
                                continue;

                            ed::PushStyleColor(ed::StyleColor_NodeBg, {});
                            ed::PushStyleColor(ed::StyleColor_NodeBorder, {});
                            builder.Begin(node.ID);

                            bool showInput = true;
                            if (m_NewLinkPin != nullptr)
                                if (m_NewLinkPin->Kind == PinKind::Input)
                                    showInput = false;
                            {
                                auto& input = node.Inputs[0];
                                builder.Input(input.ID);
                                auto alpha = ImGui::GetStyle().Alpha;
                                if (m_NewLinkPin && !CanCreateLink(m_NewLinkPin, &input) && &input != m_NewLinkPin)
                                    alpha = alpha * (48.0f / 255.0f);
                                if (showInput)
                                {
                                    auto cpos = ImGui::GetCursorPosX();
                                    ImGui::SetCursorPosX(cpos + 10.0f);
                                    DrawPinIcon(input.Data, PinKind::Input, IsPinLinked(input.ID), (int)(alpha * 255));
                                    ImGui::SetCursorPosX(cpos);
                                    ImGui::Spring(0);
                                }
                                ImGui::Dummy({ 1, 1 });
                                builder.EndInput();
                            }
                            {
                                auto& output = node.Outputs[0];
                                builder.Output(output.ID);
								auto alpha = ImGui::GetStyle().Alpha;
								if (m_NewLinkPin && !CanCreateLink(m_NewLinkPin, &output) && &output != m_NewLinkPin)
									alpha = alpha * (48.0f / 255.0f);
                                if (!showInput)
                                {
                                    DrawPinIcon(output.Data, PinKind::Input, IsPinLinked(output.ID), (int)(alpha * 255));
                                    ImGui::Spring(0);
                                }
                                ImGui::Dummy({ 1, 1 });
                                builder.EndOutput();
                            }

                            builder.End();
                            ed::PopStyleColor(2);
                        }
                    
                        for (auto& node : graph.Nodes)
                        {
                            if (node.Type != NodeType::Blueprint && node.Type != NodeType::Simple)
                                continue;
                    
                            Variable* payloadVariable = nullptr;
                            Pin* payloadPin = nullptr;

                            const auto isSimple = node.Type == NodeType::Simple;
                    
                            bool hasOutputDelegates = false;
                            for (auto& output : node.Outputs)
                                if (output.Data.Type == "Delegate")
                                    hasOutputDelegates = true;

                            builder.Begin(node.ID);
                                if (!isSimple)
                                {
                                    builder.Header(node.Color);
                                        ImGui::Spring(0);

                                        if (node.Icon != nullptr)
                                            ImGui::Image((ImTextureID)node.Icon->GetRendererID(), ImVec2(ImGui::GetTextLineHeight() * 1.25f, ImGui::GetTextLineHeight() * 1.25f), { 0, 1 }, { 1, 0 }, node.IconColor);

                                        if (node.Function == NodeFunction::EventImplementation)
                                        {
											char buffer[256];
											memset(buffer, 0, sizeof(buffer));
											std::strncpy(buffer, node.DisplayName.c_str(), sizeof(buffer));
											bool input, clicked = ImGui::SelectableInput("##CustomEventName", node.DisplayName.empty() ? 1.0f : ImGui::CalcTextSize(buffer).x, false, NULL, buffer, sizeof(buffer), input, ImGuiInputTextFlags_NoHorizontalScroll);
                                            if (input)
                                                node.DisplayName = buffer;
                                            
											ImGui::PushID("##CustomEventName");
											if (GImGui->ActiveId != ImGui::GetCurrentWindow()->GetID("##Input") && GImGui->ActiveIdPreviousFrame == ImGui::GetCurrentWindow()->GetID("##Input"))
                                                SetCustomEventName(&node, buffer);
											ImGui::PopID();
                                        }
                                        else
                                            ImGui::TextUnformatted((node.DisplayName.empty() ? node.Name : node.DisplayName).c_str());

                                        ImGui::Spring(1);
                                        ImGui::Dummy(ImVec2(0, 28));
                                        if (hasOutputDelegates)
                                        {
                                            ImGui::BeginVertical("delegates", ImVec2(0, 28));
                                            ImGui::Spring(1, 0);
                                            for (auto& output : node.Outputs)
                                            {
                                                if (output.Data.Type != "Delegate")
                                                    continue;
                    
                                                auto alpha = ImGui::GetStyle().Alpha;
                                                if (m_NewLinkPin && !CanCreateLink(m_NewLinkPin, &output) && &output != m_NewLinkPin)
                                                    alpha = alpha * (48.0f / 255.0f);
                    
                                                ed::BeginPin(output.ID, ed::PinKind::Output);
                                                ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
                                                ed::PinPivotSize(ImVec2(0, 0));
                                                ImGui::BeginHorizontal(output.ID.AsPointer());
                                                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                                                auto& pinName = output.GetName();
                                                if (pinName.empty())
                                                {
                                                    ImGui::TextUnformatted(pinName.c_str());
                                                    ImGui::Spring(0);
                                                }
                                                DrawPinIcon(output.Data, PinKind::Output, IsPinLinked(output.ID), (int)(alpha * 255));
                                                ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
                                                ImGui::EndHorizontal();
                                                ImGui::PopStyleVar();
                                                ed::EndPin();
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
                                    if ((!m_HideUnconnected || IsPinLinked(input.ID)))
                                    {
                                        auto alpha = ImGui::GetStyle().Alpha;
                                        if (m_NewLinkPin && !CanCreateLink(m_NewLinkPin, &input) && &input != m_NewLinkPin)
                                            alpha = alpha * (48.0f / 255.0f);

                                        builder.Input(input.ID);
                                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                                        DrawPinIcon(input.Data, PinKind::Input, IsPinLinked(input.ID), (int)(alpha * 255));
                                        ImGui::Spring(0);
                                        auto& pinName = input.GetName();
                                        if (!pinName.empty())
                                        {
                                            ImGui::TextUnformatted(pinName.c_str());
                                            ImGui::Spring(0);
                                        }

                                        if (!IsPinLinked(input.ID))
                                        {
                                            ImGui::PushID(input.ID.Get());
                                            DefaultValueInput(input.Data, true);
                                            ImGui::PopID();
                                        }
                                        ImGui::PopStyleVar();
                                        builder.EndInput();

                                        if (ImGui::IsItemHovered() && ImGui::IsMouseDown(0))
                                            m_CurrentLinkColor = GetIconColor(input.Data.Type);

                                        // Drag Drop Variable Inputs
                                        if (ImGui::BeginDragDropTarget())
                                        {
                                            auto payload = ImGui::GetDragDropPayload();
                                            if (strcmp(payload->DataType, "NODE_EDITOR_VARIABLE") == 0)
                                                if (m_Variables[*(int*)payload->Data].Data.Type == input.Data.Type || input.Data.Type == "Flow")
                                                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("NODE_EDITOR_VARIABLE"))
                                                    {
                                                        payloadVariable = &m_Variables[*(int*)payload->Data];
                                                        payloadPin = &input;
                                                    }
                                            ImGui::EndDragDropTarget();
                                        }
                                    }
                                }
                    
                                if (isSimple)
                                {
                                    builder.Middle();
                    
                                    ImGui::Spring(1, 0);

                                    if (node.DisplayName == "->")
                                    {
                                        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                                        ImGui::Bullet();
                                        ImGui::PopStyleColor();
                                    }
									else
                                        ImGui::TextUnformatted((node.DisplayName == "" ? node.Name : node.DisplayName).c_str());
                                    ImGui::Spring(1, 0);
                                }
                    
                                for (auto& output : node.Outputs)
		            			{
                                    if ((!m_HideUnconnected || IsPinLinked(output.ID)))
                                    {
                                        if (!isSimple && output.Data.Type == "Delegate")
                                            continue;

                                        auto alpha = ImGui::GetStyle().Alpha;
                                        if (m_NewLinkPin && !CanCreateLink(m_NewLinkPin, &output) && &output != m_NewLinkPin)
                                            alpha = alpha * (48.0f / 255.0f);

                                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                                        builder.Output(output.ID);
                                        auto& pinName = output.GetName();
                                        if (!pinName.empty())
                                        {
                                            ImGui::Spring(0);
                                            ImGui::TextUnformatted(pinName.c_str());
                                        }
                                        ImGui::Spring(0);
                                        DrawPinIcon(output.Data, PinKind::Output, IsPinLinked(output.ID), (int)(alpha * 255));
                                        ImGui::PopStyleVar();
                                        builder.EndOutput();

                                        if (ImGui::IsItemHovered() && ImGui::IsMouseDown(0))
                                            m_CurrentLinkColor = GetIconColor(output.Data.Type);

                                        // Drag Drop Variable Outputs
                                        if (ImGui::BeginDragDropTarget())
                                        {
                                            auto payload = ImGui::GetDragDropPayload();
                                            if (strcmp(payload->DataType, "NODE_EDITOR_VARIABLE") == 0)
                                                if (m_Variables[*(int*)payload->Data].Data.Type == output.Data.Type || output.Data.Type == "Flow")
                                                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("NODE_EDITOR_VARIABLE"))
                                                    {
                                                        payloadVariable = &m_Variables[*(int*)payload->Data];
                                                        payloadPin = &output;
                                                    }
                                            ImGui::EndDragDropTarget();
                                        }
                                    }
                                }
                    
    	            			if (node.AddOutputs || node.AddInputs)
    	            			{
                                    ImGui::Spring(0);
                    
    	            				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
    	            				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
    	            				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
    	            				bool newPin = ImGui::Button("Add Pin +");
    	            				ImGui::PopStyleColor(3);
                                    
                                    if (newPin)
                                    {
                                        if (node.AddOutputs)
                                        {
                                            std::string pinName = "0";
                                            if (!node.Outputs.empty())
                                                pinName = std::to_string((int)std::stof(node.Outputs[node.Outputs.size() - 1].Name) + 1);
                                            node.Outputs.emplace_back(GetNextId(), pinName.c_str(), "Flow");
                                            node.Outputs.back().Deletable = true;
                                        }
                                        else if (node.AddInputs)
                                        {
                                            node.Inputs.emplace_back(GetNextId(), "", node.Inputs.back().Data.Type).Deletable = true;
                                        }

                                        BuildNode(&node);
                                    }
    	            			}    

                                // Error Message
                                if (node.Error)
		            			{
                                    auto& nodePos = ed::GetNodePosition(node.ID);
                                    auto& nodeSize = ed::GetNodeSize(node.ID);
                                    auto fontSize = ImGui::GetFontSize();

                                    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(nodePos.x + 2.5f, nodePos.y + nodeSize.y - fontSize), ImVec2(nodePos.x + 2.5f, nodePos.y + nodeSize.y) + ImVec2(nodeSize.x - 5.0f, 0.0f), ImColor(255, 0, 0), 5.0f);
                                    ImGui::GetWindowDrawList()->AddText(ImVec2(nodePos.x + (nodeSize.x - ImGui::CalcTextSize("ERROR!").x) * 0.5f, nodePos.y + nodeSize.y - fontSize), ImColor(255, 255, 255), "ERROR!");
                                }

                            builder.End();

                            if (ImGui::IsItemClicked())
                            {
                                if (ImGui::IsMouseDoubleClicked(0))
                                {
                                    if (node.Function == NodeFunction::EventCall)
                                        NavigateToNode(node.ReferenceId);

                                    if (node.Function == NodeFunction::Macro)
                                        OpenGraph(node.ReferenceId);
                                }
                                else
                                {
                                    m_SelectedDetailsFunction = node.Function;
                                    m_SelectedDetailsReference = node.ReferenceId;
                                }
                            }

                            // Drawing shadows beneath nodes
		            		auto& pos = ed::GetNodePosition(node.ID) - ImVec2(10.0f, 10.0f);
		            		auto& size = ed::GetNodeSize(node.ID) + ImVec2(20.0f, 20.0f);
		            		auto drawList = ImGui::GetWindowDrawList();

		            		const int vert_start_idx = drawList->VtxBuffer.Size;
		            		drawList->AddRect(pos, pos + size, ImColor(0, 0, 0, 75), ed::GetStyle().NodeRounding + 5.0f, 15, 20.0f);
		            		const int vert_end_idx = drawList->VtxBuffer.Size;
		            		ImDrawVert* vert_start = drawList->VtxBuffer.Data + vert_start_idx;
		            		ImDrawVert* vert_end = drawList->VtxBuffer.Data + vert_end_idx;
		            		for (ImDrawVert* vert = vert_start; vert < vert_end; vert++)
		            			if (vert->pos.x < pos.x || vert->pos.y < pos.y || vert->pos.x > pos.x + size.x || vert->pos.y > pos.y + size.y || (pos.y + size.y - vert->pos.y) > size.y * 0.5f)
		            				vert->col = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

                            // Comments for Blueprint Nodes (Based off code for comments)
                            ImGui::PushID(node.ID.AsPointer());
                            if (node.CommentEnabled)
		            		{
		            			auto bgAlpha = static_cast<int>(ImGui::GetStyle().Alpha * 255);

		            			auto zoom = node.CommentPinned ? ed::GetCurrentZoom() : 1.0f;
		            			auto padding = ImVec2(2.5f, 2.5f);

                                auto min = ed::GetNodePosition(node.ID) - ImVec2(0, padding.y * zoom * 2.0f);

                                auto originalScale = ImGui::GetFont()->Scale;
                                ImGui::GetFont()->Scale = zoom;
                                ImGui::PushFont(ImGui::GetFont());

		            			ImGui::SetCursorScreenPos(min - ImVec2(-8, ImGui::GetTextLineHeightWithSpacing() + 4));
		            			ImGui::BeginGroup();
		            			//ImGui::TextUnformatted(node.Comment.c_str());

		            			char buffer[256];
		            			memset(buffer, 0, sizeof(buffer));
		            			std::strncpy(buffer, node.Comment.c_str(), sizeof(buffer));
                                bool input, clicked = ImGui::SelectableInput("##NodeComment", node.Comment.empty() ? 1.0f : ImGui::CalcTextSize(buffer).x, false, NULL, buffer, sizeof(buffer), input, ImGuiInputTextFlags_NoHorizontalScroll);
                                if (input) node.Comment = std::string(buffer);

                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
		            			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4());
		            			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f * zoom);

                                ImGui::SameLine();
                                if (ImGui::ImageButton((ImTextureID)(node.CommentPinned ? m_PinnedIcon : m_PinIcon)->GetRendererID(), ImVec2(15.0f, 15.0f) * zoom, { 0, 1 }, {1, 0}, 2 * zoom)) node.CommentPinned = !node.CommentPinned;
                                ImGui::SameLine();
                                if (ImGui::ImageButton((ImTextureID)m_CommentIcon->GetRendererID(), ImVec2(15.0f, 15.0f) * zoom, { 0, 1 }, { 1, 0 }, 2 * zoom)) node.CommentEnabled = false;

                                ImGui::PopStyleVar();
                                ImGui::PopStyleColor(3);

		            			ImGui::EndGroup();

                                ImGui::GetFont()->Scale = originalScale;
                                ImGui::PopFont();

                                auto drawList = ImGui::GetWindowDrawList();

		            			auto hintBounds = ImGui_GetItemRect();
		            			auto hintFrameBounds = ImRect_Expanded(hintBounds, 8, 4);

		            			drawList->AddRectFilled(
		            				hintFrameBounds.GetTL() - padding * zoom,
                                    hintFrameBounds.GetBR() + padding * zoom,//hintFrameBounds.GetTL() + (hintFrameBounds.GetSize() * ed::GetCurrentZoom()),
		            				IM_COL32(255, 255, 255, 64 * bgAlpha / 255), 4.0f * zoom);

		            			drawList->AddRect(
		            				hintFrameBounds.GetTL() - padding * zoom,
		            				hintFrameBounds.GetBR() + padding * zoom,//hintFrameBounds.GetTL() + (hintFrameBounds.GetSize() * ed::GetCurrentZoom()),
		            				IM_COL32(255, 255, 255, 128 * bgAlpha / 255), 4.0f * zoom, ImDrawCornerFlags_All, zoom);
		            		}
                            else if (ImGui::IsItemHovered() || (window.CommentOpacity > -1.0f && window.HoveredCommentID == node.ID))
                            {
                                bool nodeHovered = ImGui::IsItemHovered();
                                if (nodeHovered)
                                    window.HoveredCommentID = node.ID;

		            			auto zoom = node.CommentPinned ? ed::GetCurrentZoom() : 1.0f;
		            			auto padding = ImVec2(2.5f, 2.5f);

                                auto min = ed::GetNodePosition(node.ID) - ImVec2(0, padding.y * zoom * 2.0f);


                                ImGui::SetCursorScreenPos(min - ImVec2(-8, ImGui::GetTextLineHeightWithSpacing() + 4));
                                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, window.CommentOpacity);
                                if (ImGui::ImageButton((ImTextureID)m_CommentIcon->GetRendererID(), ImVec2(15.0f, 15.0f) * zoom, { 0, 1 }, { 1, 0 }, 2 * zoom)) { window.CommentOpacity = -2.5f; node.CommentEnabled = true; }
                                ImGui::PopStyleVar();

		            			if (ImGui::IsItemHovered() || nodeHovered)
		            				window.CommentOpacity = std::min(window.CommentOpacity + 0.1f, 1.0f);
		            			else
		            				window.CommentOpacity = std::max(window.CommentOpacity - 0.1f, -2.5f);
                            }

                            ImGui::PopID();

                            if (payloadVariable != nullptr)
                            {
                                auto nodeId = node.ID;

                                CheckLinkSafety(nullptr, payloadPin);

                                if (payloadPin->Kind == PinKind::Output || payloadPin->Data.Type == "Flow")
                                {
                                    auto variableNode = SpawnSetVariableNode(*payloadVariable);
                                    ImplicitExecuteableLinks(graph, payloadPin, variableNode);
                                }
                                else
                                    SpawnGetVariableNode(*payloadVariable);

                                ed::SetNodePosition(graph.Nodes.back().ID, ImVec2(ed::GetNodePosition(nodeId).x + (payloadPin->Kind == PinKind::Input ? -250.0f : 250.0f), ImGui::GetMousePos().y));
                                CreateLink(graph, &(payloadPin->Kind == PinKind::Input ? (graph.Nodes.back().Outputs[0]) : (graph.Nodes.back().Inputs[payloadPin->Data.Type == "Flow" ? 0 : 1])), payloadPin);

                                BuildNodes();
                            }
                        }

                    
                        for (auto& node : graph.Nodes)
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
                    
                                    if (m_NewLinkPin && !CanCreateLink(m_NewLinkPin, &pin) && &pin != m_NewLinkPin)
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
                    
                                if (m_NewLinkPin && !CanCreateLink(m_NewLinkPin, &pin) && &pin != m_NewLinkPin)
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
                    
                        for (auto& node : graph.Nodes)
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
                    
                                    if (m_NewLinkPin && !CanCreateLink(m_NewLinkPin, &pin) && &pin != m_NewLinkPin)
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
                    
                    
                                    if (m_NewLinkPin && !CanCreateLink(m_NewLinkPin, &pin) && &pin != m_NewLinkPin)
                                        outputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
                                }
                    
                                ImGui::EndHorizontal();
                            }
                    
                            ImGui::EndVertical();
                    
                            ed::EndNode();
                            ed::PopStyleVar(7);
                            ed::PopStyleColor(4);
                    
                            auto drawList = ed::GetNodeBackgroundDrawList(node.ID);
                        }
                    
                        for (auto& node : graph.Nodes)
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
                            //ImGui::TextUnformatted(node.Name.c_str());

                            //bool input;
		            		char buffer[256];
		            		memset(buffer, 0, sizeof(buffer));
		            		std::strncpy(buffer, node.Name.c_str(), sizeof(buffer));
                            ImGui::SetNextItemWidth(ImGui::GetItemRectSize().x);
                            //bool tree, ret = ImGui::TreeNodeInput("##CommentName", ImGuiTreeNodeFlags_Leaf, buffer, sizeof(buffer), tree, input);
                            bool input, clicked = ImGui::SelectableInput("##CommentName", node.Name.empty() ? 1.0f : ImGui::CalcTextSize(buffer).x, false, NULL, buffer, sizeof(buffer), input, ImGuiInputTextFlags_NoHorizontalScroll);
                            //if (tree) ImGui::TreePop();
		            		if (input) node.Name = std::string(buffer);


                            ImGui::Spring(1);
                            ImGui::EndHorizontal();
                            const ImVec2 size = node.GroupBounds.Max - node.GroupBounds.Min;
                            ed::Group(size.x != 0.0f ? size : node.Size);
                            ImGui::EndVertical();
                            ImGui::PopID();
                            ed::EndNode();if (ed::BeginGroupHint(node.ID))
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
                            ed::PopStyleColor(2);
                            ImGui::PopStyleVar();
                    
		            		// Drawing shadows beneath nodes
		            		auto& pos = ed::GetNodePosition(node.ID) - ImVec2(10.0f, 10.0f);
		            		auto& shadowSize = ed::GetNodeSize(node.ID) + ImVec2(20.0f, 20.0f);
		            		auto drawList = ImGui::GetWindowDrawList();

		            		const int vert_start_idx = drawList->VtxBuffer.Size;
		            		drawList->AddRect(pos, pos + shadowSize, ImColor(0, 0, 0, 75), ed::GetStyle().NodeRounding + 5.0f, 15, 20.0f);
		            		const int vert_end_idx = drawList->VtxBuffer.Size;
		            		ImDrawVert* vert_start = drawList->VtxBuffer.Data + vert_start_idx;
		            		ImDrawVert* vert_end = drawList->VtxBuffer.Data + vert_end_idx;
		            		for (ImDrawVert* vert = vert_start; vert < vert_end; vert++)
		            			if (vert->pos.x < pos.x || vert->pos.y < pos.y || vert->pos.x > pos.x + shadowSize.x || vert->pos.y > pos.y + shadowSize.y || (pos.y + shadowSize.y - vert->pos.y) > shadowSize.y * 0.5f)
		            				vert->col = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                            
                        }
                    
                        for (auto& link : graph.Links)
                            ed::Link(link.ID, link.StartPinID, link.EndPinID, link.Color, 2.0f);
                    
                        if (!m_CreateNewNode)
                        {
                            ed::BeginCreate(m_CurrentLinkColor, 10.0f);
                            ed::EndCreate();

                            if (ed::BeginCreate(m_CurrentLinkColor, 2.0f))
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
                                    if (startPinId.Get() == 0)
                                        DY_ASSERT(false);

                                    auto startPin = FindPin(startPinId);
                                    auto endPin   = FindPin(endPinId);
                    
                                    m_NewLinkPin = startPin ? startPin : endPin;
                    
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
                                        else if (endPin->Data.Type != startPin->Data.Type)
                                        {
                                            if (ScriptEngine::IsConversionAvalible(startPin->Data.Type, endPin->Data.Type))
                                            {
                                                std::string convertFunctionName = ScriptEngine::GetConversionName(startPin->Data.Type, endPin->Data.Type);

                                                showLabel("+ Create Conversion", ImColor(32, 45, 32, 180));
                                                if (ed::AcceptNewItem(ImColor(128, 206, 244), 4.0f))
                                                {
                                                    auto startNodeId = startPin->Node->ID;
                                                    auto endNodeId = endPin->Node->ID;

                                                    CheckLinkSafety(startPin, endPin);
                                                    
                                                    Node* node = SpawnNodeWithInternalName(convertFunctionName);

                                                    ImVec2 pos = (ed::GetNodePosition(startNodeId) + ed::GetNodePosition(endNodeId)) / 2;
                                                    ed::SetNodePosition(node->ID, pos);
                                                    
                                                    CreateLink(graph, startPin, &node->Inputs[0]);
                                                    CreateLink(graph, &node->Outputs[0], endPin);

                                                    BuildNodes();
                                                }
                                            }
                                            else if (endPin->Data.Type != startPin->Data.Type)
                                            {
                                                showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
                                                ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
                                            }
                                            else
                                            {
                                                showLabel("x Incompatible Pin Container", ImColor(45, 32, 32, 180));
                                                ed::RejectNewItem(ImColor(255, 0, 0), 1.0f);
                                            }
                                        }
                                        else
                                        {
                                            showLabel("+ Create Link", ImColor(32, 45, 32, 180));
                                            if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
                                                CreateLink(graph, startPin, endPin);
                                        }
                                    }
                                }
                    
                                ed::PinId pinId = 0;
                                if (ed::QueryNewNode(&pinId))
                                {
                                    m_NewLinkPin = FindPin(pinId);
                                    if (m_NewLinkPin)
                                        showLabel("+ Create Node", ImColor(32, 45, 32, 180));
                    
                                    if (ed::AcceptNewItem())
                                    {
                                        ResetSearchArea();
                                        m_CreateNewNode  = true;
                                        m_NewNodeLinkPin = FindPin(pinId);
                                        m_NewLinkPin = nullptr;
                                        ed::Suspend();
                                        ImGui::OpenPopup("Create New Node");
		            					UpdateSearchData();
                                        ed::Resume();
                                        m_NewNodePosition = ed::ScreenToCanvas(ImGui::GetMousePos());
                                    }
                                }
                            }
                            else
                            m_NewLinkPin = nullptr;
                    
                            ed::EndCreate();
                            if (ed::BeginDelete())
                            {
                                ed::LinkId linkId = 0;
                                while (ed::QueryDeletedLink(&linkId))
                                {
                                    if (ed::AcceptDeletedItem())
                                    {
                                        auto id = std::find_if(graph.Links.begin(), graph.Links.end(), [linkId](auto& link) { return link.ID == linkId; });
                                        if (id != graph.Links.end())
                                            graph.Links.erase(id);
                                    }
                                }

								ed::NodeId nodeId = 0;
								while (ed::QueryDeletedNode(&nodeId))
								{
									if (auto node = FindNode(nodeId))
									{
										if (node->Deletable)
										{
											if (ed::AcceptDeletedItem())
											{
												auto id = std::find_if(graph.Nodes.begin(), graph.Nodes.end(), [nodeId](auto& node) { return node.ID == nodeId; });
												if (id != graph.Nodes.end())
													graph.Nodes.erase(id);
											}
										}
									}
								}
                            }
                            ed::EndDelete();
                        }
                    
                        ImGui::SetCursorScreenPos(cursorTopLeft);
                    }
                    
# if 1
                    ed::Suspend();
                    if (ed::ShowNodeContextMenu(&m_ContextNodeId))
                        ImGui::OpenPopup("Node Context Menu");
                    else if (ed::ShowPinContextMenu(&m_ContextPinId))
                        ImGui::OpenPopup("Pin Context Menu");
                    else if (ed::ShowLinkContextMenu(&m_ContextLinkId))
                        ImGui::OpenPopup("Link Context Menu");
                    else if (ed::ShowBackgroundContextMenu())
                    {

                        ImGui::OpenPopup("Create New Node");
                        ResetSearchArea();
                        UpdateSearchData();
                        m_NewNodeLinkPin = nullptr;
                        m_NewNodePosition = ed::ScreenToCanvas(ImGui::GetMousePos());
                    }
                    ed::Resume();
                    
                    ed::Suspend();
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
                    if (ImGui::BeginPopup("Node Context Menu"))
                    {
                        auto node = FindNode(m_ContextNodeId);
                    
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
                            ImGui::Text("Unknown node: %p", m_ContextNodeId.AsPointer());
                        ImGui::Separator();
                        if (ImGui::MenuItem("Find References"))
                        {
                            ImGui::SetWindowFocus("Find Results");
                            FindResultsSearch(node->Name);
                        }

                        if (ImGui::MenuItem("Duplicate"))
                            DuplicateNodes();
                        if (ImGui::MenuItem("Delete"))
                            ed::DeleteNode(m_ContextNodeId);
                        ImGui::EndPopup();
                    }
                    
                    if (ImGui::BeginPopup("Pin Context Menu"))
                    {
                        auto pin = FindPin(m_ContextPinId);
                    
                        ImGui::TextUnformatted("Pin Context Menu");
                        ImGui::Separator();
                        if (pin)
                        {
                            ImGui::Text("ID: %p", pin->ID.AsPointer());
                            if (pin->Node)
                                ImGui::Text("Node: %p", pin->Node->ID.AsPointer());
                            else
                                ImGui::Text("Node: %s", "<none>");
                            ImGui::Text("Type: %s", pin->Data.Type);
                            ImGui::Separator();
                            if (pin->Data.Type != "Flow")
                            {
                                if (ImGui::MenuItem("Promote To Variable"))
                                {
                                    auto& variable = CreateVariable(pin->Name == "Return Value" ? "" : pin->Name, pin->Data.Type);

                                    CheckLinkSafety(nullptr, pin);

                                    bool in = pin->Kind == PinKind::Input;
                                    ImVec2 pos = ed::GetNodePosition(pin->Node->ID) + ImVec2(in ? -250.0f : 250.0f, 0.0f);
                                    Node* node;
                                    if (in)
                                        node = SpawnGetVariableNode(variable);
                                    else
                                    {
                                        node = SpawnSetVariableNode(variable);
                                        ImplicitExecuteableLinks(graph, pin, node);
                                    }
                                    ed::SetNodePosition(node->ID, pos);

                                    CreateLink(graph, &(in ? node->Outputs[0] : node->Inputs[1]), pin);

                                    BuildNodes();
                                }
                            }
                            if (pin->Deletable)
                                if (ImGui::MenuItem("Remove Pin"))
                                {
                                    for (auto& link : graph.Links)
                                        if (link.StartPinID == pin->ID || link.EndPinID == pin->ID)
                                            ed::DeleteLink(link.ID);

                                    auto& outputs = pin->Node->Outputs;
                                    auto& inputs = pin->Node->Inputs;
                                    auto& node = pin->Node;
                                    if (pin->Kind == PinKind::Output)
                                        for (int i = 0; i < outputs.size(); i++)
                                        {
                                            if (outputs[i].ID == pin->ID)
                                            {
                                                outputs.erase(outputs.begin() + i);
                                                break;
                                            }
                                        }
                                    else
		            					for (int i = 0; i < inputs.size(); i++)
		            					{
		            						if (inputs[i].ID == pin->ID)
		            						{
                                                inputs.erase(inputs.begin() + i);
		            							break;
		            						}
		            					}

                                    if (node->Name == "Sequence")
                                        for (int i = 0; i < node->Outputs.size(); i++)
                                            node->Outputs[i].Name = std::to_string(i);

                                    BuildNodes();
                                }
                        }
                        else
                            ImGui::Text("Unknown pin: %p", m_ContextPinId.AsPointer());
                    
                        ImGui::EndPopup();
                    }
                    
                    if (ImGui::BeginPopup("Link Context Menu"))
                    {
                        auto link = FindLink(m_ContextLinkId);
                    
                        ImGui::TextUnformatted("Link Context Menu");
                        ImGui::Separator();
                        if (link)
                        {
                            ImGui::Text("ID: %p", link->ID.AsPointer());
                            ImGui::Text("From: %p", link->StartPinID.AsPointer());
                            ImGui::Text("To: %p", link->EndPinID.AsPointer());
                        }
                        else
                            ImGui::Text("Unknown link: %p", m_ContextLinkId.AsPointer());
                        ImGui::Separator();
                        if (ImGui::MenuItem("Delete"))
                            ed::DeleteLink(m_ContextLinkId);
                        ImGui::EndPopup();
                    }
				    
                    if (m_DefaultColorPickerPosition.x != FLT_MAX)
                    {
                        ImGui::OpenPopup("###DefaultValueColorPicker");
                        ImGui::SetNextWindowPos(m_DefaultColorPickerPosition);
                        m_DefaultColorPickerPosition = { FLT_MAX, FLT_MAX };
                    }
				    if (ImGui::BeginPopup("###DefaultValueColorPicker"))
				    {

				    	ImGuiContext& g = *GImGui;
				    	const float square_sz = ImGui::GetFrameHeight();

				    	ImGuiColorEditFlags picker_flags_to_forward = ImGuiColorEditFlags_DataTypeMask_ | ImGuiColorEditFlags_PickerMask_ | ImGuiColorEditFlags_InputMask_ | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaBar;
				    	ImGuiColorEditFlags picker_flags = (ImGuiColorEditFlags_NoInputs & picker_flags_to_forward) | ImGuiColorEditFlags_DisplayMask_ | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf;
				        ImGui::SetNextItemWidth(square_sz * 12.0f); // Use 256 + bar sizes?
                        if (m_DefaultColorPickerValue != nullptr)
				    	    ImGui::ColorPicker4("##picker", m_DefaultColorPickerValue, picker_flags, &g.ColorPickerRef.x);
				    	ImGui::EndPopup();
				    }
                    
                    ImGui::SetNextWindowSize(ImVec2(400.0f, 400.0f));
                    if (ImGui::BeginPopup("Create New Node"))
                    {
                        Node* node = nullptr;
                    
                        ImGui::Text("All Actions for this Script");
                        ImGui::SameLine();
                        ImGui::Dummy(ImVec2(100.0f, 0.0f));
                        ImGui::SameLine();
                        if (ImGui::Checkbox("##NodeSearchPopupContextSensitiveCheckbox", &m_ContextSensitive))
                            UpdateSearchData();
                        ImGui::SameLine();
                        ImGui::Text("Context Sensitive");
                    
                        static bool SetKeyboardFocus = false;
                    
                        if (SetKeyboardFocus)
                        {
                            ImGui::SetKeyboardFocusHere();
                            SetKeyboardFocus = false;
                        }
                    
                        if (m_ResetSearchArea)
                        {
                            SetKeyboardFocus = true;
                        }
                    
                        // Search bar Input
		            	char buffer[256];
		            	memset(buffer, 0, sizeof(buffer));
		            	std::strncpy(buffer, m_SearchBuffer.c_str(), sizeof(buffer));
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
                        if (ImGui::InputTextWithHint("##NodeEditorSearchBar", "Search:", buffer, sizeof(buffer)))
                        {
                            m_SearchBuffer = std::string(buffer);
                            UpdateSearchData();
                        }
                    
                        if ((m_SearchBuffer != m_PreviousSearchBuffer) || m_ResetSearchArea)
                            m_SearchData.Open(!m_SearchBuffer.empty());
                        
                        node = DisplaySearchData(m_SearchData);
                    
                        m_ResetSearchArea = false;
                        m_PreviousSearchBuffer = m_SearchBuffer;
                    
                        if (node)
                        {
                            BuildNodes();
                    
                            m_CreateNewNode = false;

                            if (node->Type != NodeType::Comment || ed::GetSelectedObjectCount() == 0)
                                ed::SetNodePosition(node->ID, m_NewNodePosition);
                    
                            if (auto startPin = m_NewNodeLinkPin)
                            {
                                auto& pins = startPin->Kind == PinKind::Input ? node->Outputs : node->Inputs;

                                ImplicitExecuteableLinks(graph, startPin, node);

                                for (auto& pin : pins)
                                {
                                    Pin* output = startPin;
                                    Pin* input = &pin;
                                    if (output->Kind == PinKind::Input)
                                        std::swap(output, input);

                                    if (CanCreateLink(input, output))
                                    {
                                        if (ScriptEngine::IsConversionAvalible(output->Data.Type, input->Data.Type))
                                        {
                                            std::string convertFunctionName = ScriptEngine::GetConversionName(output->Data.Type, input->Data.Type);
                                        
		            						auto startNodeId = output->Node->ID;
		            						auto endNodeId = input->Node->ID;
                                        
                                            CheckLinkSafety(output, input);
                                        
		            						Node* node = SpawnNodeWithInternalName(convertFunctionName);
		            						ed::SetNodePosition(node->ID, (ed::GetNodePosition(startNodeId) + ed::GetNodePosition(endNodeId)) / 2);
                                            
                                            CreateLink(graph, output, &node->Inputs[0]);
                                            CreateLink(graph, &node->Outputs[0], input);
                                            
                                            BuildNodes();

                                            break;
                                        }
                                        else
                                        {
											CreateLink(graph, startPin, &pin);
											break;
                                        }
                                    }
                                }
                            }
                            BuildNodes();
                        }
                    
                        ImGui::EndPopup();
                    }
                    else
                        m_CreateNewNode = false;
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

                    // Update Node Position and Size (using group bounds) for all windows.

				    for (auto& node : graph.Nodes)
				    {
				    	auto& nodePos = ed::GetNodePosition(node.ID);
				    	if ((nodePos.x != node.Position.x || nodePos.y != node.Position.y) && (nodePos.x != FLT_MAX && nodePos.y != FLT_MAX))
				    	{
							// Updating Bounds held internally
                            if (node.Type == NodeType::Comment)
                            {
                                ImVec2 min, max;
                                ed::GetNodeBounds(node.ID, min, max);
							    node.GroupBounds = ImRect(min, max);
                            }

				    		node.Position = nodePos;
				    		for (auto& otherWindow : m_Windows)
				    			if (otherWindow.GraphID == graph.ID && window.ID != otherWindow.ID)
				    			{
				    				ed::SetCurrentEditor(otherWindow.InternalEditor);
				    				ed::SetNodePosition(node.ID, nodePos);
				    				ed::SetCurrentEditor(window.InternalEditor);
				    			}
				    	}
                        if (node.Type == NodeType::Comment)
                        {
                            auto& nodeSize = ed::GetNodeSize(node.ID);
                            if ((nodeSize.x != node.Size.x || nodeSize.y != node.Size.y) && (nodeSize.x != 0 && nodeSize.y != 0))
                            {
                                // Updating Bounds held internally
								ImVec2 min, max;
								ed::GetNodeBounds(node.ID, min, max);
								node.GroupBounds = ImRect(min, max);

                                node.Size = nodeSize;
                                for (auto& otherWindow : m_Windows)
                                    if (otherWindow.GraphID == graph.ID && window.ID != otherWindow.ID)
                                    {
                                        ed::SetCurrentEditor(otherWindow.InternalEditor);
                                        ed::SetNodeBounds(node.ID, min, max);
                                        ed::SetCurrentEditor(window.InternalEditor);
                                    }
                            }
                        }
				    }


                    ed::End();

                    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[2]);
                    ImGui::GetWindowDrawList()->AddText(ImGui::GetWindowPos() + ImGui::GetWindowSize() - ImGui::CalcTextSize("SCRIPTING") - ImVec2(20.0f, 20.0f), IM_COL32(255, 255, 255, 50), "SCRIPTING");
                    ImGui::PopFont();

		            // Shadows
		            auto drawList = ImGui::GetWindowDrawList();
		            const ImVec4& borderCol = ImGui::GetStyleColorVec4(ImGuiCol_BorderShadow);
		            const ImU32& minColor = ImGui::ColorConvertFloat4ToU32({ borderCol.x, borderCol.y, borderCol.z, 0.0f });
		            const ImU32& maxColor = ImGui::ColorConvertFloat4ToU32({ borderCol.x, borderCol.y, borderCol.z, 0.25f });
                    const float size = 75.0f;
		            drawList->AddRectFilledMultiColor(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImVec2(size, ImGui::GetWindowSize().y), maxColor, minColor, minColor, maxColor);
		            drawList->AddRectFilledMultiColor(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImVec2(ImGui::GetWindowSize().x, size), maxColor, maxColor, minColor, minColor);
		            drawList->AddRectFilledMultiColor(ImGui::GetWindowPos() + ImGui::GetWindowSize(), ImGui::GetWindowPos() + ImGui::GetWindowSize() - ImVec2(size, ImGui::GetWindowSize().y), maxColor, minColor, minColor, maxColor);
		            drawList->AddRectFilledMultiColor(ImGui::GetWindowPos() + ImGui::GetWindowSize(), ImGui::GetWindowPos() + ImGui::GetWindowSize() - ImVec2(ImGui::GetWindowSize().x, size), maxColor, maxColor, minColor, minColor);
                }

                ImGui::End();
                ImGui::PopID();

                if (!open)
                    m_Windows.erase(m_Windows.begin() + i);
            }
        }
    
    	ImGui::SetNextWindowClass(&windowClass);
    	ImGui::Begin("Nodes", NULL, ImGuiWindowFlags_NoNavFocus);
    	ShowLeftPane(ImGui::GetWindowSize().x);
    	ImGui::End();

        const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

        // Script Contents Section
        {
            ImGui::SetNextWindowClass(&windowClass);
            ImGui::Begin("Script Contents", NULL, ImGuiWindowFlags_NoNavFocus);

            if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
                m_SelectedVariable = 0;

            ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
                float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
                bool open = ImGui::TreeNodeEx("##VariablesList", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen, "VARIABLES");
                ImGui::PopStyleVar();
                ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
                if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
                    CreateVariable();
                if (open)
                {
                    int id_to_erase = 0;

                    for (auto& variable : m_Variables)
                    {
                        bool isDeleted = false, isDuplicated = false;

                        ImGui::PushID(variable.second.ID);

                        char buffer[256];
                        memset(buffer, 0, sizeof(buffer));
                        std::strncpy(buffer, variable.second.Name.c_str(), sizeof(buffer));
                        bool input;
                        bool clicked = ImGui::SelectableInput("##VariableTree", ImGui::GetContentRegionAvail().x, m_SelectedVariable == variable.second.ID, ImGuiSelectableFlags_None, buffer, sizeof(buffer), input);

                        if (ImGui::IsItemHovered() && !variable.second.Tooltip.empty())
                            ImGui::SetTooltip(variable.second.Tooltip.c_str());

                        ImGui::PushID("##VariableTree");
                        if (GImGui->ActiveId != ImGui::GetCurrentWindow()->GetID("##Input") && GImGui->ActiveIdPreviousFrame == ImGui::GetCurrentWindow()->GetID("##Input"))
                            SetVariableName(&variable.second, buffer);
                        ImGui::PopID();

                        if (ImGui::BeginDragDropSource())
                        {
                            ImGui::SetDragDropPayload("NODE_EDITOR_VARIABLE", &variable.second.ID, sizeof(variable.second.ID), ImGuiCond_Once);
                            ImGui::Text(variable.second.Name.c_str());
                            ImGui::EndDragDropSource();
                        }

                        if (clicked)
                        {
                            m_SelectedVariable = variable.second.ID;

                            m_SelectedDetailsFunction = NodeFunction::Variable;
                            m_SelectedDetailsReference = variable.second.ID;
                        }

                        if (ImGui::BeginPopupContextItem())
                        {
                            if (ImGui::MenuItem("Duplicate")) isDuplicated = true;
                            if (ImGui::MenuItem("Delete")) isDeleted = true;

                            ImGui::EndPopup();
                        }

                        ImGui::PopID();

                        if (isDeleted)
                            id_to_erase = variable.second.ID;
                        if (isDuplicated)
                        {
                            int id = GetNextId();

                            m_Variables[id] = variable.second;
                            m_Variables[id].ID = id;

                            std::vector<std::string> variableNames;
                            for (auto& variable : m_Variables)
                                variableNames.push_back(variable.second.Name);
                            m_Variables[id].Name = String::GetNextNameWithIndex(variableNames, m_Variables[id].Name);
                        }
                    }

                    if (id_to_erase != 0)
                    {
                        if (m_SelectedDetailsReference == id_to_erase && m_SelectedDetailsFunction == NodeFunction::Variable)
                        {
                            m_SelectedDetailsFunction = NodeFunction::Node;
                            m_SelectedDetailsReference = 0;
                        }
                        m_SelectedVariable = 0;

                        m_Variables.erase(m_Variables.find(id_to_erase));
                    }

                    ImGui::TreePop();
                }
            }

            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
                float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
                bool open = ImGui::TreeNodeEx("##GraphsList", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen, "EVENT GRAPHS");
                ImGui::PopStyleVar();
                ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
                if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
                    CreateEventGraph();
                if (open)
                {
                    for (auto& graph : m_Graphs)
                    {
                        if (graph.Type != NodeGraphType::Event)
                            continue;

                        DrawGraphOption(graph);
                    }

                    ImGui::TreePop();
                }
            }

            // Macro list section
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
                float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
                bool open = ImGui::TreeNodeEx("##MacrosList", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen, "MACROS");
                ImGui::PopStyleVar();
                ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
                if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
                    CreateMacro();
                if (open)
                {
                    for (auto& graph : m_Graphs)
                    {
                        if (graph.Type != NodeGraphType::Macro)
                            continue;

                        DrawGraphOption(graph);
                    }

                    ImGui::TreePop();
                }
            }

            // Class properties section
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
				bool open = ImGui::TreeNodeEx("##ClassProperties", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen, "CLASS PROPERTIES");
				ImGui::PopStyleVar();
				if (open)
				{
					ImGui::TextDisabled("Parent");
					if (ImGui::BeginCombo("##ParentSelectionDropdown", m_ScriptClass.c_str()))
					{
						for (auto& [name, entity] : ScriptEngine::GetEntityClasses())
						{
							if (ImGui::Selectable(name.c_str()))
								m_ScriptClass = name;
						}


						ImGui::EndCombo();
					}
					ImGui::TreePop();
				}
			}

            // Assembly diagnostics section
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
				bool open = ImGui::TreeNodeEx("##AssemblyDiagnostics", treeNodeFlags, "ASSEMBLY DIAGNOSTICS");
				ImGui::PopStyleVar();
				if (open)
				{
					ImGui::TextDisabled("Overridable Methods");
					ImGui::Indent();
					for (auto& overridableMethod : ScriptEngine::GetEntityClassOverridableMethods(m_ScriptClass))
						ImGui::TextUnformatted(overridableMethod.c_str());
					ImGui::Unindent();

					ImGui::TextDisabled("Static Methods");
					ImGui::Indent();
					for (auto& [name, method] : ScriptEngine::GetStaticMethodDeclarations())
						ImGui::TextUnformatted(name.c_str());
					ImGui::Unindent();

					ImGui::TreePop();
				}
			}


            ImGui::End();
        }

		// Details Section
        {
            ImGui::SetNextWindowClass(&windowClass);
            ImGui::Begin("Details", NULL, ImGuiWindowFlags_NoNavFocus);

            float columnWidth = ImGui::GetWindowWidth() / 3.0f;
            ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

            switch (m_SelectedDetailsFunction)
            {
            case NodeFunction::Variable:
                if (m_Variables.find(m_SelectedDetailsReference) != m_Variables.end())
                {
                    auto& variable = m_Variables[m_SelectedDetailsReference];
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
                        bool open = ImGui::TreeNodeEx("##VariableInfo", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen, "VARIABLE");
                        ImGui::PopStyleVar();
                        if (open)
                        {
                            ImGui::Columns(2);
                            ImGui::SetColumnWidth(0, columnWidth);
                            ImGui::Text("Variable Name");
                            ImGui::NextColumn();
                            ImGui::SetNextItemWidth(-1);
                            char buffer[256];
                            memset(buffer, 0, sizeof(buffer));
                            std::strncpy(buffer, variable.Name.c_str(), sizeof(buffer));
                            ImGui::InputText("##VariableName", buffer, sizeof(buffer));
                            if ((ImGui::IsItemActivePreviousFrame() && !ImGui::IsItemActive()) || (ImGui::IsItemActive() && ImGui::IsKeyPressed(io.KeyMap[ImGuiKey_Enter])))
                                SetVariableName(&variable, buffer);

                            ImGui::Columns(1);

                            ImGui::Columns(2);
                            ImGui::SetColumnWidth(0, columnWidth);
                            ImGui::Text("Variable Type");
                            ImGui::NextColumn();
                            ImGui::SetNextItemWidth(-20);
                            if (ValueTypeSelection(variable.Data))
                                SetVariableType(&variable, variable.Data.Type);
                            ImGui::Columns(1);

                            ImGui::Columns(2);
                            ImGui::SetColumnWidth(0, columnWidth);
                            ImGui::Text("Tooltip");
                            ImGui::NextColumn();
                            ImGui::SetNextItemWidth(-1);
                            char tooltipBuffer[256];
                            memset(tooltipBuffer, 0, sizeof(tooltipBuffer));
                            std::strncpy(tooltipBuffer, variable.Tooltip.c_str(), sizeof(tooltipBuffer));
                            if (ImGui::InputText("##VariableTooltip", tooltipBuffer, sizeof(tooltipBuffer)))
                                variable.Tooltip = std::string(tooltipBuffer);
                            ImGui::Columns(1);

                            ImGui::Columns(2);
                            ImGui::SetColumnWidth(0, columnWidth);
                            ImGui::Text("Public");
                            ImGui::NextColumn();
                            ImGui::Checkbox("##VariablePublicCheckbox", &variable.Public);
                            ImGui::Columns(1);

                            ImGui::TreePop();
                        }
                    }

                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
                        bool open = ImGui::TreeNodeEx("##DefaultValue", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen, "DEFAULT VALUE");
                        ImGui::PopStyleVar();
                        if (open)
                        {
                            ImGui::Columns(2);
                            ImGui::SetColumnWidth(0, columnWidth);
                            ImGui::Text(variable.Name.c_str());
                            ImGui::NextColumn();
                            ImGui::PushID(variable.ID);
                            DefaultValueInput(variable.Data);
                            ImGui::PopID();
                            ImGui::TreePop();
                            ImGui::Columns(1);
                        }
                    }
                }
                break;

            case NodeFunction::EventImplementation:
                for (auto& graph : m_Graphs)
                    for (auto& node : graph.Nodes)
                        if (node.ID == (ed::NodeId)m_SelectedDetailsReference)
                        {
                            {
                                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
                                bool open = ImGui::TreeNodeEx("##EventInfo", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen, "EVENT");
                                ImGui::PopStyleVar();
                                if (open)
                                {
                                    ImGui::Columns(2);
                                    ImGui::SetColumnWidth(0, columnWidth);
                                    ImGui::Text("Event Name");
                                    ImGui::NextColumn();
                                    ImGui::SetNextItemWidth(-1);
                                    char buffer[256];
                                    memset(buffer, 0, sizeof(buffer));
                                    std::strncpy(buffer, node.Name.c_str(), sizeof(buffer));
                                    ImGui::InputText("##VariableName", buffer, sizeof(buffer));
                                    if ((ImGui::IsItemActivePreviousFrame() && !ImGui::IsItemActive()))
                                        SetCustomEventName(&node, buffer);

                                    ImGui::TreePop();
                                    ImGui::Columns(1);
                                }
                            }

                            {
                                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
                                float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
                                bool open = ImGui::TreeNodeEx("##InputInfo", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen, "INPUTS");
                                ImGui::PopStyleVar();
                                ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
                                if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
                                    AddCustomEventInput(&node);
                                if (open)
                                {
                                    for (auto& input : node.Outputs)
                                    {
                                        if (input.Data.Type == "Flow")
                                            continue;

                                        ImGui::PushID(input.ID.Get());

										char buffer[256];
										memset(buffer, 0, sizeof(buffer));
										std::strncpy(buffer, input.Name.c_str(), sizeof(buffer));
                                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
										ImGui::InputText("##Input", buffer, sizeof(buffer));
                                        if ((ImGui::IsItemActivePreviousFrame() && !ImGui::IsItemActive()))
                                        {
                                            input.Name = buffer;
                                            UpdateCustomEventInput(&input);
                                        }

                                        ImGui::SameLine();
                                        ImGui::SetNextItemWidth(-25);
                                        if (ValueTypeSelection(input.Data))
                                            UpdateCustomEventInput(&input);

                                        ImGui::SameLine();
                                        DefaultValueInput(input.Data);

                                        ImGui::PopID();
                                    }

                                    ImGui::TreePop();
                                }
                            }

                            break;
                        }
                break;
            case NodeFunction::Macro:
            case NodeFunction::MacroBlock:
				if (auto macro = FindGraphByID(m_SelectedDetailsReference))
				{
					{
						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
						bool open = ImGui::TreeNodeEx("##MacroInfo", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen, "MACRO");
						ImGui::PopStyleVar();
						if (open)
						{
							ImGui::Columns(2);
							ImGui::SetColumnWidth(0, columnWidth);
							ImGui::Text("Macro Name");
							ImGui::NextColumn();
							ImGui::SetNextItemWidth(-1);
							char buffer[256];
							memset(buffer, 0, sizeof(buffer));
							std::strncpy(buffer, macro->Name.c_str(), sizeof(buffer));
							ImGui::InputText("##MacroName", buffer, sizeof(buffer));
							if ((ImGui::IsItemActivePreviousFrame() && !ImGui::IsItemActive()))
								SetMacroName(macro->ID, buffer);

							ImGui::TreePop();
							ImGui::Columns(1);
						}
					}

					{
                        // Macro Inputs
						for (auto& node : macro->Nodes)
                            if (node.Function == NodeFunction::MacroBlock && node.Name == "Inputs")
                            {
                                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
                                float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
                                bool open = ImGui::TreeNodeEx("##InputInfo", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen, "INPUTS");
                                ImGui::PopStyleVar();
                                ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
                                if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
                                    AddMacroPin(macro->ID, true);
                                if (open)
                                {
                                    for (auto& input : node.Outputs)
                                    {
                                        ImGui::PushID(input.ID.Get());

                                        const float width = ImGui::GetContentRegionAvailWidth() * (1.0f / 3.0f);

                                        char buffer[256];
                                        memset(buffer, 0, sizeof(buffer));
                                        std::strncpy(buffer, input.Name.c_str(), sizeof(buffer));
                                        ImGui::SetNextItemWidth(width);
                                        ImGui::InputText("##Input", buffer, sizeof(buffer));
                                        if ((ImGui::IsItemActivePreviousFrame() && !ImGui::IsItemActive()))
                                        {
                                            input.Name = buffer;
                                            UpdateMacroPin(macro->ID, &input);
                                        }

                                        ImGui::SameLine();
                                        ImGui::SetNextItemWidth(width - 25.0f);
                                        if (ValueTypeSelection(input.Data, true))
                                            UpdateMacroPin(macro->ID, &input);

                                        ImGui::SameLine();
                                        DefaultValueInput(input.Data);

                                        ImGui::PopID();
                                    }
                                    ImGui::TreePop();
                                }

                                break;
                            }

                        // Macro Outputs
						for (auto& node : macro->Nodes)
							if (node.Function == NodeFunction::MacroBlock && node.Name == "Outputs")
							{
								ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
								float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
								bool open = ImGui::TreeNodeEx("##OutputInfo", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen, "OUTPUTS");
								ImGui::PopStyleVar();
								ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
								if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
									AddMacroPin(macro->ID, false);
								if (open)
								{
                                    ImGui::Unindent();

									for (auto& output : node.Inputs)
									{
										ImGui::PushID(output.ID.Get());

                                        ImGui::PushStyleColor(ImGuiCol_Header, {});
                                        ImGui::PushStyleColor(ImGuiCol_HeaderActive, {});
                                        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {});
                                        bool open = ImGui::TreeNodeBehavior(ImGui::GetID("##DetailDropdown"), ImGuiTreeNodeFlags_OpenOnArrow, "");
                                        ImGui::PopStyleColor(3);
                                        ImGui::SameLine();

										const float width = ImGui::GetContentRegionAvailWidth() * 0.5f;

										char buffer[256];
										memset(buffer, 0, sizeof(buffer));
										std::strncpy(buffer, output.Name.c_str(), sizeof(buffer));
										ImGui::SetNextItemWidth(width);
										ImGui::InputText("##Output", buffer, sizeof(buffer));
										if ((ImGui::IsItemActivePreviousFrame() && !ImGui::IsItemActive()))
										{
                                            output.Name = buffer;
											UpdateMacroPin(macro->ID, &output);
										}

										ImGui::SameLine();
										ImGui::SetNextItemWidth(width - 25.0f - lineHeight - style.WindowPadding.x * 2.0f);
										if (ValueTypeSelection(output.Data, true))
											UpdateMacroPin(macro->ID, &output);

                                        ImGui::SameLine();

                                        if (ImGui::ImageButton((ImTextureID)m_RemoveIcon->GetRendererID(), ImVec2(lineHeight * 0.5f, lineHeight * 0.5f), { 0, 1 }, { 1, 0 }))
                                            RemoveMacroPin(macro->ID, &output);

                                        if (open)
                                        {
										    DefaultValueInput(output.Data);
                                            ImGui::TreePop();
                                        }

										ImGui::PopID();
									}

                                    ImGui::Indent();
									ImGui::TreePop();
								}

								break;
							}
					}
				}

                break;
            }

            ImGui::End();
        }

        // Compiler Results Window
        {
            ImGui::SetNextWindowClass(&windowClass);
            ImGui::Begin("Compiler Results", NULL, ImGuiWindowFlags_NoNavFocus);

            auto drawList = ImGui::GetWindowDrawList();

            for (auto& result : m_CompilerResults)
            {
                float fontSize = ImGui::GetFontSize();
                if (result.Type == CompilerResultType::Compile)
                {
                    drawList->AddTriangleFilled(ImGui::GetCursorScreenPos() + ImVec2(fontSize * 0.25f, fontSize * 0.25f), ImGui::GetCursorScreenPos() + ImVec2(fontSize * 0.25f, fontSize * 0.75f), ImGui::GetCursorScreenPos() + ImVec2(fontSize * 0.75f, fontSize / 2.0f), ImColor(255, 255, 255));
                }
                else if (result.Type == CompilerResultType::Error)
                {
                    float radiusAdd = std::sqrt(std::pow(fontSize / 2.0f, 2));
                    drawList->AddCircle(ImGui::GetCursorScreenPos() + ImVec2(fontSize / 2.0f, fontSize / 2.0f), fontSize / 2.0f, ImColor(255, 0, 0), 0, 2.0f);
                    drawList->AddLine(ImGui::GetCursorScreenPos() + ImVec2(radiusAdd / 4.0f, radiusAdd / 4.0f), ImGui::GetCursorScreenPos() + ImVec2(1.5f * radiusAdd, 1.5f * radiusAdd), ImColor(255, 0, 0), 2.0f);
                }
                else
                {
                    drawList->AddCircleFilled(ImGui::GetCursorScreenPos() + ImVec2(fontSize / 2.0f, fontSize / 2.0f), fontSize / 4.0f, ImColor(85, 200, 255));
                }
                ImGui::Dummy(ImVec2(fontSize, fontSize));
                ImGui::SameLine();
                ImGui::Selectable(result.Message.c_str());
                if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0) && result.ID.Get())
                    NavigateToNode(result.ID);
            }

            ImGui::End();
        }

        // Find Results Window
        {
            ImGui::SetNextWindowClass(&windowClass);
            ImGui::Begin("Find Results", NULL, ImGuiWindowFlags_NoNavFocus);

            auto drawList = ImGui::GetWindowDrawList();

            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            std::strncpy(buffer, m_FindResultsSearchBar.c_str(), sizeof(buffer));
            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextWithHint("##FindResultsSearchBar", "Enter function or event name to find references...", buffer, sizeof(buffer));
            if ((ImGui::IsItemActivePreviousFrame() && !ImGui::IsItemActive()) || (ImGui::IsItemActive() && ImGui::IsKeyPressed(io.KeyMap[ImGuiKey_Enter])))
                FindResultsSearch(buffer);

            if (m_FindResultsData.empty())
                ImGui::Text("No results found");
            else
            {
                const float lineHeight = ImGui::GetTextLineHeight();
                for (auto& graph : m_FindResultsData)
                {
                    ImGui::PushID(graph.ID);
                    if (ImGui::Selectable(graph.Name.c_str())) OpenGraph(graph.ID);
                    ImGui::Indent();
                    for (auto& node : graph.SubData)
                    {
                        ImGui::PushID(node.ID);
                        if (node.Icon != nullptr)
                        {
                            ImGui::Image((ImTextureID)node.Icon->GetRendererID(), ImVec2(lineHeight, lineHeight), { 0, 1 }, { 1, 0 }, node.Color);
                            ImGui::SameLine();
                        }
                        if (ImGui::Selectable(node.Name.c_str())) NavigateToNode(node.ID);
                        ImGui::Indent();
                        for (auto& pin : node.SubData)
                        {
                            ImGui::PushID(node.ID);
                            ImGui::GetWindowDrawList()->AddCircleFilled(ImGui::GetCursorScreenPos() + ImVec2(lineHeight / 2.0f, lineHeight / 2.0f), lineHeight / 2.5f, pin.Color);
                            ImGui::Dummy(ImVec2(lineHeight / 2.0f, lineHeight / 2.0f));
                            ImGui::SameLine();
                            if (ImGui::Selectable(pin.Name.c_str())) NavigateToNode(node.ID);
                            ImGui::PopID();
                        }
                        ImGui::Unindent();
                        ImGui::PopID();
                    }
                    ImGui::Unindent();
                    ImGui::PopID();
                }
            }

            ImGui::End();
        }
    }
}

Variable& ScriptEditorInternal::CreateVariable(const std::string& name, const std::string& type)
{
	std::vector<std::string> vec;
	for (auto& variable : m_Variables)
		vec.push_back(variable.second.Name);
    int id = GetNextId();
    m_Variables[id] = { id, Dymatic::String::GetNextNameWithIndex(vec, name.empty() ? "NewVar" : name), (type == "System.Void" ? m_RecentPinType : type)};

    return m_Variables[id];
}

void ScriptEditorInternal::SetVariableType(Variable* variable, const std::string& type)
{
	m_RecentPinType = type;

    if (variable != nullptr)
    {
        variable->Data.Type = type;
        for (auto& graph : m_Graphs)
            for (auto& node : graph.Nodes)
                if (node.Function == NodeFunction::Variable && node.ReferenceId == variable->ID)
                {
                    for (auto& input : node.Inputs)
                        if (input.Data.Type != "Flow")
                            input.Data.Type = type;

                    for (auto& output : node.Outputs)
                        if (output.Data.Type != "Flow")
                            output.Data.Type = type;
                }
    }
}

void ScriptEditorInternal::SetVariableName(Variable* variable, const std::string& name)
{
	if (variable != nullptr)
	{
		variable->Name = name;
		for (auto& graph : m_Graphs)
			for (auto& node : graph.Nodes)
				if (node.Function == NodeFunction::Variable && node.ReferenceId == variable->ID)
				{
					node.Name = (node.Type == NodeType::Simple ? "DymaticVariable_Get " : "DymaticVariable_Set ") + name;
					node.DisplayName = (node.Type == NodeType::Simple ? "Get " : "Set ") + name;
				}
	}
}

void ScriptEditorInternal::CreateEventGraph()
{
    // Check for existing graph names
	std::vector<std::string> names;
	for (auto& graph : m_Graphs)
		names.push_back(graph.Name);

    // Create event graph
	m_Graphs.emplace_back(GetNextId(), Dymatic::String::GetNextNameWithIndex(names, "New Event Graph"), NodeGraphType::Event);
}

void ScriptEditorInternal::CreateMacro()
{
    // Check for existing graph names
	std::vector<std::string> names;
	for (auto& graph : m_Graphs)
        names.push_back(graph.Name);

    // Create Macro
	auto& macro = m_Graphs.emplace_back(GetNextId(), Dymatic::String::GetNextNameWithIndex(names, "New Macro"), NodeGraphType::Macro);

    // Setup input and output node
	{
		auto& input = macro.Nodes.emplace_back(GetNextId(), "Inputs", ImColor(128, 195, 248), NodeFunction::MacroBlock);
		input.Deletable = false;
		input.Outputs.emplace_back(GetNextId(), "", "Flow");
		input.Icon = m_MacroIcon;
        input.ReferenceId = macro.ID;
		BuildNode(&input);
		SetNodePosition(input.ID, { -100.0f, 0.0f });

		auto& output = macro.Nodes.emplace_back(GetNextId(), "Outputs", ImColor(128, 195, 248), NodeFunction::MacroBlock);
		output.Deletable = false;
		output.Inputs.emplace_back(GetNextId(), "", "Flow");
		output.Icon = m_MacroIcon;
        output.ReferenceId = macro.ID;
		BuildNode(&output);
		SetNodePosition(output.ID, { 100.0f, 0.0f });

		CreateLink(macro, &macro.Nodes[0].Outputs[0], &macro.Nodes[1].Inputs[0]);
	}

	OpenGraphInNewTab(macro.ID);
}

void ScriptEditorInternal::SetMacroName(int macroId, const std::string& name)
{
    if (auto macro = FindGraphByID(macroId))
    {
        if (macro->Editable)
        {
            macro->Name = name;
            for (auto& graph : m_Graphs)
                for (auto& node : graph.Nodes)
                    if (node.Function == NodeFunction::Macro)
                    {
                        node.Name = name;
                        node.DisplayName = name;
                    }
        }
    }
}

void ScriptEditorInternal::AddMacroPin(int macroId, bool input)
{
    // Find macro
    if (auto macro = FindGraphByID(macroId))
    {
        // Add to input/output blocks
        for (auto& node : macro->Nodes)
            if (node.Function == NodeFunction::MacroBlock)
                if (input && node.Name == "Inputs" || !input && node.Name == "Outputs")
                {
                    auto& pins = input ? node.Outputs : node.Inputs;
                    auto& name = "Param " + std::to_string(pins.size());
                    pins.emplace_back(GetNextId(), name, m_RecentPinType);
                    BuildNode(&node);

                    // Add to macro calls
                    for (auto& graph : m_Graphs)
                        for (auto& node : graph.Nodes)
                            if (node.Function == NodeFunction::Macro)
                            {
                                auto& pins = input ? node.Inputs : node.Outputs;
                                pins.emplace_back(GetNextId(), name, m_RecentPinType);
                                BuildNode(&node);
                            }

                    break;
                }
    }
}

void ScriptEditorInternal::UpdateMacroPin(int macroId, Pin* pin)
{
    if (auto macro = FindGraphByID(macroId))
    {
        size_t index = 0;
        auto& macro_pins = (pin->Kind == PinKind::Input ? pin->Node->Inputs : pin->Node->Outputs);

        for (auto& macro_pin : macro_pins)
        {
            if (macro_pin.ID == pin->ID)
                break;
            index++;
        }

        if (index != macro_pins.size())
        {
            for (auto& graph : m_Graphs)
                for (auto& node : graph.Nodes)
                    if (node.Function == NodeFunction::Macro && node.ReferenceId == macroId)
                    {
                        auto& pins = (pin->Kind == PinKind::Input ? node.Outputs : node.Inputs);
                        if (index >= pins.size())
                            break;

						pins[index].Name = pin->Name;
						pins[index].Data = pin->Data;

                        break;
                    }
        }
    }
}

void ScriptEditorInternal::RemoveMacroPin(int macroId, Pin* pin)
{
    if (auto macro = FindGraphByID(macroId))
    {
        auto& pins = pin->Kind == PinKind::Input ? pin->Node->Inputs : pin->Node->Outputs;
        size_t index = 0;
        for (auto& p : pins)
        {
            if (p.ID == pin->ID)
                break;
            index++;
        }

        if (index != pins.size())
        {
            pins.erase(pins.begin() + index);
        }
    }
}

void ScriptEditorInternal::SetCustomEventName(Node* event, const std::string& name)
{
    event->Name = name;
    event->DisplayName = name;

    for (auto& graph : m_Graphs)
        for (auto& node : graph.Nodes)
            if (node.Function == NodeFunction::EventCall && node.ReferenceId == event->ID.Get())
            {
                node.Name = name;
                node.DisplayName = name;
            }
}

void ScriptEditorInternal::AddCustomEventInput(Node* event)
{
    auto& name = "Param " + std::to_string(event->Outputs.size());
    event->Outputs.emplace_back(GetNextId(), name, m_RecentPinType);
    BuildNode(event);

    for (auto& graph : m_Graphs)
        for (auto& node : graph.Nodes)
            if (node.Function == NodeFunction::EventCall && node.ReferenceId == event->ID.Get())
            {
                node.Inputs.emplace_back(GetNextId(), name, m_RecentPinType);
                BuildNode(&node);
            }
}

void ScriptEditorInternal::UpdateCustomEventInput(Pin* input)
{
    size_t index = 0;
    for (auto& pin : input->Node->Outputs)
    {
        if (pin.ID == input->ID)
            break;
        index++;
    }

    if (index != input->Node->Outputs.size())
    {
        for (auto& graph : m_Graphs)
            for (auto& node : graph.Nodes)
                if (node.Function == NodeFunction::EventCall && node.ReferenceId == input->Node->ID.Get())
                {
                    if (index >= node.Inputs.size())
                        break;

                    node.Inputs[index].Name = input->Name;
                    node.Inputs[index].Data = input->Data;

                    break;
                }
    }
}

void ScriptEditorInternal::FindResultsSearch(const std::string& search)
{
    m_FindResultsSearchBar = search;
    m_FindResultsData.clear();

    for (auto& graph : m_Graphs)
    {
        bool graphCreated = graph.Name.find(m_FindResultsSearchBar) != std::string::npos;
        if (graphCreated)
            m_FindResultsData.emplace_back(graph.Name, ImColor(255, 255, 255), graph.ID);
        for (auto& node : graph.Nodes)
        {
            bool created = node.Name.find(m_FindResultsSearchBar) != std::string::npos || node.DisplayName.find(m_FindResultsSearchBar) != std::string::npos;
            if (created)
            {
                if (!graphCreated)
                {
                    m_FindResultsData.emplace_back(graph.Name, ImColor(255, 255, 255), graph.ID);
                    graphCreated = true;
                }
                auto& nodeData = m_FindResultsData.back().SubData.emplace_back(node.Name, node.IconColor, node.ID.Get());
                nodeData.Icon = node.Icon;
            }

            for (auto& pin : node.Inputs)
                if (pin.GetName().find(m_FindResultsSearchBar) != std::string::npos)
                {
                    if (!graphCreated)
                    {
                        m_FindResultsData.emplace_back(graph.Name, ImColor(255, 255, 255), graph.ID);
                        graphCreated = true;
                    }
                    if (!created)
                    {
                        m_FindResultsData.back().SubData.emplace_back(node.Name, node.Color, node.ID.Get());
                        created = true;
                    }
                    m_FindResultsData.back().SubData.back().SubData.emplace_back(pin.GetName(), GetIconColor(pin.Data.Type), node.ID.Get());
                }

            for (auto& pin : node.Outputs)
                if (pin.GetName().find(m_FindResultsSearchBar) != std::string::npos)
                {
                    if (!graphCreated)
                    {
                        m_FindResultsData.emplace_back(graph.Name, ImColor(255, 255, 255), graph.ID);
                        graphCreated = true;
                    }
                    if (!created)
                    {
                        m_FindResultsData.back().SubData.emplace_back(node.Name, node.Color, node.ID.Get());
                        created = true;
                    }
                    m_FindResultsData.back().SubData.back().SubData.emplace_back(pin.GetName(), GetIconColor(pin.Data.Type), node.ID.Get());
                }
        }
    }
}

ScriptEditorPannel::ScriptEditorPannel()
{
	m_InternalEditor = new ScriptEditorInternal();
}

ScriptEditorPannel::~ScriptEditorPannel()
{
	delete m_InternalEditor;
}

void ScriptEditorPannel::OnEvent(Event& e)
{
	m_InternalEditor->OnEvent(e);
}

void ScriptEditorPannel::CompileNodes()
{
	m_InternalEditor->CompileNodes();
}

void ScriptEditorPannel::CopyNodes()
{
	m_InternalEditor->CopyNodes();
}

void ScriptEditorPannel::PasteNodes()
{
	m_InternalEditor->PasteNodes();
}

void ScriptEditorPannel::DuplicateNodes()
{
	m_InternalEditor->DuplicateNodes();
}

void ScriptEditorPannel::DeleteNodes()
{
	m_InternalEditor->DeleteNodes();
}

void ScriptEditorPannel::OnImGuiRender()
{
	m_InternalEditor->OnImGuiRender();
}

}