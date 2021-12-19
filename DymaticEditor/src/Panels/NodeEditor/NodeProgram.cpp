#include "NodeProgram.h"

#include <imgui/imgui.h>
#include "utilities/builders.h"
#include "utilities/widgets.h"

#include <ImGuiNode/imgui_node_editor.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_internal.h>

#include "../ImGuiCustom.h"

#include "../../TextSymbols.h"

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
    Void,
    Bool,
    Int,
    Float,
    String,
    Vector,
    Color,
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
    Function,
    Variable
};

enum class NodeType
{
    Blueprint,
    Simple,
    Tree,
    Comment,
    Houdini
};

enum class ContainerType
{
    Single,
    Array,
    Set,
    Map
};

struct TypeValue
{
    bool Bool = false;
    int Int = 0;
    float Float = 0.0f;
    std::string String = "";
    glm::vec3 Vector = { 0.0f, 0.0f, 0.0f };
    glm::vec4 Color{};
};

struct Node;

struct PinData
{
    PinData() = default;
    PinData(PinType type, ContainerType container = ContainerType::Single)
        : Type(type), Container(container)
    {
    }

	PinType Type = PinType::Void;
	ContainerType Container = ContainerType::Single;
    TypeValue Value;

    // Compile Side Info
    bool Reference = false;
    bool Pointer = false;

    bool Static = false;
    bool Const = false;
};

struct Pin
{
	ed::PinId   ID;
	Node* Node;
	std::string Name;
	std::string Tooltip;
    PinData Data;
	PinKind Kind;
    bool Deletable = false;

    // Compile Time Data
    bool Written = false;
    int ArgumentID = -1;

    Pin() = default;
	Pin(int id, std::string name, PinType type) :
		ID(id), Node(nullptr), Name(name), Kind(PinKind::Input)
	{
        Data.Type = type;
	}

	Pin(PinData data, PinKind kind)
		: Data(data), Kind(kind)
	{
	}
};

struct Node
{
	ed::NodeId ID;
	std::string Name;
    std::string DisplayName = "";

    Ref<Texture2D> Icon = nullptr;
    ImColor IconColor = ImColor(1.0f, 1.0f, 1.0f);

	std::vector<Pin> Inputs;
	std::vector<Pin> Outputs;
	ImColor Color;
	NodeType Type;
    NodeFunction Function;
	ImVec2 Size;
    bool Internal = false;

	std::string State;
	std::string SavedState;

    // Node Comment Data
    bool CommentEnabled = false;
    bool CommentPinned = false;
    std::string Comment = "";
    float Opacity = -2.5f;

    bool AddInputs = false;
    bool AddOutputs = false;

    int FunctionID = -1;
    bool Pure = false;

    unsigned int VariableId = 0;

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

struct SpawnNodeData
{
    SpawnNodeData() = default;
    
    SpawnNodeData(unsigned int variableId)
        : VariableID(variableId)
    {
    }

	unsigned int VariableID = 0;
};

struct Variable
{
    unsigned int ID;
    std::string Name;
    std::string Tooltip;
    PinData Data;

    Variable() = default;

	Variable(int id, std::string name, PinType type)
		: ID(id), Name(name), Data(type)
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
typedef Node* (NodeEditorInternal::* spawnNodeFunction)(SpawnNodeData);

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

enum class FunctionFlags
{
    FUNC_None,
    FUNC_Const,
    FUNC_Static
};

enum class CompileReadType
{
    None,
    DebugMetadata,
};

struct SearchResultData
{
	SearchResultData() = default;

	SearchResultData(std::string name, int id)
		: Name(name), ID(id)
	{
	}

	SearchResultData(std::string name, spawnNodeFunction Function, SpawnNodeData spawnData)
		: Name(name), Function(Function), SpawnData(spawnData)
	{
	}

	std::string Name;
	int ID = -1;
	spawnNodeFunction Function = nullptr;
    SpawnNodeData SpawnData;
};

struct SearchData
{
	std::string Name;
	std::vector<SearchData> LowerTree;
	std::vector<SearchResultData> Results;

	bool open = false;
	bool confirmed = true;

	SearchData& PushBackTree(std::string name)
	{
		int pos = 0;
		for (auto& tree : LowerTree)
			if (name > tree.Name) pos++;
			else break;

		LowerTree.insert(LowerTree.begin() + pos, { name });
		return LowerTree[pos];
	}

    void PushBackResult(const std::string& name, spawnNodeFunction Function, SpawnNodeData spawnData)
	{
		int pos = 0;
		for (auto& result : Results)
			if (name > result.Name) pos++;
			else break;

        Results.insert(Results.begin() + pos, {name, Function, spawnData});
	}

	void PushBackResult(const std::string& name, const int& id)
	{
		int pos = 0;
		for (auto& result : Results)
			if (name > result.Name) pos++;
			else break;

        Results.insert(Results.begin() + pos, { name, id });
	}

    void Clear()
    {
        LowerTree = {};
        Results = {};

		bool open = false;
		bool confirmed = true;
    }
};

struct FindResultsData
{
    std::string Name;
    ImColor Color;
    ed::NodeId ID;
    std::vector<FindResultsData> SubData;

    FindResultsData(const std::string& name, const ImColor& color, ed::NodeId id)
        : Name(name), Color(color), ID(id)
    {
    }
};

struct Ubergraph
{
    std::vector<ed::NodeId> Nodes;
    unsigned int ID;
    bool Linear = true;
    Ubergraph(unsigned int id)
        : ID(id)
    {
    }
};

class NodeCompiler;
class NodeLibrary;

class NodeEditorInternal
{
public:
    //Visibility
    bool& GetNodeEditorVisible() { return m_NodeEditorVisible; }

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

	// Development
	Node* SpawnAssertNode(SpawnNodeData in);
	Node* SpawnErrorNode(SpawnNodeData in);

    // Events
	Node* SpawnOnCreateNode(SpawnNodeData in);
	Node* SpawnOnUpdateNode(SpawnNodeData in);
	Node* SpawnOnDestroyNode(SpawnNodeData in);

    // Input -> Keyboard


    // Input -> Mouse

    // Math -> Boolean
    Node* SpawnMakeLiteralBoolNode(SpawnNodeData in);
    Node* SpawnANDBooleanNode(SpawnNodeData in);
    Node* SpawnEqualBooleanNode(SpawnNodeData in);
    Node* SpawnNANDBooleanNode(SpawnNodeData in);
	Node* SpawnNORBooleanNode(SpawnNodeData in);
	Node* SpawnNOTBooleanNode(SpawnNodeData in);
	Node* SpawnNotEqualBooleanNode(SpawnNodeData in);
	Node* SpawnORBooleanNode(SpawnNodeData in);
	Node* SpawnXORBooleanNode(SpawnNodeData in);

    // Math -> Int
    Node* SpawnMakeLiteralIntNode(SpawnNodeData in);

    // Math -> Float
    Node* SpawnMakeLiteralFloatNode(SpawnNodeData in);
	Node* SpawnModuloFloatNode(SpawnNodeData in);
	Node* SpawnAbsoluteFloatNode(SpawnNodeData in);
	Node* SpawnCeilNode(SpawnNodeData in);
	Node* SpawnClampFloatNode(SpawnNodeData in);
	Node* SpawnCompareFloatNode(SpawnNodeData in);
	Node* SpawnDecrementFloatNode(SpawnNodeData in);
	Node* SpawnDivisionWholeAndRemainderNode(SpawnNodeData in);
	Node* SpawnEqualFloatNode(SpawnNodeData in);
	Node* SpawnExpFloatNode(SpawnNodeData in);
	Node* SpawnFInterpEaseInOutNode(SpawnNodeData in);
	Node* SpawnFixedTurnNode(SpawnNodeData in);
	Node* SpawnSubtractionFloatNode(SpawnNodeData in);
	Node* SpawnMultiplicationFloatNode(SpawnNodeData in);
	Node* SpawnDivisionFloatNode(SpawnNodeData in);
	Node* SpawnAdditionFloatNode(SpawnNodeData in);
	Node* SpawnLessThanFloatNode(SpawnNodeData in);
	Node* SpawnLessThanOrEqualFloatNode(SpawnNodeData in);
	Node* SpawnGreaterThanFloatNode(SpawnNodeData in);
	Node* SpawnGreaterThanOrEqualFloatNode(SpawnNodeData in);
	Node* SpawnFloorNode(SpawnNodeData in);
	Node* SpawnFractionNode(SpawnNodeData in);
	Node* SpawnHypotenuseNode(SpawnNodeData in);
	Node* SpawnIncrementFloatNode(SpawnNodeData in);
	Node* SpawnInRangeFloatNode(SpawnNodeData in);
	Node* SpawnIntMultipliedFloatNode(SpawnNodeData in);
	Node* SpawnLerpAngleNode(SpawnNodeData in);
	Node* SpawnLerpFloatNode(SpawnNodeData in);
	Node* SpawnLogFloatNode(SpawnNodeData in);
	Node* SpawnLogeFloatNode(SpawnNodeData in);
	Node* SpawnMakePulsatingValueNode(SpawnNodeData in);
	Node* SpawnMapRangeClampedFloatNode(SpawnNodeData in);
	Node* SpawnMapRangeUnclampedFloatNode(SpawnNodeData in);
	Node* SpawnMaxFloatNode(SpawnNodeData in);
	Node* SpawnMaxOfFloatArrayNode(SpawnNodeData in);
	Node* SpawnMinFloatNode(SpawnNodeData in);
	Node* SpawnMinOfFloatArrayNode(SpawnNodeData in);
	Node* SpawnMultiplyByPiNode(SpawnNodeData in);
	Node* SpawnNearlyEqualFloatNode(SpawnNodeData in);
	Node* SpawnNegateFloatNode(SpawnNodeData in);
	Node* SpawnNormalizeAngleNode(SpawnNodeData in);
	Node* SpawnNormalizeToRangeNode(SpawnNodeData in);
	Node* SpawnNotEqualFloatNode(SpawnNodeData in);
	Node* SpawnPowerFloatNode(SpawnNodeData in);
	Node* SpawnRoundFloatNode(SpawnNodeData in);
	Node* SpawnSafeDivideNode(SpawnNodeData in);
	Node* SpawnSelectFloatNode(SpawnNodeData in);
	Node* SpawnSnapToGridFloatNode(SpawnNodeData in);
	Node* SpawnSignFloatNode(SpawnNodeData in);
	Node* SpawnSquareRootNode(SpawnNodeData in);
	Node* SpawnSquareNode(SpawnNodeData in);
	Node* SpawnTruncateNode(SpawnNodeData in);
	Node* SpawnWrapFloatNode(SpawnNodeData in);

	//Math -> Interpolation
	Node* SpawnInterpEaseInNode(SpawnNodeData in);
	Node* SpawnInterpEaseOutNode(SpawnNodeData in);

    // Math -> Trig
	Node* SpawnAcosDegreesNode(SpawnNodeData in);
	Node* SpawnAcosRadiansNode(SpawnNodeData in);
	Node* SpawnAsinDegreesNode(SpawnNodeData in);
	Node* SpawnAsinRadiansNode(SpawnNodeData in);
	Node* SpawnAtanDegreesNode(SpawnNodeData in);
	Node* SpawnAtanRadiansNode(SpawnNodeData in);
	Node* SpawnAtan2DegreesNode(SpawnNodeData in);
	Node* SpawnAtan2RadiansNode(SpawnNodeData in);
	Node* SpawnCosDegreesNode(SpawnNodeData in);
	Node* SpawnCosRadiansNode(SpawnNodeData in);
	Node* SpawnDegreesToRadiansNode(SpawnNodeData in);
	Node* SpawnGetPiNode(SpawnNodeData in);
	Node* SpawnGetTAUNode(SpawnNodeData in);
	Node* SpawnSinDegreesNode(SpawnNodeData in);
	Node* SpawnSinRadiansNode(SpawnNodeData in);
	Node* SpawnTanDegreesNode(SpawnNodeData in);
	Node* SpawnTanRadiansNode(SpawnNodeData in);

    // Math -> Vector

    // Math -> Conversions
    Node* SpawnIntToBoolNode(SpawnNodeData in);
    Node* SpawnFloatToBoolNode(SpawnNodeData in);
    Node* SpawnStringToBoolNode(SpawnNodeData in);
	Node* SpawnBoolToFloatNode(SpawnNodeData in);
	Node* SpawnIntToFloatNode(SpawnNodeData in);
	Node* SpawnStringToFloatNode(SpawnNodeData in);
	Node* SpawnBoolToIntNode(SpawnNodeData in);
	Node* SpawnFloatToIntNode(SpawnNodeData in);
	Node* SpawnStringToIntNode(SpawnNodeData in);
	Node* SpawnFloatToVectorNode(SpawnNodeData in);
	Node* SpawnStringToVectorNode(SpawnNodeData in);

    // Math -> Random
	Node* SpawnRandomBoolNode(SpawnNodeData in);
	Node* SpawnRandomFloatNode(SpawnNodeData in);

    // Utilities -> String
    Node* SpawnMakeLiteralStringNode(SpawnNodeData in);
    Node* SpawnBoolToStringNode(SpawnNodeData in);
    Node* SpawnIntToStringNode(SpawnNodeData in);
    Node* SpawnFloatToStringNode(SpawnNodeData in);
    Node* SpawnVectorToStringNode(SpawnNodeData in);
    Node* SpawnPrintStringNode(SpawnNodeData in);

    // Utilities -> Flow Control
    Node* SpawnBranchNode(SpawnNodeData in);
	Node* SpawnDoNNode(SpawnNodeData in);
	Node* SpawnDoOnceNode(SpawnNodeData in);
	Node* SpawnForLoopNode(SpawnNodeData in);
	Node* SpawnForLoopWithBreakNode(SpawnNodeData in);
    Node* SpawnSequenceNode(SpawnNodeData in);
    Node* SpawnWhileLoopNode(SpawnNodeData in);

	// Variables
	Node* SpawnGetVariableNode(SpawnNodeData in);
	Node* SpawnSetVariableNode(SpawnNodeData in);

    // No Category
    Node* SpawnComment(SpawnNodeData in);

    void NodeEditorInternal::ConversionAvalible(Pin* A, Pin* B, spawnNodeFunction& Function);
    void NodeEditorInternal::ConversionAvalible(PinData& A, PinData& B, spawnNodeFunction& Function);
    void BuildNodes();
    const char* Application_GetName();
    ImColor GetIconColor(PinType type);
    void DrawPinIcon(const PinData& data, bool connected, int alpha);
    void DrawTypeIcon(ContainerType container, PinType type);

    void ShowStyleEditor(bool* show = nullptr);
    void ShowLeftPane(float paneWidth);

    void OnEvent(Dymatic::Event& e);
    bool OnKeyPressed(Dymatic::KeyPressedEvent& e);

    // Node Compilation
	void CompileNodes();
    void RecursiveNodeWrite(Node& node, NodeCompiler& source, NodeCompiler& header, NodeCompiler& ubergraphBody, std::vector<ed::NodeId>& pureNodeList, std::vector<ed::NodeId>& localNodeList);
    void RecursivePinWrite(Pin& pin, NodeCompiler& source, NodeCompiler& header, NodeCompiler& ubergraphBody, std::string& line, std::vector<ed::NodeId>& pureNodeList, std::vector<ed::NodeId>& localNodeList);
    bool IsPinGlobal(Pin& pin, int ubergraphID);
    void RecursiveUbergraphOrganisation(Node& node, std::vector<Ubergraph>& ubergraphs, unsigned int& currentId);
    Node& GetNextExecNode(Pin& node);

    static std::string ConvertNodeFunctionToString(const NodeFunction& nodeFunction);
    static NodeFunction ConvertStringToNodeFunction(const std::string& string);
    static std::string ConvertNodeTypeToString(const NodeType& nodeType);
    static NodeType ConvertStringToNodeType(const std::string& string);
    static std::string ConvertPinTypeToString(const PinType& pinType);
	static PinType ConvertStringToPinType(const std::string& string);
	static std::string ConvertPinKindToString(const PinKind& pinKind);
	static PinKind ConvertStringToPinKind(const std::string& string);
	static std::string ConvertPinValueToString(const PinType& type, const TypeValue& value);
	static TypeValue ConvertStringToPinValue(const PinType& type, const std::string& string);
	static std::string ConvertContainerTypeToString(const ContainerType& type);
	static ContainerType ConvertStringToContainerType(const std::string& string);

    void OpenNodes();
    void OpenNodes(const std::filesystem::path& path);
    void SaveNodes();
    void SaveNodes(const std::filesystem::path& path);

    void CopyNodes();
    void PasteNodes();
    void DuplicateNodes();
    void DeleteNodes();

    void DefaultValueInput(PinData& data, bool spring = false);

    void OpenSearchList(SearchData* searchData);
    void CloseSearchList(SearchData* searchData);
    SearchResultData* DisplaySearchData(SearchData& searchData, bool origin = true);

    void CheckLinkSafety(Pin* startPin, Pin* endPin);

    void UpdateSearchData();
    void AddSearchData(std::string name, std::string category, std::vector<std::string> keywords, std::vector<Pin> params, PinData returnVal, bool pure, spawnNodeFunction Function = nullptr, SpawnNodeData spawnData = {}, int id = -1);
    void AddSearchDataReference(std::string& name, std::string& category, std::vector<std::string>& keywords, std::vector<Pin>& params, PinData& returnVal, bool& pure, int& id, spawnNodeFunction Function = nullptr, SpawnNodeData spawnData = {});

    void Application_Initialize();
    void Application_Finalize();
    void OnImGuiRender();

    Variable* GetVariableById(unsigned int id);
	void SetVariableType(Variable* variable, const PinType type);
	void SetVariableName(Variable* variable, const std::string name);

    inline void ResetSearchArea() { m_ResetSearchArea = true; m_SearchBuffer = ""; m_PreviousSearchBuffer = ""; m_NewNodeLinkPin = nullptr; }

    void FindResultsSearch(const std::string& search);

    void LoadNodeLibrary(const std::filesystem::path& path);
    inline static bool IsCharacterNewWord(char& character) { return !isalnum(character); }
private:
    bool m_NodeEditorVisible = false;

    ImColor m_CurrentLinkColor = ImColor(255, 255, 255);

