#include "Panels/Assets/MaterialPanel.h"

#include "EditorResources.h"
#include "Thumbnails/ThumbnailManager.h"
#include "Panels/Assets/Utils/AssetViewerPanelUtils.h"

#include <imgui_stdlib.h>
#include <imgui_node_editor.h>

#include "Panels/UI.h"
#include "Panels/Nodes/Utilities/Graph.h"
#include "Panels/Nodes/Utilities/Widgets.h"
#include "Panels/Nodes/Utilities/Search.h"


#include <glm/gtc/type_ptr.hpp>

namespace Dymatic {

	using namespace Editor;

	namespace ed = ax::NodeEditor;
	using namespace ax;
	using ax::Widgets::IconType;

	namespace Utils {

		static std::string GenerateDataString(const MaterialPinType type, const MaterialPinData& data)
		{
			switch (type)
			{
			case MaterialPinType::Bool: return data.Bool ? "True" : "False";
			case MaterialPinType::Float: return fmt::format("{}", data.Float);
			case MaterialPinType::Float2: return fmt::format("({}, {})", data.Float2.x, data.Float2.y);
			case MaterialPinType::Float3: return fmt::format("({}, {}, {})", data.Float3.x, data.Float2.y, data.Float3.z);
			case MaterialPinType::Float4: return fmt::format("({}, {}, {}, {})", data.Float4.x, data.Float4.y, data.Float4.z, data.Float4.w);
			}

			DY_CORE_ASSERT(false, "Unknown material pin type");
			return std::string();
		}

		static ImColor GetNodeColor(const Ref<MaterialNode> node)
		{
			switch (node->Function)
			{
			case MaterialNodeFunction::Result: return UI::GraphConstants::ResultColor;
			case MaterialNodeFunction::TextureSample: return UI::GraphConstants::AssetNodeColor;

			case MaterialNodeFunction::Pi:
			case MaterialNodeFunction::Infinity:
			case MaterialNodeFunction::Euler:
			case MaterialNodeFunction::Tau:
				return UI::GraphConstants::ConstantFloatColor;
			
			case MaterialNodeFunction::Parameter:
			case MaterialNodeFunction::Constant:
			{
				switch (node->GetInput(0)->Type)
				{
				case MaterialPinType::Bool: return UI::GraphConstants::BoolPinColor;
				case MaterialPinType::Float: return UI::GraphConstants::ConstantFloatColor;
				default: return UI::GraphConstants::ConstantVectorColor;
				}
			}

			case MaterialNodeFunction::WorldPosition:
			case MaterialNodeFunction::WorldNormal:
			case MaterialNodeFunction::VertexColor:
			case MaterialNodeFunction::VertexDepth:
			case MaterialNodeFunction::VertexLinearDepth:
			case MaterialNodeFunction::TextureCoordinates:
			case MaterialNodeFunction::ObjectPosition:
			case MaterialNodeFunction::EntityID:
			case MaterialNodeFunction::SubmeshIndex:
			case MaterialNodeFunction::VertexIndex:
			case MaterialNodeFunction::CameraPosition:
			case MaterialNodeFunction::CameraDirection:
			case MaterialNodeFunction::CameraForward:
			case MaterialNodeFunction::CameraRight:
			case MaterialNodeFunction::CameraUp:
			case MaterialNodeFunction::CameraNear:
			case MaterialNodeFunction::CameraFar:
			case MaterialNodeFunction::PixelPosition:
			case MaterialNodeFunction::ViewSize:
			case MaterialNodeFunction::Time:
			case MaterialNodeFunction::ParticleLifetime:
			case MaterialNodeFunction::ParticleLifeRemaining:
			case MaterialNodeFunction::ParticleLifeWeight:
			case MaterialNodeFunction::ParticleVelocity:
			case MaterialNodeFunction::SceneColor:
			case MaterialNodeFunction::SceneAlbedo:
			case MaterialNodeFunction::ScenePosition:
			case MaterialNodeFunction::SceneDepth:
			case MaterialNodeFunction::SceneLinearDepth:
			case MaterialNodeFunction::SceneNormal:
			case MaterialNodeFunction::SceneEmissive:
			case MaterialNodeFunction::SceneRoughness:
			case MaterialNodeFunction::SceneMetallic:
			case MaterialNodeFunction::SceneSpecular:
			case MaterialNodeFunction::SceneAmbientOcclusion:
			case MaterialNodeFunction::SceneEntityID:
			case MaterialNodeFunction::SceneSubmeshIndex:
				return UI::GraphConstants::InputNodeColor;
			}

			return UI::GraphConstants::FunctionNodeColor;
		}

