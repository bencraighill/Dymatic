#include "Panels/Assets/AnimationGraphPanel.h"

#include "EditorResources.h"
#include "Fonts.h"
#include "Thumbnails/ThumbnailManager.h"

#include "Dymatic/Asset/AssetManager.h"
#include "Panels/Assets/Utils/AssetViewerPanelUtils.h"

#include <imgui_stdlib.h>
#include <imgui_node_editor.h>

#include "Panels/UI.h"
#include "Panels/Nodes/Utilities/Widgets.h"
#include "Panels/Nodes/Utilities/Graph.h"

#include <glm/gtc/type_ptr.hpp>

#include "Dymatic/Animation/DelaunayTriangleGenerator.h"

namespace Dymatic {

	namespace ed = ax::NodeEditor;
	using namespace ax;
	using ax::Widgets::IconType;

	using namespace Editor;	

	namespace Utils {

		static ImColor GetNodeFunctionColor(AnimationNodeFunction function)
		{
			switch (function)
			{
			case AnimationNodeFunction::Output: return UI::GraphConstants::ResultColor;
			case AnimationNodeFunction::Player: return UI::GraphConstants::PlayerNodeColor;
			case AnimationNodeFunction::Blend: return UI::GraphConstants::FunctionNodeColor;
			case AnimationNodeFunction::BoolBlend: return UI::GraphConstants::FunctionNodeColor;
			case AnimationNodeFunction::IntBlend: return UI::GraphConstants::FunctionNodeColor;
			case AnimationNodeFunction::LayeredBoneBlend: return UI::GraphConstants::FunctionNodeColor;
			case AnimationNodeFunction::Additive: return UI::GraphConstants::FunctionNodeColor;
			case AnimationNodeFunction::Parameter: return UI::GraphConstants::ParameterNodeColor;
			case AnimationNodeFunction::TwoBoneIK: return UI::GraphConstants::TransformNodeColor;
			case AnimationNodeFunction::FABRIK: return UI::GraphConstants::TransformNodeColor;
			case AnimationNodeFunction::TransformBone: return UI::GraphConstants::TransformNodeColor;
			case AnimationNodeFunction::LocalToComponent: return UI::GraphConstants::SpaceConversionColor;
			case AnimationNodeFunction::ComponentToLocal: return UI::GraphConstants::SpaceConversionColor;
			case AnimationNodeFunction::Blendspace: return UI::GraphConstants::StateNodeColor;
			case AnimationNodeFunction::Blendspace1D: return UI::GraphConstants::StateNodeColor;
			case AnimationNodeFunction::BlendspaceAnimation: return UI::GraphConstants::PlayerNodeColor;
			case AnimationNodeFunction::BlendspaceGraph: return ImColor(180, 180, 180, 255);
			case AnimationNodeFunction::StateMachine: return UI::GraphConstants::StateNodeColor;
			case AnimationNodeFunction::State: return UI::GraphConstants::StateNodeColor;
			case AnimationNodeFunction::Entry: return UI::GraphConstants::StateNodeColor;
			case AnimationNodeFunction::Transition: return UI::GraphConstants::ResultColor;
			case AnimationNodeFunction::TimeRemainingRatio: return UI::GraphConstants::ParameterNodeColor;
			}

			return ImColor(255, 255, 255);
		}

		static const char* GetNodeFunctionIcon(AnimationNodeFunction function)
		{
			switch (function)
			{
			case AnimationNodeFunction::Output: return FA_CIRCLE_DOT;
			case AnimationNodeFunction::Player: return FA_PLAY;
			case AnimationNodeFunction::Blend: return FA_MERGE;
			case AnimationNodeFunction::BoolBlend: return FA_OPTION;
			case AnimationNodeFunction::IntBlend: return FA_FILTER_LIST;
			case AnimationNodeFunction::LayeredBoneBlend: return FA_LAYER_GROUP;
			case AnimationNodeFunction::Additive: return FA_PLUS;
			case AnimationNodeFunction::Parameter: return FA_SLIDER;
			case AnimationNodeFunction::TwoBoneIK: return FA_HAND;
			case AnimationNodeFunction::FABRIK: return FA_HAND;
			case AnimationNodeFunction::TransformBone: return FA_BONE;
			case AnimationNodeFunction::LocalToComponent: return FA_ARROWS_TURN_TO_DOTS;
			case AnimationNodeFunction::ComponentToLocal: return FA_ARROWS_TURN_TO_DOTS;
			case AnimationNodeFunction::Blendspace: return FA_CHART_SCATTER;
			case AnimationNodeFunction::Blendspace1D: return FA_ELLIPSIS;
			case AnimationNodeFunction::BlendspaceAnimation: return FA_PERSON_RUNNING;
			case AnimationNodeFunction::BlendspaceGraph: return FA_HASHNODE;
			case AnimationNodeFunction::StateMachine: return FA_CHART_NETWORK;
			case AnimationNodeFunction::State: return FA_HASHNODE;
			case AnimationNodeFunction::Entry: return FA_RIGHT_FROM_LINE;
			case AnimationNodeFunction::Transition: return FA_ARROW_PROGRESS;
			case AnimationNodeFunction::TimeRemainingRatio: return FA_TIMER;
			case AnimationNodeFunction::AND: return FA_MICROCHIP;
			case AnimationNodeFunction::OR: return FA_MICROCHIP;
			case AnimationNodeFunction::NAND: return FA_MICROCHIP;
			case AnimationNodeFunction::NOR: return FA_MICROCHIP;
			case AnimationNodeFunction::XOR: return FA_MICROCHIP;
			case AnimationNodeFunction::NOT: return FA_BAN;
			case AnimationNodeFunction::Equality: return FA_EQUALS;
			case AnimationNodeFunction::Inequality: return FA_NOT_EQUAL;
			case AnimationNodeFunction::LessThan: return FA_LESS_THAN;
			case AnimationNodeFunction::LessThanOrEqual: return FA_LESS_THAN_EQUAL;
			case AnimationNodeFunction::GreaterThan: return FA_GREATER_THAN;
			case AnimationNodeFunction::GreaterThanOrEqual: return FA_GREATER_THAN_EQUAL;
			case AnimationNodeFunction::Add: return FA_PLUS;
			case AnimationNodeFunction::Subtract: return FA_MINUS;
			case AnimationNodeFunction::Multiply: return FA_XMARK;
			case AnimationNodeFunction::Divide: return FA_DIVIDE;
			}
		}