	const int            m_PinIconSize = 24;
	std::vector<Node>    m_Nodes;
	std::vector<Link>    m_Links;
    std::vector<Variable> m_Variables;

    bool m_HideUnconnected = false;

    Dymatic::Ref<Dymatic::Texture2D> m_HeaderBackground = nullptr;
    ImTextureID          m_SampleImage = nullptr;
	Dymatic::Ref<Dymatic::Texture2D> m_SaveIcon = nullptr;
	Dymatic::Ref<Dymatic::Texture2D> m_RestoreIcon = nullptr;
	Dymatic::Ref<Dymatic::Texture2D> m_CommentIcon = nullptr;
	Dymatic::Ref<Dymatic::Texture2D> m_PinIcon = nullptr;
	Dymatic::Ref<Dymatic::Texture2D> m_PinnedIcon = nullptr;

    // Node Icons
    Dymatic::Ref<Dymatic::Texture2D> m_EventIcon = nullptr;
    Dymatic::Ref<Dymatic::Texture2D> m_FunctionIcon = nullptr;
    Dymatic::Ref<Dymatic::Texture2D> m_SequenceIcon = nullptr;
    Dymatic::Ref<Dymatic::Texture2D> m_BranchIcon = nullptr;
    Dymatic::Ref<Dymatic::Texture2D> m_SwitchIcon = nullptr;

	const float          m_TouchTime = 1.0f;
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

	bool m_OpenVarPopup = false;
	bool m_ResetBackNode = false;

    bool m_ResetSearchArea = false;

    unsigned int m_SelectedVariable = 0;

    std::vector<NodeLibrary> m_NodeLibraries;
};

static std::vector<NodeEditorInternal> EditorInternalStack;

class NodeCompiler
{
public:
    NodeCompiler() = default;
    std::string GenerateFunctionSignature(Node& node, const std::string& Namespace = "");
    std::string GenerateFunctionName(Node& node);

    void NewLine();
    void Write(std::string text);
    void WriteLine(std::string line);

    static std::string ConvertPinValueToCompiledString(const PinType& type, TypeValue& value);
    static std::string ConvertPinTypeToCompiledString(const PinType& pinType, bool beginCaps = false);
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
    String::ReplaceAll(String, "_", " ");
    for (int i = 0; i < String.length() - 1; i++)
        if (islower(String[i]) && isupper(String[i + 1]))
        {
            String.insert(String.begin() + (i + 1), ' ');
            i++;
        }
    return String;
}

class FunctionDeclaration
{
public:
    FunctionDeclaration(int id)
        : ID(id)
    {
    }

    inline std::string GetName() { return ConvertToDisplayString(FUNC_DisplayName.empty() ? FUNC_Name : FUNC_DisplayName); }
public:

    int ID = -1;

    bool FUNC_Static = false;
    bool FUNC_Const = false;
    Pin FUNC_Return;

    std::string FUNC_Name = "";
    std::string FUNC_DisplayName = "";
    std::string FUNC_NodeDisplayName = "";

    std::vector<Pin> FUNC_Params;

    std::string FUNC_Tooltip;

    std::string FUNC_Category = "";
    std::vector<std::string> FUNC_Keywords;

    bool FUNC_Pure = false;
    bool FUNC_CompactNode = false;
    bool FUNC_NoPinLabels = false;

    bool FUNC_Internal = false;
};

class NodeLibrary
{
public:
    NodeLibrary(int id, std::string name)
        : ID(id), m_Name(name)
    {
    }
public:
    int ID = -1;

    std::string m_Name = "Node Library";
    std::vector<FunctionDeclaration> m_FunctionDeclarations;
};

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
    spawnNodeFunction Function;
    ConversionAvalible(a, b, Function);
    if (!a || !b || a == b || a->Kind == b->Kind || ((Function != nullptr) ? false : (a->Data.Type != b->Data.Type)) || a->Node == b->Node || a->Data.Container != b->Data.Container)
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

// Development
Node* NodeEditorInternal::SpawnAssertNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Assert", ImColor(128, 195, 248), NodeFunction::Node);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Condition", PinType::Bool);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Message", PinType::String);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnErrorNode(SpawnNodeData in)
{
    m_Nodes.emplace_back(GetNextId(), "Error", ImColor(128, 195, 248), NodeFunction::Node);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Message", PinType::String);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

// Events
Node* NodeEditorInternal::SpawnOnCreateNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "On Create", ImColor(255, 128, 128), NodeFunction::Event);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);
    m_Nodes.back().Icon = m_EventIcon;

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnOnUpdateNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "On Update", ImColor(255, 128, 128), NodeFunction::Event);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Delta Seconds", PinType::Float);
    m_Nodes.back().Icon = m_EventIcon;

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnOnDestroyNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "On Destroy", ImColor(255, 128, 128), NodeFunction::Event);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);
	m_Nodes.back().Icon = m_EventIcon;

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

// Variables
Node* NodeEditorInternal::SpawnGetVariableNode(SpawnNodeData in)
{
	for (auto& variable : m_Variables)
		if (variable.ID == in.VariableID)
		{
			m_Nodes.emplace_back(GetNextId(), "Get " + variable.Name, ImColor(170, 242, 172), NodeFunction::Variable);
			m_Nodes.back().DisplayName = "Get " + variable.Name;
			m_Nodes.back().Type = NodeType::Simple;
			m_Nodes.back().Outputs.emplace_back(GetNextId(), "", variable.Data.Type);
			m_Nodes.back().VariableId = variable.ID;
            
			BuildNode(&m_Nodes.back());
            
			return &m_Nodes.back();
		}
    return nullptr;
}

Node* NodeEditorInternal::SpawnSetVariableNode(SpawnNodeData in)
{
	for (auto& variable : m_Variables)
		if (variable.ID == in.VariableID)
		{
			m_Nodes.emplace_back(GetNextId(), "Set " + variable.Name, ImColor(128, 195, 248), NodeFunction::Variable);
			m_Nodes.back().DisplayName = "Set " + variable.Name;
			m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
			m_Nodes.back().Inputs.emplace_back(GetNextId(), "", variable.Data.Type);
			m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);
			m_Nodes.back().Outputs.emplace_back(GetNextId(), "", variable.Data.Type);
			m_Nodes.back().VariableId = variable.ID;

			BuildNode(&m_Nodes.back());

			return &m_Nodes.back();
		}
	return nullptr;
}

// Math -> Boolean
Node* NodeEditorInternal::SpawnMakeLiteralBoolNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Make Literal Bool", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Value", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnANDBooleanNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "AND Boolean", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().DisplayName = "AND";
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Bool);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnEqualBooleanNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Equal Boolean", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().DisplayName = "==";
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Bool);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnNANDBooleanNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "NAND Boolean", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().DisplayName = "NAND";
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Bool);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnNORBooleanNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "NOR Boolean", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().DisplayName = "NOR";
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Bool);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnNOTBooleanNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "NOT Boolean", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().DisplayName = "NOT";
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnNotEqualBooleanNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Not Equal Boolean", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().DisplayName = "!=";
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Bool);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnORBooleanNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "OR Boolean", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().DisplayName = "OR";
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Bool);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnXORBooleanNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "XOR Boolean", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().DisplayName = "XOR";
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Bool);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

// Math -> Int
Node* NodeEditorInternal::SpawnMakeLiteralIntNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Make Literal Int", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Value", PinType::Int);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::Int);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

// Math -> Float
Node* NodeEditorInternal::SpawnMakeLiteralFloatNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Make Literal Float", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Value", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnModuloFloatNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Modulo Float", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().DisplayName = "%";
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnAbsoluteFloatNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Absolute Float", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().DisplayName = "ABS";
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnCeilNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Ceil", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::Int);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnClampFloatNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Clamp Float", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Value", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Min", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Max", PinType::Float).Data.Value.Float = 1.0f;
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnCompareFloatNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Compare Float", ImColor(255, 255, 255), NodeFunction::Node);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Exec", PinType::Flow);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Input", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Compare With", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), ">", PinType::Flow);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "==", PinType::Flow);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "<", PinType::Flow);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* SpawnDecrementFloatNode(SpawnNodeData in);
Node* SpawnDivisionWholeAndRemainderNode(SpawnNodeData in);

Node* NodeEditorInternal::SpawnEqualFloatNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Equal Float", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().DisplayName = "==";
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnExpFloatNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Exp Float", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().DisplayName = "e";
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnFInterpEaseInOutNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "FInterp Ease in Out", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Alpha", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Exponent", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnSubtractionFloatNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Float Subtraction", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().DisplayName = "-";
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnMultiplicationFloatNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Float Multiplication", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().DisplayName = "*";
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);
    m_Nodes.back().AddInputs = true;

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnDivisionFloatNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Float Division", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().DisplayName = "/";
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnAdditionFloatNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Float Addition", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().DisplayName = "+";
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);
    m_Nodes.back().AddInputs = true;

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* SpawnLessThanFloatNode(SpawnNodeData in);
Node* SpawnLessThanOrEqualFloatNode(SpawnNodeData in);
Node* SpawnGreaterThanFloatNode(SpawnNodeData in);
Node* SpawnGreaterThanOrEqualFloatNode(SpawnNodeData in);
Node* SpawnFloorNode(SpawnNodeData in);
Node* SpawnFractionNode(SpawnNodeData in);
Node* SpawnHypotenuseNode(SpawnNodeData in);
Node* SpawnIncrementFloatNode(SpawnNodeData in);
Node* SpawnInRangeFloatNode(SpawnNodeData in);
Node* SpawnIntMultipliedFloatNode(SpawnNodeData in);

Node* NodeEditorInternal::SpawnLerpAngleNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Lerp Angle", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Alpha", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Min Angle Degrees", PinType::Float).Data.Value.Float = -180.0f;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Max Angle Degrees", PinType::Float).Data.Value.Float = 180.0f;
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* SpawnLerpFloatNode(SpawnNodeData in);
Node* SpawnLogFloatNode(SpawnNodeData in);
Node* SpawnLogeFloatNode(SpawnNodeData in);
Node* SpawnMakePulsatingValueNode(SpawnNodeData in);
Node* SpawnMapRangeClampedFloatNode(SpawnNodeData in);
Node* SpawnMapRangeUnclampedFloatNode(SpawnNodeData in);
Node* SpawnMaxFloatNode(SpawnNodeData in);
Node* SpawnMaxOfFloatArrayNode(SpawnNodeData in);
Node* SpawnMinFloatNode(SpawnNodeData in);
Node* SpawnMinOfFloatArrayNode(SpawnNodeData in);
Node* SpawnMultiplyByPiNode(SpawnNodeData in);
Node* SpawnNearlyEqualFloatNode(SpawnNodeData in);
Node* SpawnNegateFloatNode(SpawnNodeData in);

Node* NodeEditorInternal::SpawnNormalizeAngleNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Normalize Angle", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Angle Degrees", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Min Angle Degrees", PinType::Float).Data.Value.Float = -180.0f;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Max Angle Degrees", PinType::Float).Data.Value.Float = 180.0f;
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* SpawnNormalizeToRangeNode(SpawnNodeData in);
Node* SpawnNotEqualFloatNode(SpawnNodeData in);
Node* SpawnPowerFloatNode(SpawnNodeData in);
Node* SpawnRoundFloatNode(SpawnNodeData in);
Node* SpawnSafeDivideNode(SpawnNodeData in);
Node* SpawnSelectFloatNode(SpawnNodeData in);
Node* SpawnSnapToGridFloatNode(SpawnNodeData in);
Node* SpawnSignFloatNode(SpawnNodeData in);
Node* SpawnSquareRootNode(SpawnNodeData in);
Node* SpawnNode(SpawnNodeData in);
Node* SpawnSquareNode(SpawnNodeData in);
Node* SpawnTruncateNode(SpawnNodeData in);
Node* SpawnWrapFloatNode(SpawnNodeData in);

//Math -> Interpolation
Node* NodeEditorInternal::SpawnInterpEaseInNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Interp Ease In", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Alpha", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Exponent", PinType::Float).Data.Value.Float = 2.0f;
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Result", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}
Node* NodeEditorInternal::SpawnInterpEaseOutNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Interp Ease Out", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Alpha", PinType::Float);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Exponent", PinType::Float).Data.Value.Float = 2.0f;
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Result", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

// Math -> Trig
Node* SpawnAcosDegreesNode(SpawnNodeData in);
Node* SpawnAcosRadiansNode(SpawnNodeData in);
Node* SpawnAsinDegreesNode(SpawnNodeData in);
Node* SpawnAsinRadiansNode(SpawnNodeData in);
Node* SpawnAtanDegreesNode(SpawnNodeData in);
Node* SpawnAtanRadiansNode(SpawnNodeData in);
Node* SpawnAtan2DegreesNode(SpawnNodeData in);
Node* SpawnAtan2RadiansNode(SpawnNodeData in);
Node* SpawnCosDegreesNode(SpawnNodeData in);
Node* SpawnCosRadiansNode(SpawnNodeData in);
Node* SpawnDegreesToRadiansNode(SpawnNodeData in);
Node* SpawnGetPiNode(SpawnNodeData in);
Node* SpawnGetTAUNode(SpawnNodeData in);
Node* SpawnSinDegreesNode(SpawnNodeData in);
Node* SpawnSinRadiansNode(SpawnNodeData in);
Node* SpawnTanDegreesNode(SpawnNodeData in);
Node* SpawnTanRadiansNode(SpawnNodeData in);

// Math -> Conversions
Node* NodeEditorInternal::SpawnIntToBoolNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Int To Bool", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Int);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnFloatToBoolNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Float To Bool", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnStringToBoolNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "String To Bool", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::String);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Bool);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnBoolToFloatNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Bool To Float", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnIntToFloatNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Int To Float", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Int);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnStringToFloatNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "String To Float", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::String);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnBoolToIntNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Bool To Int", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Int);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnFloatToIntNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Float To Int", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Int);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnStringToIntNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "String To Int", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::String);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Int);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnFloatToVectorNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Float To Vector", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Vector);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnStringToVectorNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "String To Vector", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::String);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Vector);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

// Utilities -> String
Node* NodeEditorInternal::SpawnMakeLiteralStringNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Make Literal String", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Value", PinType::String);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::String);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnBoolToStringNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Bool To String", ImColor(170, 242, 172), NodeFunction::Node);
    m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Bool);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::String);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnIntToStringNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Int To String", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Int);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::String);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnFloatToStringNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Float To String", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::String);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnVectorToStringNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Vector To String", ImColor(170, 242, 172), NodeFunction::Node);
	m_Nodes.back().Type = NodeType::Simple;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Vector);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::String);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnPrintStringNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Print String", ImColor(128, 195, 248), NodeFunction::Node);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
    m_Nodes.back().Inputs.back().Data.Container = ContainerType::Array;
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "In String", PinType::String);
    m_Nodes.back().Inputs.back().Data.Container = ContainerType::Set;
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

// Utilities -> Flow Control
Node* NodeEditorInternal::SpawnBranchNode(SpawnNodeData in)
{
    m_Nodes.emplace_back(GetNextId(), "Branch");
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "Condition", PinType::Bool);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "True", PinType::Flow);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "False", PinType::Flow);
    m_Nodes.back().Icon = m_BranchIcon;
    m_Nodes.back().Internal = true;

    BuildNode(&m_Nodes.back());

    return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnDoNNode(SpawnNodeData in)
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

Node* NodeEditorInternal::SpawnDoOnceNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Do Once");
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Reset", PinType::Flow);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Start Closed", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Completed", PinType::Flow);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnForLoopNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "For Loop");
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "First Index", PinType::Int);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Last Index", PinType::Int);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Loop Body", PinType::Flow);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Index", PinType::Int);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Completed", PinType::Flow);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnForLoopWithBreakNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "For Loop with Break");
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "First Index", PinType::Int);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Last Index", PinType::Int);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Break", PinType::Flow);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Loop Body", PinType::Flow);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Index", PinType::Int);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Completed", PinType::Flow);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnSequenceNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "Sequence");
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "0", PinType::Flow);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "1", PinType::Flow);
    m_Nodes.back().AddOutputs = true;
    m_Nodes.back().Icon = m_SequenceIcon;
    m_Nodes.back().Internal = true;

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

Node* NodeEditorInternal::SpawnWhileLoopNode(SpawnNodeData in)
{
	m_Nodes.emplace_back(GetNextId(), "While Loop");
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	m_Nodes.back().Inputs.emplace_back(GetNextId(), "Condition", PinType::Bool);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Loop Body", PinType::Flow);
	m_Nodes.back().Outputs.emplace_back(GetNextId(), "Completed", PinType::Flow);

	BuildNode(&m_Nodes.back());

	return &m_Nodes.back();
}

// No Category
Node* NodeEditorInternal::SpawnComment(SpawnNodeData in)
{
    m_Nodes.emplace_back(GetNextId(), "Comment");
    m_Nodes.back().Type = NodeType::Comment;

    if (ed::GetSelectedObjectCount() > 0)
    {
        float padding = 20.0f;
        ImVec2 min, max;
        ed::GetSelectionBounds(min, max);

        ed::SetNodePosition(m_Nodes.back().ID, ImVec2(min.x - padding, min.y - padding * 2.0f));
        m_Nodes.back().Size = ImVec2(max.x - min.x + padding, max.y - min.y + padding);
    }
    else
        m_Nodes.back().Size = ImVec2(300, 100);


    return &m_Nodes.back();
}

void NodeEditorInternal::ConversionAvalible(Pin* A, Pin* B, spawnNodeFunction& Function)
{
	if (A->Kind == PinKind::Input)
	{
		std::swap(A, B);
	}
    ConversionAvalible(A->Data, B->Data, Function);
}