		static const char* GetNodeFunctionIcon(const MaterialNodeFunction function)
		{
			switch (function)
			{
			case MaterialNodeFunction::Result: return FA_CIRCLE_DOT;
			case MaterialNodeFunction::CustomExpression: return FA_CODE;
			case MaterialNodeFunction::TextureSample: return FILE_ICON_TEXTURE;
			case MaterialNodeFunction::Constant: return FA_CIRCLE;
			case MaterialNodeFunction::Parameter: return FA_SLIDER;
			case MaterialNodeFunction::ComponentMask: return FA_CIRCLE_HALF_STROKE;
			case MaterialNodeFunction::ComponentAppend: return FA_MERGE;
			case MaterialNodeFunction::WorldPosition: return FA_GLOBE;
			case MaterialNodeFunction::WorldNormal: return FA_ARROW_RIGHT_FROM_LINE;
			case MaterialNodeFunction::VertexColor: return FA_PAINTBRUSH;
			case MaterialNodeFunction::VertexDepth: return FA_BINOCULARS;
			case MaterialNodeFunction::VertexLinearDepth: return FA_BINOCULARS;
			case MaterialNodeFunction::TextureCoordinates: return FA_MAP;
			case MaterialNodeFunction::ObjectPosition: return FA_CUBE;
			case MaterialNodeFunction::EntityID: return FA_KEY;
			case MaterialNodeFunction::SubmeshIndex: return FA_LAYER_GROUP;
			case MaterialNodeFunction::VertexIndex: return FA_HASHTAG;
			case MaterialNodeFunction::CameraPosition: return FA_CAMERA_MOVIE;

			case MaterialNodeFunction::CameraDirection:
			case MaterialNodeFunction::CameraForward:
			case MaterialNodeFunction::CameraRight:
			case MaterialNodeFunction::CameraUp:
				return FA_LOCATION_ARROW;

			case MaterialNodeFunction::CameraNear: return FA_MOUSE_FIELD;
			case MaterialNodeFunction::CameraFar: return FA_MOUNTAINS;
			case MaterialNodeFunction::PixelPosition: return FA_GRID_ROUND_5;
			case MaterialNodeFunction::ViewSize: return FA_DISPLAY;
			case MaterialNodeFunction::Time: return FA_CLOCK;

			case MaterialNodeFunction::ParticleLifetime:
			case MaterialNodeFunction::ParticleLifeRemaining:
			case MaterialNodeFunction::ParticleLifeWeight:
			case MaterialNodeFunction::ParticleVelocity:
				return FILE_ICON_PARTICLE_SYSTEM;
				
			case MaterialNodeFunction::SceneColor:
			case MaterialNodeFunction::SceneAlbedo:
			case MaterialNodeFunction::ScenePosition:
			case MaterialNodeFunction::SceneDepth:
			case MaterialNodeFunction::SceneLinearDepth:
			case MaterialNodeFunction::SceneNormal:
			case MaterialNodeFunction::SceneEmissive:
			case MaterialNodeFunction::SceneRoughness:
			case MaterialNodeFunction::SceneMetallic:
			case MaterialNodeFunction::SceneSpecular:
			case MaterialNodeFunction::SceneAmbientOcclusion:
			case MaterialNodeFunction::SceneEntityID:
			case MaterialNodeFunction::SceneSubmeshIndex:
				return CHARACTER_ICON_MEMORY;

			case MaterialNodeFunction::Add: return FA_PLUS;
			case MaterialNodeFunction::Subtract: return FA_MINUS;
			case MaterialNodeFunction::Multiply: return FA_XMARK;
			case MaterialNodeFunction::Divide: return FA_DIVIDE;

			case MaterialNodeFunction::Pi: return FA_PI;
			case MaterialNodeFunction::Infinity: return FA_INFINITY;
			case MaterialNodeFunction::Euler: return CHARACTER_ICON_EULER;
			case MaterialNodeFunction::Tau: return CHARACTER_ICON_GREEK_TAU;

			case MaterialNodeFunction::Abs: return FA_VALUE_ABSOLUTE;
			case MaterialNodeFunction::Length: return FA_RULER;
			case MaterialNodeFunction::Distance: return FA_RULER;
			case MaterialNodeFunction::Radians: return FA_CHART_PIE_SIMPLE;
			case MaterialNodeFunction::Degrees: return FA_COMPASS_DRAFTING;
			case MaterialNodeFunction::Sine:
			case MaterialNodeFunction::Cosine:
			case MaterialNodeFunction::Tangent:
			case MaterialNodeFunction::ArcSine:
			case MaterialNodeFunction::ArcCosine:
			case MaterialNodeFunction::ArcTangent:
			case MaterialNodeFunction::HyperbolicSine:
			case MaterialNodeFunction::HyperbolicCosine:
			case MaterialNodeFunction::HyperbolicTangent:
			case MaterialNodeFunction::ArcHyperbolicSine:
			case MaterialNodeFunction::ArcHyperbolicCosine:
			case MaterialNodeFunction::ArcHyperbolicTangent:
				return FA_WAVE_SINE;
			case MaterialNodeFunction::Ceil: return FA_ARROWS_UP_TO_LINE;
			case MaterialNodeFunction::Floor: return FA_ARROWS_DOWN_TO_LINE;
			case MaterialNodeFunction::Clamp: return FA_TIMELINE;
			case MaterialNodeFunction::Truncate: return FA_SCISSORS;
			case MaterialNodeFunction::SquareRoot: return FA_SQUARE_ROOT;
			case MaterialNodeFunction::InverseSquareRoot: return FA_SQUARE_ROOT;
			case MaterialNodeFunction::CrossProduct: return FA_XMARK;
			case MaterialNodeFunction::DotProduct: return FA_CIRCLE_SMALL;
			case MaterialNodeFunction::Reflect: return FA_REFLECT_VERTICAL;
			case MaterialNodeFunction::Refract: return FA_RAINBOW;
			case MaterialNodeFunction::Min: return FA_SORT_DOWN;
			case MaterialNodeFunction::Max: return FA_SORT_UP;
			case MaterialNodeFunction::Normalize: return FA_CIRCLE_ARROW_UP_RIGHT;
			case MaterialNodeFunction::FMod: return FA_PERCENT;
			case MaterialNodeFunction::Fract: return FA_DIVIDE;
			case MaterialNodeFunction::Step: return FA_STAIRS;
			case MaterialNodeFunction::SmoothStep: return FA_STAIRS;
			case MaterialNodeFunction::Round: return FA_CIRCLE_1;
			case MaterialNodeFunction::RoundEven: return FA_CIRCLE_2;
			case MaterialNodeFunction::Power: return FA_SUPERSCRIPT;
			case MaterialNodeFunction::Exponential: return FA_SUPERSCRIPT;
			case MaterialNodeFunction::Exponential2: return FA_SUPERSCRIPT;
			case MaterialNodeFunction::Log: return FA_FUNCTION;
			case MaterialNodeFunction::Log2: return FA_FUNCTION;
			case MaterialNodeFunction::Sign: return FA_PLUS_MINUS;
			case MaterialNodeFunction::OneMinus: return FA_REPEAT_1;
			case MaterialNodeFunction::Negate: return FA_PLUS_MINUS;
			case MaterialNodeFunction::Saturate: return FA_DROPLET;
			case MaterialNodeFunction::Desaturate: return FA_DROPLET_SLASH;
			case MaterialNodeFunction::Mix: return FA_SHUFFLE;
			case MaterialNodeFunction::If: return FA_SPLIT;
			case MaterialNodeFunction::Switch: return FA_QUESTION;

			case MaterialNodeFunction::ConstructVector2:
			case MaterialNodeFunction::ConstructVector3:
			case MaterialNodeFunction::ConstructVector4:
				return FA_MERGE;

			case MaterialNodeFunction::SplitVector2:
			case MaterialNodeFunction::SplitVector3:
			case MaterialNodeFunction::SplitVector4:
				return FA_SPLIT;
			}
		}

		static void DrawDefaultValueInput(MaterialPinData& data, const MaterialPinType type)
		{
			switch (type)
			{
			case MaterialPinType::Bool: return UI::DrawNodeDefaultValueInput(UI::ValueInputType::Bool, &data.Bool);
			case MaterialPinType::Float:
			case MaterialPinType::FloatFamily:
				return UI::DrawNodeDefaultValueInput(UI::ValueInputType::Float, &data.Float);
			case MaterialPinType::Float2: return UI::DrawNodeDefaultValueInput(UI::ValueInputType::Vector2, &data.Float2);
			case MaterialPinType::Float3: return UI::DrawNodeDefaultValueInput(UI::ValueInputType::Vector3, &data.Float3);
			case MaterialPinType::Float4: return UI::DrawNodeDefaultValueInput(UI::ValueInputType::Vector4, &data.Float4);
			case MaterialPinType::String: return UI::DrawNodeDefaultValueInput(UI::ValueInputType::String, &data.String);
			case MaterialPinType::StringMultiline: return UI::DrawNodeDefaultValueInput(UI::ValueInputType::StringMultiline, &data.String);
			case MaterialPinType::Texture: return UI::DrawNodeDefaultValueInput(UI::ValueInputType::Handle, &data.Handle, AssetType::Texture);
			}
		}