		static bool IsLogicalNodeFunction(AnimationNodeFunction function)
		{
			switch (function)
			{
			case AnimationNodeFunction::AND:
			case AnimationNodeFunction::OR:
			case AnimationNodeFunction::NAND:
			case AnimationNodeFunction::NOR:
			case AnimationNodeFunction::XOR:
			case AnimationNodeFunction::NOT:
				return true;
			}

			return false;
		}

		static ImColor GetPinColor(AnimationPinType type)
		{
			// TODO: Move to styling preferences
			switch (type)
			{
			case AnimationPinType::Pose: return UI::GraphConstants::BasicPinColor;
			case AnimationPinType::Animation: return UI::GraphConstants::AssetPinColor;
			case AnimationPinType::Bool: return UI::GraphConstants::BoolPinColor;
			case AnimationPinType::Int: return UI::GraphConstants::IntPinColor;
			case AnimationPinType::Float: return UI::GraphConstants::FloatPinColor;
			case AnimationPinType::Vector2: return UI::GraphConstants::VectorPinColor;
			case AnimationPinType::Vector3: return UI::GraphConstants::VectorPinColor;
			case AnimationPinType::Vector4: return UI::GraphConstants::VectorPinColor;
			case AnimationPinType::Transform: return UI::GraphConstants::TransformPinColor;
			case AnimationPinType::Player: return UI::GraphConstants::BasicPinColor;
			}

			return ImColor(128, 128, 128);
		}

		static std::string GetNodeName(const Ref<AnimationEditorNode> node)
		{
			if (node->Function == AnimationNodeFunction::Player || node->Function == Editor::AnimationNodeFunction::BlendspaceAnimation)
			{
				const AssetHandle handle = node->GetInput(0)->Data.Handle;

				if (handle != 0 && AssetManager::DoesAssetExist(handle))
					return AssetManager::GetMetadata(handle).FilePath.stem().string();
			}

			return AnimationEditorGraph::GetNodeFunctionName(node->Function);
		}

	}

	AnimationGraphPanel::AnimationGraphPanel(Ref<AnimationGraph> animationGraph)
		: GraphAssetPanel(animationGraph->m_EditorGraph), m_AnimationGraph(animationGraph), m_Compiler(animationGraph->m_EditorGraph)
	{
		// Setup the compiler
		const Ref<Skeleton> skeleton = animationGraph->GetSkeleton();
		m_Compiler.TargetHandle = animationGraph->Handle;
		m_Compiler.TargetSkeleton = (skeleton ? skeleton->Handle : 0);
	}

	void AnimationGraphPanel::OnUpdate(Timestep ts)
	{
		m_Viewport.BeginViewportRender(ts);
		m_Viewport.SubmitViewportRender();
		m_Viewport.EndViewportRender();
	}

	static void SetupDockspace(ImGuiID dockspace, uint64_t handle)
	{
		// Clear the current dockspace layout
		ImGui::DockBuilderRemoveNodeChildNodes(dockspace);

		const float sidePanelSizeRatio = 0.2f;
		const float verticalSplitRatio = 0.3f;

		ImGuiID graphPanel = ImGui::DockBuilderGetNode(dockspace)->ID;

		// Split dockspace
		ImGuiID leftPanel, rightPanel, resultsPanel, previewPanel, previewParametersPanel, graphPropertiesPanel, detailsPanel;
		ImGui::DockBuilderSplitNode(graphPanel, ImGuiDir_Left, sidePanelSizeRatio, &leftPanel, &graphPanel);
		ImGui::DockBuilderSplitNode(graphPanel, ImGuiDir_Right, sidePanelSizeRatio / (1.0f - sidePanelSizeRatio), &rightPanel, &graphPanel);
		ImGui::DockBuilderSplitNode(graphPanel, ImGuiDir_Down, sidePanelSizeRatio, &resultsPanel, &graphPanel);
		ImGui::DockBuilderSplitNode(leftPanel, ImGuiDir_Up, verticalSplitRatio, &previewPanel, &previewParametersPanel);
		ImGui::DockBuilderSplitNode(rightPanel, ImGuiDir_Up, verticalSplitRatio, &graphPropertiesPanel, &detailsPanel);

		// Ensure that required windows are locked by default
		ImGui::DockBuilderGetNode(graphPanel)->LocalFlags |= ImGuiDockNodeFlags_HiddenTabBar;
		ImGui::DockBuilderGetNode(previewPanel)->LocalFlags |= ImGuiDockNodeFlags_HiddenTabBar;

		// Insert windows to dockspace slots
		ImGui::DockBuilderDockWindow(UI::GetFixedWindowNameWithID(CHARACTER_ICON_PROJECTION_ORTHOGRAPHIC " Animation Graph", handle).c_str(), graphPanel);
		ImGui::DockBuilderDockWindow(UI::GetFixedWindowNameWithID(FA_TERMINAL " Compiler Results", handle).c_str(), resultsPanel);
		ImGui::DockBuilderDockWindow(UI::GetFixedWindowNameWithID("Preview", handle).c_str(), previewPanel);
		ImGui::DockBuilderDockWindow(UI::GetFixedWindowNameWithID("Preview Parameters", handle).c_str(), previewParametersPanel);
		ImGui::DockBuilderDockWindow(UI::GetFixedWindowNameWithID("Graph Properties", handle).c_str(), graphPropertiesPanel);
		ImGui::DockBuilderDockWindow(UI::GetFixedWindowNameWithID("Details", handle).c_str(), detailsPanel);

		ImGui::DockBuilderFinish(dockspace);
	}