void NodeEditorInternal::ConversionAvalible(PinData& A, PinData& B, spawnNodeFunction& Function)
{
	Function = nullptr;

	auto a = A.Type;
	auto b = B.Type;

	if (A.Container == ContainerType::Single && B.Container == ContainerType::Single)
	{
		if (a == PinType::Int && b == PinType::Bool) { Function = &NodeEditorInternal::SpawnIntToBoolNode; }
		else if (a == PinType::Float && b == PinType::Bool) { Function = &NodeEditorInternal::SpawnFloatToBoolNode; }
		else if (a == PinType::String && b == PinType::Bool) { Function = &NodeEditorInternal::SpawnStringToBoolNode; }
		else if (a == PinType::Bool && b == PinType::Int) { Function = &NodeEditorInternal::SpawnBoolToIntNode; }
		else if (a == PinType::Float && b == PinType::Int) { Function = &NodeEditorInternal::SpawnFloatToIntNode; }
		else if (a == PinType::String && b == PinType::Int) { Function = &NodeEditorInternal::SpawnStringToIntNode; }
		else if (a == PinType::Bool && b == PinType::Float) { Function = &NodeEditorInternal::SpawnBoolToFloatNode; }
		else if (a == PinType::Int && b == PinType::Float) { Function = &NodeEditorInternal::SpawnIntToFloatNode; }
		else if (a == PinType::String && b == PinType::Float) { Function = &NodeEditorInternal::SpawnStringToFloatNode; }
		else if (a == PinType::Bool && b == PinType::String) { Function = &NodeEditorInternal::SpawnBoolToStringNode; }
		else if (a == PinType::Int && b == PinType::String) { Function = &NodeEditorInternal::SpawnIntToStringNode; }
		else if (a == PinType::Float && b == PinType::String) { Function = &NodeEditorInternal::SpawnFloatToStringNode; }
		else if (a == PinType::Vector && b == PinType::String) { Function = &NodeEditorInternal::SpawnVectorToStringNode; }
		else if (a == PinType::Float && b == PinType::Vector) { Function = &NodeEditorInternal::SpawnFloatToVectorNode; }
		else if (a == PinType::String && b == PinType::Vector) { Function = &NodeEditorInternal::SpawnStringToVectorNode; }
	}
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
        case PinType::String:   return ImColor(218,   0, 183);
        case PinType::Vector:   return ImColor(242, 192,  33);
        case PinType::Color:    return ImColor(0  , 87,  200);
        case PinType::Object:   return ImColor( 51, 150, 215);
        case PinType::Function: return ImColor(218,   0, 183);
        case PinType::Delegate: return ImColor(255,  48,  48);
    }
};

void NodeEditorInternal::DrawPinIcon(const PinData& data, bool connected, int alpha)
{
    IconType iconType;
    ImColor  color = GetIconColor(data.Type);
    color.Value.w = alpha / 255.0f;
    switch (data.Type)
    {
        case PinType::Flow:     iconType = IconType::Flow;   break;
        case PinType::Bool:     iconType = IconType::Circle; break;
        case PinType::Int:      iconType = IconType::Circle; break;
        case PinType::Float:    iconType = IconType::Circle; break;
        case PinType::String:   iconType = IconType::Circle; break;
        case PinType::Vector:   iconType = IconType::Circle; break;
        case PinType::Color:    iconType = IconType::Circle; break;
        case PinType::Object:   iconType = IconType::Circle; break;
        case PinType::Function: iconType = IconType::Circle; break;
        case PinType::Delegate: iconType = IconType::Square; break;
        default:
            return;
    }

    if (data.Reference && data.Const)
        iconType = IconType::Diamond;

    switch (data.Container)
    {
    case ContainerType::Single:                                   break;
    case ContainerType::Array:  iconType = IconType::Grid;        break;
    case ContainerType::Set:    iconType = IconType::Braces;      break;
    case ContainerType::Map:    iconType = IconType::RoundSquare; break;
    }

    ax::Widgets::Icon(ImVec2(m_PinIconSize, m_PinIconSize), iconType, connected, color, ImColor(32, 32, 32, alpha));
};

void NodeEditorInternal::DrawTypeIcon(ContainerType container, PinType type)
{
	IconType iconType;
	ImColor  color = GetIconColor(type);
    switch (container)
    {
	case ContainerType::Single: iconType = IconType::Capsule;     break;
	case ContainerType::Array:  iconType = IconType::Grid;     break;
	case ContainerType::Set:    iconType = IconType::Braces;     break;
	case ContainerType::Map:    iconType = IconType::RoundSquare; break;
    }

    auto& cursorPos = ImGui::GetCursorScreenPos();
    ax::Drawing::DrawIcon(ImGui::GetWindowDrawList(), cursorPos, cursorPos + ImVec2(m_PinIconSize, m_PinIconSize), iconType, true, color, ImColor(32, 32, 32));
}

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
    ImGui::TextUnformatted("NODES");
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

std::string NodeCompiler::ConvertPinValueToCompiledString(const PinType& type, TypeValue& value)
{
	switch (type)
	{
	case PinType::Bool: {return value.Bool ? "true" : "false"; }
	case PinType::Int: {return std::to_string(value.Int); }
	case PinType::Float: {return (std::to_string(value.Float) + "f"); }
	case PinType::String: { Dymatic::String::ReplaceAll(value.String, "\\", "\\\\"); Dymatic::String::ReplaceAll(value.String, "\"", "\\\""); return "\"" + value.String + "\""; }
	case PinType::Vector: {return "{ " + std::to_string(value.Vector.x) + ", " + std::to_string(value.Vector.y) + ", " + std::to_string(value.Vector.z) + " }"; }
	case PinType::Color: {return "{ " + std::to_string(value.Color.x) + ", " + std::to_string(value.Color.y) + ", " + std::to_string(value.Color.z) + ", " + std::to_string(value.Color.w) + " }"; }
	}
	DY_CORE_ASSERT("Node value conversion to string not found");
}

std::string NodeCompiler::ConvertPinTypeToCompiledString(const PinType& pinType, bool beginCaps)
{
	switch (pinType)
	{
	case PinType::Flow:     return beginCaps ? "Flow" : "flow";
	case PinType::Bool:     return beginCaps ? "Bool" : "bool";
	case PinType::Int:      return beginCaps ? "Int" : "int";
	case PinType::Float:    return beginCaps ? "Float" : "float";
	case PinType::String:   return beginCaps ? "String" : "std::string";
	case PinType::Vector:   return beginCaps ? "Vector" : "glm::vec3";
	case PinType::Color:    return beginCaps ? "Color" : "glm::vec4";
	}
	DY_ASSERT("Pin type conversion to string not found");
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

void NodeEditorInternal::CompileNodes()
{
    BuildNodes();

    int compileTimeNextId = m_NextId + 1;

	std::string NodeScriptName = "UnrealBlueprintClass";
    NodeCompiler header;
    NodeCompiler source;

	for (auto& link : m_Links)
		ed::Flow(link.ID);

	// Clear Previous Compilation
	m_CompilerResults.clear();
    for (auto& node : m_Nodes)
    {
		node.Error = false;
        node.UbergraphID = -1;
    }

	// Set Window Focus
	ImGui::SetWindowFocus("Compiler Results");

	m_CompilerResults.push_back({ CompilerResultType::Info, "Build started [C++] - Dymatic Nodes V1.2.2 (" + NodeScriptName + ")" });

	// Pre Compile Checks
	m_CompilerResults.push_back({ CompilerResultType::Info, "Initializing Pre Compile Link Checks" });
	// Invalid Pin Links
	for (auto& link : m_Links)
	{
		auto startPin = FindPin(link.StartPinID);
		auto endPin = FindPin(link.EndPinID);
		if (startPin->Data.Type != endPin->Data.Type)
		{
			m_CompilerResults.push_back({ CompilerResultType::Error, "Can't connect pins : Data type is not the same.", startPin->Node->ID });
			startPin->Node->Error = true;
			endPin->Node->Error = true;
		}
		if (startPin->Data.Container != endPin->Data.Container)
		{
            m_CompilerResults.push_back({ CompilerResultType::Error, "Can't connect pins : Container type is not the same.", startPin->Node->ID });
			startPin->Node->Error = true;
			endPin->Node->Error = true;
		}
	}

    // Pre Compile Node Checks
    m_CompilerResults.push_back({ CompilerResultType::Info, "Initializing Pre Compile Node Checks" });
    for (auto& node : m_Nodes)
        for (auto& pin : node.Inputs)
            if (pin.Data.Reference)
                if (!IsPinLinked(pin.ID))
                {
                    m_CompilerResults.push_back({ CompilerResultType::Error, "The pin '" + pin.Name + "' in node '" + node.Name + "' is a reference and expects a linked input.", node.ID });
                    node.Error = true;
                }

	header.WriteLine("#pragma once");
	header.WriteLine("//Dymatic C++ Node Script - Header - V1.2.2");
    header.NewLine();

	source.WriteLine("//Dymatic C++ Node Script - Source - V1.2.2");
	source.WriteLine("#include \"" + NodeScriptName + ".h\"");
	source.WriteLine("#include \"CustomNodeLibrary.h\"");
    source.NewLine();

    // Open Dymatic Namespace Source File
	source.WriteLine("namespace Dymatic {");
	source.Indent();
	source.NewLine();

	header.WriteLine("#include <Dymatic/Scene/ScriptableEntity.h>");
	header.WriteLine("#include \"NodeCore.h\"");
    header.NewLine();

    // Open Dymatic Namespace Header File
    header.WriteLine("namespace Dymatic {");
    header.Indent();
    header.NewLine();

    header.WriteLine("class " + NodeScriptName + " : public ScriptableEntity");
    header.OpenScope();

    header.Unindent();
    header.WriteLine("public:");
    header.Indent();

    // Write All Variables
    for (auto& variable : m_Variables)
        header.WriteLine(header.ConvertPinTypeToCompiledString(variable.Data.Type) + " " + header.UnderscoreSpaces("bpv__" + variable.Name + "_" + std::to_string(variable.ID) + "__pf") + ";");

    header.WriteLine(NodeScriptName + "();");
    source.WriteLine(NodeScriptName + "::" + NodeScriptName + "()");
    source.OpenScope();

    for (auto& variable : m_Variables)
        source.WriteLine(source.UnderscoreSpaces(("bpv__" + variable.Name + "_" + std::to_string(variable.ID) + "__pf")) + " = " + source.ConvertPinValueToCompiledString(variable.Data.Type, variable.Data.Value) + ";");

    source.CloseScope();
    source.NewLine();

    header.WriteLine("DYFUNCTION(BlueprintCallable)");
    header.WriteLine("virtual void OnCreate() override;");
    source.WriteLine("void " + NodeScriptName + "::OnCreate()");
    source.OpenScope();
	for (auto& node : m_Nodes)
		if (node.Type != NodeType::Comment)
			if (node.Name == "On Create")
                source.WriteLine(source.GenerateFunctionName(node) + "();");
    source.CloseScope();
    source.NewLine();

	header.WriteLine("DYFUNCTION(BlueprintCallable)");
	header.WriteLine("virtual void OnDestroy() override;");
	source.WriteLine("void " + NodeScriptName + "::OnDestroy()");
	source.OpenScope();
	for (auto& node : m_Nodes)
		if (node.Type != NodeType::Comment)
			if (node.Name == "On Destroy")
				source.WriteLine(source.GenerateFunctionName(node) + "();");
	source.CloseScope();
	source.NewLine();

    header.WriteLine("DYFUNCTION(BlueprintCallable)");
    header.WriteLine("virtual void OnUpdate(Timestep ts) override;");
    source.WriteLine("void " + NodeScriptName + "::OnUpdate(Timestep ts)");
    source.OpenScope();
	for (auto& node : m_Nodes)
		if (node.Type != NodeType::Comment)
			if (node.Name == "On Update")
                source.WriteLine(source.GenerateFunctionName(node) + "(ts.GetSeconds());");
    source.CloseScope();
    source.NewLine();

    // Generate Ubergraphs
    std::vector<Ubergraph> ubergraphs;

    for (auto& node : m_Nodes)
        if (node.Function == NodeFunction::Event)
        {
            unsigned int id = compileTimeNextId++;
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
        for (auto& node : m_Nodes)
        {
			node.Written = false;
			for (auto& input : node.Inputs)
				input.Written = false;
			for (auto& output : node.Outputs)
				output.Written = false;
        }

        header.WriteLine("void bpf__ExecuteUbergraph_" + source.UnderscoreSpaces(NodeScriptName) + "__pf_" + std::to_string(ubergraph.ID) + "(int32_t bpp__EntryPoint__pf);");


        source.WriteLine("void " + source.UnderscoreSpaces(NodeScriptName) + "::bpf__ExecuteUbergraph_" + source.UnderscoreSpaces(NodeScriptName) + "__pf_" + std::to_string(ubergraph.ID) + "(int32_t bpp__EntryPoint__pf)");
        source.OpenScope();

        NodeCompiler ubergraphBody;
        ubergraphBody.GetIndentation() = source.GetIndentation();
        ubergraphBody.NewLine();

        if (ubergraph.Nodes.size() == 1)
        {
            ubergraphBody.WriteLine("check(bpp__EntryPoint__pf == " + std::to_string(ubergraph.Nodes[0].Get()) + ");");
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
                ubergraphBody.WriteLine("StateStack __StateStack;");

            if (nonLinearUbergraph)
            {
                ubergraphBody.WriteLine("int32_t __CurrentState = bpp__EntryPoint__pf;");
                ubergraphBody.WriteLine("do");
                ubergraphBody.OpenScope();
                ubergraphBody.WriteLine("switch( __CurrentState )");
                ubergraphBody.OpenScope();
            }
            else
                ubergraphBody.WriteLine("check(bpp__EntryPoint__pf == " + std::to_string(ubergraph.Nodes[0].Get()) + ");");

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
                        RecursivePinWrite(node.Inputs[1], source, header, ubergraphBody, line, pureList, localList);

                        ubergraphBody.WriteLine("if (!" + line + ")");
                        ubergraphBody.OpenScope();

                        if (IsPinLinked(node.Outputs[1].ID))
                        {
                            ubergraphBody.WriteLine("__CurrentState = " + std::to_string(GetNextExecNode(node.Outputs[1]).ID.Get()) + ";");
                        }
                        else
                            ubergraphBody.WriteLine(Sequences ? "__CurrentState = (__StateStack.Num() > 0) ? __StateStack.Pop() : -1;" : (nonLinearUbergraph ? "__CurrentState = -1;" : "return; // Termination end of function"));
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
                            }
                        }
                        else
                        {
                            ubergraphBody.WriteLine(Sequences ? "__CurrentState = (__StateStack.Num() > 0) ? __StateStack.Pop() : -1;" : (nonLinearUbergraph ? "__CurrentState = -1;" : "return; // Termination end of function"));
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
                            ubergraphBody.WriteLine("__CurrentState = (__StateStack.Num() > 0) ? __StateStack.Pop() : -1;");
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
                else if (node.VariableId != 0)
                {
                    if (nonLinearUbergraph)
                    {
                        ubergraphBody.WriteLine("case " + std::to_string(node.ID.Get()) + ":");
                        ubergraphBody.OpenScope();
                    }

                    bool found = false;
                    for (auto& variable : m_Variables)
                        if (variable.ID == node.VariableId)
                        {
                            found = true;

                            std::string line;
                            std::vector<ed::NodeId> pureList;
                            std::vector<ed::NodeId> localList;
                            RecursivePinWrite(node.Inputs[1], source, header, ubergraphBody, line, pureList, localList);

                            ubergraphBody.WriteLine(ubergraphBody.UnderscoreSpaces("bpv__" + variable.Name + "_" + std::to_string(variable.ID) + "__pf") + " = " + line + ";");
                        }
                    if (!found)
                        m_CompilerResults.push_back({ CompilerResultType::Error, "Couldn't find associated variable for node '" + node.Name + "'", node.ID });

                    if (nonLinearUbergraph)
                        ubergraphBody.CloseScope();
                }
                // Event Specifics
                else if (node.Function == NodeFunction::Event)
                {
                    if (nonLinearUbergraph)
                    {
                        if (IsPinLinked(node.Outputs[0].ID))
                        {
                            ubergraphBody.WriteLine("case " + std::to_string(node.ID.Get()) + ":");
                            ubergraphBody.OpenScope();


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
                                    ubergraphBody.WriteLine("__CurrentState = " + std::to_string(nextNode.ID.Get()) + ";");
                                    ubergraphBody.WriteLine("break;");
                                }
                            }
                            else
                            {
                                ubergraphBody.WriteLine("__CurrentState = -1;");
                                ubergraphBody.WriteLine("break;");
                            }

                            if (nonLinearUbergraph)
                                ubergraphBody.CloseScope();
                        }
                    }
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
                    RecursiveNodeWrite(node, source, header, ubergraphBody, pureList, localList);

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
                        }
                    }
                    else
                    {
                        // Terminate if end of line
                        ubergraphBody.WriteLine(Sequences ? "__CurrentState = (__StateStack.Num() > 0) ? __StateStack.Pop() : -1;" : (nonLinearUbergraph ? "__CurrentState = -1;" : "return; // Termination end of function"));
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
                ubergraphBody.WriteLine("check(false); // Invalid state");
                ubergraphBody.WriteLine("break;");
                ubergraphBody.Unindent();

                ubergraphBody.CloseScope();
                ubergraphBody.CloseScope();
                ubergraphBody.WriteLine("while( __CurrentState != -1 );");
            }
        }

        source.Write(ubergraphBody.Contents());
        source.GetIndentation() = ubergraphBody.GetIndentation();
        source.CloseScope();
    }

    for (auto& node : m_Nodes)
    {
        if (node.Function == NodeFunction::Event)
        {
            for (auto& output : node.Outputs)
                if (output.Data.Type != PinType::Flow)
                    header.WriteLine(header.ConvertPinTypeToCompiledString(output.Data.Type) + " b0l__NodeEvent_" + header.GenerateFunctionName(node) + "_" + header.UnderscoreSpaces(output.Name) + "__pf{};");
            header.WriteLine("DYFUNCTION(BlueprintCallable)");
            header.WriteLine("virtual " + header.GenerateFunctionSignature(node) + ";");
            source.WriteLine(source.GenerateFunctionSignature(node, NodeScriptName));
            source.OpenScope();
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

    header.CloseScope(true);
    header.NewLine();

    // Closes Dymatic Namespace
    header.CloseScope();
    source.CloseScope();

	std::string filepath = "src/Nodes/" + NodeScriptName;

	unsigned int warningCount = 0;
	unsigned int errorCount = 0;
	for (auto& result : m_CompilerResults)
	{
		if (result.Type == CompilerResultType::Error)
			errorCount++;
		else if (result.Type == CompilerResultType::Warning)
			warningCount++;
	}

	if (errorCount == 0)
	{
		header.OutputToFile(filepath + ".h");
		source.OutputToFile(filepath + ".cpp");
		m_CompilerResults.push_back({ CompilerResultType::Info, "Compile of " + NodeScriptName + " completed [C++] - Dymatic Nodes V1.2.2 (" + filepath + ")" });
	}
	else
		m_CompilerResults.push_back({ CompilerResultType::Info, "Compile of " + NodeScriptName + " failed [C++] - " + std::to_string(errorCount) + " Error(s) " + std::to_string(warningCount) + " Warnings(s) - Dymatic Nodes V1.2.2 (" + filepath + ")" });
}

void NodeEditorInternal::RecursivePinWrite(Pin& pin, NodeCompiler& source,NodeCompiler& header, NodeCompiler& ubergraphBody, std::string& line, std::vector<ed::NodeId>& pureNodeList, std::vector<ed::NodeId>& localNodeList)
{
	if (IsPinLinked(pin.ID))
	{
		auto link = GetPinLinks(pin.ID)[0];
		auto otherPin = FindPin(link->StartPinID == pin.ID ? link->EndPinID : link->StartPinID);
        auto otherNode = otherPin->Node;

        if (otherNode->VariableId != 0)
            for (auto& variable : m_Variables)
            {
                if (variable.ID == otherNode->VariableId)
                {
                    line += ubergraphBody.UnderscoreSpaces(header.UnderscoreSpaces("bpv__" + variable.Name + "_" + std::to_string(variable.ID) + "__pf"));
                    break;
                }
            }
        else
        {

            if (otherNode->Pure)
            {
                std::vector<ed::NodeId> newList = pureNodeList;
                newList.push_back(pin.Node->ID);
                RecursiveNodeWrite(*otherNode, source, header, ubergraphBody, newList, localNodeList);
            }

            // Lookup function of node
            bool found = false;
            for (auto& library : m_NodeLibraries)
                for (auto& Function : library.m_FunctionDeclarations)
                    if (Function.ID == otherNode->FunctionID)
                    {
                        found = true;
                        bool found = false;
                        if (otherPin->ArgumentID == Function.FUNC_Return.ArgumentID)
                        {
                            found = true;
                            line += ubergraphBody.UnderscoreSpaces("b0l__CallFunc_" + Function.FUNC_Name + "_" + std::to_string(otherNode->ID.Get()) + "_" + Function.FUNC_Return.Name + "__pf");
                            break;
                        }
                        else
                        {
                            for (auto& param : Function.FUNC_Params)
                                if (param.ArgumentID == otherPin->ArgumentID)
                                {
                                    found = true;
                                    line += ubergraphBody.UnderscoreSpaces("b0l__CallFunc_" + Function.FUNC_Name + "_" + std::to_string(otherNode->ID.Get()) + "_" + param.Name + "__pf");
                                    break;
                                }
                        }

                        if (!found)
                        {
                            m_CompilerResults.push_back({ CompilerResultType::Error, "Couldn't find associated argument for pin '" + otherPin->Name + "' of node '" + otherNode->Name + "'", otherNode->ID });
                            otherNode->Error = true;
                        }
                    }
            if (!found)
            {
                m_CompilerResults.push_back({ CompilerResultType::Error, "Couldn't find associated function for '" + otherNode->Name + "'", otherNode->ID });
                otherNode->Error = true;
            }
        }
	}
	else
		line += ubergraphBody.ConvertPinValueToCompiledString(pin.Data.Type, pin.Data.Value);
}

void NodeEditorInternal::RecursiveNodeWrite(Node& node, NodeCompiler& source, NodeCompiler& header, NodeCompiler& ubergraphBody, std::vector<ed::NodeId>& pureNodeList, std::vector<ed::NodeId>& localNodeList)
{
    std::string line;

    // Look for dependacy cycles and call error if detected.
	bool found = false;
	for (auto& nodeId : pureNodeList)
		if (nodeId == node.ID)
			found = true;
	if (found)
		for (auto& nodeId : pureNodeList)
		{
			auto node = FindNode(nodeId);
			m_CompilerResults.push_back({ CompilerResultType::Error, "Dependency cycle detected, preventing node '" + node->Name + "' from being scheduled.", nodeId });
			node->Error = true;
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
        for (auto& library : m_NodeLibraries)
            for (auto& Function : library.m_FunctionDeclarations)
                if (Function.ID == node.FunctionID)
                {
                    if (Function.FUNC_Return.Data.Type != PinType::Void)
                    {
                        bool found = false;
                        for (auto& output : node.Outputs)
                            if (output.ArgumentID == Function.FUNC_Return.ArgumentID)
                            {
                                auto name = "b0l__CallFunc_" + Function.FUNC_Name + "_" + std::to_string(node.ID.Get()) + "_Return_Value__pf";
                                if (!node.Written)
                                {
                                    node.Written = true;
                                    ((IsPinGlobal(output, node.UbergraphID) && !Function.FUNC_Pure) ? header : source).WriteLine(source.ConvertPinTypeToCompiledString(Function.FUNC_Return.Data.Type) + " " + source.UnderscoreSpaces(name) + "{};");
                                }
                                line += name + " = ";

                                found = true;
                                break;
                            }
                        if (!found) m_CompilerResults.push_back({ CompilerResultType::Error, "Couldn't find associated pin for argument '" + Function.FUNC_Return.Name + "' of function '" + Function.GetName() + "'", node.ID });
                    }
                    line += "DYTestNodeLibrary::" + Function.FUNC_Name + "(";
                    bool first = true;
                    for (auto& param : Function.FUNC_Params)
                    {
                        Pin* paramPin = nullptr;
                        for (auto& pin : node.Inputs)
                            if (pin.ArgumentID == param.ArgumentID)
                            {
                                paramPin = &pin;
                                break;
                            }
                        for (auto& pin : node.Outputs)
                            if (pin.ArgumentID == param.ArgumentID)
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
                                RecursivePinWrite(pin, source, header, ubergraphBody, line, pureNodeList, localNodeList);
                            else
                            {
                                auto name = "b0l__CallFunc_" + Function.FUNC_Name + "_" + std::to_string(node.ID.Get()) + "_" + param.Name + "__pf";
                                if (!pin.Written)
                                {
                                    pin.Written = true;
                                    ((IsPinGlobal(pin, node.UbergraphID) && !Function.FUNC_Pure) ? header : source).WriteLine(source.ConvertPinTypeToCompiledString(param.Data.Type) + " " + source.UnderscoreSpaces(name) + "{};");
                                }
                                line += "/*out*/ " + ubergraphBody.UnderscoreSpaces(name);
                            }
                        }
                        else
                        {
                            m_CompilerResults.push_back({ CompilerResultType::Error, "Couldn't find associated pin for argument '" + param.Name + "' of function '" + Function.GetName() + "'", node.ID });
                            node.Error = true;
                        }
                    }
                    line += ");";

                    ubergraphBody.WriteLine(line);
                    found = true;
                    break;
                }
        if (!found)
        {
            m_CompilerResults.push_back({ CompilerResultType::Error, "Couldn't find associated function for '" + node.Name + "'", node.ID });
            node.Error = true;
        }
    }
}