		static bool DrawExpressionPinField(Ref<MaterialPin> pin)
		{
			ImGui::PushID(pin->ID);
			const float width = (ImGui::GetContentRegionAvailWidth() - 30.0f - ImGui::GetStyle().FramePadding.x * 6.0f) * 0.5f;
			ImGui::SetNextItemWidth(width);
			ImGui::InputText("##MaterialPinInputText", &pin->Name);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(width);

			if (ImGui::BeginCombo("##MaterialPinTypeDropdown", MaterialPin::GetPinTypeString(pin->Type)))
			{
				if (ImGui::MenuItem("Bool")) pin->SetType(MaterialPinType::Bool);
				if (ImGui::MenuItem("Float")) pin->SetType(MaterialPinType::Float);
				if (ImGui::MenuItem("Float2")) pin->SetType(MaterialPinType::Float2);
				if (ImGui::MenuItem("Float3")) pin->SetType(MaterialPinType::Float3);
				if (ImGui::MenuItem("Float4")) pin->SetType(MaterialPinType::Float4);
				if (ImGui::MenuItem("Texture")) pin->SetType(MaterialPinType::Texture);

				ImGui::EndCombo();
			}

			ImGui::SameLine();
			const bool remove = ImGui::Button(FA_CIRCLE_XMARK, ImVec2(30.0f, 0.0f));
			ImGui::PopID();

			return remove;
		}
	}

	MaterialPanel::MaterialPanel(Ref<MaterialSource> material)
		: GraphAssetPanel(material->m_EditorGraph), m_Material(material), m_Compiler(material->m_EditorGraph)
	{
		// Setup the compiler
		m_Compiler.TargetHandle = material->Handle;
		m_Compiler.TargetProperties = material->GetProperties();
	}

	void MaterialPanel::OnUpdate(Timestep ts)
	{
		m_Viewport.BeginViewportRender(ts, m_Material);
	}

	static void SetupDockspace(const ImGuiID dockspace, AssetHandle handle)
	{
		// Clear the current dockspace layout
		ImGui::DockBuilderRemoveNodeChildNodes(dockspace);

		const float leftSidePanelSizeRatio = 0.2f;
		const float rightSidePanelSizeRatio = 0.25f;
		const float leftVerticalSplitRatio = 0.3f;
		const float rightVerticalSplitRatio = 0.5f;

		ImGuiID graphPanel = ImGui::DockBuilderGetNode(dockspace)->ID;

		// Split dockspace
		ImGuiID leftPanel, rightPanel, resultsPanel, previewPanel, outputPanel, graphPropertiesPanel, detailsPanel;
		ImGui::DockBuilderSplitNode(graphPanel, ImGuiDir_Left, leftSidePanelSizeRatio, &leftPanel, &graphPanel);
		ImGui::DockBuilderSplitNode(graphPanel, ImGuiDir_Right, rightSidePanelSizeRatio / (1.0f - leftSidePanelSizeRatio), &rightPanel, &graphPanel);
		ImGui::DockBuilderSplitNode(graphPanel, ImGuiDir_Down, leftSidePanelSizeRatio, &resultsPanel, &graphPanel);
		ImGui::DockBuilderSplitNode(leftPanel, ImGuiDir_Up, leftVerticalSplitRatio, &graphPropertiesPanel, &detailsPanel);
		ImGui::DockBuilderSplitNode(rightPanel, ImGuiDir_Up, rightVerticalSplitRatio, &previewPanel, &outputPanel);

		// Ensure that required windows are locked by default
		ImGui::DockBuilderGetNode(graphPanel)->LocalFlags |= ImGuiDockNodeFlags_HiddenTabBar;
		ImGui::DockBuilderGetNode(previewPanel)->LocalFlags |= ImGuiDockNodeFlags_HiddenTabBar;

		// Insert windows to dockspace slots
		ImGui::DockBuilderDockWindow(UI::GetFixedWindowNameWithID(CHARACTER_ICON_PROJECTION_ORTHOGRAPHIC " Material Graph", handle).c_str(), graphPanel);
		ImGui::DockBuilderDockWindow(UI::GetFixedWindowNameWithID(FA_TERMINAL " Compiler Results", handle).c_str(), resultsPanel);
		ImGui::DockBuilderDockWindow(UI::GetFixedWindowNameWithID(FA_MAGNIFYING_GLASS " Preview", handle).c_str(), previewPanel);
		ImGui::DockBuilderDockWindow(UI::GetFixedWindowNameWithID(FA_CIRCLE_INFO " Compiler Output", handle).c_str(), outputPanel);
		ImGui::DockBuilderDockWindow(UI::GetFixedWindowNameWithID(FA_SLIDERS " Material Properties", handle).c_str(), graphPropertiesPanel);
		ImGui::DockBuilderDockWindow(UI::GetFixedWindowNameWithID(FA_CIRCLE_INFO " Details", handle).c_str(), detailsPanel);

		ImGui::DockBuilderFinish(dockspace);
	}

	void MaterialPanel::OnImGuiRender(bool& open)
	{
		m_Viewport.OnPreImGuiRender();

		const std::filesystem::path assetPath = AssetManager::GetMetadata(m_Material->Handle).FilePath;

		ImGui::PushID(m_Material->Handle);
		ImGui::PushID("##Animation");

		if (m_Focus)
		{
			ImGui::SetNextWindowFocus();
			m_Focus = false;
		}

		ImGuiWindowClass windowClass = UI::CreateDockingRestrictionClass(fmt::format("##MaterialDockClass{}", m_Material->Handle).c_str());
		UI::CenterAppearingWindow(ImVec2(1750.0f, 850.0f));
		const ImGuiID dockspace = UI::BeginDockspaceWindow(Utils::GetViewerWindowName(FILE_ICON_MATERIAL, m_Material->Handle, assetPath, "Material Graph Viewer").c_str(), &open, ImGuiWindowFlags_MenuBar, &windowClass);
		DrawMenuBar();

		// Setup dockspace
		if (ImGui::IsWindowAppearing())
			SetupDockspace(dockspace, m_Material->Handle);

		ImGui::End();

		// Draw main node graph
		ImGui::SetNextWindowClass(&windowClass);
		UI::BeginDockedGraphWindow(CHARACTER_ICON_PROJECTION_ORTHOGRAPHIC " Material Graph", m_Material->Handle);
		DrawNodeGraph();
		ImGui::End();

		// Draw other editor windows
		ImGui::SetNextWindowClass(&windowClass);
		UI::DrawCompilerResultWindow(m_Material->Handle, m_Compiler.GetCompilerResult());

		ImGui::SetNextWindowClass(&windowClass);
		DrawGraphPropertiesPanel();

		ImGui::SetNextWindowClass(&windowClass);
		DrawDetailsPanel();

		ImGui::SetNextWindowClass(&windowClass);
		DrawCompilerOutputPanel();

		ImGui::SetNextWindowClass(&windowClass);
		UI::BeginDockedGraphWindow(FA_MAGNIFYING_GLASS " Preview", m_Material->Handle, m_Viewport.IsHovered() ? ImGuiWindowFlags_NoMove : 0);
		m_Viewport.OnImGuiRender();
		ImGui::End();

		ImGui::PopID();
		ImGui::PopID();
	}

