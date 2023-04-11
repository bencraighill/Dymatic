#include "MaterialEditor.h"

#include "Dymatic/Renderer/Texture.h"
#include "Dymatic/Math/StringUtils.h"

#include <imgui/imgui.h>
#include "NodeUtilities/builders.h"
#include "NodeUtilities/widgets.h"

#include <ImGuiNode/imgui_node_editor.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>

#include "../../TextSymbols.h"

#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <utility>

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

	class MaterialEditorInternal
	{
	public:
		enum class PinType
		{
			None = 0,

			Bool,

			FloatFamily,
			Float,
			Float2,
			Float3,
			Float4
		};

		enum class PinKind
		{
			Output,
			Input
		};

		enum class NodeType
		{
			Blueprint = 0,
			Simple,
			Comment
		};

		enum class NodeFunction
		{
			Math = 0,
			Result,
			Constant
		};
		
		union PinData
		{
			bool Bool;

			float Float;
			glm::vec2 Float2;
			glm::vec3 Float3;
			glm::vec4 Float4;
		};

		struct Node;

		struct Pin
		{
			ed::PinId ID;
			Node* Node = nullptr;
			std::string Name;
			PinKind Kind;
			PinType Type;
			PinData Data;

			Pin() = default;
			Pin(uint64_t id, const std::string& name, PinType type = PinType::Float, PinKind kind = PinKind::Input) :
				ID(id), Name(name), Type(type), Kind(kind)
			{
			}
		};

		struct Node
		{
			Node() = default;
			Node(uint64_t id, const std::string& name, const ImColor& color, NodeFunction function = NodeFunction::Math)
				: ID(id), Name(name), Color(color), Function(function)
			{
			}

			ed::NodeId ID;
			std::string Name;
			std::string DisplayName;

			Ref<Texture2D> Icon;
			ImColor IconColor = ImColor(255, 255, 255);

			std::vector<Pin> Inputs;
			std::vector<Pin> Outputs;
			ImColor Color;
			NodeType Type = NodeType::Blueprint;
			NodeFunction Function = NodeFunction::Math;

			ImVec2 Position = {};
			ImVec2 Size = ImVec2(0, 0);
			ImRect GroupBounds;

			bool Internal = false;

			std::string State;
			std::string SavedState;

			// Node Comment Data
			bool CommentEnabled = false;
			bool CommentPinned = false;
			std::string Comment;

			// Compile Time Data
			bool Written = false;
			bool Error = false;
		};

		struct Link
		{
			ed::LinkId ID;

			ed::PinId StartPinID;
			ed::PinId EndPinID;

			ImColor Color;

			Link() = default;

			Link(const ed::LinkId& id, const ed::PinId& startPinId, const ed::PinId& endPinId)
				: ID(id), StartPinID(startPinId), EndPinID(endPinId), Color(255, 255, 255)
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
			SearchResultData(const std::string& name, std::function<Node* ()> callback)
				: Name(name), Callback(callback)
			{
			}

			std::string Name;
			std::function<Node* ()> Callback;
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

			void AddResult(const std::string& name, std::function<Node* ()> callback)
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

		class MaterialCompiler
		{
		public:
			MaterialCompiler() = default;

			void NewLine()
			{
				m_Buffer += '\n';
				m_IndentedCurrentLine = false;
			}
			
			void Write(const std::string& text)
			{
				if (!m_IndentedCurrentLine)
				{
					m_IndentedCurrentLine = true;
					for (int i = 0; i < m_IndentLevel; i++)
						m_Buffer += '\t';
				}
				m_Buffer += text;
			}

			void WriteLine(const std::string& line)
			{
				Write(line);
				NewLine();
			}

			std::string UnderscoreSpaces(std::string inString)
			{
				for (int i = 0; i < inString.size(); i++)
					if (inString[i] == ' ')
						inString[i] = '_';
				return inString;
			}

			inline void Indent(int level = 1) { m_IndentLevel += level; }
			inline void Unindent(int level = 1) { m_IndentLevel -= level; }
			inline int& GetIndentation() { return m_IndentLevel; }

			inline void OpenScope() { WriteLine("{"); Indent(); }
			inline void CloseScope(bool semicolon = false) { Unindent(); WriteLine(semicolon ? "};" : "}"); }

			void OutputToFile(const std::filesystem::path& path)
			{
				std::ofstream fout(path);
				fout << m_Buffer;
			}

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
		
	public:
		MaterialEditorInternal()
		{
			Init();
		}
		
		
		// Touch
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
		bool AreTypesCompatible(const PinType& a, const PinType& b);
		void BuildNode(Node* node);

		// Utiltity Nodes
		Node* SpawnResultNode();
		Node* SpawnComment();
		
		// Math Nodes
		Node* SpawnAddNode();
		Node* SpawnSubtractNode();

		void BuildNodes();
		ImColor GetIconColor(const std::string& type);
		void DrawPinIcon(bool connected, int alpha);
		void DrawTypeIcon(const std::string& type);

		void OnEvent(Dymatic::Event& e);
		bool OnKeyPressed(Dymatic::KeyPressedEvent& e);

		// Node Compilation
		void CompileNodes();
		std::string UnderscoreSpaces(std::string inString);
		std::string GenerateDataString(const PinType& type, const PinData& data);
		std::string GenerateTypeString(const PinType& type);
		std::string GenerateNodeSignature(const Node& node);
		void RecursivePinWrite(Pin& pin, MaterialCompiler& compiler, std::string& line);
		void RecursiveNodeWrite(Node& node, MaterialCompiler& compiler);

		void CopyNodes();
		void PasteNodes();
		void DuplicateNodes();
		void DeleteNodes();

		void ShowFlow();

		void ClearSelection();
		void SetNodePosition(ed::NodeId id, ImVec2 position);
		void NavigateToContent();
		void NavigateToNode(ed::NodeId id);

		void DefaultValueInput(PinData& data, const PinType& type, bool spring = false);

		void DeleteAllPinLinkAttachments(Pin* pin);
		void CheckLinkSafety(Pin* startPin, Pin* endPin);
		void CreateLink(Pin* a, Pin* b);

		void UpdateSearchData();
		void AddSearchData(const std::string& name, const std::vector<std::string>& categories, const std::string& keywords, const std::vector<PinType>& inputs, const std::vector<PinType>& outputs, std::function<Node* ()> callback);
		Node* DisplaySearchData(SearchData& searchData, bool origin = true);

		inline void ResetSearchArea()
		{
			m_ResetSearchArea = true;
			m_SearchBuffer = "";
			m_PreviousSearchBuffer = "";
			m_NewNodeLinkPin = nullptr;
		}

		void FindResultsSearch(const std::string& search);

		void Init();
		void OnImGuiRender();


	private:
		bool m_NodeEditorVisible = false;

		std::string m_ScriptClass;

		ImColor m_CurrentLinkColor = ImColor(255, 255, 255);

		const int m_PinIconSize = 24;
		int m_CurrentGraphID = 0;
		int m_CurrentWindowID = 0;

		std::vector<Node> m_Nodes;
		std::vector<Link> m_Links;

		bool m_HideUnconnected = false;

		Ref<Texture2D> m_HeaderBackground;
		Ref<Texture2D> m_SaveIcon;
		Ref<Texture2D> m_RestoreIcon;
		Ref<Texture2D> m_CommentIcon;
		Ref<Texture2D> m_PinIcon;
		Ref<Texture2D> m_PinnedIcon;
		Ref<Texture2D> m_RemoveIcon;

		const float m_TouchTime = 1.0f;
		std::map<ed::NodeId, float, NodeIdLess> m_NodeTouchTime;

		int m_NextId = 1;

		// Node Search Data
		SearchData m_SearchData;
		bool m_ContextSensitive = false;
		std::string m_SearchBuffer;
		std::string m_PreviousSearchBuffer;

		// Compiler and Results
		std::vector<CompilerResult> m_CompilerResults;
		std::string m_FindResultsSearchBar = "";
		std::vector<FindResultsData> m_FindResultsData;
		std::string m_CompilerOutput;

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
		
		ed::NodeId m_SelectedNode = 0;

		float m_CommentOpacity = 0.0f;
		ed::NodeId m_HoveredCommentID = 0;
	};

	int MaterialEditorInternal::GetNextId()
	{
		return m_NextId++;
	}

	ed::LinkId MaterialEditorInternal::GetNextLinkId()
	{
		return ed::LinkId(GetNextId());
	}

	void MaterialEditorInternal::TouchNode(ed::NodeId id)
	{
		m_NodeTouchTime[id] = m_TouchTime;
	}

	float MaterialEditorInternal::GetTouchProgress(ed::NodeId id)
	{
		auto it = m_NodeTouchTime.find(id);
		if (it != m_NodeTouchTime.end() && it->second > 0.0f)
			return (m_TouchTime - it->second) / m_TouchTime;
		else
			return 0.0f;
	}

	void MaterialEditorInternal::UpdateTouch()
	{
		const auto deltaTime = ImGui::GetIO().DeltaTime;
		for (auto& entry : m_NodeTouchTime)
		{
			if (entry.second > 0.0f)
				entry.second -= deltaTime;
		}
	}

	MaterialEditorInternal::Node* MaterialEditorInternal::FindNode(ed::NodeId id)
	{
		for (auto& node : m_Nodes)
			if (node.ID == id)
				return &node;

		return nullptr;
	}

	MaterialEditorInternal::Link* MaterialEditorInternal::FindLink(ed::LinkId id)
	{
		for (auto& link : m_Links)
			if (link.ID == id)
				return &link;

		return nullptr;
	}

	std::vector<MaterialEditorInternal::Link*> MaterialEditorInternal::GetPinLinks(ed::PinId id)
	{
		std::vector<Link*> linksToReturn;
		for (auto& link : m_Links)
			if (link.StartPinID == id || link.EndPinID == id)
				linksToReturn.push_back(&link);

		return linksToReturn;
	}

	MaterialEditorInternal::Pin* MaterialEditorInternal::FindPin(ed::PinId id)
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

	bool MaterialEditorInternal::IsPinLinked(ed::PinId id)
	{
		if (!id)
			return false;
		
		for (auto& link : m_Links)
			if (link.StartPinID == id || link.EndPinID == id)
				return true;

		return false;
	}

	bool MaterialEditorInternal::CanCreateLink(Pin* a, Pin* b)
	{
		if (a->Kind == PinKind::Input)
			std::swap(a, b);

		if (!a || !b || a == b || a->Kind == b->Kind || !AreTypesCompatible(a->Type, b->Type) || a->Node == b->Node)
			return false;
		
		if ((a->Type == PinType::Float || a->Type == PinType::Float2 || a->Type == PinType::Float3 || a->Type == PinType::Float4) && (b->Type == PinType::Float || b->Type == PinType::Float2 || b->Type == PinType::Float3 || b->Type == PinType::Float4))
			return true;
		else if (a->Type == b->Type)
			return true;
		else
			return false;
	}

	bool MaterialEditorInternal::AreTypesCompatible(const PinType& a, const PinType& b)
	{
		// All float types are compatible with each other
		if ((a == PinType::FloatFamily || a == PinType::Float || a == PinType::Float2 || a == PinType::Float3 || a == PinType::Float4) && (b == PinType::FloatFamily || b == PinType::Float || b == PinType::Float2 || b == PinType::Float3 || b == PinType::Float4))
			return true;
		// All other types must match
		else if (a == b)
			return true;
		return false;
	}

	void MaterialEditorInternal::BuildNode(Node* node)
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

	MaterialEditorInternal::Node* MaterialEditorInternal::SpawnResultNode()
	{
		auto& node = m_Nodes.emplace_back(GetNextId(), "Result", ImColor(166, 143, 119), NodeFunction::Result);
		node.Inputs.emplace_back(GetNextId(), "Albedo", PinType::Float3);
		node.Inputs.emplace_back(GetNextId(), "Metallic", PinType::Float);
		node.Inputs.emplace_back(GetNextId(), "Specular", PinType::Float3);
		node.Inputs.emplace_back(GetNextId(), "Roughness", PinType::Float);
		node.Inputs.emplace_back(GetNextId(), "Emissive", PinType::Float3);
		node.Inputs.emplace_back(GetNextId(), "Normal", PinType::Float3);
		node.Inputs.emplace_back(GetNextId(), "Alpha", PinType::Float);

		BuildNode(&node);
		return &node;
	}

	MaterialEditorInternal::Node* MaterialEditorInternal::SpawnComment()
	{
		auto& node = m_Nodes.emplace_back(GetNextId(), "Comment", ImColor(255, 255, 255));
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

	MaterialEditorInternal::Node* MaterialEditorInternal::SpawnAddNode()
	{
		auto& node = m_Nodes.emplace_back(GetNextId(), "Add", ImColor(143, 190, 137));
		node.Inputs.emplace_back(GetNextId(), "A", PinType::Float);
		node.Inputs.emplace_back(GetNextId(), "B", PinType::Float);
		node.Outputs.emplace_back(GetNextId(), "", PinType::Float);

		BuildNode(&node);
		return &node;
	}

	MaterialEditorInternal::Node* MaterialEditorInternal::SpawnSubtractNode()
	{
		auto& node = m_Nodes.emplace_back(GetNextId(), "Subtract", ImColor(143, 190, 137));
		node.Inputs.emplace_back(GetNextId(), "A", PinType::Float);
		node.Inputs.emplace_back(GetNextId(), "B", PinType::Float);
		node.Outputs.emplace_back(GetNextId(), "", PinType::Float);

		BuildNode(&node);
		return &node;
	}

	void MaterialEditorInternal::BuildNodes()
	{
		for (auto& node : m_Nodes)
			BuildNode(&node);
	}

	void MaterialEditorInternal::DrawPinIcon(bool connected, int alpha)
	{
		ImColor color = ImColor(255, 255, 255);
		color.Value.w = alpha / 255.0f;

		ax::Widgets::Icon(ImVec2(m_PinIconSize, m_PinIconSize), IconType::Circle, connected, color, ImColor(32, 32, 32, alpha));
	};

	void MaterialEditorInternal::CompileNodes()
	{
		m_CompilerResults.clear();

		// Ensure the 'Written' state of all nodes is reset
		for (auto& node : m_Nodes)
			node.Written = false;
		
		ShowFlow();
		ImGui::SetWindowFocus("Compiler Results##Material");

		m_CompilerResults.push_back({ CompilerResultType::Info, "Build Started [Vulkan GLSL] - Dymatic Material Nodes Version 23.1.0" });
		
		m_CompilerResults.push_back({ CompilerResultType::Info, "Initializing Pre Compile Link Checks" });
		for (uint32_t linkIndex = 0; linkIndex < m_Links.size(); linkIndex++)
		{
			auto& link = m_Links[linkIndex];

			Pin* startPin = FindPin(link.StartPinID);
			Pin* endPin = FindPin(link.EndPinID);
			if (startPin != nullptr && endPin != nullptr)
			{
				if (!AreTypesCompatible(startPin->Type, endPin->Type))
				{
					m_CompilerResults.push_back({ CompilerResultType::Error, "Can't connect pins : Data type is not the same.", startPin->Node->ID });
					startPin->Node->Error = true;
					endPin->Node->Error = true;
				}
			}
			else
			{
				m_Links.erase(m_Links.begin() + linkIndex);
				linkIndex--;
			}
		}

		MaterialCompiler compiler;
		compiler.WriteLine("// Copyright (c) Dymatic Technologies 2023, All Rights Reserved");
		compiler.WriteLine("// Generated by Dymatic Material Node Editor Version 23.1.0");
		compiler.WriteLine("// Usage: Dymatic Debug Shader");
		compiler.WriteLine("// Target: Vulkan GLSL");
		
		compiler.NewLine();
		
		compiler.WriteLine("#type vertex");
		compiler.WriteLine("#version 450 core");
		compiler.NewLine();
		compiler.WriteLine("void main()");
		compiler.OpenScope();
		compiler.CloseScope();

		compiler.NewLine();

		compiler.WriteLine("#type fragment");
		compiler.WriteLine("#version 450 core");
		compiler.NewLine();
		compiler.WriteLine("#include \"Resources/Shaders/Include/Math.glslh\"");
		compiler.NewLine();
		compiler.WriteLine("void main()");
		compiler.OpenScope();
		Node* resultNode = nullptr;
		for (auto& node : m_Nodes)
		{
			if (node.Function == NodeFunction::Result)
			{
				resultNode = &node;
				break;
			}
		}
		
		if (resultNode)
		{
			std::string line = "o_Color = ";
			RecursivePinWrite(resultNode->Inputs[0], compiler, line);
			line += ";";
			compiler.WriteLine(line);
		}
		else
			m_CompilerResults.push_back({ CompilerResultType::Error, "Could not find material result node : Aborting material compilation." });
				
		compiler.CloseScope();

		compiler.NewLine();
		compiler.Write("// End of generated file");

		// Check error count and output message.
		uint32_t warningCount = 0;
		uint32_t errorCount = 0;
		for (auto& result : m_CompilerResults)
		{
			if (result.Type == CompilerResultType::Error)
				errorCount++;
			else if (result.Type == CompilerResultType::Warning)
				warningCount++;
		}

		if (errorCount == 0)
		{
			compiler.OutputToFile("C:/dev/DymaticNodeAssembly/GeneratedMaterial.glsl");
			m_CompilerOutput = compiler.Contents();
			m_CompilerResults.push_back({ CompilerResultType::Info, "Compile of 'Default Material' completed [Vulkan GLSL] - Dymatic Material Nodes Version 23.1.0" });
		}
		else
			m_CompilerResults.push_back({ CompilerResultType::Info, "Compile of 'Default Material' failed [Vulkan GLSL] - " + std::to_string(errorCount) + " Error(s) " + std::to_string(warningCount) + " Warnings(s) - Dymatic Material Nodes Version 23.1.0" });
	}

	std::string MaterialEditorInternal::UnderscoreSpaces(std::string inString)
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

	std::string MaterialEditorInternal::GenerateDataString(const PinType& type, const PinData& data)
	{
		switch (type)
		{
		case PinType::Bool: return data.Bool ? "true" : "false";
		case PinType::Float: return std::to_string(data.Float);
		case PinType::Float2: return fmt::format("vec2({}, {})", data.Float2.x, data.Float2.y);
		case PinType::Float3: return fmt::format("vec3({}, {}, {})", data.Float3.x, data.Float2.y, data.Float3.z);
		case PinType::Float4: return fmt::format("vec4({}, {}, {}, {})", data.Float4.x, data.Float4.y, data.Float4.z, data.Float4.w);
		}
		DY_CORE_ASSERT(false, "Unknown material pin type");
		return {};
	}

	std::string MaterialEditorInternal::GenerateTypeString(const PinType& type)
	{
		switch (type)
		{
		case PinType::Bool: return "bool";
		case PinType::Float: return "float";
		case PinType::Float2: return "vec2";
		case PinType::Float3: return "vec3";
		case PinType::Float4: return "vec4";
		}
	}

	std::string MaterialEditorInternal::GenerateNodeSignature(const Node& node)
	{
		return "mnf_" + UnderscoreSpaces(node.Name) + "_" + std::to_string(node.ID.Get()) + "_pf";
	}

	void MaterialEditorInternal::RecursivePinWrite(Pin& pin, MaterialCompiler& compiler, std::string& line)
	{	
		if (IsPinLinked(pin.ID))
		{
			Link* link = GetPinLinks(pin.ID)[0];
			Pin* otherPin = FindPin(link->StartPinID == pin.ID ? link->EndPinID : link->StartPinID);
			Node* otherNode = otherPin->Node;

			if (!otherNode->Written)
			{
				RecursiveNodeWrite(*otherNode, compiler);
			}

			line += GenerateNodeSignature(*otherNode);
		}
		else
		{
			line += GenerateDataString(pin.Type, pin.Data);
		}
	}

	void MaterialEditorInternal::RecursiveNodeWrite(Node& node, MaterialCompiler& compiler)
	{
		if (node.Written)
			return;
		
		if (node.Function == NodeFunction::Math)
		{
			std::string line = GenerateTypeString(node.Outputs[0].Type) + " " + GenerateNodeSignature(node) + " = " + UnderscoreSpaces("DymaticMaterial_" + node.Name) + "(";
			bool first = true;
			for (auto& input : node.Inputs)
			{
				if (first)
					first = false;
				else
					line += ", ";
				RecursivePinWrite(input, compiler, line);
			}
			line += ");";
			compiler.WriteLine(line);
			node.Written = false;
		}
	}

	void MaterialEditorInternal::CopyNodes()
	{

	}

	void MaterialEditorInternal::PasteNodes()
	{

	}

	void MaterialEditorInternal::DuplicateNodes()
	{

	}

	void MaterialEditorInternal::DeleteNodes()
	{

	}

	void MaterialEditorInternal::ShowFlow()
	{
		for (auto& link : m_Links)
			ed::Flow(link.ID);
	}

	void MaterialEditorInternal::ClearSelection()
	{
		ed::ClearSelection();
	}

	void MaterialEditorInternal::SetNodePosition(ed::NodeId id, ImVec2 position)
	{
		auto p_node = FindNode(id);
		if (p_node != nullptr)
		{
			p_node->Position = position;
			ed::SetNodePosition(id, position);
		}
	}

	void MaterialEditorInternal::NavigateToContent()
	{
		ed::NavigateToContent();
	}

	void MaterialEditorInternal::NavigateToNode(ed::NodeId id)
	{
		for (auto& node : m_Nodes)
		{
			if (node.ID == id)
			{
				ed::SelectNode(node.ID);
				ed::NavigateToSelection();
			}
		}
	}

	void MaterialEditorInternal::DefaultValueInput(PinData& data, const PinType& type, bool spring)
	{
		if (type == PinType::Bool)
		{
			ImGui::Checkbox("##MaterialEditorCheckbox", &data.Bool);
			if (spring) ImGui::Spring(0);
		}
		else if (type == PinType::Float)
		{
			ImGui::DragFloat("##MaterialEditorDragFloat", &data.Float);
			if (spring) ImGui::Spring(0);
		}
		else if (type == PinType::Float2)
		{
			ImGui::DragFloat2("##MaterialEditorDragFloat2", glm::value_ptr(data.Float2));
			if (spring) ImGui::Spring(0);
		}
		else if (type == PinType::Float3)
		{
			ImGui::DragFloat3("##MaterialEditorDragFloat3", glm::value_ptr(data.Float3));
			if (spring) ImGui::Spring(0);
		}
		else if (type == PinType::Float4)
		{
			ImGui::DragFloat4("##MaterialEditorDragFloat4", glm::value_ptr(data.Float4));
			if (spring) ImGui::Spring(0);
		}
	}

	MaterialEditorInternal::Node* MaterialEditorInternal::DisplaySearchData(SearchData& searchData, bool origin)
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

	void MaterialEditorInternal::DeleteAllPinLinkAttachments(Pin* pin)
	{
		for (auto& link : m_Links)
			if (link.StartPinID == pin->ID || link.EndPinID == pin->ID)
				ed::DeleteLink(link.ID);
	}

	void MaterialEditorInternal::CheckLinkSafety(Pin* startPin, Pin* endPin)
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
			if (endPin != nullptr)
				if ((link.EndPinID == endPin->ID || link.StartPinID == endPin->ID))
					ed::DeleteLink(link.ID);
		}
	}

	void MaterialEditorInternal::CreateLink(Pin* a, Pin* b)
	{
		if (a->Kind == PinKind::Input)
			std::swap(a, b);

		CheckLinkSafety(a, b);

		m_Links.emplace_back(GetNextLinkId(), a->ID, b->ID);
		m_Links.back().Color = ImColor(255, 255, 255);
	}

	void MaterialEditorInternal::UpdateSearchData()
	{
		m_SearchData.Clear();
		m_SearchData.Name = "Dymatic Nodes";

		// Built In
		if (m_NewNodeLinkPin == nullptr || !m_ContextSensitive)
			AddSearchData(ed::GetSelectedObjectCount() > 0 ? "Add Comment to Selection" : "Add Comment...", { "" }, "label", {}, {}, [=]() { return SpawnComment(); });

		// Constant
		AddSearchData("Constant Float", { "Constants" }, "", {}, { PinType::Float }, [=]() { return SpawnSubtractNode(); });

		// Math
		AddSearchData("Add", { "Math" }, "+ sum", {}, {}, [=]() { return SpawnAddNode(); });
		AddSearchData("Subtract", { "Math" }, "- take difference", {}, {}, [=]() { return SpawnSubtractNode(); });
	}

	void MaterialEditorInternal::AddSearchData(const std::string& name, const std::vector<std::string>& categories, const std::string& keywords, const std::vector<PinType>& inputs, const std::vector<PinType>& outputs, std::function<Node* ()> callback)
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

		if (visible && m_ContextSensitive && m_NewNodeLinkPin != nullptr)
		{
			visible = false;
			for (auto& param : (m_NewNodeLinkPin->Kind == PinKind::Input ? inputs : outputs))
			{
				if (param == m_NewNodeLinkPin->Type)
				{
					visible = true;
					break;
				}
			}
		}

		if (visible)
		{
			for (auto& category : categories)
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
	}

	void MaterialEditorInternal::Init()
	{
		ed::Config config;
		config.SettingsFile = "";

		m_HeaderBackground = Dymatic::Texture2D::Create("Resources/Icons/NodeEditor/BlueprintBackground.png");
		m_SaveIcon = Dymatic::Texture2D::Create("Resources/Icons/NodeEditor/ic_save_white_24dp.png");
		m_RestoreIcon = Dymatic::Texture2D::Create("Resources/Icons/NodeEditor/ic_restore_white_24dp.png");
		m_CommentIcon = Dymatic::Texture2D::Create("Resources/Icons/NodeEditor/ic_comment_white_24dp.png");
		m_PinIcon = Dymatic::Texture2D::Create("Resources/Icons/NodeEditor/ic_pin_white_24dp.png");
		m_PinnedIcon = Dymatic::Texture2D::Create("Resources/Icons/NodeEditor/ic_pinned_white_24dp.png");

		m_RemoveIcon = Dymatic::Texture2D::Create("Resources/Icons/SceneHierarchy/ClearIcon.png");

		// Initial Node Loading
		{
			Node* node;

			node = SpawnResultNode();
			SetNodePosition(node->ID, { 0.0f, 0.0f });
			node->CommentEnabled = true;
			node->CommentPinned = true;
			node->Comment = "Result node of the Material.";
		}

		BuildNodes();

		NavigateToContent();
	}

	void MaterialEditorInternal::OnImGuiRender()
	{
        auto& io = ImGui::GetIO();
        
        ImGuiWindowClass windowClass;
        windowClass.ClassId = ImGui::GetID("##MaterialDockSpaceClass");
        windowClass.DockingAllowUnclassed = false;

        // DockSpace
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
        ImGui::Begin(CHARACTER_ICON_MATERIAL " Material Node Editor", &m_NodeEditorVisible, ImGuiWindowFlags_MenuBar);
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
        if (ImGui::MenuItem("Compile Shader")) CompileNodes();

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
        
		// Main Material Window
		{
			ImGui::SetNextWindowClass(&windowClass);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
			ImGui::Begin("Material Graph", nullptr, ImGuiWindowFlags_NoNavFocus);
			ImGui::PopStyleVar();
			UpdateTouch();
			
			ed::Begin("Node Editor Panel");
			{
				auto cursorTopLeft = ImGui::GetCursorScreenPos();

				util::BlueprintNodeBuilder builder(reinterpret_cast<void*>(m_HeaderBackground->GetRendererID()), m_HeaderBackground->GetWidth(), m_HeaderBackground->GetHeight());

				// Drag Drop Target
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
					}
					ImGui::EndDragDropTarget();
				}

				for (auto& node : m_Nodes)
				{
					if (node.Type != NodeType::Blueprint && node.Type != NodeType::Simple)
						continue;

					const auto isSimple = node.Type == NodeType::Simple;

					builder.Begin(node.ID);
					if (!isSimple)
					{
						builder.Header(node.Color);
						ImGui::Spring(0);

						if (node.Icon != nullptr)
							ImGui::Image((ImTextureID)node.Icon->GetRendererID(), ImVec2(ImGui::GetTextLineHeight() * 1.25f, ImGui::GetTextLineHeight() * 1.25f), { 0, 1 }, { 1, 0 }, node.IconColor);
							
						ImGui::TextUnformatted((node.DisplayName.empty() ? node.Name : node.DisplayName).c_str());

						ImGui::Spring(1);
						ImGui::Dummy(ImVec2(0, 28));
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
							DrawPinIcon(IsPinLinked(input.ID), (int)(alpha * 255));
							ImGui::Spring(0);
							auto& pinName = input.Name;
							if (!pinName.empty())
							{ 
								ImGui::TextUnformatted(pinName.c_str());
								ImGui::Spring(0);
							}
							
							ImGui::PopStyleVar();
							builder.EndInput();

							if (ImGui::IsItemHovered() && ImGui::IsMouseDown(0))
								m_CurrentLinkColor = ImColor(255, 255, 255);
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
							ImGui::TextUnformatted((node.DisplayName.empty() ? node.Name : node.DisplayName).c_str());
						ImGui::Spring(1, 0);
					}

					for (auto& output : node.Outputs)
					{
						if ((!m_HideUnconnected || IsPinLinked(output.ID)))
						{
							auto alpha = ImGui::GetStyle().Alpha;
							if (m_NewLinkPin && !CanCreateLink(m_NewLinkPin, &output) && &output != m_NewLinkPin)
								alpha = alpha * (48.0f / 255.0f);

							ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
							builder.Output(output.ID);
							auto& pinName = output.Name;
							if (!pinName.empty())
							{
								ImGui::Spring(0);
								ImGui::TextUnformatted(pinName.c_str());
							}
							ImGui::Spring(0);
							DrawPinIcon(IsPinLinked(output.ID), (int)(alpha * 255));
							ImGui::PopStyleVar();
							builder.EndOutput();

							if (ImGui::IsItemHovered() && ImGui::IsMouseDown(0))
								m_CurrentLinkColor = ImColor(255, 255, 255);
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
						}
						else
							m_SelectedNode = node.ID;
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
						if (ImGui::ImageButton((ImTextureID)(node.CommentPinned ? m_PinnedIcon : m_PinIcon)->GetRendererID(), ImVec2(15.0f, 15.0f) * zoom, { 0, 1 }, { 1, 0 }, 2 * zoom)) node.CommentPinned = !node.CommentPinned;
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
					else if (ImGui::IsItemHovered() || (m_CommentOpacity > -1.0f && m_HoveredCommentID == node.ID))
					{
						bool nodeHovered = ImGui::IsItemHovered();
						if (nodeHovered)
							m_HoveredCommentID = node.ID;

						auto zoom = node.CommentPinned ? ed::GetCurrentZoom() : 1.0f;
						auto padding = ImVec2(2.5f, 2.5f);

						auto min = ed::GetNodePosition(node.ID) - ImVec2(0, padding.y * zoom * 2.0f);


						ImGui::SetCursorScreenPos(min - ImVec2(-8, ImGui::GetTextLineHeightWithSpacing() + 4));
						ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_CommentOpacity);
						if (ImGui::ImageButton((ImTextureID)m_CommentIcon->GetRendererID(), ImVec2(15.0f, 15.0f) * zoom, { 0, 1 }, { 1, 0 }, 2 * zoom)) { m_CommentOpacity = -2.5f; node.CommentEnabled = true; }
						ImGui::PopStyleVar();

						if (ImGui::IsItemHovered() || nodeHovered)
							m_CommentOpacity = std::min(m_CommentOpacity + 0.1f, 1.0f);
						else
							m_CommentOpacity = std::max(m_CommentOpacity - 0.1f, -2.5f);
					}

					ImGui::PopID();
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
					const ImVec2 size = node.GroupBounds.Max - node.GroupBounds.Min;
					ed::Group(size.x != 0.0f ? size : node.Size);
					ImGui::EndVertical();
					ImGui::PopID();
					ed::EndNode(); if (ed::BeginGroupHint(node.ID))
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

						auto hintBounds = ImGui_GetItemRect();
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
							auto endPin = FindPin(endPinId);

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
								else if (!AreTypesCompatible(endPin->Type, startPin->Type))
								{
									showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
									ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
								}
								else
								{
									showLabel("+ Create Link", ImColor(32, 45, 32, 180));
									if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
										CreateLink(startPin, endPin);
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
								m_CreateNewNode = true;
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
							if (auto node = FindNode(nodeId))
							{
								if (ed::AcceptDeletedItem())
								{
									auto id = std::find_if(m_Nodes.begin(), m_Nodes.end(), [nodeId](auto& node) { return node.ID == nodeId; });
									if (id != m_Nodes.end())
										m_Nodes.erase(id);
								}
							}
						}
					}
					ed::EndDelete();
				}

				ImGui::SetCursorScreenPos(cursorTopLeft);
			}
			
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
				}
				else
					ImGui::Text("Unknown node: %p", m_ContextNodeId.AsPointer());
				ImGui::Separator();
				if (ImGui::MenuItem("Find References"))
				{
					ImGui::SetWindowFocus("Find Results##Material");
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

				ImGui::Text("All Actions for this Material");
				ImGui::SameLine();
				ImGui::Dummy(ImVec2(100.0f, 0.0f));
				ImGui::SameLine();
				if (ImGui::Checkbox("##MaterialNodeSearchPopupContextSensitiveCheckbox", &m_ContextSensitive))
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
				if (ImGui::InputTextWithHint("##MaterialEditorSearchBar", "Search:", buffer, sizeof(buffer)))
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

						for (auto& pin : pins)
						{
							Pin* output = startPin;
							Pin* input = &pin;
							if (output->Kind == PinKind::Input)
								std::swap(output, input);

							if (CanCreateLink(input, output))
							{
								CreateLink(startPin, &pin);
								break;
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
			ed::End();

			ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[2]);
			ImGui::GetWindowDrawList()->AddText(ImGui::GetWindowPos() + ImGui::GetWindowSize() - ImGui::CalcTextSize("MATERIAL") - ImVec2(20.0f, 20.0f), IM_COL32(255, 255, 255, 50), "MATERIAL");
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
		}

        const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

        // Script Contents Section
        {
            ImGui::SetNextWindowClass(&windowClass);
            ImGui::Begin("Material Properties", NULL, ImGuiWindowFlags_NoNavFocus);
            ImGui::End();
        }

        // Details Section
        {
            ImGui::SetNextWindowClass(&windowClass);
            ImGui::Begin("Details##Material", nullptr, ImGuiWindowFlags_NoNavFocus);
            ImGui::End();
        }

        // Compiler Results Window
        {
            ImGui::SetNextWindowClass(&windowClass);
            ImGui::Begin("Compiler Results##Material", nullptr, ImGuiWindowFlags_NoNavFocus);

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
            ImGui::Begin("Find Results##Material", NULL, ImGuiWindowFlags_NoNavFocus);

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
				for (auto& node : m_FindResultsData)
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
            }

            ImGui::End();
        }

		// Compiler Output Window
		{
			ImGui::SetNextWindowClass(&windowClass);
			ImGui::Begin("Compiler Output##Material", NULL, ImGuiWindowFlags_NoNavFocus);
			
			if (ImGui::Button("Copy"))
				ImGui::SetClipboardText(m_CompilerOutput.c_str());
			
			ImGui::InputTextMultiline("##CompilerOutputInputText", &m_CompilerOutput, ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly);
			
			ImGui::End();
		}
	}

	void MaterialEditorInternal::FindResultsSearch(const std::string& search)
	{
		m_FindResultsSearchBar = search;
		m_FindResultsData.clear();
		
		for (auto& node : m_Nodes)
		{
			bool created = node.Name.find(m_FindResultsSearchBar) != std::string::npos || node.DisplayName.find(m_FindResultsSearchBar) != std::string::npos;
			if (created)
			{
				auto& nodeData = m_FindResultsData.emplace_back(node.Name, node.IconColor, node.ID.Get());
				nodeData.Icon = node.Icon;
			}

			for (auto& pin : node.Inputs)
			{
				if (pin.Name.find(m_FindResultsSearchBar) != std::string::npos)
				{
					if (!created)
					{
						m_FindResultsData.emplace_back(node.Name, node.Color, node.ID.Get());
						created = true;
					}
					m_FindResultsData.back().SubData.emplace_back(pin.Name, ImColor(255, 255, 255), node.ID.Get());
				}
			}

			for (auto& pin : node.Outputs)
			{
				if (pin.Name.find(m_FindResultsSearchBar) != std::string::npos)
				{
					if (!created)
					{
						m_FindResultsData.emplace_back(node.Name, node.Color, node.ID.Get());
						created = true;
					}
					m_FindResultsData.back().SubData.back().SubData.emplace_back(pin.Name, ImColor(255, 255, 255), node.ID.Get());
				}
			}
		}
	}

	MaterialEditor::MaterialEditor()
	{
		m_InternalEditor = new MaterialEditorInternal;
	}

	MaterialEditor::~MaterialEditor()
	{
		delete m_InternalEditor;
	}

	void MaterialEditor::OnImGuiRender()
	{
		m_InternalEditor->OnImGuiRender();
	}
}