bool NodeEditorInternal::IsPinGlobal(Pin& pin, int ubergraphID)
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

void NodeEditorInternal::RecursiveUbergraphOrganisation(Node& node, std::vector<Ubergraph>& ubergraphs, unsigned int& currentId)
{
    // Search all ubergraphs for existing node definition and merge graphs if found.
    bool found = false;
    bool foundAll = false;
    for (auto& ubergraph : ubergraphs)
    {
            for (auto& nodeId : ubergraph.Nodes)
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
            if (output.Data.Type == PinType::Flow)
                if (IsPinLinked(output.ID))
                {
                    auto link = GetPinLinks(output.ID)[0];
                    auto& id = output.ID.Get() == link->StartPinID.Get() ? link->EndPinID : link->StartPinID;
                    auto& nextNode = *FindPin(id)->Node;

                    RecursiveUbergraphOrganisation(nextNode, ubergraphs, currentId);
                }
    }
}

Node& NodeEditorInternal::GetNextExecNode(Pin& pin)
{
	auto link = GetPinLinks(pin.ID)[0];
	auto& id = pin.ID.Get() == link->StartPinID.Get() ? link->EndPinID : link->StartPinID;
	return *FindPin(id)->Node;
}

std::string NodeCompiler::GenerateFunctionSignature(Node& node, const std::string& Namespace)
{
    std::string output = "void " + (Namespace.empty() ? "" : Namespace + "::") + GenerateFunctionName(node) + "(";

    bool first = true;
    for (auto& pin : node.Inputs)
    {
        if (pin.Data.Type != PinType::Flow)
        {
            if (first) first = false;
            else output += ", ";
            output += "const " + ConvertPinTypeToCompiledString(pin.Data.Type) + " " + pin.Name;
        }
    }

    for (auto& pin : node.Outputs)
    {
        if (pin.Data.Type != PinType::Flow)
        {
            if (first) first = false;
            else output += ", ";
            output += ConvertPinTypeToCompiledString(pin.Data.Type) + " " + UnderscoreSpaces(pin.Name);
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

void NodeCompiler::Write(std::string text)
{
    if (!m_IndentedCurrentLine)
    {
        m_IndentedCurrentLine = true;
        for (int i = 0; i < m_IndentLevel; i++)
            m_Buffer += '\t';
    }
	m_Buffer += text;
}

void NodeCompiler::WriteLine(std::string line)
{
    Write(line);
    NewLine();
}

void NodeCompiler::OutputToFile(const std::filesystem::path& path)
{
	std::ofstream fout(path);
    fout << m_Buffer;
}

std::string NodeEditorInternal::ConvertNodeFunctionToString(const NodeFunction& nodeFunction)
{
	switch (nodeFunction)
	{
    case NodeFunction::Node:        return "Node";
    case NodeFunction::Event :      return "Event";
    case NodeFunction::Function :   return "Function";
    case NodeFunction::Variable :   return "Variable";
	}
	DY_ASSERT(false, "Node function conversion to string not found");
}

NodeFunction NodeEditorInternal::ConvertStringToNodeFunction(const std::string& string)
{
	if (string == "Node")           return NodeFunction::Node;
	else if (string == "Event")     return NodeFunction::Event;
	else if (string == "Function")  return NodeFunction::Function;
	else if (string == "Variable")  return NodeFunction::Variable;
	else
		DY_ASSERT(false, "String conversion to node function not found");
}

std::string NodeEditorInternal::ConvertNodeTypeToString(const NodeType& nodeType)
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

NodeType NodeEditorInternal::ConvertStringToNodeType(const std::string& string)
{
	     if (string == "Blueprint") return NodeType::Blueprint;
	else if (string == "Simple")    return NodeType::Simple;
	else if (string == "Tree")      return NodeType::Tree;
	else if (string == "Comment")   return NodeType::Comment;
	else if (string == "Houdini")   return NodeType::Houdini;
	else
		DY_ASSERT(false, "String conversion to node type not found");
}

std::string NodeEditorInternal::ConvertPinTypeToString(const PinType& pinType)
{
	switch (pinType)
	{
	case PinType::Flow:     return "Flow";
	case PinType::Void:     return "Void";
	case PinType::Bool:     return "Boolean";
	case PinType::Int:      return "Integer";
	case PinType::Float:    return "Float";
	case PinType::String:   return "String";
	case PinType::Vector:   return "Vector";
	case PinType::Color:    return "Color";
	}
	DY_ASSERT(false, "Pin type conversion to string not found");
}

PinType NodeEditorInternal::ConvertStringToPinType(const std::string& string)
{
         if (string == "Flow" || string == "flow")       return PinType::Flow;
    else if (string == "Void" || string == "void")       return PinType::Void;
    else if (string == "Bool" || string == "bool" || string == "Boolean")       return PinType::Bool;
    else if (string == "Int" || string == "int" || string == "Integer")         return PinType::Int;
    else if (string == "Float" || string == "float")     return PinType::Float;
    else if (string == "String" || string == "string")   return PinType::String;
    else if (string == "Vector" || string == "vector")   return PinType::Vector;
    else if (string == "Color" || string == "color")   return PinType::Color;
    else
        DY_ASSERT(false, "String conversion to pin type not found");
}

std::string NodeEditorInternal::ConvertPinKindToString(const PinKind& pinKind)
{
    switch (pinKind)
    {
    case PinKind::Input: return "Input";
    case PinKind::Output: return "Output";
    }
	DY_ASSERT(false, "Pin kind conversion to string not found");
}

PinKind NodeEditorInternal::ConvertStringToPinKind(const std::string& string)
{
	     if (string == "Input")     return PinKind::Input;
	else if (string == "Output")    return PinKind::Output;
	else
		DY_ASSERT(false, "String conversion to pin kind not found");
}

std::string NodeEditorInternal::ConvertPinValueToString(const PinType& type, const TypeValue& value)
{
	switch (type)
	{
    case PinType::Flow: { return ""; }
	case PinType::Bool: { return value.Bool ? "true" : "false"; }
	case PinType::Int: { return std::to_string(value.Int); }
    case PinType::Float: { return std::to_string(value.Float); }
	case PinType::String: { return value.String; }
	case PinType::Vector: { return std::to_string(value.Vector.x) + ", " + std::to_string(value.Vector.y) + ", " + std::to_string(value.Vector.z); }
	case PinType::Color: { return std::to_string(value.Color.x) + ", " + std::to_string(value.Color.y) + ", " + std::to_string(value.Color.z) + ", " + std::to_string(value.Color.w); }
	}
	DY_CORE_ASSERT(false, "Node Value conversion to string not found");
}

TypeValue NodeEditorInternal::ConvertStringToPinValue(const PinType& type, const std::string& string)
{
    TypeValue value;
	switch (type)
	{
    case PinType::Flow: { break; }
    case PinType::Bool: { value.Bool = string == "true"; break; }
    case PinType::Int: { value.Int = std::stoi(string); break; }
    case PinType::Float: { value.Float = std::stof(string); break; }
    case PinType::String: { value.String = string; break; }
    case PinType::Vector: { value.Vector.x = std::stof(string.substr(0, Dymatic::String::Find_nth_of(string, ",", 1))); DY_ASSERT("Finish This"); break; }
    case PinType::Color: { value.Color.x = std::stof(string.substr(0, Dymatic::String::Find_nth_of(string, ",", 1))); DY_ASSERT("Finish This"); break; }
    default: DY_CORE_ASSERT(false, "String conversion to Node Value not found");
	}
    return value;
}

std::string NodeEditorInternal::ConvertContainerTypeToString(const ContainerType& type)
{
	switch (type)
	{
    case ContainerType::Single: { return "Single"; }
    case ContainerType::Array:  { return "Array" ; }
    case ContainerType::Set:    { return "Set"   ; }
    case ContainerType::Map:    { return "Map"   ; }
	}
	DY_CORE_ASSERT(false, "Container Type conversion to string not found");
}

ContainerType NodeEditorInternal::ConvertStringToContainerType(const std::string& string)
{
         if (string == "Single")return ContainerType::Single;
    else if (string == "Array") return ContainerType::Array;
    else if (string == "Set")   return ContainerType::Set;
    else if (string == "Map")   return ContainerType::Map;
	else
		DY_ASSERT(false, "String conversion to Container Type not found");
}

void NodeEditorInternal::OpenNodes()
{
    std::string filepath = Dymatic::FileDialogs::OpenFile("Node Graph (*.node)\0*.node\0");
    if (!filepath.empty())
    {
        OpenNodes(filepath);
    }
}

void NodeEditorInternal::OpenNodes(const std::filesystem::path& path)
{
	std::string result;
	std::ifstream in(path, std::ios::in | std::ios::binary); // ifstream closes itself due to RAII
	if (in)
	{
		in.seekg(0, std::ios::end);
		size_t size = in.tellg();
		if (size != -1)
		{
			result.resize(size);
			in.seekg(0, std::ios::beg);
			in.read(&result[0], size);
		}
		else
		{
			DY_CORE_ERROR("Could not read from node graph file '{0}'", path);
			return;
		}
	}


	while (result.find_first_of("\n") != -1)
	{
		result = result.erase(result.find_first_of("\n"), 1);
	}

	while (result.find_first_of("\r") != -1)
	{
		result = result.erase(result.find_first_of("\r"), 1);
	}

	bool openName = false;
	bool openValueName = false;
	bool openValue = false;
	std::string CurrentName = "";
	std::string CurrentValueName = "";
	std::string CurrentValue = "";

    Node currentNode = {};
    Pin currentPin = {};
    Link currentLink = {};
    Variable currentVariable = {};

	for (int i = 0; i < result.length(); i++)
	{
		std::string character = result.substr(i, 1);

		if (character == "]") { openName = false; }
		if (openName) { CurrentName += character; }
        if (character == "[") { openName = true; CurrentName = ""; }

		if (character == ">") { openValueName = false; }
		if (openValueName) { CurrentValueName += character; }
		if (character == "<") { openValueName = true; CurrentValueName = ""; }

		if (character == "}") { openValue = false; }
		if (openValue) { CurrentValue += character; }
		if (character == "{") { openValue = true; CurrentValue = ""; }

		if (character == "}" && CurrentValue != "")
		{
            if (CurrentName == "Node")
            {
                if (CurrentValueName == "Id") { currentNode.ID = std::stoi(CurrentValue); }
                else if (CurrentValueName == "Name") { currentNode.Name = CurrentValue; }
                else if (CurrentValueName == "DisplayName") { currentNode.DisplayName = CurrentValue; }
                else if (CurrentValueName == "PositionX") { ed::SetNodePosition(currentNode.ID, ImVec2(std::stof(CurrentValue), ed::GetNodePosition(currentNode.ID).y)); }
                else if (CurrentValueName == "PositionY") { ed::SetNodePosition(currentNode.ID, ImVec2(ed::GetNodePosition(currentNode.ID).x, std::stof(CurrentValue))); }
                else if (CurrentValueName == "ColorX") { currentNode.Color.Value.x = std::stof(CurrentValue); }
                else if (CurrentValueName == "ColorY") { currentNode.Color.Value.y = std::stof(CurrentValue); }
                else if (CurrentValueName == "ColorZ") { currentNode.Color.Value.z = std::stof(CurrentValue); }
                else if (CurrentValueName == "Type") { currentNode.Type = ConvertStringToNodeType(CurrentValue); }
                else if (CurrentValueName == "Function") { currentNode.Function = ConvertStringToNodeFunction(CurrentValue); }
                else if (CurrentValueName == "SizeX") { currentNode.Size.x = std::stof(CurrentValue); }
                else if (CurrentValueName == "SizeY") { currentNode.Size.y = std::stof(CurrentValue); }
                else if (CurrentValueName == "State") { currentNode.State = CurrentValue; }
                else if (CurrentValueName == "SavedState") { currentNode.SavedState = CurrentValue; }
                else if (CurrentValueName == "CommentEnabled") { currentNode.CommentEnabled = CurrentValue == "true"; }
                else if (CurrentValueName == "CommentPinned") { currentNode.CommentPinned = CurrentValue == "true"; }
                else if (CurrentValueName == "Comment") { currentNode.Comment = CurrentValue; }
                else if (CurrentValueName == "AddInputs") { currentNode.AddInputs = CurrentValue == "true"; }
                else if (CurrentValueName == "AddOutputs") { currentNode.AddOutputs = CurrentValue == "true"; }

				else if (CurrentValueName == "Initialize") { m_Nodes.emplace_back(currentNode); BuildNode(&m_Nodes.back()); currentNode = {}; }
            }

            else if (CurrentName == "Input")
            {
                if (CurrentValueName == "Id") { currentPin.ID = std::stoi(CurrentValue); }
                else if (CurrentValueName == "Name") { currentPin.Name = CurrentValue; }
                else if (CurrentValueName == "Type") { currentPin.Data.Type = ConvertStringToPinType(CurrentValue); }
                else if (CurrentValueName == "Kind") { currentPin.Kind = ConvertStringToPinKind(CurrentValue); }
                else if (CurrentValueName == "Value") { currentPin.Data.Value = ConvertStringToPinValue(currentPin.Data.Type, CurrentValue); }
                else if (CurrentValueName == "Container") { currentPin.Data.Container = ConvertStringToContainerType(CurrentValue); }
                else if (CurrentValueName == "Deletable") { currentPin.Deletable = CurrentValue == "true"; }

                else if (CurrentValueName == "Initialize") { currentNode.Inputs.emplace_back(currentPin); currentPin = {}; }
            }

			else if (CurrentName == "Output")
			{
				if (CurrentValueName == "Id") { currentPin.ID = std::stoi(CurrentValue); }
				else if (CurrentValueName == "Name") { currentPin.Name = CurrentValue; }
				else if (CurrentValueName == "Type") { currentPin.Data.Type = ConvertStringToPinType(CurrentValue); }
				else if (CurrentValueName == "Kind") { currentPin.Kind = ConvertStringToPinKind(CurrentValue); }
				else if (CurrentValueName == "Value") { currentPin.Data.Value = ConvertStringToPinValue(currentPin.Data.Type, CurrentValue); }
                else if (CurrentValueName == "Container") { currentPin.Data.Container = ConvertStringToContainerType(CurrentValue); }
				else if (CurrentValueName == "Deletable") { currentPin.Deletable = CurrentValue == "true"; }

				else if (CurrentValueName == "Initialize") { currentNode.Outputs.emplace_back(currentPin); currentPin = {}; }
			}

			else if (CurrentName == "Link")
			{
				if (CurrentValueName == "Id") { currentLink.ID = std::stoi(CurrentValue); }
				else if (CurrentValueName == "StartPinId") { currentLink.StartPinID = std::stoi(CurrentValue); }
				else if (CurrentValueName == "EndPinId") { currentLink.EndPinID = std::stoi(CurrentValue); }
				else if (CurrentValueName == "ColorX") { currentLink.Color.Value.x = std::stof(CurrentValue); }
				else if (CurrentValueName == "ColorY") { currentLink.Color.Value.y = std::stof(CurrentValue); }
				else if (CurrentValueName == "ColorZ") { currentLink.Color.Value.z = std::stof(CurrentValue); }

				else if (CurrentValueName == "Initialize") { currentLink.Color.Value.w = 1.0f; m_Links.emplace_back(currentLink); currentLink = {}; }
			}

			else if (CurrentName == "Variable")
			{
				if (CurrentValueName == "Id") { currentVariable.ID = std::stoi(CurrentValue); }
                else if (CurrentValueName == "Name") { currentVariable.Name = CurrentValue; }
                else if (CurrentValueName == "Type") { currentVariable.Data.Type = ConvertStringToPinType(CurrentValue); }
                else if (CurrentValueName == "Value") { currentVariable.Data.Value = ConvertStringToPinValue(currentVariable.Data.Type, CurrentValue); }

                else if (CurrentValueName == "Initialize") { m_Variables.emplace_back(currentVariable); currentVariable = {}; }
			}

			else if (CurrentName == "Graph")
			{
                if (CurrentValueName == "NextId") { m_NextId = std::stof(CurrentValue); }
			}
		}
	}

    BuildNodes();
}

void NodeEditorInternal::SaveNodes()
{
	std::string filepath = Dymatic::FileDialogs::SaveFile("Node Graph (*.node)\0*.node\0");
	if (!filepath.empty())
	{
		SaveNodes(filepath);
	}
}

void NodeEditorInternal::SaveNodes(const std::filesystem::path& path)
{
    std::string out = "";
    for (auto& node : m_Nodes)
    {
        out += "[Node] ";
		out += "<Id> {" + std::to_string(node.ID.Get()) + "} ";
		out += "<Name> {" + node.Name + "} ";
		out += "<DisplayName> {" + node.DisplayName + "} ";
        out += "<PositionX> {" + std::to_string(ed::GetNodePosition(node.ID).x) + "} ";
        out += "<PositionY> {" + std::to_string(ed::GetNodePosition(node.ID).y) + "} ";
        out += "<ColorX> {" + std::to_string(node.Color.Value.x) + "} ";
        out += "<ColorY> {" + std::to_string(node.Color.Value.y) + "} ";
        out += "<ColorZ> {" + std::to_string(node.Color.Value.z) + "} ";
        out += "<Type> {" + ConvertNodeTypeToString(node.Type) + "} ";
		out += "<Function> {" + ConvertNodeFunctionToString(node.Function) + "} ";
		out += "<SizeX> {" + std::to_string(node.Size.x) + "} ";
		out += "<SizeY> {" + std::to_string(node.Size.y) + "} ";
		out += "<State> {" + node.State + "} ";
		out += "<SavedState> {" + node.SavedState + "} ";
		out += "<CommentEnabled> {" + std::string(node.CommentEnabled ? "true" : "false") + "} ";
		out += "<CommentPinned> {" + std::string(node.CommentPinned ? "true" : "false") + "} ";
		out += "<Comment> {" + node.Comment + "} ";
		out += "<AddInputs> {" + std::string(node.AddInputs ? "true" : "false") + "}";
		out += "<AddOutputs> {" + std::string(node.AddOutputs ? "true" : "false") + "}";

        for (auto& input : node.Inputs)
        {
            out += "\r\n        [Input]";
            out += "<Id> {" + std::to_string(input.ID.Get()) + "} ";
            out += "<Name> {" + input.Name + "} ";
            out += "<Type> {" + ConvertPinTypeToString(input.Data.Type) + "} ";
            out += "<Kind> {" + ConvertPinKindToString(input.Kind) + "} ";
            out += "<Value> {" + ConvertPinValueToString(input.Data.Type, input.Data.Value) + "} ";
            out += "<Container> {" + ConvertContainerTypeToString(input.Data.Container) + "} ";
            out += "<Deletable> {" + std::string(input.Deletable ? "true" : "false") + "} ";
            out += "<Initialize> {Pin}";
        }

		for (auto& output : node.Outputs)
		{
			out += "\r\n        [Output]";
			out += "<Id> {" + std::to_string(output.ID.Get()) + "} ";
			out += "<Name> {" + output.Name + "} ";
			out += "<Type> {" + ConvertPinTypeToString(output.Data.Type) + "} ";
			out += "<Kind> {" + ConvertPinKindToString(output.Kind) + "} ";
			out += "<Value> {" + ConvertPinValueToString(output.Data.Type, output.Data.Value) + "} ";
            out += "<Container> {" + ConvertContainerTypeToString(output.Data.Container) + "} ";
			out += "<Deletable> {" + std::string(output.Deletable ? "true" : "false") + "} ";
			out += "<Initialize> {Pin}";
		}

        out += "\r\n[Node] ";
		out += "<Initialize> {Node}\r\n";
    }

    for (auto& link : m_Links)
    {
        out += "[Link] ";
        out += "<Id> {" + std::to_string(link.ID.Get()) + "} ";
        out += "<StartPinId> {" + std::to_string(link.StartPinID.Get()) + "} ";
        out += "<EndPinId> {" + std::to_string(link.EndPinID.Get()) + "} ";
        out += "<ColorX> {" + std::to_string(link.Color.Value.x) + "} ";
        out += "<ColorY> {" + std::to_string(link.Color.Value.y) + "} ";
        out += "<ColorZ> {" + std::to_string(link.Color.Value.z) + "} ";

        out += "<Initialize> {Link}\r\n";
    }

    for (auto& variable : m_Variables)
    {
        out += "[Variable] ";
        out += "<Id> {" + std::to_string(variable.ID) + "} ";
        out += "<Name> {" + variable.Name + "} ";
        out += "<Type> {" + ConvertPinTypeToString(variable.Data.Type) +"} ";
        out += "<Value> {" + ConvertPinValueToString(variable.Data.Type, variable.Data.Value) + "} ";

        out += "<Initialize> {Variable}\r\n";
    }

    out += "\r\n[Graph] ";
    out += "<NextId> {" + std::to_string(m_NextId) + "}";

	std::ofstream fout(path);
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
    
    BuildNodes();
}

void NodeEditorInternal::DeleteNodes()
{

}

void NodeEditorInternal::DefaultValueInput(PinData& data, bool spring)
{
    if (data.Container == ContainerType::Single && !data.Reference)
    {
        switch (data.Type)
        {
        case PinType::Bool: {
            ImGui::Custom::Checkbox("##NodeEditorBoolCheckbox", &data.Value.Bool);
            if (spring) ImGui::Spring(0);
            break;
        }
        case PinType::Int: {
            ImGui::SetNextItemWidth(std::clamp(ImGui::CalcTextSize((std::to_string(data.Value.Int)).c_str()).x + 20, 50.0f, 300.0f));
            ImGui::DragInt("##NodeEditorIntSlider", &data.Value.Int, 0.1f, 0, 0);
            if (spring) ImGui::Spring(0);
            break;
        }
        case PinType::Float: {
            ImGui::SetNextItemWidth(std::clamp(ImGui::CalcTextSize((std::to_string(data.Value.Float)).c_str()).x - 10, 50.0f, 300.0f));
            ImGui::DragFloat("##NodeEditorFloatSlider", &data.Value.Float, 0.1f, 0, 0, "%.3f");
            if (spring) ImGui::Spring(0);
            break;
        }
        case PinType::String: {
            char buffer[512];
            memset(buffer, 0, sizeof(buffer));
            std::strncpy(buffer, data.Value.String.c_str(), sizeof(buffer));
            const float size = ImGui::CalcTextSize(buffer).x + 10.0f;
            ImGui::SetNextItemWidth(std::clamp(size, 25.0f, 500.0f));
            if (ImGui::InputText("##NodeEditorStringInputText", buffer, sizeof(buffer), size > 500.0f ? 0 : ImGuiInputTextFlags_NoHorizontalScroll))
            {
                data.Value.String = std::string(buffer);
            }
            if (spring) ImGui::Spring(0);
            break;
        }
        case PinType::Vector: {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4({}));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4({}));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4({}));

            if (spring) ImGui::Spring(0);
            ImGui::Button("X");
            if (!spring) ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::CalcTextSize(std::to_string(data.Value.Vector.x).c_str()).x);
            ImGui::DragFloat("##NodeEditorVectorSliderX", &data.Value.Vector.x, 0.1f, 0, 0, "%.3f");
            if (spring) ImGui::Spring(0); else ImGui::SameLine();
            ImGui::Button("Y");
            if (!spring) ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::CalcTextSize(std::to_string(data.Value.Vector.y).c_str()).x);
            ImGui::DragFloat("##NodeEditorVectorSliderY", &data.Value.Vector.y, 0.1f, 0, 0, "%.3f");
            if (spring) ImGui::Spring(0); else ImGui::SameLine();
            ImGui::Button("Z");
            if (!spring) ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::CalcTextSize(std::to_string(data.Value.Vector.z).c_str()).x);
            ImGui::DragFloat("##NodeEditorVectorSliderZ", &data.Value.Vector.z, 0.1f, 0, 0, "%.3f");

            ImGui::PopStyleColor(3);
            break;
        }
        case PinType::Color: {
			//ImGui::GetCurrentWindow()->DC.CursorStartPos = ImVec2(100.0f, 100.0f);// = ImGui::GetCurrentWindow()->DC.CursorStartPos;
			//
			//static ImVec2 prepos = {};
			//static bool lock = true;
			//if (Input::IsKeyPressed(Key::Z)) lock = false;
			//if (Input::IsKeyPressed(Key::X)) lock = true;
			//auto cpos = ImGui::GetCursorPos();
			//if (Input::IsKeyPressed(Key::B) || !lock)
			//{
			//    ImGui::SetCursorPos(prepos);
			//    ImGui::SetNextWindowPos(prepos);
			//}
			//ImGui::ColorEdit4("##NodeEditorColor", glm::value_ptr(data.Value.Color), ImGuiColorEditFlags_NoInputs);
			//ImGui::SetCursorPos(cpos);
			//if (Input::IsKeyPressed(Key::A))
			//prepos = ed::CanvasToScreen(ImGui::GetCurrentWindow()->DC.LastItemRect.GetBL() + ImVec2(-1, ImGui::GetStyle().ItemSpacing.y));//ed::CanvasToScreen();//ImGui::GetCurrentWindow()->DC.LastItemRect.GetBL();//ed::ScreenToCanvas(); //
            if (spring) ImGui::Spring(0);
            break;
        }
        }
    }
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