	const CompilerResult& MaterialPanel::Compile()
	{
		m_Compiler.Compile();

		// If compilation succeeded override the active material
		if (const Ref<MaterialSource> newMaterial = m_Compiler.GetMaterial())
		{
			m_Material = AssetManager::OverrideAsset<MaterialSource>(m_Material->Handle, newMaterial);
			AssetManager::SerializeAsset(m_Material->Handle);

			// Invalidate the asset thumbnail
			ThumbnailManager::InvalidateThumbnail(m_Material->Handle);
		}

		return m_Compiler.GetCompilerResult();
	}

	NodeHandle MaterialPanel::OnAssetDropped(const AssetMetadata& metadata)
	{
		if (AssetManager::IsAssetTypeCompatible(AssetType::Texture, metadata.Type))
			return m_Material->m_EditorGraph->SpawnTextureSampleNode(metadata.Handle);

		return 0;
	}

	glm::vec4 MaterialPanel::GetNodeColor(const Ref<EditorNode> internalNode) const
	{
		return ImVec4(Utils::GetNodeColor(As<MaterialNode>(internalNode)));
	}

	void MaterialPanel::DrawNodeHeader(const Ref<EditorNode> internalNode)
	{
		Ref<MaterialNode> node = As<MaterialNode>(internalNode);

		if (node->Function == MaterialNodeFunction::Parameter || node->Function == MaterialNodeFunction::CustomExpression)
		{
			ImGui::TextUnformatted(Utils::GetNodeFunctionIcon(node->Function));
			ImGui::SelectableInput("##MaterialParameterInputText", node->Name.empty() ? 1.0f : ImGui::CalcTextSize(node->Name.c_str()).x, false, 0, &node->Name, nullptr, ImGuiInputTextFlags_NoHorizontalScroll);
			return;
		}

		std::string name = fmt::format("{} ", Utils::GetNodeFunctionIcon(node->Function));
		if (node->Function == MaterialNodeFunction::Constant)
			name += Utils::GenerateDataString(node->GetInput(0)->Type, node->GetInput(0)->Data);
		else if (node->Function == MaterialNodeFunction::ComponentMask)
			name = fmt::format("Mask ({}{}{}{})",
				node->GetInput(1)->Data.Bool ? "R" : "",
				node->GetInput(2)->Data.Bool ? "G" : "",
				node->GetInput(3)->Data.Bool ? "B" : "",
				node->GetInput(4)->Data.Bool ? "A" : ""
			);
		else
			name += MaterialGraph::GetNodeFunctionName(node->Function);

		ImGui::TextUnformatted(name.c_str());
	}

	void MaterialPanel::DrawNodeHeaderSimple(const Ref<EditorNode> internalNode) const
	{
		Ref<MaterialNode> node = As<MaterialNode>(internalNode);
		ImGui::TextUnformatted(Utils::GetNodeFunctionIcon(node->Function));
	}

	void MaterialPanel::DrawNodeThumbnail(const Ref<EditorNode> internalNode) const
	{
		const Ref<MaterialNode> node = As<MaterialNode>(internalNode);

		if (node->Function != MaterialNodeFunction::TextureSample)
			return;

		if (AssetHandle textureHandle = node->GetInput(1)->Data.Handle)
		{
			if (Ref<Texture2D> thumbnail = ThumbnailManager::GetOrCreateThumbnail(textureHandle))
				ImGui::Image((ImTextureID)thumbnail->GetRendererID(), ImVec2(100.0f, 100.0f), { 0, 1 }, { 1, 0 });
		}
	}

	void MaterialPanel::DrawPinIcon(const Ref<EditorPin> internalPin, const bool linked, const float alpha) const
	{
		ImColor color = GetPinColor(internalPin);
		color.Value.w = alpha;
		ax::Widgets::Icon(ImVec2(UI::GraphConstants::PinIconSize, UI::GraphConstants::PinIconSize), IconType::Circle, linked, color, ImColor(32, 32, 32, (int)(alpha * 255.0f)));
	}