	void AnimationGraphPanel::OnImGuiRender(bool& open)
	{
		m_Viewport.OnPreImGuiRender();

		const std::filesystem::path assetPath = AssetManager::GetMetadata(m_AnimationGraph->Handle).FilePath;

		ImGui::PushID(m_AnimationGraph->Handle);
		ImGui::PushID("##Animation");

		if (m_Focus)
		{
			ImGui::SetNextWindowFocus();
			m_Focus = false;
		}

		ImGuiWindowClass windowClass = UI::CreateDockingRestrictionClass(fmt::format("##AnimationDockClass{}", m_AnimationGraph->Handle).c_str());
		UI::CenterAppearingWindow(ImVec2(1750.0f, 850.0f));
		const ImGuiID dockspace = UI::BeginDockspaceWindow(Utils::GetViewerWindowName(FILE_ICON_ANIMATION_GRAPH, m_AnimationGraph->Handle, assetPath, "Animation Graph Viewer").c_str(), &open, ImGuiWindowFlags_MenuBar, &windowClass);
		DrawMenuBar();

		// Setup dockspace
		if (ImGui::IsWindowAppearing())
			SetupDockspace(dockspace, m_AnimationGraph->Handle);

		ImGui::End();

		// Draw main node graph
		ImGui::SetNextWindowClass(&windowClass);
		UI::BeginDockedGraphWindow(CHARACTER_ICON_PROJECTION_ORTHOGRAPHIC " Animation Graph", m_AnimationGraph->Handle);
		DrawNodeGraph();
		ImGui::End();

		// Draw other editor windows
		ImGui::SetNextWindowClass(&windowClass);
		UI::DrawCompilerResultWindow(m_AnimationGraph->Handle, m_Compiler.GetCompilerResult());

		ImGui::SetNextWindowClass(&windowClass);
		DrawGraphPropertiesPanel();

		ImGui::SetNextWindowClass(&windowClass);
		DrawDetailsPanel();

		if (m_ShowPreview)
		{
			ImGui::SetNextWindowClass(&windowClass);
			UI::BeginDockedGraphWindow("Preview", m_AnimationGraph->Handle);
			m_Viewport.OnImGuiRender();
			ImGui::End();

			ImGui::SetNextWindowClass(&windowClass);
			UI::BeginDockedGraphWindow("Preview Parameters", m_AnimationGraph->Handle);
			ImGui::End();
		}

		ImGui::PopID();
		ImGui::PopID();
	}

	const CompilerResult& AnimationGraphPanel::Compile()
	{
		m_Compiler.Compile();

		// If compilation succeeded override the active material
		if (const Ref<AnimationGraph> newAnimationGraph = m_Compiler.GetAnimationGraph())
		{
			m_AnimationGraph = AssetManager::OverrideAsset<AnimationGraph>(m_AnimationGraph->Handle, newAnimationGraph);
			AssetManager::SerializeAsset(m_AnimationGraph->Handle);

			// Invalidate the asset thumbnail
			ThumbnailManager::InvalidateThumbnail(m_AnimationGraph->Handle);
		}

		return m_Compiler.GetCompilerResult();
	}

	NodeHandle AnimationGraphPanel::OnAssetDropped(const AssetMetadata& metadata)
	{
		if (metadata.Type == AssetType::Animation)
		{
			const Ref<AnimationEditorNode> context = As<AnimationEditorNode>(GetCurrentLayerContext());

			if (context && (context->Function == AnimationNodeFunction::Blendspace || context->Function == AnimationNodeFunction::Blendspace1D))
				return m_AnimationGraph->m_EditorGraph->SpawnBlendspaceAnimationNode(metadata.Handle);

			return m_AnimationGraph->m_EditorGraph->SpawnPlayerNode(metadata.Handle);
		}

		return 0;
	}

	glm::vec4 AnimationGraphPanel::GetNodeColor(const Ref<EditorNode> internalNode) const
	{
		const Ref<AnimationEditorNode> node = As<AnimationEditorNode>(internalNode);
		return ImVec4(Utils::GetNodeFunctionColor(node->Function));
	}