SearchResultData* NodeEditorInternal::DisplaySearchData(SearchData& searchData, bool origin)
{
	SearchResultData* data = nullptr;
	if (!searchData.confirmed)
	{
		ImGui::SetNextTreeNodeOpen(searchData.open);
		searchData.confirmed = true;
	}
	bool searchEmpty = !m_SearchBuffer.empty();
	if (searchEmpty) ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
	bool open = origin ? true : ImGui::TreeNodeEx(searchData.Name.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth);
	if (searchEmpty) ImGui::PopItemFlag();
	if (open)
	{
		for (auto& tree : searchData.LowerTree)
		{
			data = DisplaySearchData(tree, false);
			if (data != nullptr) break;
		}
		for (auto& result : searchData.Results)
		{
			if (ImGui::MenuItem(result.Name.c_str()) && data == nullptr)
				data = &result;
		}
		if (!origin)
			ImGui::TreePop();
	}
	return data;
}

void NodeEditorInternal::CheckLinkSafety(Pin* startPin, Pin* endPin)
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

    for (auto& link : m_Links)
    {
        bool endPinCheck = false;
        bool startPinCheck = false;
        if (endPin != nullptr)
            endPinCheck = (link.EndPinID == endPin->ID || link.StartPinID == endPin->ID) && endPin->Data.Type != PinType::Flow;
        if (startPin != nullptr)
            startPinCheck = (link.EndPinID == startPin->ID || link.StartPinID == startPin->ID) && /*endPin->Type*/startPin->Data.Type == PinType::Flow;
        if (endPinCheck || startPinCheck)
            ed::DeleteLink(link.ID);
    }
}