	void MaterialPanel::UpdateSearch()
	{
		NodeSearchTreeBuilder<MaterialPinType> builder(&m_SearchTree, m_SearchBuffer);

		if (m_ContextSensitive && m_NewNodeLinkPinID != 0)
		{
			Ref<MaterialPin> pin = m_Material->m_EditorGraph->FindPin(m_NewNodeLinkPinID);
			builder.SetContext(pin->Type, pin->Kind);
			builder.SetCompatibilityCallback([&](MaterialPinType a, MaterialPinType b) { return m_Material->m_EditorGraph->AreTypesCompatible(a, b); });
		}

		// Built in comment
		if (m_NewNodeLinkPinID == 0 || !m_ContextSensitive)
		{
			const bool hasSelection = ed::GetSelectedObjectCount() > 0;
			if (hasSelection)
			{
				ImVec2 min, max;
				ed::GetSelectionBounds(min, max);
				const ImVec2 size = max - min + ImVec2(UI::GraphConstants::CommentPadding, UI::GraphConstants::CommentPadding) * 2.0f;
				builder.AddResult("Add Comment to Selection", { "" }, "label description", {}, {}, [size, this]() { return m_Material->m_EditorGraph->SpawnCommentNode(size); });
			}
			else
				builder.AddResult("Add Comment...", { "" }, "label description", {}, {}, [&]() { return m_Material->m_EditorGraph->SpawnCommentNode(); });
		}

		// Inputs
		builder.AddResult("World Position", { "Input|Mesh" }, "translation", {}, { MaterialPinType::Float3 }, [&]() { return m_Material->m_EditorGraph->SpawnWorldPositionNode(); });
		builder.AddResult("World Normal", { "Input|Mesh" }, {}, {}, { MaterialPinType::Float3 }, [&]() { return m_Material->m_EditorGraph->SpawnWorldNormalNode(); });
		builder.AddResult("Vertex Color", { "Input|Mesh" }, {}, {}, { MaterialPinType::Float4 }, [&]() { return m_Material->m_EditorGraph->SpawnVertexColorNode(); });
		builder.AddResult("Vertex Depth", { "Input|Mesh" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnVertexDepthNode(); });
		builder.AddResult("Vertex Linear Depth", { "Input|Mesh" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnVertexLinearDepthNode(); });
		builder.AddResult("Texture Coordinates", { "Input|Mesh" }, {}, {}, { MaterialPinType::Float2 }, [&]() { return m_Material->m_EditorGraph->SpawnTextureCoordinatesNode(); });
		builder.AddResult("Object Position", { "Input|Mesh" }, {}, {}, { MaterialPinType::Float3 }, [&]() { return m_Material->m_EditorGraph->SpawnObjectPositionNode(); });
		builder.AddResult("Entity ID", { "Input|Mesh" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnEntityIDNode(); });
		builder.AddResult("Submesh Index", { "Input|Mesh" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnSubmeshIndexNode(); });
		builder.AddResult("Vertex Index", { "Input|Mesh" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnVertexIndexNode(); });

		builder.AddResult("Camera Position", { "Input|View" }, {}, {}, { MaterialPinType::Float3 }, [&]() { return m_Material->m_EditorGraph->SpawnCameraPositionNode(); });
		builder.AddResult("Camera Direction", { "Input|View" }, {}, {}, { MaterialPinType::Float3 }, [&]() { return m_Material->m_EditorGraph->SpawnCameraDirectionNode(); });
		builder.AddResult("Camera Forward", { "Input|View" }, {}, {}, { MaterialPinType::Float3 }, [&]() { return m_Material->m_EditorGraph->SpawnCameraForwardNode(); });
		builder.AddResult("Camera Right", { "Input|View" }, {}, {}, { MaterialPinType::Float3 }, [&]() { return m_Material->m_EditorGraph->SpawnCameraRightNode(); });
		builder.AddResult("Camera Up", { "Input|View" }, {}, {}, { MaterialPinType::Float3 }, [&]() { return m_Material->m_EditorGraph->SpawnCameraUpNode(); });
		builder.AddResult("Camera Near", { "Input|View" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnCameraNearNode(); });
		builder.AddResult("Camera Far", { "Input|View" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnCameraFarNode(); });
		builder.AddResult("Pixel Position", { "Input|View" }, {}, {}, { MaterialPinType::Float2 }, [&]() { return m_Material->m_EditorGraph->SpawnPixelPositionNode(); });
		builder.AddResult("View Size", { "Input|View" }, "screen", {}, {MaterialPinType::Float2}, [&]() { return m_Material->m_EditorGraph->SpawnViewSizeNode(); });
		builder.AddResult("Time", { "Input" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnTimeNode(); });

		const auto& properties = m_Compiler.TargetProperties;

		if (properties.Usage == MaterialAsset::MaterialUsage::Particle)
		{
			builder.AddResult("Particle Lifetime", { "Input|Particle" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnParticleLifetimeNode(); });
			builder.AddResult("Particle Life Remaining", { "Input|Particle" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnParticleLifeRemainingNode(); });
			builder.AddResult("Particle Life Weight", { "Input|Particle" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnParticleLifeWeightNode(); });
			builder.AddResult("Particle Velocity", { "Input|Particle" }, {}, {}, { MaterialPinType::Float3 }, [&]() { return m_Material->m_EditorGraph->SpawnParticleVelocityNode(); });
		}

		if (properties.Usage == MaterialAsset::MaterialUsage::PostProcessing || properties.AlphaBlendMode == MaterialAsset::AlphaBlendMode::Translucent)
		{
			builder.AddResult("Scene Color", { "Input|Scene" }, {}, {}, { MaterialPinType::Float4 }, [&]() { return m_Material->m_EditorGraph->SpawnSceneColorNode(); });
			builder.AddResult("Scene Albedo", { "Input|Scene" }, {}, {}, { MaterialPinType::Float4 }, [&]() { return m_Material->m_EditorGraph->SpawnSceneAlbedoNode(); });
			builder.AddResult("Scene Position", { "Input|Scene" }, "translation", {}, { MaterialPinType::Float3 }, [&]() { return m_Material->m_EditorGraph->SpawnScenePositionNode(); });
			builder.AddResult("Scene Depth", { "Input|Scene" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnSceneDepthNode(); });
			builder.AddResult("Scene Linear Depth", { "Input|Scene" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnSceneLinearDepthNode(); });
			builder.AddResult("Scene Normal", { "Input|Scene" }, {}, {}, { MaterialPinType::Float3 }, [&]() { return m_Material->m_EditorGraph->SpawnSceneNormalNode(); });
			builder.AddResult("Scene Emissive", { "Input|Scene" }, {}, {}, { MaterialPinType::Float3 }, [&]() { return m_Material->m_EditorGraph->SpawnSceneEmissiveNode(); });
			builder.AddResult("Scene Roughness", { "Input|Scene" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnSceneRoughnessNode(); });
			builder.AddResult("Scene Metallic", { "Input|Scene" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnSceneMetallicNode(); });
			builder.AddResult("Scene Specular", { "Input|Scene" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnSceneSpecularNode(); });
			builder.AddResult("Scene Ambient Occlusion", { "Input|Scene" }, "ao", {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnSceneAmbientOcclusionNode(); });
			builder.AddResult("Scene Entity ID", { "Input|Scene" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnSceneEntityIDNode(); });
			builder.AddResult("Scene Submesh Index", { "Input|Scene" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnSceneSubmeshIndexNode(); });
		}

		// Expressions
		builder.AddResult("Custom Expression", { "Expression" }, "shader code glsl program", {}, {}, [&]() { return m_Material->m_EditorGraph->SpawnCustomExpressionNode(); });

		// Textures
		builder.AddResult("Texture Sample", { "Texture" }, {}, { MaterialPinType::Float2 }, { MaterialPinType::Float3, MaterialPinType::Float, MaterialPinType::Float4 }, [&]() { return m_Material->m_EditorGraph->SpawnTextureSampleNode(); });

		// Constant
		builder.AddResult("Constant Bool", { "Constant" }, "value true false binary boolean", {}, { MaterialPinType::Bool }, [&]() { return m_Material->m_EditorGraph->SpawnConstantNode(MaterialPinType::Bool); });
		builder.AddResult("Constant Float", { "Constant" }, "value single", {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnConstantNode(MaterialPinType::Float); });
		builder.AddResult("Constant Vector 2", { "Constant" }, "value vector vec2 vector2", {}, { MaterialPinType::Float2 }, [&]() { return m_Material->m_EditorGraph->SpawnConstantNode(MaterialPinType::Float2); });
		builder.AddResult("Constant Vector 3", { "Constant" }, "value vector vec3 vector3", {}, { MaterialPinType::Float3 }, [&]() { return m_Material->m_EditorGraph->SpawnConstantNode(MaterialPinType::Float3); });
		builder.AddResult("Constant Vector 4", { "Constant" }, "value vector vec4 vector4", {}, { MaterialPinType::Float4 }, [&]() { return m_Material->m_EditorGraph->SpawnConstantNode(MaterialPinType::Float4); });

		// Parameter
		builder.AddResult("Parameter Bool", { "Parameter" }, "variable true false binary boolean", {}, { MaterialPinType::Bool }, [&]() { return m_Material->m_EditorGraph->SpawnParameterNode(MaterialPinType::Bool); });
		builder.AddResult("Parameter Float", { "Parameter" }, "variable single", {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnParameterNode(MaterialPinType::Float); });
		builder.AddResult("Parameter Vector 2", { "Parameter" }, "variable vector vec2", {}, { MaterialPinType::Float2 }, [&]() { return m_Material->m_EditorGraph->SpawnParameterNode(MaterialPinType::Float2); });
		builder.AddResult("Parameter Vector 3", { "Parameter" }, "variable vector vec3", {}, { MaterialPinType::Float3 }, [&]() { return m_Material->m_EditorGraph->SpawnParameterNode(MaterialPinType::Float3); });
		builder.AddResult("Parameter Vector 4", { "Parameter" }, "variable vector vec4", {}, { MaterialPinType::Float4 }, [&]() { return m_Material->m_EditorGraph->SpawnParameterNode(MaterialPinType::Float4); });

		// Components
		builder.AddResult("Component Mask", { "Components" }, "xyzw rgba", { MaterialPinType::FloatFamily }, { MaterialPinType::FloatFamily }, [&]() { return m_Material->m_EditorGraph->SpawnComponentMaskNode(); });
		builder.AddResult("Component Append", { "Components" }, "concatenate make combine merge", { MaterialPinType::FloatFamily }, { MaterialPinType::FloatFamily }, [&]() { return m_Material->m_EditorGraph->SpawnComponentAppendNode(); });
		builder.AddResult("Construct Vector 2", { "Components|Construct" }, "make combine merge vec2 vector2", { MaterialPinType::Float }, { MaterialPinType::Float2 }, [&]() { return m_Material->m_EditorGraph->SpawnConstructVector2Node(); });
		builder.AddResult("Construct Vector 3", { "Components|Construct" }, "make combine merge vec3 vector3", { MaterialPinType::Float }, { MaterialPinType::Float3 }, [&]() { return m_Material->m_EditorGraph->SpawnConstructVector3Node(); });
		builder.AddResult("Construct Vector 4", { "Components|Construct" }, "make combine merge vec4 vector4", { MaterialPinType::Float }, { MaterialPinType::Float4 }, [&]() { return m_Material->m_EditorGraph->SpawnConstructVector4Node(); });
		builder.AddResult("Split Vector 2", { "Components|Split" }, "break vec2 vector2", { MaterialPinType::Float }, { MaterialPinType::Float2 }, [&]() { return m_Material->m_EditorGraph->SpawnSplitVector2Node(); });
		builder.AddResult("Split Vector 3", { "Components|Split" }, "break vec3 vector3", { MaterialPinType::Float }, { MaterialPinType::Float3 }, [&]() { return m_Material->m_EditorGraph->SpawnSplitVector3Node(); });
		builder.AddResult("Split Vector 4", { "Components|Split" }, "break vec4 vector4", { MaterialPinType::Float }, { MaterialPinType::Float4 }, [&]() { return m_Material->m_EditorGraph->SpawnSplitVector4Node(); });

		// Math
		const std::vector<MaterialPinType> floatFamily = { MaterialPinType::FloatFamily };

		builder.AddResult("Add", { "Math" }, "+ sum", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnAddNode(); });
		builder.AddResult("Subtract", { "Math" }, "- take difference", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnSubtractNode(); });
		builder.AddResult("Multiply", { "Math" }, "* times", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnMultiplyNode(); });
		builder.AddResult("Divide", { "Math" }, "/ by", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnDivideNode(); });

		builder.AddResult("Pi", { "Math|Constant" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnPiNode(); });
		builder.AddResult("Infinity", { "Math|Constant" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnInfinityNode(); });
		builder.AddResult("Euler", { "Math|Constant" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnEulerNode(); });
		builder.AddResult("Tau", { "Math|Constant" }, {}, {}, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnTauNode(); });

		builder.AddResult("Abs", { "Math" }, "absolute", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnAbsNode(); });
		builder.AddResult("Length", { "Math" }, {}, floatFamily, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnLengthNode(); });
		builder.AddResult("Distance", { "Math" }, {}, floatFamily, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnDistanceNode(); });
		builder.AddResult("Radians", { "Math" }, "angle convert conversion", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnRadiansNode(); });
		builder.AddResult("Degrees", { "Math" }, "angle convert conversion", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnDegreesNode(); });
		builder.AddResult("Sine", { "Math" }, "trigonometry", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnSineNode(); });
		builder.AddResult("Cosine", { "Math" }, "trigonometry", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnCosineNode(); });
		builder.AddResult("Tangent", { "Math" }, "trigonometry", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnTangentNode(); });
		builder.AddResult("Arc Sine", { "Math" }, "trigonometry", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnArcSineNode(); });
		builder.AddResult("Arc Cosine", { "Math" }, "trigonometry", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnArcCosineNode(); });
		builder.AddResult("Arc Tangent", { "Math" }, "trigonometry", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnArcTangentNode(); });
		builder.AddResult("Hyperbolic Sine", { "Math" }, "trigonometry", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnHyperbolicSineNode(); });
		builder.AddResult("Hyperbolic Cosine", { "Math" }, "trigonometry", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnHyperbolicCosineNode(); });
		builder.AddResult("Hyperbolic Tangent", { "Math" }, "trigonometry", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnHyperbolicTangentNode(); });
		builder.AddResult("Arc Hyperbolic Sine", { "Math" }, "trigonometry", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnArcHyperbolicSineNode(); });
		builder.AddResult("Arc Hyperbolic Cosine", { "Math" }, "trigonometry", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnArcHyperbolicCosineNode(); });
		builder.AddResult("Arc Hyperbolic Tangent", { "Math" }, "trigonometry", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnArcHyperbolicTangentNode(); });
		builder.AddResult("Ceil", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnCeilNode(); });
		builder.AddResult("Floor", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnFloorNode(); });
		builder.AddResult("Clamp", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnClampNode(); });
		builder.AddResult("Truncate", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnTruncateNode(); });
		builder.AddResult("Square Root", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnSquareRootNode(); });
		builder.AddResult("Inverse Square Root", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnInverseSquareRootNode(); });
		builder.AddResult("Cross Product", { "Math" }, {}, { MaterialPinType::Float3 }, { MaterialPinType::Float3 }, [&]() { return m_Material->m_EditorGraph->SpawnCrossProductNode(); });
		builder.AddResult("Dot Product", { "Math" }, {}, floatFamily, { MaterialPinType::Float }, [&]() { return m_Material->m_EditorGraph->SpawnDotProductNode(); });
		builder.AddResult("Reflect Vector", { "Math|Vector" }, "mirror reflection", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnReflectVectorNode(); });
		builder.AddResult("Refract Vector", { "Math|Vector" }, "ior eta refraction", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnRefractVectorNode(); });
		builder.AddResult("Min", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnMinNode(); });
		builder.AddResult("Max", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnMaxNode(); });
		builder.AddResult("Normalize", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnNormalizeNode(); });
		builder.AddResult("FMod", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnFModNode(); });
		builder.AddResult("Fract", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnFractNode(); });
		builder.AddResult("Step", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnStepNode(); });
		builder.AddResult("Smooth Step", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnSmoothStepNode(); });
		builder.AddResult("Round", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnRoundNode(); });
		builder.AddResult("Round Even", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnRoundEvenNode(); });
		builder.AddResult("Power", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnPowerNode(); });
		builder.AddResult("Exponential", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnExponentialNode(); });
		builder.AddResult("Exponential2", { "Math" }, "exp2", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnExponential2Node(); });
		builder.AddResult("Log", { "Math" }, "log10 logarithm", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnLogNode(); });
		builder.AddResult("Log2", { "Math" }, "logarithm", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnLog2Node(); });
		builder.AddResult("Sign", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnSignNode(); });
		builder.AddResult("One Minus", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnOneMinusNode(); });
		builder.AddResult("Negate", { "Math" }, "flip sign", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnNegateNode(); });
		builder.AddResult("Saturate", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnSaturateNode(); });
		builder.AddResult("Desaturate", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnDesaturateNode(); });
		builder.AddResult("Mix", { "Math" }, "lerp blend", floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnMixNode(); });
		builder.AddResult("If", { "Math" }, {}, floatFamily, floatFamily, [&]() { return m_Material->m_EditorGraph->SpawnIfNode(); });
		
		// Boolean Math
		builder.AddResult("Switch", { "Math|Boolean" }, {}, { MaterialPinType::FloatFamily, MaterialPinType::Bool }, { MaterialPinType::FloatFamily }, [&]() { return m_Material->m_EditorGraph->SpawnSwitchNode(); });
	}

	const glm::vec4 MaterialPanel::GetPinColor(const Ref<EditorPin> internalPin) const
	{
		Ref<MaterialPin> pin = As<MaterialPin>(internalPin);

		// Color texture node pins
		if (pin->Kind == PinKind::Output && pin->Type == MaterialPinType::Float)
		{
			Ref<MaterialNode> node = m_Material->m_EditorGraph->FindNode(pin->Node);

			if (node && node->Function == MaterialNodeFunction::TextureSample)
			{
				if (pin->Name == "R") return ImVec4(UI::GraphConstants::RedChannelPinColor);
				if (pin->Name == "G") return ImVec4(UI::GraphConstants::GreenChannelPinColor);
				if (pin->Name == "B") return ImVec4(UI::GraphConstants::BlueChannelPinColor);
				if (pin->Name == "A") return ImVec4(UI::GraphConstants::AlphaChannelPinColor);
			}
		}

		return ImVec4(UI::GraphConstants::BasicPinColor);
	}

	void MaterialPanel::OnNodeDoubleClicked(const Ref<Editor::EditorNode> internalNode)
	{
	}

	void MaterialPanel::DrawGraphPropertiesPanel()
	{
		UI::BeginDockedGraphWindow(FA_SLIDERS " Material Properties", m_Material->Handle);

		auto& properties = m_Compiler.TargetProperties;

		// Material Usage
		ImGui::TextDisabled("Usage");
		ImGui::SetNextItemWidth(-1);
		if (ImGui::BeginCombo("##MaterialUsageCombo", MaterialAsset::MaterialUsageToString(properties.Usage)))
		{
			for (uint32_t i = 0; i < MaterialAsset::MaterialUsage::MATERIAL_USGAE_SIZE; i++)
				if (ImGui::MenuItem(MaterialAsset::MaterialUsageToString((MaterialAsset::MaterialUsage)i)))
					properties.Usage = (MaterialAsset::MaterialUsage)i;

			ImGui::EndCombo();
		}

		// Alpha Blend Mode (if relevant)
		if (properties.Usage == MaterialAsset::MaterialUsage::Surface || properties.Usage == MaterialAsset::MaterialUsage::Particle)
		{
			ImGui::TextDisabledUnformatted("Alpha Blend Mode");
			ImGui::SetNextItemWidth(-1);
			if (ImGui::BeginCombo("##MaterialAlphaBlendModeCombo", MaterialAsset::AlphaBlendModeToString(properties.AlphaBlendMode)))
			{
				for (uint32_t i = 0; i < MaterialAsset::AlphaBlendMode::ALPHA_BLEND_MODE_SIZE; i++)
					if (ImGui::MenuItem(MaterialAsset::AlphaBlendModeToString((MaterialAsset::AlphaBlendMode)i)))
						properties.AlphaBlendMode = (MaterialAsset::AlphaBlendMode)i;

				ImGui::EndCombo();
			}

			ImGui::TextDisabledUnformatted("Cast Shadows");
			ImGui::SameLine();
			ImGui::Checkbox("##MaterialCastShadowsCheckbox", &properties.CastShadows);
		}

		if (properties.Usage == MaterialAsset::MaterialUsage::Surface)
		{
			ImGui::TextDisabledUnformatted("Two Sided");
			ImGui::SameLine();
			ImGui::Checkbox("##MaterialTwoSidedCheckbox", &properties.TwoSided);

			ImGui::Separator();

			ImGui::TextDisabledUnformatted("Tessellation");
			ImGui::SameLine();
			ImGui::Checkbox("##MaterialTessellationCheckbox", &properties.Tessellation);

			if (properties.Tessellation)
			{
				const auto& config = m_Material->m_EditorGraph->GetConfig();

				const char* modes[] = { "Equal", "Even Fractional", "Odd Fractional" };
				ImGui::TextDisabledUnformatted(FA_DISTRIBUTE_SPACING_HORIZONTAL " Tessellation Spacing");
				ImGui::SameLine();
				ImGui::Combo("##MaterialTessellationSpacing", (int*)&config.TessellationSpacing, modes, IM_ARRAYSIZE(modes));
			}
		}

		if (properties.Usage == MaterialAsset::MaterialUsage::Particle)
		{
			ImGui::TextDisabledUnformatted("Lit");
			ImGui::SameLine();
			ImGui::Checkbox("##MaterialLitCheckbox", &properties.Lit);
		}

		ImGui::End();
	}

	void MaterialPanel::DrawDetailsPanel()
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		UI::BeginDockedGraphWindow(FA_CIRCLE_INFO " Details", m_Material->Handle);

		if (Ref<MaterialNode> node = m_Material->m_EditorGraph->FindNode(m_SelectedNodeID))
		{
			const bool commentNode = node->Type == NodeType::Comment;

			ImGui::TextDisabled(FA_BARS " Node Properties");
			if (node->Function == MaterialNodeFunction::Parameter || node->Function == MaterialNodeFunction::CustomExpression)
				ImGui::InputText("##MaterialNodeNameInput", &node->Name, ImGuiInputTextFlags_AutoSelectAll);
			else
				ImGui::Text(MaterialGraph::GetNodeFunctionName(node->Function));

			ImGui::Separator();

			ImGui::Text(FA_COMMENT " Comment");

			if (!commentNode)
			{
				ImGui::SameLine();
				ImGui::Checkbox("##MaterialCommentCheckbox", &node->CommentEnabled);
			}

			if (node->CommentEnabled || commentNode)
			{
				const float colorButtonSize = ImGui::GetFrameHeight();

				if (!commentNode)
				{
					if (ImGui::Button(node->CommentPinned ? FA_LOCATION_PIN_SLASH : FA_LOCATION_PIN))
						node->CommentPinned = !node->CommentPinned;
					ImGui::SameLine();
				}


				ImGui::SetNextItemWidth(-(colorButtonSize + style.FramePadding.x * 2.0f));
				ImGui::InputText("##AnimationCommentInputText", &node->Comment);
				ImGui::SameLine();
				ImGui::ColorEdit3("##AnimationCommentColorPicker", glm::value_ptr(node->Color), ImGuiColorEditFlags_NoInputs);
			}

			ImGui::Separator();

			if (UI::CollapsingHeader("Inputs"))
			{
				auto& pins = node->Inputs;

				for (auto& internalPin : pins)
				{
					Ref<MaterialPin> pin = As<MaterialPin>(internalPin);

					if (!pin->Editable || (m_Material->m_EditorGraph->IsPinLinked(pin->ID) && pin->Kind == PinKind::Input))
						continue;

					ImGui::PushID(pin->ID);
					ImGui::Text(pin->Name.c_str());

					if (pin->Type != MaterialPinType::String && pin->Type != MaterialPinType::StringMultiline)
						ImGui::SameLine();

					Utils::DrawDefaultValueInput(pin->Data, pin->Type);
					ImGui::PopID();
				}

				ImGui::TreePop();
			}

			// Custom Expression Handling
			if (node->Function == MaterialNodeFunction::CustomExpression)
			{
				if (UI::CollapsingHeader("Expression Fields"))
				{
					// Display all input fields (excluding the default ones)
					if (UI::CollapsingHeader("Inputs"))
					{
						for (uint32_t i = 3; i < node->Inputs.size(); i++)
						{
							Ref<MaterialPin> input = node->GetInput(i);
							if (Utils::DrawExpressionPinField(input))
							{
								m_Material->m_EditorGraph->RemovePin(input->ID);
								i--;
							}
						}

						ImGui::TreePop();
					}

					// Display all output fields
					if (UI::CollapsingHeader("Outputs"))
					{
						for (uint32_t i = 0; i < node->Outputs.size(); i++)
						{
							Ref<MaterialPin> output = node->GetOutput(i);
							if (Utils::DrawExpressionPinField(output))
							{
								m_Material->m_EditorGraph->RemovePin(output->ID);
								i--;
							}
						}

						ImGui::TreePop();
					}

					const ImVec2 size = ImVec2(ImGui::GetContentRegionAvailWidth(), 30.0f);
					if (ImGui::Button(FA_CIRCLE_PLUS " Add Input", size))
						m_Material->m_EditorGraph->AddDynamicPin(node->ID, fmt::format("Input{}", node->Inputs.size()), PinKind::Input);

					if (ImGui::Button(FA_CIRCLE_PLUS " Add Output", size))
						m_Material->m_EditorGraph->AddDynamicPin(node->ID, fmt::format("Output{}", node->Outputs.size()), PinKind::Output);

					ImGui::TreePop();
				}
			}
		}

		ImGui::End();
	}

	void MaterialPanel::DrawCompilerOutputPanel()
	{
		UI::BeginDockedGraphWindow(FA_CIRCLE_INFO " Compiler Output", m_Material->Handle);
		ImGui::BeginTabBar("##MaterialCompilerOutputTabBar");

		auto& compilerOutput = m_Compiler.GetCompilerOutput();
		for (auto& [stage, output] : compilerOutput)
		{
			if (ImGui::BeginTabItem(MaterialAsset::MaterialRenderStageToString(stage)))
			{
				if (ImGui::Button(FA_COPY " Copy"))
					ImGui::SetClipboardText(output.c_str());

				ImGui::InputTextMultiline("##CompilerOutputInputText", &output, ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly);

				ImGui::EndTabItem();
			}
		}

		ImGui::EndTabBar();
		ImGui::End();
	}

	bool MaterialPanel::CanDrawPin(const Ref<EditorNode> node, const Ref<EditorPin> internalPin, const uint32_t pinIndex)
	{
		if (As<MaterialNode>(node)->Function != MaterialNodeFunction::Result)
			return true;

		const auto& properties = m_Compiler.TargetProperties;

		switch (pinIndex)
		{
		case MaterialResultPinType::Albedo:
			return true;
		case MaterialResultPinType::IOR:
			if (properties.AlphaBlendMode != MaterialAsset::AlphaBlendMode::Translucent) return false;
		case MaterialResultPinType::Alpha:
			if (properties.AlphaBlendMode == MaterialAsset::AlphaBlendMode::Opaque) return false;
		case MaterialResultPinType::Normal:
		case MaterialResultPinType::Roughness:
		case MaterialResultPinType::Metallic:
		case MaterialResultPinType::Specular:
		case MaterialResultPinType::AmbientOcclusion:
		case MaterialResultPinType::Emissive:
			if (pinIndex != MaterialResultPinType::Alpha && !properties.Lit) return false;
		case MaterialResultPinType::WorldDisplacement:
			return properties.Usage == MaterialAsset::MaterialUsage::Surface || properties.Usage == MaterialAsset::MaterialUsage::Particle;
		case MaterialResultPinType::TessellationMultiplier:
			return properties.Usage == MaterialAsset::Surface && properties.Tessellation;
		case MaterialResultPinType::DepthOffset:
			return properties.Usage == MaterialAsset::MaterialUsage::Surface;
		case MaterialResultPinType::ParticleSize:
			return properties.Usage == MaterialAsset::MaterialUsage::Particle;
		}

		return true;
	}

}