	void AnimationGraphPanel::DrawNodeHeader(const Ref<EditorNode> internalNode)
	{
		Ref<AnimationEditorNode> node = As<AnimationEditorNode>(internalNode);

		if (node->Function == AnimationNodeFunction::Parameter || node->Function == Editor::AnimationNodeFunction::State)
		{
			ImGui::TextUnformatted(Utils::GetNodeFunctionIcon(node->Function));
			ImGui::SelectableInput("##AnimationParameterInputText", node->Name.empty() ? 1.0f : ImGui::CalcTextSize(node->Name.c_str()).x, false, 0, &node->Name, nullptr, ImGuiInputTextFlags_NoHorizontalScroll);
		}
		else
			ImGui::TextUnformatted(fmt::format("{} {}", Utils::GetNodeFunctionIcon(node->Function), Utils::GetNodeName(node)).c_str());
	}

	void AnimationGraphPanel::DrawNodeHeaderSimple(const Ref<EditorNode> internalNode) const
	{
		const Ref<AnimationEditorNode> node = As<AnimationEditorNode>(internalNode);

		if (Utils::IsLogicalNodeFunction(node->Function))
			ImGui::TextUnformatted(AnimationEditorGraph::GetNodeFunctionName(node->Function));
		else
			ImGui::TextUnformatted(Utils::GetNodeFunctionIcon(node->Function));
	}

	void AnimationGraphPanel::DrawNodeThumbnail(const Ref<EditorNode> internalNode) const
	{
	}

	void AnimationGraphPanel::DrawPinIcon(const Ref<EditorPin> internalPin, const bool linked, const float alpha) const
	{
		Ref<AnimationPin> pin = As<AnimationPin>(internalPin);

		ImVec4 color = Utils::GetPinColor(pin->Type);
		color.w = alpha;

		const ImVec2 size = ImVec2(UI::GraphConstants::PinIconSize, UI::GraphConstants::PinIconSize);

		if (pin->Type == AnimationPinType::Pose)
		{
			UI::PushFont(linked ? FontType::FASolidIcons : FontType::FARegularIcons);
			ImGui::GetWindowDrawList()->AddText(ImGui::GetCursorPos() - ImVec2(2.0f, 12.0f), ImGui::GetColorU32(color), FA_PERSON);
			ImGui::Dummy(size);
			UI::PopFont();
		}
		else
			ax::Widgets::Icon(size, IconType::Circle, linked, color, ImColor(32, 32, 32, (int)(alpha * 255.0f)));
	}

	void AnimationGraphPanel::DrawGraphPropertiesPanel()
	{
		UI::BeginDockedGraphWindow("Graph Properties", m_AnimationGraph->Handle);

		UI::DrawAssetSelectionDropdown(AssetType::Skeleton, m_Compiler.TargetSkeleton);

		ImGui::End();
	}

	void AnimationGraphPanel::DrawDetailsPanel()
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		UI::BeginDockedGraphWindow("Details", m_AnimationGraph->Handle);

		if (Ref<AnimationEditorNode> node = m_AnimationGraph->m_EditorGraph->FindNode(m_SelectedNodeID))
		{
			const bool commentNode = node->Type == NodeType::Comment;

			ImGui::TextDisabled(FA_BARS " Node Properties");
			ImGui::Text(AnimationEditorGraph::GetNodeFunctionName(node->Function));
			ImGui::Separator();

			ImGui::Text(FA_COMMENT " Comment");

			if (!commentNode)
			{
				ImGui::SameLine();
				ImGui::Checkbox("##AnimationCommentCheckbox", &node->CommentEnabled);
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
			
			if (!node->Inputs.empty() && UI::CollapsingHeader("Inputs"))
			{
				for (auto& pin : node->Inputs)
				{
					Ref<AnimationPin> input = As<AnimationPin>(pin);

					if (input->Editable)
					{
						const bool linked = ed::PinHadAnyLinks(input->ID);

						ImGui::PushID(input->ID);
						ImGui::TextColored(Utils::GetPinColor(input->Type), input->Type == AnimationPinType::Pose ? FA_PERSON : linked ? FA_CIRCLE : FA_CIRCLE_DOT);
						ImGui::SameLine();
						ImGui::Text(input->Name.c_str());

						if (!linked)
						{
							ImGui::SameLine();
							ImGui::SetNextItemWidth(-1);
							DrawNodeDefaultValueInput(node->Function, input->Name, input->Type, input->Data);
						}

						ImGui::PopID();
					}
					else
						ImGui::TextDisabled(input->Name.c_str());
				}

				ImGui::TreePop();
			}

			if (node->Function == AnimationNodeFunction::LayeredBoneBlend && UI::CollapsingHeader("Configuration"))
			{
				auto& blendPoseFilters = As<AnimationEditorLayeredBoneBlendNode>(node)->BlendPoseFilters;

				// Resize as needed
				const uint32_t blendPosePinCount = (node->Inputs.size() - 1) / 2;
				if (blendPoseFilters.size() != blendPosePinCount)
					blendPoseFilters.resize(blendPosePinCount);
 
				for (uint32_t blendPoseIndex = 0; blendPoseIndex < blendPosePinCount; blendPoseIndex++)
				{
					auto& filters = blendPoseFilters[blendPoseIndex];

					bool open = UI::CollapsingHeader(fmt::format("Blend {}", blendPoseIndex).c_str());

					if (open)
						ImGui::Unindent();
					ImGui::SameLine(ImGui::GetContentRegionAvailWidth());
					if (UI::DrawTextIconButton(FA_PLUS))
						filters.emplace_back();

					if (open)
						ImGui::Indent();

					if (open)
					{
						uint32_t filterToRemove = -1;

						for (uint32_t filterIndex = 0; filterIndex < filters.size(); filterIndex++)
						{
							ImGui::PushID(filterIndex);
							auto& filter = filters[filterIndex];

							ImGui::TextDisabled(fmt::format("Filter {}", filterIndex).c_str());
							ImGui::SameLine(ImGui::GetContentRegionAvailWidth() + style.IndentSpacing);
							if (UI::DrawTextIconButton(FA_TRASH))
								filterToRemove = filterIndex;

							ImGui::Indent();

							ImGui::Text("Bone Name");
							ImGui::SameLine();
							ImGui::InputText("##BoneNameInput", &filter.BoneName);

							ImGui::Text("Blend Depth");
							ImGui::SameLine();
							ImGui::DragInt("##BlendDepthInput", &filter.BlendDepth);

							ImGui::Unindent();
							ImGui::PopID();
						}

						if (filterToRemove != -1)
							filters.erase(filters.begin() + filterToRemove);

						ImGui::TreePop();
					}
				}

				ImGui::TreePop();
			}
		}

		ImGui::End();
	}