void NodeEditorInternal::UpdateSearchData()
{
    m_SearchData.Clear();
    m_SearchData.Name = "Dymatic Nodes";
    
    for (auto& nodeLibrary : m_NodeLibraries)
        for (auto& Function : nodeLibrary.m_FunctionDeclarations)
            AddSearchDataReference(Function.GetName(), Function.FUNC_Category, Function.FUNC_Keywords, Function.FUNC_Params, Function.FUNC_Return.Data, Function.FUNC_Pure, Function.ID);
    
    AddSearchData("Branch", "Utilities|Flow Control", { "if" }, { {{ PinType::Bool }, PinKind::Input} }, {}, false, &NodeEditorInternal::SpawnBranchNode);
    AddSearchData("Sequence", "Utilities|Flow Control", { "series" }, {}, {}, false, &NodeEditorInternal::SpawnSequenceNode);
    if (m_NewNodeLinkPin == nullptr || !m_ContextSensitive) AddSearchData(ed::GetSelectedObjectCount() > 0 ? "Add Comment to Selection" : "Add Comment...", "", { "label" }, {}, {}, true, &NodeEditorInternal::SpawnComment);

    AddSearchData("On Create", "Add Event", { "begin" }, {}, {}, false, &NodeEditorInternal::SpawnOnCreateNode);
    AddSearchData("On Update", "Add Event", { "tick", "frame" }, {}, { PinType::Float }, false, &NodeEditorInternal::SpawnOnUpdateNode);
    AddSearchData("On Destroy", "Add Event", { "end" }, {}, {}, false, &NodeEditorInternal::SpawnOnDestroyNode);

    for (auto& variable : m_Variables)
    {
		AddSearchData("Get " + variable.Name, "Variables", {}, {}, variable.Data, true, &NodeEditorInternal::SpawnGetVariableNode, { variable.ID });
		AddSearchData("Set " + variable.Name, "Variables", {}, { {{variable.Data }, PinKind::Input} }, variable.Data, false, &NodeEditorInternal::SpawnSetVariableNode, { variable.ID });
    }

    //m_SearchData.Name = "Dymatic Nodes";
    //auto Development = m_SearchData.PushBackTree("Development");
    //    Development->PushBackResult("Assert", &NodeEditorInternal::SpawnAssertNode);
    //    Development->PushBackResult("Error", &NodeEditorInternal::SpawnErrorNode);
    //auto Event = m_SearchData.PushBackTree("Events");
    //    Event->PushBackResult("On Create", &NodeEditorInternal::SpawnOnCreateNode);
    //    Event->PushBackResult("On Update", &NodeEditorInternal::SpawnOnUpdateNode);
    //    auto CustomEvent = Event->PushBackTree("Custom Events");
    //auto Variable = m_SearchData.PushBackTree("Variables");
    //    for (int i = 0; i < m_Variables.size(); i++)
    //    {
    //        SpawnNodeStruct newStruct = {};
    //        newStruct.variableName = m_Variables[i].Name;
    //        newStruct.variableType = m_Variables[i].Type;
    //        newStruct.variableId =  m_Variables[i].ID;
    //        Variable->PushBackResult("Get " + m_Variables[i].Name, &NodeEditorInternal::SpawnGetVariableNode, newStruct);
    //        Variable->PushBackResult("Set " + m_Variables[i].Name, &NodeEditorInternal::SpawnSetVariableNode, newStruct);
    //    }
    //auto Math = m_SearchData.PushBackTree("Math");
    //    auto Boolean = Math->PushBackTree("Boolean");
    //        Boolean->PushBackResult("Make Literal Bool", &NodeEditorInternal::SpawnMakeLiteralBoolNode);
    //        Boolean->PushBackResult("AND Boolean", &NodeEditorInternal::SpawnANDBooleanNode);
    //        Boolean->PushBackResult("Equal Boolean", &NodeEditorInternal::SpawnEqualBooleanNode);
    //        Boolean->PushBackResult("NAND Boolean", &NodeEditorInternal::SpawnNANDBooleanNode);
    //        Boolean->PushBackResult("NOR Boolean", &NodeEditorInternal::SpawnNORBooleanNode);
    //        Boolean->PushBackResult("NOT Boolean", &NodeEditorInternal::SpawnNOTBooleanNode);
    //        Boolean->PushBackResult("Not Equal Boolean", &NodeEditorInternal::SpawnNotEqualBooleanNode);
    //        Boolean->PushBackResult("OR Boolean", &NodeEditorInternal::SpawnORBooleanNode);
    //        Boolean->PushBackResult("XOR Boolean", &NodeEditorInternal::SpawnXORBooleanNode);
    //    auto Int = Math->PushBackTree("Int");
    //        Int->PushBackResult("Make Literal Int", &NodeEditorInternal::SpawnMakeLiteralIntNode);
    //    auto Float = Math->PushBackTree("Float");
    //        Float->PushBackResult("Make Literal Float", &NodeEditorInternal::SpawnMakeLiteralFloatNode);
    //        Float->PushBackResult("Modulo (Float)", &NodeEditorInternal::SpawnModuloFloatNode);
    //        Float->PushBackResult("Absolute (Float)", &NodeEditorInternal::SpawnAbsoluteFloatNode);
    //        Float->PushBackResult("Ceil", &NodeEditorInternal::SpawnCeilNode);
    //        Float->PushBackResult("Clamp Float", &NodeEditorInternal::SpawnClampFloatNode);
    //        Float->PushBackResult("Compare Float", &NodeEditorInternal::SpawnCompareFloatNode);
    //        Float->PushBackResult("Normalize Angle", &NodeEditorInternal::SpawnNormalizeAngleNode);
    //        Float->PushBackResult("Equal Float", &NodeEditorInternal::SpawnEqualFloatNode);
    //        Float->PushBackResult("Exp Float", &NodeEditorInternal::SpawnExpFloatNode);
    //        Float->PushBackResult("FInterp Ease in Out", &NodeEditorInternal::SpawnFInterpEaseInOutNode);
    //        Float->PushBackResult("float - float", &NodeEditorInternal::SpawnSubtractionFloatNode);
    //        Float->PushBackResult("float * float", &NodeEditorInternal::SpawnMultiplicationFloatNode);
    //        Float->PushBackResult("float / float", &NodeEditorInternal::SpawnDivisionFloatNode);
    //        Float->PushBackResult("float + float", &NodeEditorInternal::SpawnAdditionFloatNode);
    //        Float->PushBackResult("Lerp Angle", &NodeEditorInternal::SpawnLerpAngleNode);
    //    auto Interpolation = Math->PushBackTree("Interpolation");
    //        Interpolation->PushBackResult("Interp Ease In", &NodeEditorInternal::SpawnInterpEaseInNode);
    //        Interpolation->PushBackResult("Interp Ease Out", &NodeEditorInternal::SpawnInterpEaseOutNode);
    //    auto Trigonometry = Math->PushBackTree("Trigonometry");
    //    auto Conversions = Math->PushBackTree("Conversions");
	//	Conversions->PushBackResult("Int To Bool", &NodeEditorInternal::SpawnIntToBoolNode);
	//	Conversions->PushBackResult("Float To Bool", &NodeEditorInternal::SpawnFloatToBoolNode);
	//	Conversions->PushBackResult("String To Bool", &NodeEditorInternal::SpawnStringToBoolNode);
    //    Conversions->PushBackResult("Bool To Int", &NodeEditorInternal::SpawnBoolToIntNode);
    //    Conversions->PushBackResult("Float To Int", &NodeEditorInternal::SpawnFloatToIntNode);
    //    Conversions->PushBackResult("String To Int", &NodeEditorInternal::SpawnStringToIntNode);
    //    Conversions->PushBackResult("Bool To Float", &NodeEditorInternal::SpawnBoolToFloatNode);
    //    Conversions->PushBackResult("Int To Float", &NodeEditorInternal::SpawnIntToFloatNode);
    //    Conversions->PushBackResult("String To Float", &NodeEditorInternal::SpawnStringToFloatNode);
    //    Conversions->PushBackResult("Float To Vector", &NodeEditorInternal::SpawnFloatToVectorNode);
    //    Conversions->PushBackResult("String To Vector", &NodeEditorInternal::SpawnStringToVectorNode);
    //auto Utilities = m_SearchData.PushBackTree("Utilities");
    //    auto String = Utilities->PushBackTree("String");
    //        String->PushBackResult("Make Literal String", &NodeEditorInternal::SpawnMakeLiteralStringNode);
    //        String->PushBackResult("Bool To String", &NodeEditorInternal::SpawnBoolToStringNode);
    //        String->PushBackResult("Int To String", &NodeEditorInternal::SpawnIntToStringNode);
    //        String->PushBackResult("Float To String", &NodeEditorInternal::SpawnFloatToStringNode);
    //        String->PushBackResult("Vector To String", &NodeEditorInternal::SpawnVectorToStringNode);
    //        String->PushBackResult("Print String", &NodeEditorInternal::SpawnPrintStringNode);
    //    auto FlowControl = Utilities->PushBackTree("Flow Control");
    //        FlowControl->PushBackResult("Branch", &NodeEditorInternal::SpawnBranchNode);
    //        FlowControl->PushBackResult("Do N", &NodeEditorInternal::SpawnDoNNode);
    //        FlowControl->PushBackResult("Do Once", &NodeEditorInternal::SpawnDoOnceNode);
    //        FlowControl->PushBackResult("For Loop", &NodeEditorInternal::SpawnForLoopNode);
    //        FlowControl->PushBackResult("For Loop with Break", &NodeEditorInternal::SpawnForLoopWithBreakNode);
    //        FlowControl->PushBackResult("Sequence", &NodeEditorInternal::SpawnSequenceNode);
    //        FlowControl->PushBackResult("While Loop", &NodeEditorInternal::SpawnWhileLoopNode);
    //m_SearchData.PushBackResult(ed::GetSelectedObjectCount() > 0 ? "Add Comment to Selection" : "Add Comment...", &NodeEditorInternal::SpawnComment);
}

void NodeEditorInternal::AddSearchData(std::string name, std::string category, std::vector<std::string> keywords, std::vector<Pin> params, PinData returnVal, bool pure, spawnNodeFunction Function, SpawnNodeData spawnData, int id)
{
    AddSearchDataReference(name, category, keywords, params, returnVal, pure, id, Function, spawnData);
}

void NodeEditorInternal::AddSearchDataReference(std::string& name, std::string& category, std::vector<std::string>& keywords, std::vector<Pin>& params, PinData& returnVal, bool& pure, int& id, spawnNodeFunction Function, SpawnNodeData spawnData)
{
	bool visible = true;
	if (!m_SearchBuffer.empty())
	{
		std::vector<std::string> searchWords;
		String::SplitStringByDelimiter(m_SearchBuffer, searchWords, ' ');

		for (auto& search : searchWords)
			if (!search.empty())
			{
				String::ToLower(search);
				bool found = false;
				for (auto word : keywords)
				{
					String::ToLower(word);
					if (word.find(search) != std::string::npos)
						found = true;
				}

				std::string funcName = name;
				String::ToLower(funcName);
				if (funcName.find(search) != std::string::npos)
					found = true;
				if (!found)
					visible = false;
			}
	}
	if (m_ContextSensitive && m_NewNodeLinkPin != nullptr)
	{
        if (m_NewNodeLinkPin->Data.Type == PinType::Flow)
        {
            if (pure)
                visible = false;
        }
        else
        {
            bool found = false;
            for (auto& param : params)
            {
                spawnNodeFunction Function;
                ConversionAvalible(&param, m_NewNodeLinkPin, Function);
                if ((param.Data.Type == m_NewNodeLinkPin->Data.Type || Function != nullptr) && param.Kind != m_NewNodeLinkPin->Kind)
                    found = true;
            }
            if (!found)
            {
                if (returnVal.Type == PinType::Void)
                    visible = false;
                else
                {
                    spawnNodeFunction Function;
                    ConversionAvalible(returnVal, m_NewNodeLinkPin->Data, Function);
                    if ((returnVal.Type != m_NewNodeLinkPin->Data.Type && Function == nullptr) || m_NewNodeLinkPin->Kind == PinKind::Output)
                        visible = false;
                }
            }
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
			for (auto& branch : searchData->LowerTree)
				if (branch.Name == segment)
				{
					searchData = &branch;
					found = true;
				}
			// If function doesn't exist create new data scope and update to current one
			if (!found)
			{
				searchData = &searchData->PushBackTree(segment);
			}
		}

		// Add Function To Corresponding Category
        if (Function != nullptr)
		    searchData->PushBackResult(name, Function, spawnData);
        else
		    searchData->PushBackResult(name, id);
	}
}

void NodeEditorInternal::Application_Initialize()
{

    //ed::EnableShortcuts(true);

	ed::Config config;
    config.SettingsFile = "";

    LoadNodeLibrary("src/Nodes/CustomNodeLibrary.h");

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

	m_HeaderBackground = Dymatic::Texture2D::Create("src/Panels/NodeEditor/data/BlueprintBackground.png");
	m_SaveIcon = Dymatic::Texture2D::Create("src/Panels/NodeEditor/data/ic_save_white_24dp.png");
	m_RestoreIcon = Dymatic::Texture2D::Create("src/Panels/NodeEditor/data/ic_restore_white_24dp.png");
	m_CommentIcon = Dymatic::Texture2D::Create("src/Panels/NodeEditor/data/ic_comment_white_24dp.png");
	m_PinIcon = Dymatic::Texture2D::Create("src/Panels/NodeEditor/data/ic_pin_white_24dp.png");
	m_PinnedIcon = Dymatic::Texture2D::Create("src/Panels/NodeEditor/data/ic_pinned_white_24dp.png");

    // Node Icons
	m_EventIcon = Dymatic::Texture2D::Create("src/Panels/NodeEditor/data/NodeEvent.png");
	m_FunctionIcon = Dymatic::Texture2D::Create("src/Panels/NodeEditor/data/NodeFunction.png");
	m_SequenceIcon = Dymatic::Texture2D::Create("src/Panels/NodeEditor/data/NodeSequence.png");
	m_BranchIcon = Dymatic::Texture2D::Create("src/Panels/NodeEditor/data/NodeBranch.png");
	m_SwitchIcon = Dymatic::Texture2D::Create("src/Panels/NodeEditor/data/NodeSwitch.png");

    m_Editor = ed::CreateEditor(&config);
	ed::SetCurrentEditor(m_Editor);

    // Initial Node Loading
	{
		Node* node;
		node = SpawnOnCreateNode({}); ed::SetNodePosition(node->ID, ImVec2(0, -100)); node->CommentEnabled = true; node->CommentPinned = true; node->Comment = "Executed when script starts.";
		node = SpawnOnUpdateNode({}); ed::SetNodePosition(node->ID, ImVec2(0, 100)); node->CommentEnabled = true; node->CommentPinned = true; node->Comment = "Executed every frame and provides delta time.";
	}
    
	BuildNodes();

    m_Variables.emplace_back(Variable(GetNextId(), "Test Bool", PinType::Bool));
    m_Variables.back().Data.Value.Bool = true;
    m_Variables.emplace_back(Variable(GetNextId(), "Test Float", PinType::Float));
    m_Variables.back().Data.Value.Float = 5.423f;
    m_Variables.emplace_back(Variable(GetNextId(), "Test Int", PinType::Int));
    m_Variables.back().Data.Value.Int = 12;

    ed::NavigateToContent();

	//auto& io = ImGui::GetIO();
}

Dymatic::NodeEditorPannel::NodeEditorPannel()
{
    EditorInternalStack.push_back({});
    this->stackIndex = EditorInternalStack.size() - 1;
}