	void AnimationGraphPanel::NodeSelectionInput(Editor::NodeHandle& handle, const AnimationNodeFunction function)
	{
		const float buttonSize = ImGui::GetTextLineHeight();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth() - buttonSize - ImGui::GetStyle().FramePadding.x * 2.0f);

		if (ImGui::BeginCombo("##NodeSelection", handle == 0 ? "Select Node" : (m_InternalEditorGraph->DoesNodeExist(handle) ? Utils::GetNodeName(m_AnimationGraph->m_EditorGraph->FindNode(handle)).c_str() : "Unknown Node")))
		{
			const auto& nodes = m_InternalEditorGraph->Nodes;
			for (const auto& [nodeID, internalNode] : nodes)
			{
				const Ref<AnimationEditorNode> node = As<AnimationEditorNode>(internalNode);

				if (node->Function != function)
					continue;

				if (ImGui::MenuItem(Utils::GetNodeName(node).c_str()))
					handle = nodeID;
			}

			ImGui::EndCombo();
		}

		ImGui::SameLine();

		
		if (UI::DrawTextIconButton(FA_MAGNIFYING_GLASS, ImVec2(buttonSize, buttonSize)))
			NavigateToNode(handle);
	}

	void AnimationGraphPanel::DrawNodeDefaultValueInput(const AnimationNodeFunction nodeFunction, const std::string& name, const AnimationPinType type, AnimationPinData& data)
	{
		// Handle specific nodes (such as enums)
		if (nodeFunction == AnimationNodeFunction::FABRIK && name == "Tip Rotation Source")
		{
			const char* const values[] = { "Keep Local Space Rotation", "Copy From Effector Target", "Keep Component Space Rotation" };
			ImGui::Combo("##RotationSourceDropdown", &data.Int, values, IM_ARRAYSIZE(values));
			return;
		}

		if (nodeFunction == AnimationNodeFunction::TransformBone && type == AnimationPinType::Int)
		{
			const char* const values[] = { "Ignore", "Replace Existing", "Add to Existing" };
			ImGui::Combo("##TransformModeDropdown", &data.Int, values, IM_ARRAYSIZE(values));
			return;
		}

		// Otherwise use default input
		switch (type)
		{
		case AnimationPinType::Pose: return ImGui::Dummy(ImVec2());
		case AnimationPinType::Bone: return UI::DrawNodeDefaultValueInput(UI::ValueInputType::String, &data.String);
		case AnimationPinType::Bool: return UI::DrawNodeDefaultValueInput(UI::ValueInputType::Bool, &data.Bool);
		case AnimationPinType::Int: return UI::DrawNodeDefaultValueInput(UI::ValueInputType::Int, &data.Int);
		case AnimationPinType::Float: return UI::DrawNodeDefaultValueInput(UI::ValueInputType::Float, &data.Float);
		case AnimationPinType::Vector2: return UI::DrawNodeDefaultValueInput(UI::ValueInputType::Vector2, &data.Vector2);
		case AnimationPinType::Vector3: return UI::DrawNodeDefaultValueInput(UI::ValueInputType::Vector3, &data.Vector3);
		case AnimationPinType::Vector4: return UI::DrawNodeDefaultValueInput(UI::ValueInputType::Vector4, &data.Vector4);
		case AnimationPinType::Transform: return UI::DrawNodeDefaultValueInput(UI::ValueInputType::Transform, &data.Transform);
		case AnimationPinType::Animation: return UI::DrawNodeDefaultValueInput(UI::ValueInputType::Handle, &data.Handle, AssetType::Animation);
		case AnimationPinType::Player: return NodeSelectionInput(data.Handle, AnimationNodeFunction::Player);
		}
	}

	std::string AnimationGraphPanel::GetLayerStackLabel(const Ref<Editor::EditorNode> internalNode) const
	{
		Ref<AnimationEditorNode> node = As<AnimationEditorNode>(internalNode);
		return fmt::format("{} {}", Utils::GetNodeFunctionIcon(node->Function), AnimationEditorGraph::GetNodeFunctionName(node->Function));
	}

	void AnimationGraphPanel::UpdateSearch()
	{
		NodeSearchTreeBuilder<AnimationPinType> builder(&m_SearchTree, m_SearchBuffer);

		if (m_ContextSensitive && m_NewNodeLinkPinID != 0)
		{
			Ref<AnimationPin> pin = m_AnimationGraph->m_EditorGraph->FindPin(m_NewNodeLinkPinID);
			builder.SetContext(pin->Type, pin->Kind);
		}

		if (m_NewNodeLinkPinID == 0 || !m_ContextSensitive)
		{
			const bool hasSelection = ed::GetSelectedObjectCount() > 0;
			if (hasSelection)
			{
				ImVec2 min, max;
				ed::GetSelectionBounds(min, max);
				const ImVec2 size = max - min + ImVec2(UI::GraphConstants::CommentPadding, UI::GraphConstants::CommentPadding) * 2.0f;
				builder.AddResult("Add Comment to Selection", { "" }, "label description", {}, {}, [size, this]() { return m_AnimationGraph->m_EditorGraph->SpawnCommentNode(size); });
			}
			else
				builder.AddResult("Add Comment...", { "" }, "label description", {}, {}, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnCommentNode(); });
		}

		// TODO: Display all animation files available on the skeleton
		builder.AddResult("Player", { "Animation" }, "animation", { AnimationPinType::Animation }, { AnimationPinType::Pose }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnPlayerNode(0); });

		builder.AddResult("Additive", { "Blending" }, "addition add combine", { AnimationPinType::Pose, AnimationPinType::Float }, { AnimationPinType::Pose }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnAdditiveNode(); });
		builder.AddResult("Blend", { "Blending" }, "combine morph", { AnimationPinType::Pose, AnimationPinType::Float }, { AnimationPinType::Pose }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnBlendNode(); });
		builder.AddResult("Blend by Bool", { "Blending" }, "combine morph switch if or boolean", { AnimationPinType::Pose, AnimationPinType::Bool }, { AnimationPinType::Pose }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnBoolBlendNode(); });
		builder.AddResult("Blend by Int", { "Blending" }, "combine morph switch integer", { AnimationPinType::Pose, AnimationPinType::Int }, { AnimationPinType::Pose }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnIntBlendNode(); });
		builder.AddResult("Layered Blend per Bone", { "Blending" }, "combine morph", { AnimationPinType::Pose, AnimationPinType::Float }, { AnimationPinType::Pose }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnLayeredBoneBlendNode(); });
		
		builder.AddResult("State Machine", { "States" }, "states controller actions", {}, { AnimationPinType::Pose }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnStateMachineNode(); });
		builder.AddResult("State", { "States" }, "states actions", { AnimationPinType::Pose }, { AnimationPinType::Pose }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnStateNode(); });
		builder.AddResult("Blendspace", { "Blending", "States" }, "morph offset 2d dimension", { AnimationPinType::Float }, { AnimationPinType::Pose }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnBlendspaceNode(); });
		builder.AddResult("Blendspace 1D", { "Blending", "States" }, "morph offset dimension", { AnimationPinType::Float }, { AnimationPinType::Pose }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnBlendspace1DNode(); });
		builder.AddResult("Blendspace Animation", { "Blending", "States" }, {}, {}, {}, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnBlendspaceAnimationNode(0); });
		builder.AddResult("Blendspace Subgraph", { "Blending", "States" }, {}, {}, {}, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnBlendspaceGraphNode(); });
		
		builder.AddResult("Two Bone IK", { "Inverse Kinematics" }, "inverse kinematics solver joint constraints", { AnimationPinType::Vector3, AnimationPinType::Pose, AnimationPinType::Float }, { AnimationPinType::Pose }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnTwoBoneIKNode(); });
		builder.AddResult("FABRIK", { "Inverse Kinematics" }, "inverse kinematics solver joint constraints", { AnimationPinType::Transform, AnimationPinType::Pose, AnimationPinType::Float }, { AnimationPinType::Pose }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnFABRIKNode(); });
		
		builder.AddResult("Transform Bone", { "Transform" }, "move translate rotate scale", { AnimationPinType::Vector3, AnimationPinType::Pose, AnimationPinType::Float }, { AnimationPinType::Pose }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnTransformBoneNode(); });

		builder.AddResult("Component To Local Space", { "Space", "Conversions" }, "bone convert conversion space", { AnimationPinType::Pose }, { AnimationPinType::Pose }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnComponentToLocalSpaceNode(); });
		builder.AddResult("Local To Component Space", { "Space", "Conversions" }, "bone convert conversion space", { AnimationPinType::Pose }, { AnimationPinType::Pose }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnLocalToComponentSpaceNode(); });

		builder.AddResult("Parameter Bool", { "Parameters" }, "variable boolean", {}, { AnimationPinType::Bool }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnParameterNode(AnimationPinType::Bool); });
		builder.AddResult("Parameter Int", { "Parameters" }, "variable integer", {}, { AnimationPinType::Int }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnParameterNode(AnimationPinType::Int); });
		builder.AddResult("Parameter Float", { "Parameters" }, "variable single", {}, { AnimationPinType::Float }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnParameterNode(AnimationPinType::Float); });
		builder.AddResult("Parameter Vector2", { "Parameters" }, "variable float2", {}, { AnimationPinType::Vector2 }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnParameterNode(AnimationPinType::Vector2); });
		builder.AddResult("Parameter Vector3", { "Parameters" }, "variable float3", {}, { AnimationPinType::Vector3 }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnParameterNode(AnimationPinType::Vector3); });
		builder.AddResult("Parameter Vector4", { "Parameters" }, "variable float4", {}, { AnimationPinType::Vector4 }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnParameterNode(AnimationPinType::Vector4); });
		builder.AddResult("Parameter Transform", { "Parameters" }, "variable", {}, { AnimationPinType::Transform }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnParameterNode(AnimationPinType::Transform); });
		
		builder.AddResult("Time Remaining (Ratio)", { "Animation" }, "animation asset", {}, { AnimationPinType::Float }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnTimeRemainingRatioNode(); });
		
		builder.AddResult("AND", { "Logic|Boolean" }, "&&", { AnimationPinType::Bool }, { AnimationPinType::Bool }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnANDNode(); });
		builder.AddResult("OR", { "Logic|Boolean" }, "||", { AnimationPinType::Bool }, { AnimationPinType::Bool }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnORNode(); });
		builder.AddResult("NAND", { "Logic|Boolean" }, {}, { AnimationPinType::Bool }, { AnimationPinType::Bool }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnNANDNode(); });
		builder.AddResult("NOR", { "Logic|Boolean" }, {}, { AnimationPinType::Bool }, { AnimationPinType::Bool }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnNORNode(); });
		builder.AddResult("XOR", { "Logic|Boolean" }, "^", { AnimationPinType::Bool }, { AnimationPinType::Bool }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnXORNode(); });
		builder.AddResult("NOT", { "Logic|Boolean" }, "~ !", { AnimationPinType::Bool }, { AnimationPinType::Bool }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnNOTNode(); });
		
		builder.AddResult("Equals", { "Logic|Numeric" }, "equality ==", { AnimationPinType::Float }, { AnimationPinType::Bool }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnEqualityNode(); });
		builder.AddResult("Not Equals", { "Logic|Numeric" }, "inequality !=", { AnimationPinType::Float }, { AnimationPinType::Bool }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnInequalityNode(); });
		builder.AddResult("Less Than", { "Logic|Numeric" }, "fewer lower <", { AnimationPinType::Float }, { AnimationPinType::Bool }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnLessThanNode(); });
		builder.AddResult("Less Than Or Equal", { "Logic|Numeric" }, "fewer lower <=", { AnimationPinType::Float }, { AnimationPinType::Bool }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnLessThanOrEqualNode(); });
		builder.AddResult("Greater Than", { "Logic|Numeric" }, "more higher >", { AnimationPinType::Float }, { AnimationPinType::Bool }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnGreaterThanNode(); });
		builder.AddResult("Greater Than Or Equal", { "Logic|Numeric" }, "more higher >=", { AnimationPinType::Float }, { AnimationPinType::Bool }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnGreaterThanOrEqualNode(); });
		
		builder.AddResult("Add", { "Math" }, "plus +", { AnimationPinType::Float }, { AnimationPinType::Float }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnAddNode(); });
		builder.AddResult("Subtract", { "Math" }, "minus -", { AnimationPinType::Float }, { AnimationPinType::Float }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnSubtractNode(); });
		builder.AddResult("Multiply", { "Math" }, "times *", { AnimationPinType::Float }, { AnimationPinType::Float }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnMultiplyNode(); });
		builder.AddResult("Divide", { "Math" }, "fraction /", { AnimationPinType::Float }, { AnimationPinType::Float }, [&]() { return m_AnimationGraph->m_EditorGraph->SpawnDivideNode(); });
	}

	const glm::vec4 AnimationGraphPanel::GetPinColor(const Ref<EditorPin> pin) const
	{
		return ImVec4(Utils::GetPinColor(As<AnimationPin>(pin)->Type));
	}

	static void DrawScaleAxis(const ImVec2 start, const ImVec2 end, const float scaleFactor, const bool drawScale = true)
	{
		const float zoom = ed::GetCurrentZoom();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddLine(start, end, IM_COL32(200, 200, 200, 255), 2.0f * zoom);

		if (!drawScale)
			return;

		const float scale = 32.0f;
		const float incrementStep = round(zoom) * scale;

		// Calculate perpendicular vector
		const ImVec2 direction = end - start;
		ImVec2 perp(-direction.y, direction.x);
		float length = sqrtf(direction.x * direction.x + direction.y * direction.y);
		if (length > 0.0f)
			perp = ImVec2(perp.x / length, perp.y / length);

		for (float t = 0.0f; t < length; t += incrementStep)
		{
			const ImVec2 pointOnAxis = ImVec2(roundf(start.x / scale) * scale, roundf(start.y / scale) * scale) + direction * (t / length);
			const ImVec2 tickStart = pointOnAxis - perp * 5.0f * zoom;
			const ImVec2 tickEnd = pointOnAxis + perp * 5.0f * zoom;

			const float sign = glm::length(glm::vec2(pointOnAxis + glm::normalize(glm::vec2(direction)) * 0.001f)) < glm::length(glm::vec2(pointOnAxis)) ? -1.0f : 1.0f;

			drawList->AddLine(tickStart, tickEnd, IM_COL32(255, 255, 255, 200));
			const std::string label = fmt::format("{}", sign * scaleFactor * roundf(sqrt(pointOnAxis.x * pointOnAxis.x + pointOnAxis.y * pointOnAxis.y) / scale));
			drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * zoom, tickEnd + perp * zoom, IM_COL32_WHITE, label.c_str());
		}
	}

	void AnimationGraphPanel::DrawGraphContents()
	{
		// Draw grid for blendspace graphs
		const Ref<AnimationEditorNode> context = As<AnimationEditorNode>(GetCurrentLayerContext());
		if (!context || (context->Function != AnimationNodeFunction::Blendspace && context->Function != AnimationNodeFunction::Blendspace1D))
			return;

		const ImGuiStyle& style = ImGui::GetStyle();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		const bool isBlendspace = context->Function == AnimationNodeFunction::Blendspace;

		ImVec2 min, max;
		ed::GetViewRect(min, max);
		DrawScaleAxis(ImVec2(min.x, 0.0f), ImVec2(max.x, 0.0f), context->GetInput(isBlendspace ? 2 : 1)->Data.Float);
		DrawScaleAxis(ImVec2(0.0f, max.y), ImVec2(0.0f, min.y), isBlendspace ? context->GetInput(3)->Data.Float : 0.0f, isBlendspace);

		// Draw triangulation lines

		if (isBlendspace && (m_ShowBlendspaceTriangulation || m_ShowBlendspaceOuterEdge))
		{
			std::vector<DelaunayVertex> vertices;
			std::vector<DelaunayTriangleResult> triangles;

			for (const auto& [nodeID, node] : m_InternalEditorGraph->Nodes)
				if (node->Type == NodeType::Point)
					vertices.emplace_back(node->Position);

			DelaunaryTriangulate(vertices, triangles);

			if (m_ShowBlendspaceTriangulation)
			{
				const ImU32 triangleColor = IM_COL32(100, 100, 100, 255);
				for (const auto& triangle : triangles)
				{
					drawList->AddLine(vertices[triangle.V0], vertices[triangle.V1], triangleColor, 2.0f);
					drawList->AddLine(vertices[triangle.V1], vertices[triangle.V2], triangleColor, 2.0f);
					drawList->AddLine(vertices[triangle.V2], vertices[triangle.V0], triangleColor, 2.0f);
				}
			}

			if (m_ShowBlendspaceOuterEdge)
			{
				std::vector<DelaunayEdgeResult> edges;
				DelaunaryTraceOutsideEdges(triangles, edges);

				const ImU32 edgeColor = IM_COL32(120, 70, 70, 255);
				for (const auto& edge : edges)
					drawList->AddLine(vertices[edge.V0], vertices[edge.V1], edgeColor, 3.0f);
			}
		}

		// Draw Labels
		if (m_ShowBlendspaceLabels)
		{
			const LayerHandle currentLayer = GetCurrentLayerID();

			for (const auto& [nodeID, node] : m_InternalEditorGraph->Nodes)
			{
				if (node->Type != NodeType::Point)
					continue;

				if (node->Layer != currentLayer)
					continue;

				const ImVec2 labelPosition = node->Position + glm::vec2(0.0f, 30.0f);
				const std::string label = Utils::GetNodeName(As<AnimationEditorNode>(node));
				const ImVec2 labelHalfSize = ImGui::CalcTextSize(label.c_str()) * 0.5f;
				drawList->AddRectFilled(labelPosition - labelHalfSize - style.FramePadding, labelPosition + labelHalfSize + style.FramePadding, IM_COL32(50, 50, 50, 255), 5.0f);
				drawList->AddText(labelPosition - labelHalfSize, IM_COL32_WHITE, label.c_str());
			}
		}
	}

	void AnimationGraphPanel::DrawViewMenu()
	{
		ImGui::MenuItem(FA_MAGNIFYING_GLASS " Show Preview", nullptr, &m_ShowPreview);

		const Ref<AnimationEditorNode> context = As<AnimationEditorNode>(GetCurrentLayerContext());
		if (!context || (context->Function != AnimationNodeFunction::Blendspace && context->Function != AnimationNodeFunction::Blendspace1D))
			return;

		ImGui::Separator();
		ImGui::MenuItem(FA_TAG " Show Labels", nullptr, &m_ShowBlendspaceLabels);

		if (context->Function == AnimationNodeFunction::Blendspace1D)
			return;

		ImGui::MenuItem(FA_TRIANGLE " Show Triangulation", nullptr, &m_ShowBlendspaceTriangulation);
		ImGui::MenuItem(FA_VECTOR_POLYGON " Highlight Outer Edge", nullptr, &m_ShowBlendspaceOuterEdge);
	}

	void AnimationGraphPanel::OnNodeTooltip(const Ref<EditorNode> internalNode)
	{
		const Ref<AnimationEditorNode> node = As<AnimationEditorNode>(internalNode);

		if (node->Function != Editor::AnimationNodeFunction::BlendspaceAnimation && node->Function != Editor::AnimationNodeFunction::BlendspaceGraph)
			return;

		const Ref<AnimationEditorNode> context = As<AnimationEditorNode>(GetCurrentLayerContext());

		if (!context)
			return;

		ImGui::BeginTooltip();

		if (node->Function == Editor::AnimationNodeFunction::BlendspaceAnimation)
		{
			const AssetHandle handle = node->GetInput(0)->Data.Handle;
			ImGui::TextUnformatted(UI::GetAssetHandleLabel(handle).c_str());
		}

		if (context->Function == AnimationNodeFunction::Blendspace)
		{
			const float scaleX = context->GetInput(2)->Data.Float;
			const float scaleY = context->GetInput(3)->Data.Float;
			ImGui::TextDisabledUnformatted(fmt::format("({}, {})", node->Position.x * scaleX / 32.0f, node->Position.y * scaleY / 32.0f).c_str());
		}
		else if (context->Function == AnimationNodeFunction::Blendspace1D)
		{
			const float scaleX = context->GetInput(1)->Data.Float;
			ImGui::TextDisabledUnformatted(fmt::format("({})", node->Position.x * scaleX / 32.0f).c_str());
		}

		ImGui::EndTooltip();
	}

	void AnimationGraphPanel::OnNodeDoubleClicked(const Ref<EditorNode> internalNode)
	{
	}

}