bool& Dymatic::NodeEditorPannel::GetNodeEditorVisible()
{
    return EditorInternalStack[this->stackIndex].GetNodeEditorVisible();
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

void Dymatic::NodeEditorPannel::OnImGuiRender()
{
    EditorInternalStack[this->stackIndex].OnImGuiRender();
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

void NodeEditorInternal::OnImGuiRender()
{
    if (m_NodeEditorVisible)
    {
        auto& io = ImGui::GetIO();
    
        ImGuiWindowClass windowClass;
        windowClass.ClassId = ImGui::GetID("##Node DockSpace Class");
        windowClass.DockingAllowUnclassed = false;
    
    	// DockSpace
    	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
        ImGui::Begin((std::string(CHARACTER_WINDOW_ICON_NODE_EDITOR) + " Node Editor").c_str(), &m_NodeEditorVisible, ImGuiWindowFlags_MenuBar);
        ImGui::PopStyleVar();
    
    
    	ImGuiStyle& style = ImGui::GetStyle();
    	float minWinSizeX = style.WindowMinSize.x;
    	style.WindowMinSize.x = 150.0f;
    	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    	{
    		ImGuiID dockspace_id = ImGui::GetID("Node DockSpace");
    		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags, &windowClass);
    	}
        style.WindowMinSize.x = minWinSizeX;

    	ImGui::BeginMenuBar();
    	if (ImGui::BeginMenu("File"))
    	{
            if (ImGui::MenuItem("Open...")) OpenNodes();
            if (ImGui::MenuItem("Save As...")) SaveNodes();
    		ImGui::EndMenu();
    	}
        if (ImGui::MenuItem("Compile")) CompileNodes();

		if (ImGui::BeginMenu("View"))
		{
            ImGui::MenuItem("Hide Unconnected Pins", "", &m_HideUnconnected);
            ImGui::Separator();
			if (ImGui::MenuItem("Zoom to Content"))
				ed::NavigateToContent();
			if (ImGui::MenuItem("Show Flow"))
			{
				for (auto& link : m_Links)
					ed::Flow(link.ID);
			}
			ImGui::EndMenu();
		}
    	ImGui::EndMenuBar();
        ImGui::End();
        
    	ImGui::SetNextWindowClass(&windowClass);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
    	ImGui::Begin("Node Graph", NULL, ImGuiWindowFlags_NoNavFocus);
        ImGui::PopStyleVar();
    	UpdateTouch();
    
    	std::vector<ed::NodeId> selectedNodes;
    	std::vector<ed::LinkId> selectedLinks;
    	selectedNodes.resize(ed::GetSelectedObjectCount());
    	selectedLinks.resize(ed::GetSelectedObjectCount());
    
    	int nodeCount = ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
    	int linkCount = ed::GetSelectedLinks(selectedLinks.data(), static_cast<int>(selectedLinks.size()));
    
    	selectedNodes.resize(nodeCount);
    	selectedLinks.resize(linkCount);
    
    	ed::SetCurrentEditor(m_Editor);
    
# if 0
    	{
    	    for (auto x = -io.DisplaySize.y; x < io.DisplaySize.x; x += 10.0f)
    	    {
    	        ImGui::GetWindowDrawList()->AddLine(ImVec2(x, 0), ImVec2(x + io.DisplaySize.y, io.DisplaySize.y),
    	            IM_COL32(255, 255, 0, 255));
    	    }
    	}
# endif

		auto var = GetVariableById(m_SelectedVariable);
		if (var != nullptr)
		{
			const ImGuiID id = ImGui::GetID("##AddVariablePopup");
			if (m_OpenVarPopup) { ImGui::OpenPopupEx(id, ImGuiPopupFlags_None); m_OpenVarPopup = false; }

			if (ImGui::BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings))
			{
				ImGui::SetWindowPos(ImVec2(Dymatic::Input::GetMousePosition().x, Dymatic::Input::GetMousePosition().y), ImGuiCond_Appearing);
				ImGui::TextDisabled(var->Name.c_str());
				ImGui::Separator();
                if (ImGui::MenuItem(("Get " + var->Name).c_str(), "Ctrl")) { NodeEditorInternal::SpawnGetVariableNode({ var->ID }); m_ResetBackNode = true; }
                else if (ImGui::MenuItem(("Set " + var->Name).c_str(), "Alt")) { NodeEditorInternal::SpawnSetVariableNode({ var->ID }); m_ResetBackNode = true; }
				ImGui::EndPopup();
			}
			else m_OpenVarPopup = false;
		}
        
        ed::Begin("Node Editor Panel");
        {
            auto cursorTopLeft = ImGui::GetCursorScreenPos();
        
            util::BlueprintNodeBuilder builder(reinterpret_cast<void*>(m_HeaderBackground->GetRendererID()), m_HeaderBackground->GetWidth(), m_HeaderBackground->GetHeight());

            // Drag Drop Target
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("NODE_EDITOR_VARIABLE"))
				{
					int index = *(int*)payload->Data;
                    m_SelectedVariable = m_Variables[index].ID;

					if (io.KeyCtrl)
					{
						SpawnGetVariableNode({ m_Variables[index].ID });
						ed::SetNodePosition(m_Nodes.back().ID, ImGui::GetMousePos());
                        BuildNodes();
					}
					else if (io.KeyAlt)
					{
						SpawnSetVariableNode({ m_Variables[index].ID });
						ed::SetNodePosition(m_Nodes.back().ID, ImGui::GetMousePos());
                        BuildNodes();
					}
					else
					{
                        m_OpenVarPopup = true;
					}
				}
				ImGui::EndDragDropTarget();
			}

            if (m_ResetBackNode)
            {
                ed::SetNodePosition(m_Nodes.back().ID, ImGui::GetMousePos());
                BuildNodes();
                m_ResetBackNode = false;
            }
        
            for (auto& node : m_Nodes)
            {
                if (node.Type != NodeType::Blueprint && node.Type != NodeType::Simple)
                    continue;
        
                Variable* payloadVariable = nullptr;
                Pin* payloadPin = nullptr;

                const auto isSimple = node.Type == NodeType::Simple;
        
                bool hasOutputDelegates = false;
                for (auto& output : node.Outputs)
                    if (output.Data.Type == PinType::Delegate)
                        hasOutputDelegates = true;

                builder.Begin(node.ID);
                    if (!isSimple)
                    {
                        builder.Header(node.Color);
                            ImGui::Spring(0);

                            if (node.Icon != nullptr)
                                ImGui::Image((ImTextureID)node.Icon->GetRendererID(), ImVec2(ImGui::GetTextLineHeight() * 1.25f, ImGui::GetTextLineHeight() * 1.25f), { 0, 1 }, { 1, 0 }, node.IconColor);

                            ImGui::TextUnformatted((node.DisplayName == "" ? node.Name : node.DisplayName).c_str());
                            ImGui::Spring(1);
                            ImGui::Dummy(ImVec2(0, 28));
                            if (hasOutputDelegates)
                            {
                                ImGui::BeginVertical("delegates", ImVec2(0, 28));
                                ImGui::Spring(1, 0);
                                for (auto& output : node.Outputs)
                                {
                                    if (output.Data.Type != PinType::Delegate)
                                        continue;
        
                                    auto alpha = ImGui::GetStyle().Alpha;
                                    if (m_NewLinkPin && !CanCreateLink(m_NewLinkPin, &output) && &output != m_NewLinkPin)
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
                                    DrawPinIcon(output.Data, IsPinLinked(output.ID), (int)(alpha * 255));
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
                        if (!m_HideUnconnected || IsPinLinked(input.ID))
                        {
                            auto alpha = ImGui::GetStyle().Alpha;
                            if (m_NewLinkPin && !CanCreateLink(m_NewLinkPin, &input) && &input != m_NewLinkPin)
                                alpha = alpha * (48.0f / 255.0f);

                            builder.Input(input.ID);
                            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                            DrawPinIcon(input.Data, IsPinLinked(input.ID), (int)(alpha * 255));
                            ImGui::Spring(0);
                            if (!input.Name.empty())
                            {
                                ImGui::TextUnformatted(input.Name.c_str());
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
                                    if (m_Variables[*(int*)payload->Data].Data.Type == input.Data.Type || input.Data.Type == PinType::Flow)
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
                        ImGui::TextUnformatted((node.DisplayName == "" ? node.Name : node.DisplayName).c_str());
                        ImGui::Spring(1, 0);
                    }
        
                    for (auto& output : node.Outputs)
					{
                        if (!m_HideUnconnected || IsPinLinked(output.ID))
                        {
                            if (!isSimple && output.Data.Type == PinType::Delegate)
                                continue;

                            auto alpha = ImGui::GetStyle().Alpha;
                            if (m_NewLinkPin && !CanCreateLink(m_NewLinkPin, &output) && &output != m_NewLinkPin)
                                alpha = alpha * (48.0f / 255.0f);

                            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                            builder.Output(output.ID);
                            if (!output.Name.empty())
                            {
                                ImGui::Spring(0);
                                ImGui::TextUnformatted(output.Name.c_str());
                            }
                            ImGui::Spring(0);
                            DrawPinIcon(output.Data, IsPinLinked(output.ID), (int)(alpha * 255));
                            ImGui::PopStyleVar();
                            builder.EndOutput();

                            if (ImGui::IsItemHovered() && ImGui::IsMouseDown(0))
                                m_CurrentLinkColor = GetIconColor(output.Data.Type);

                            // Drag Drop Variable Outputs
                            if (ImGui::BeginDragDropTarget())
                            {
                                auto payload = ImGui::GetDragDropPayload();
                                if (strcmp(payload->DataType, "NODE_EDITOR_VARIABLE") == 0)
                                    if (m_Variables[*(int*)payload->Data].Data.Type == output.Data.Type || output.Data.Type == PinType::Flow)
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
                                node.Outputs.emplace_back(GetNextId(), pinName.c_str(), PinType::Flow);
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
                else if (ImGui::IsItemHovered() || node.Opacity > -1.0f)
                {
					auto zoom = node.CommentPinned ? ed::GetCurrentZoom() : 1.0f;
					auto padding = ImVec2(2.5f, 2.5f);

                    auto min = ed::GetNodePosition(node.ID) - ImVec2(0, padding.y * zoom * 2.0f);

                    bool nodeHovered = ImGui::IsItemHovered();

                    ImGui::SetCursorScreenPos(min - ImVec2(-8, ImGui::GetTextLineHeightWithSpacing() + 4));
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, node.Opacity);
                    if (ImGui::ImageButton((ImTextureID)m_CommentIcon->GetRendererID(), ImVec2(15.0f, 15.0f) * zoom, { 0, 1 }, { 1, 0 }, 2 * zoom)) { node.Opacity = -2.5f; node.CommentEnabled = true; }
                    ImGui::PopStyleVar();

					if (ImGui::IsItemHovered() || nodeHovered)
						node.Opacity = std::min(node.Opacity + 0.1f, 1.0f);
					else
						node.Opacity = std::max(node.Opacity - 0.1f, -2.5f);
                }
                else if (node.Opacity > -2.5f)
                    node.Opacity = std::max(node.Opacity - 0.1f, -2.5f);

                ImGui::PopID();

                if (payloadVariable != nullptr)
                {
                    auto pinId = payloadPin->ID;
                    auto nodeId = node.ID;

                    CheckLinkSafety(nullptr, payloadPin);

                    if (payloadPin->Kind == PinKind::Output || payloadPin->Data.Type == PinType::Flow)
                        SpawnSetVariableNode({ payloadVariable->ID });
                    else
                        SpawnGetVariableNode({ payloadVariable->ID });

                    ed::SetNodePosition(m_Nodes.back().ID, ImVec2(ed::GetNodePosition(nodeId).x + (payloadPin->Kind == PinKind::Input ? -250.0f : 250.0f), ImGui::GetMousePos().y));
					m_Links.emplace_back(Link(GetNextLinkId(), (payloadPin->Kind == PinKind::Input ? (m_Nodes.back().Outputs[0]) : (m_Nodes.back().Inputs[payloadPin->Data.Type == PinType::Flow ? 0 : 1])).ID, pinId));
					m_Links.back().Color = GetIconColor(payloadPin->Data.Type);

                    BuildNodes();
                }
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
                ed::Group(node.Size);
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
                
            }
        
            for (auto& link : m_Links)
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
                            else if (endPin->Data.Type != startPin->Data.Type || endPin->Data.Container != startPin->Data.Container)
                            {
                                spawnNodeFunction Function;
                                ConversionAvalible(startPin, endPin, Function);
                                if (Function != nullptr)
                                {
                                    showLabel("+ Create Conversion", ImColor(32, 45, 32, 180));
                                    if (ed::AcceptNewItem(ImColor(128, 206, 244), 4.0f))
                                    {
                                        auto startNodeId = startPin->Node->ID;
                                        auto endNodeId = endPin->Node->ID;
                                        auto startPinType = startPin->Data.Type;
                                        auto endPinType = endPin->Data.Type;

                                        CheckLinkSafety(startPin, endPin);

                                        Node* node = (this->*Function)({});

                                        ImVec2 pos = (ed::GetNodePosition(startNodeId) + ed::GetNodePosition(endNodeId)) / 2;
                                        ed::SetNodePosition(node->ID, pos);

                                        m_Links.emplace_back(Link(GetNextLinkId(), startPinId, node->Inputs[0].ID));
                                        m_Links.back().Color = GetIconColor(startPinType);

                                        m_Links.emplace_back(Link(GetNextLinkId(), node->Outputs[0].ID, endPinId));
                                        m_Links.back().Color = GetIconColor(endPinType);

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
                                {
                                    CheckLinkSafety(startPin, endPin);

                                    m_Links.emplace_back(Link(GetNextId(), startPinId, endPinId));
                                    m_Links.back().Color = GetIconColor(startPin->Data.Type);
                                }
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
                ImGui::Separator();
                if (pin->Data.Type != PinType::Flow)
                    if (ImGui::MenuItem("Promote To Variable"))
                    {
                        std::vector<std::string> vec;
                        for (auto& var : m_Variables)
                            vec.push_back(var.Name);
                        m_Variables.push_back({ GetNextId(), Dymatic::String::GetNextNameWithIndex(vec, pin->Name.empty() ? "NewVar" : pin->Name), pin->Data.Type });

                        auto pinId = pin->ID;

                        CheckLinkSafety(nullptr, pin);

                        bool in = pin->Kind == PinKind::Input;
                        ImVec2 pos = ed::GetNodePosition(pin->Node->ID) + ImVec2(in ? -250.0f : 250.0f, 0.0f);
                        Node* node;
                        if (in)
						    node = SpawnGetVariableNode({ m_Variables.back().ID });
                        else
						    node = SpawnSetVariableNode({ m_Variables.back().ID });
						ed::SetNodePosition(node->ID, pos);

						m_Links.emplace_back(Link(GetNextLinkId(), (in ? node->Outputs[0] : node->Inputs[1]).ID, pinId));
						m_Links.back().Color = GetIconColor(m_Variables.back().Data.Type);

                        BuildNodes();
                    }
                if (pin->Deletable)
                    if (ImGui::MenuItem("Remove Pin"))
                    {
                        for (auto& link : m_Links)
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
        
        ImGui::SetNextWindowSize(ImVec2(400.0f, 400.0f));
        if (ImGui::BeginPopup("Create New Node"))
        {
            Node* node = nullptr;
        
            ImGui::Text("All Actions for this File");
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(100.0f, 0.0f));
            ImGui::SameLine();
            if (ImGui::Custom::Checkbox("##NodeSearchPopupContextSensitiveCheckbox", &m_ContextSensitive))
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
        
            if (((m_SearchBuffer != m_PreviousSearchBuffer) || m_ResetSearchArea))
            {
                if (m_SearchBuffer == "") CloseSearchList(&m_SearchData);
                else OpenSearchList(&m_SearchData);
            }
        
            //node = DisplaySearchData(&m_SearchData, m_SearchBuffer);

            //ImGui::BeginChild("##NewNodeSearchDisplay", ImVec2(), false, ImGuiWindowFlags_NoNav);
            SearchResultData* data = DisplaySearchData(m_SearchData);
            if (data != nullptr)
            {
                if (data->Function)
                    node = (this->*(data->Function))(data->SpawnData);
                else
                {
                    for (auto& library : m_NodeLibraries)
                        for (auto& Function : library.m_FunctionDeclarations)
                            if (Function.ID == data->ID)
                            {
								m_Nodes.emplace_back(GetNextId(), Function.GetName().c_str(), Function.FUNC_Pure ? ImColor(170, 242, 172) : ImColor(128, 195, 248), NodeFunction::Node);
                                m_Nodes.back().Icon = m_FunctionIcon;
                                m_Nodes.back().IconColor = Function.FUNC_Pure ? ImColor(170, 242, 172) : ImColor(128, 195, 248);
                                m_Nodes.back().FunctionID = Function.ID;
                                m_Nodes.back().DisplayName = Function.FUNC_NodeDisplayName;
                                m_Nodes.back().Pure = Function.FUNC_Pure;
                                if (Function.FUNC_CompactNode) m_Nodes.back().Type = NodeType::Simple;

                                if (!Function.FUNC_Pure)
                                {
                                    m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
                                    m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);
                                }

                                if (Function.FUNC_Return.Data.Type != PinType::Void)
                                {
                                    m_Nodes.back().Outputs.emplace_back(GetNextId(), Function.FUNC_NoPinLabels ? "" : "Return Value", Function.FUNC_Return.Data.Type);
                                    m_Nodes.back().Outputs.back().ArgumentID = Function.FUNC_Return.ArgumentID;
                                }
                                for (auto& param : Function.FUNC_Params)
                                    if (param.Kind == PinKind::Input)
                                    {
                                        auto& input = m_Nodes.back().Inputs.emplace_back(param);
                                        input.ID = GetNextId();
                                        input.ArgumentID = param.ArgumentID;
                                        if (Function.FUNC_NoPinLabels) input.Name = "";
                                    }
                                    else
									{
										auto& output = m_Nodes.back().Outputs.emplace_back(param);
										output.ID = GetNextId();
										output.ArgumentID = param.ArgumentID;
										if (Function.FUNC_NoPinLabels) output.Name = "";
									}

								BuildNode(&m_Nodes.back());

								node = &m_Nodes.back();
                                break;
                            }
                }
            }
            //ImGui::EndChild();
        
            m_ResetSearchArea = false;
            m_PreviousSearchBuffer = m_SearchBuffer;
        
            if (node)
            {
                BuildNodes();
        
                m_CreateNewNode = false;

                if (node->Type != NodeType::Comment || selectedNodes.empty())
                    ed::SetNodePosition(node->ID, m_NewNodePosition);
        
                if (auto startPin = m_NewNodeLinkPin)
                {
                    auto& pins = startPin->Kind == PinKind::Input ? node->Outputs : node->Inputs;
        
                    if (startPin->Data.Type != PinType::Flow)
                        for (auto& pin : (startPin->Kind == PinKind::Input ? startPin->Node->Inputs : startPin->Node->Outputs))
                            if (pin.Data.Type == PinType::Flow)
                            {
                                for (auto& otherPin : pins)
                                    if (otherPin.Data.Type == PinType::Flow)
                                    {
										CheckLinkSafety(&pin, &otherPin);

										m_Links.emplace_back(Link(GetNextLinkId(), pin.ID, otherPin.ID));
										m_Links.back().Color = GetIconColor(pin.Data.Type);
                                        break;
                                    }
                                break;
                            }

                    for (auto& pin : pins)
                    {
                        if (CanCreateLink(startPin, &pin))
                        {
                            spawnNodeFunction Function;
                            ConversionAvalible(startPin, &pin, Function);
                            if (Function == nullptr)
                            {

                                auto endPin = &pin;
                                if (startPin->Kind == PinKind::Input)
                                    std::swap(startPin, endPin);

                                CheckLinkSafety(startPin, endPin);

                                m_Links.emplace_back(Link(GetNextLinkId(), startPin->ID, endPin->ID));
                                m_Links.back().Color = GetIconColor(startPin->Data.Type);

                                break;
                            }
                            else
                            {
                                auto endPin = &pin;
								if (startPin->Kind == PinKind::Input)
									std::swap(endPin, startPin);
                            
								auto startNodeId = startPin->Node->ID;
								auto endNodeId = endPin->Node->ID;
                            
                                CheckLinkSafety(startPin, endPin);
                            
								Node* node = (this->*Function)({});
								ed::SetNodePosition(node->ID, (ed::GetNodePosition(startNodeId) + ed::GetNodePosition(endNodeId)) / 2);
                            
								m_Links.emplace_back(Link(GetNextLinkId(), startPin->ID, node->Inputs[0].ID));
								m_Links.back().Color = GetIconColor(startPin->Data.Type);
                                
								m_Links.emplace_back(Link(GetNextLinkId(), node->Outputs[0].ID, endPin->ID));
								m_Links.back().Color = GetIconColor(endPin->Data.Type);

                                BuildNodes();

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

        ImGui::End();
    
    	ImGui::SetNextWindowClass(&windowClass);
    	ImGui::Begin("Nodes", NULL, ImGuiWindowFlags_NoNavFocus);
    	ShowLeftPane(ImGui::GetWindowSize().x);
    	ImGui::End();

        // Script Contents Section
		ImGui::SetNextWindowClass(&windowClass);
		ImGui::Begin("Script Contents", NULL, ImGuiWindowFlags_NoNavFocus);

		if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
			m_SelectedVariable = 0;
		
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
		ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		bool open = ImGui::TreeNodeEx("##VariablesList", treeNodeFlags, "VARIABLES");
		ImGui::PopStyleVar();
		ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
		if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
		{
			std::vector<std::string> vec;
			for (auto& var : m_Variables)
				vec.push_back(var.Name);

            m_Variables.emplace_back(Variable(GetNextId(), Dymatic::String::GetNextNameWithIndex(vec, "NewVar"), PinType::Bool));
		}
        if (open)
        {
			for (int i = 0; i < m_Variables.size(); i++)
		    {
                auto& variable = m_Variables[i];

		        ImGui::PushID(variable.ID);
		    	ImGuiTreeNodeFlags flags = ((variable.ID == m_SelectedVariable) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
		    
		    	char buffer[256];
		    	memset(buffer, 0, sizeof(buffer));
		    	std::strncpy(buffer, variable.Name.c_str(), sizeof(buffer));
		        bool input;

				bool clicked = ImGui::SelectableInput("##VariableTree", ImGui::GetContentRegionAvail().x, m_SelectedVariable == variable.ID, flags, buffer, sizeof(buffer), input);
                
                if (ImGui::IsItemHovered() && variable.Tooltip != "")
                    ImGui::SetTooltip(variable.Tooltip.c_str());
		        if (input) variable.Name = std::string(buffer);

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("NODE_EDITOR_VARIABLE", &i, sizeof(i), ImGuiCond_Once);
					ImGui::Text(variable.Name.c_str());
					ImGui::EndDragDropSource();
				}

                if (clicked) m_SelectedVariable = variable.ID;
		    
		    	if (ImGui::BeginPopupContextItem())
		    	{
		            if (ImGui::MenuItem("Duplicate"))
		            {
		                m_Variables.push_back(variable);
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
                    if (ImGui::MenuItem("Delete")) { m_Variables.erase(m_Variables.begin() + i); m_SelectedVariable = 0; }
		    
		    		ImGui::EndPopup();
		    	}

				ImGui::SameLine();
				DrawTypeIcon(variable.Data.Container, variable.Data.Type);
				ImGui::Dummy(ImVec2(m_PinIconSize, m_PinIconSize));

		        ImGui::PopID();
		    }

            ImGui::TreePop();
        }

		ImGui::End();

		// Details Section
		ImGui::SetNextWindowClass(&windowClass);
		ImGui::Begin("Details", NULL, ImGuiWindowFlags_NoNavFocus);

        float columnWidth = ImGui::GetWindowWidth() / 3.0f;
        if (m_SelectedVariable != 0)
        {
            auto variable = GetVariableById(m_SelectedVariable);
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
                bool open = ImGui::TreeNodeEx("##VariableInfo", treeNodeFlags, "VARIABLE");
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
					std::strncpy(buffer, variable->Name.c_str(), sizeof(buffer));
                    ImGui::InputText("##VariableName", buffer, sizeof(buffer));
					if ((ImGui::IsItemActivePreviousFrame() && !ImGui::IsItemActive()) || (ImGui::IsItemActive() && ImGui::IsKeyPressed(io.KeyMap[ImGuiKey_Enter])))
					{
                        SetVariableName(variable, buffer);
					}
                    ImGui::Columns(1);

					ImGui::Columns(2);
					ImGui::SetColumnWidth(0, columnWidth);
                    ImGui::Text("Variable Type");
                    ImGui::NextColumn();
                    ImGui::SetNextItemWidth(-20);
                    auto& cpos1 = ImGui::GetCursorPos();
                    if (ImGui::BeginCombo("##VariableType", ("       " + ConvertPinTypeToString(variable->Data.Type)).c_str()))
                    {
                        auto& cpos1 = ImGui::GetCursorPos(); if (ImGui::MenuItem("        Boolean")) SetVariableType(variable, PinType::Bool); auto& cpos2 = ImGui::GetCursorPos(); ImGui::SetCursorPos(cpos1); DrawTypeIcon(ContainerType::Array, PinType::Bool); ImGui::SetCursorPos(cpos2);
						if (ImGui::MenuItem("    Integer")) SetVariableType(variable, PinType::Int);
						if (ImGui::MenuItem("    Float"))   SetVariableType(variable, PinType::Float);
						if (ImGui::MenuItem("    String"))  SetVariableType(variable, PinType::String);
						if (ImGui::MenuItem("    Vector"))  SetVariableType(variable, PinType::Vector);
						if (ImGui::MenuItem("    Color"))   SetVariableType(variable, PinType::Color);
                        ImGui::EndCombo();
                    }
                    auto& cpos2 = ImGui::GetCursorPos(); ImGui::SetCursorPos(cpos1); DrawTypeIcon(variable->Data.Container, variable->Data.Type); ImGui::SetCursorPos(cpos2);
                    ImGui::SameLine();
                    DrawTypeIcon(variable->Data.Container, variable->Data.Type);
                    ImGui::Dummy(ImVec2(m_PinIconSize, m_PinIconSize));
                    if (ImGui::IsItemClicked())
                        ImGui::OpenPopup("##UpdateVariableContainer");
                    ImGui::Columns(1);

					ImGui::Columns(2);
					ImGui::SetColumnWidth(0, columnWidth);
					ImGui::Text("Tooltip");
                    ImGui::NextColumn();
                    ImGui::SetNextItemWidth(-1);
					char tooltipBuffer[256];
					memset(tooltipBuffer, 0, sizeof(tooltipBuffer));
					std::strncpy(tooltipBuffer, variable->Tooltip.c_str(), sizeof(tooltipBuffer));
					if (ImGui::InputText("##VariableTooltip", tooltipBuffer, sizeof(tooltipBuffer)))
					{
						variable->Tooltip = std::string(tooltipBuffer);
					}
                    ImGui::Columns(1);

                    ImGui::TreePop();
                }
            }

            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
                bool open = ImGui::TreeNodeEx("##DefaultValue", treeNodeFlags, "DEFAULT VALUE");
                ImGui::PopStyleVar();
                if (open)
                {
					ImGui::Columns(2);
					ImGui::SetColumnWidth(0, columnWidth);
                    ImGui::Text(variable->Name.c_str());
                    ImGui::NextColumn();
                    ImGui::PushID(variable->ID);
                    DefaultValueInput(variable->Data);
                    ImGui::PopID();
                    ImGui::TreePop();
                    ImGui::Columns(1);
                }
            }
        }

        ImGui::End();

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
                {
                    ed::SelectNode(result.ID);
                    ed::NavigateToSelection();
                }
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
                ImGui::Text("No Results found");
            else
                for (auto& Function : m_FindResultsData)
                {
                    ImGui::PushID(Function.ID.Get());

					auto& cursorPos = ImGui::GetCursorScreenPos();
				    auto fontSize = ImGui::GetFontSize();

                    ImGui::Indent();
                    if (ImGui::TreeNodeEx("##FindResultsItem", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | (Function.SubData.empty() ? ImGuiTreeNodeFlags_Leaf : ImGuiTreeNodeFlags_None), Function.Name.c_str()))
                    {
                        for (auto& pin : Function.SubData)
                        {
                            ImGui::PushID(pin.ID.Get());

							auto& cursorPos = ImGui::GetCursorScreenPos();
							auto fontSize = ImGui::GetFontSize();

                            if (ImGui::TreeNodeEx("##FindResultsItem", ImGuiTreeNodeFlags_Leaf, pin.Name.c_str()))
                                ImGui::TreePop();

                            if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
                            {
                                ed::ClearSelection();
                                ed::SelectNode(pin.ID);
                                ed::NavigateToSelection();
                            }

							drawList->AddCircleFilled(cursorPos + ImVec2(fontSize / 2.0f, fontSize / 2.0f), fontSize / 4.0f, pin.Color);

                            ImGui::PopID();
                        }
                        ImGui::TreePop();
                    }
                    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
                    {
                        ed::ClearSelection();
                        ed::SelectNode(Function.ID);
                        ed::NavigateToSelection();
                    }
                    ImGui::Unindent();

                    drawList->AddRectFilled(cursorPos + ImVec2(fontSize * 0.15f, fontSize * 0.25f), cursorPos + ImVec2(fontSize * 0.85f, fontSize * 0.85f), Function.Color, fontSize / 4.0f);

                    ImGui::PopID();
                }

            ImGui::End();
        }
    }
}

Variable* NodeEditorInternal::GetVariableById(unsigned int id)
{
    for (auto& variable : m_Variables)
        if (variable.ID == id)
            return &variable;
    return nullptr;
}

void NodeEditorInternal::SetVariableType(Variable* variable, PinType type)
{
    if (variable != nullptr)
    {
        variable->Data.Type = type;
            for (auto& node : m_Nodes)
                if (node.VariableId == variable->ID)
                {
                    for (auto& input : node.Inputs) if (input.Data.Type != PinType::Flow) input.Data.Type = type;
                    for (auto& output : node.Outputs) if (output.Data.Type != PinType::Flow) output.Data.Type = type;
                }
    }
}

void NodeEditorInternal::SetVariableName(Variable* variable, const std::string name)
{
	if (variable != nullptr)
	{
		variable->Name = name;
		for (auto& node : m_Nodes)
			if (node.VariableId == variable->ID)
			{
                node.Name =         (node.Type == NodeType::Simple ? "DymaticVariable_Get " : "DymaticVariable_Set ") + name;
                node.DisplayName =  (node.Type == NodeType::Simple ? "Get " : "Set ") + name;
			}
	}
}

void NodeEditorInternal::FindResultsSearch(const std::string& search)
{
	m_FindResultsSearchBar = search;
	m_FindResultsData.clear();

	for (auto& node : m_Nodes)
	{
		bool created = node.Name.find(m_FindResultsSearchBar) != std::string::npos || node.DisplayName.find(m_FindResultsSearchBar) != std::string::npos;
		if (created)
			m_FindResultsData.push_back({ node.Name, node.Color, node.ID });

		for (auto& pin : node.Inputs, node.Outputs)
			if (pin.Name.find(m_FindResultsSearchBar) != std::string::npos)
			{
				if (!created)
					m_FindResultsData.push_back({ node.Name, node.Color, node.ID });
				m_FindResultsData.back().SubData.push_back({ pin.Name, GetIconColor(pin.Data.Type), node.ID });
			}
	}
}

void NodeEditorInternal::LoadNodeLibrary(const std::filesystem::path& path)
{
	std::string result;
	std::ifstream in(path, std::ios::in | std::ios::binary); // ifstream closes itself due to RAII
	if (in)
	{
		in.seekg(0, std::ios::end);
		size_t size = in.tellg();
		if (size != -1)
		{
			result.resize(size);
			in.seekg(0, std::ios::beg);
			in.read(&result[0], size);
		}
		else
		{
			DY_CORE_ERROR("Could not read from node library file '{0}'", path);
			return;
		}
	}

    // Begin Data Parsing //

    // Create New Library
    m_NodeLibraries.push_back({GetNextId(), "Node Library"});
    auto& nodeLibrary = m_NodeLibraries.back();

	// Setup Word Buffer and separate words
    std::string buffer = "";
	bool characterWordState = false;
	std::vector<std::string> words;

	for (auto& character : result)
	{
		bool newWord = IsCharacterNewWord(character);
		if (newWord != characterWordState || newWord)
		{
			words.push_back(buffer);

			// Update
			buffer = "";
			characterWordState = newWord;
		}
		buffer += character;
	}

    // Remove Unnecessary Words
    {
        //bool commentOpen = false;
        bool quoteOpen = false;
        for (int i = 0; i < words.size(); i++)
        {

			//// ------------//
			//if (i < words.size() - 2)
			//    if (words[i] == "/" && words[i + 1] == "*" && words[i + 2] == "*") { commentOpen = true; }
			//if (i < words.size() - 1)
			//{
			//    if (words[i] == "/" && words[i + 1] == "*") { commentOpen = true; }
			//    if (words[i] == "*" && words[i + 1] == "/") { commentOpen = false; }
			//}
			//// ------------//
			//
			//if (!commentOpen)

            if (words[i] == "\"")
                quoteOpen = !quoteOpen;

            if (!quoteOpen)
            {
                if (words[i] == " " || words[i] == "\t" || words[i] == "\r" || words[i] == "\n")
                {
                    words.erase(words.begin() + i);
                    i--;
                }
            }
        }
    }

	// Comments
	bool commentOpen = false;
	bool metaCommentOpen = false;

	// DYFUNCTION Data
    int bracketScope = 0;
	bool dyfunctionOpen = false;

	// Function Declaration
	bool readingFunction = false;
	bool openArgs = false;

    // Args
    bool markedArgName = false;
    
    struct metaList
    {
        std::string name;
        std::string tooltip;
    };

    std::vector<metaList> paramMetaList;

	// Iterate through words
	for (int i = 0; i < words.size(); i++)
	{
		auto& word = words[i];

        if (i < words.size() - 2)
            if (word == "/" && words[i + 1] == "*" && words[i + 2] == "*") { metaCommentOpen = true; i += 2; continue; }
        if (i < words.size() - 1)
        {
            if (word == "/" && words[i + 1] == "*") { commentOpen = true; i++; continue; }
            if (word == "*" && words[i + 1] == "/") { commentOpen = false; metaCommentOpen = false; i++; continue; }
        }

        if (commentOpen) {}
		else if (metaCommentOpen && !nodeLibrary.m_FunctionDeclarations.empty())
		{
            auto& Function = nodeLibrary.m_FunctionDeclarations.back();
            if (word.find("*") != std::string::npos)
            {
                if (words[i + 1].find("@") != std::string::npos)
                {
                    i++;
                    if (words[i + 1].find("param") != std::string::npos)
                    {
                        i++;
                        paramMetaList.push_back({});
                        paramMetaList.back().name = words[i + 1];
                        i+= 2;
						while (true)
						{
							if (words[i].find("*") == std::string::npos)
							{
                                paramMetaList.back().tooltip += words[i] + " ";
								i++;
							}
							else break;
						}
                        i--;
						continue;
                    }
                }
                else
                {
                    i++;
                    while (true)
                    {
                        if (words[i].find("*") == std::string::npos)
                        {
                            Function.FUNC_Tooltip += words[i] + " ";
                            i++;
                        }
                        else break;
                    }
                    Function.FUNC_Tooltip += "\n";
                    i--;
                    continue;
                }
            }
        }
		else
		{
			if (word == "DYFUNCTION")
			{
				nodeLibrary.m_FunctionDeclarations.push_back({GetNextId()});
                nodeLibrary.m_FunctionDeclarations.back().FUNC_Return.ArgumentID = GetNextId();
                nodeLibrary.m_FunctionDeclarations.back().FUNC_Return.Name = "Return Value";
                if (words[i + 1] == "(")
                {
                    readingFunction = true;
                    dyfunctionOpen = true;
                    bracketScope = -1;
                }
			}
            else if (readingFunction)
            {
                auto& Function = nodeLibrary.m_FunctionDeclarations.back();

                if (dyfunctionOpen)
                {
                    if (word.find("(") != std::string::npos)
                        bracketScope++;

                    if (words[i - 1].find(",") != std::string::npos || words[i - 1].find("(") != std::string::npos)
                    {
                        // Current word is keyword
                        std::string value;
                        if (words[i + 1].find("=") != std::string::npos)
                        {
                            i += 2;
                            for (; i < words.size(); i++)
                            {
                                if (words[i].find("(") != std::string::npos)
                                    bracketScope++;
                                if ((words[i].find(",") != std::string::npos || words[i].find(")") != std::string::npos) && !bracketScope)
                                    break;
                                if (words[i].find(")") != std::string::npos)
                                    bracketScope--;
                                value += words[i];
                            }
                            i--;
                        }
                        
                        // Execute Action Based on command
                        if (word == "Keywords") { Dymatic::String::EraseAllOfCharacter(value, '"'); String::SplitStringByDelimiter(value, Function.FUNC_Keywords, ' '); }
                        else if (word == "Category") { Dymatic::String::EraseAllOfCharacter(value, '"'); Function.FUNC_Category = value; }
                        else if (word == "Pure") { Function.FUNC_Pure = true; }
                        else if (word == "DisplayName") { Dymatic::String::EraseAllOfCharacter(value, '"'); Function.FUNC_DisplayName = value; }
                        else if (word == "CompactNodeTitle") { Dymatic::String::EraseAllOfCharacter(value, '"'); Function.FUNC_NodeDisplayName = value; Function.FUNC_CompactNode = true; }
                        else if (word == "NoPinLabels") { Dymatic::String::EraseAllOfCharacter(value, '"'); Function.FUNC_NoPinLabels = true; }
                        else DY_WARN("DYFUNCTION keyword '{0}' is undefined", word);
                    }

                    if (word.find(")") != std::string::npos)
                    {
                        if (!bracketScope)
                            dyfunctionOpen = false;
                        else
                            bracketScope--;
                    }
                }
                else if (openArgs)
                {
					if (word.find(")") != std::string::npos)
						openArgs = false;
                    else if (word.find(",") != std::string::npos)
                    {
                        Function.FUNC_Params.push_back({});
                        Function.FUNC_Params.back().ArgumentID = GetNextId();
                        markedArgName = false;
                    }
                    else if (!markedArgName && (words[i + 1].find(",") != std::string::npos || words[i + 1].find(")") != std::string::npos || words[i + 1].find("=") != std::string::npos))
                    {
                        auto& param = Function.FUNC_Params.back();
                        markedArgName = true;

                        // Set Parameter Name
                        param.Name = word;

                        // Set Parameter Type
						int typeOffset = 1;
						while (true)
						{
							if (words[i - typeOffset] == "&")
							{
                                param.Data.Reference = true;
								typeOffset++;
							}
							else if (words[i - typeOffset] == "*")
							{
								param.Data.Pointer = true;
								typeOffset++;
							}
							else if (!isalnum(words[i - typeOffset][0]))
								typeOffset++;
							else
								break;
						}
                        param.Data.Type = ConvertStringToPinType(words[i - typeOffset]);

                        // Get Default Value
                        if (words[i + 1] == "=")
                        {
                            int offset = 2;
                            std::string defaultValue = "";
                            while (true)
                            {
                                if (words[i + offset].find(",") == std::string::npos && words[i + offset].find(")") == std::string::npos)
                                {
                                    defaultValue += words[i + offset];
                                    offset++;
                                }
                                else break;
                            }
                            param.Data.Value = ConvertStringToPinValue(param.Data.Type, defaultValue);
                        }
                    }
                    else if (word.length() > 1)
                    {
                        if (Function.FUNC_Params.empty())
                        {
                            Function.FUNC_Params.push_back({});
                            Function.FUNC_Params.back().ArgumentID = GetNextId();
                            markedArgName = false;
                        }

                        auto& param = Function.FUNC_Params.back();
                        if (word == "static") param.Data.Static = true;
                        if (word == "const") param.Data.Const = true;
                    }
                }
                else
                {

                    if (word == "static") { Function.FUNC_Static = true; Function.FUNC_Return.Data.Static = true; }
                    if (word == "const") { Function.FUNC_Const = true; Function.FUNC_Return.Data.Const = true; }

                    if (word.find("(") != std::string::npos)
                    {
                        // Grab Namespace
                        // Grab Is Reference

                        int typeOffset = 2;
                        while (true)
                        {
							if (words[i - typeOffset] == "&")
							{
                                Function.FUNC_Return.Data.Reference = true;
								typeOffset++;
							}
							else if (words[i - typeOffset] == "*")
							{
								Function.FUNC_Return.Data.Pointer = true;
								typeOffset++;
							}
							else if (!isalnum(words[i - typeOffset][0]))
								typeOffset++;
							else
								break;
                        }
                        Function.FUNC_Name = words[i - 1];
                        Function.FUNC_Return.Data.Type = ConvertStringToPinType(words[i - typeOffset]);

                        openArgs = true;
                    }

                    if (word.find(";") != std::string::npos)
                    {
                        for (auto& param : Function.FUNC_Params)
                            if (param.Data.Reference && !param.Data.Const)
                                param.Kind = PinKind::Output;
                            else
                                param.Kind = PinKind::Input;
                        // Commit Function
                        readingFunction = false;

                        // Write Meta Data
                        for (auto& meta : paramMetaList)
                            for (auto& param : Function.FUNC_Params)
                                if (param.Name == meta.name)
                                    param.Tooltip = meta.tooltip;
                        paramMetaList.clear();
                    }
                }
            }
		}
	}
}
}