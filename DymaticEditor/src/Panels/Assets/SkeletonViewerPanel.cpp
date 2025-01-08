#include "Panels/Assets/SkeletonViewerPanel.h"

#include "Dymatic/Asset/AssetManager.h"
#include "Dymatic/Renderer/SceneRenderer.h"

#include "EditorResources.h"
#include "Panels/Assets/Utils/AssetViewerPanelUtils.h"
#include "Fonts.h"
#include <imgui_internal.h>

namespace Dymatic {

	static const glm::mat4& PopulateSkeletonBoneTransforms(const BoneNodeData& node, const std::unordered_map<std::string, BoneInfo>& boneInfoMap,
		std::vector<SkeletonViewerPanel::DebugBone>& debugBones, std::vector<std::pair<glm::vec3, glm::vec3>>& lines, const glm::mat4& parentTransform)
	{
		if (boneInfoMap.find(node.Name) == boneInfoMap.end())
			return glm::mat4(1.0f);

		const auto& boneInfo = boneInfoMap.at(node.Name);
		auto& debugBone = debugBones[boneInfo.id];

		debugBone.Transform = glm::inverse(boneInfo.offset);
		const glm::vec3 position = debugBone.Transform[3];

		float boneLength;
		std::vector<glm::vec3> childPositions;

		if (node.Children.empty())
		{
			// If no children, transform is based off parent
			boneLength = glm::distance(position, glm::vec3(parentTransform[3]));
		}
		else
		{
			// Transform is based off average distance to all children
			boneLength = 0.0f;
			for (const auto& child : node.Children)
			{
				const glm::mat4& childTransform = PopulateSkeletonBoneTransforms(child, boneInfoMap, debugBones, lines, debugBone.Transform);
				const glm::vec3 childPosition = glm::vec3(childTransform[3]);
				childPositions.emplace_back(childPosition);
				boneLength += glm::distance(position, childPosition);
			}
			boneLength /= (float)node.Children.size();
		}

		debugBone.Length = boneLength;

		// Scale the bone by the desired length then apply the original translation and rotation
		debugBone.Transform *= glm::scale(glm::mat4(1.0f), glm::vec3(boneLength));

		// Add a 'connection' line indicator if there are multiple children to all of them.
		// This line should go from the 'end' of the current bone to the head of the child
		if (node.Children.size() > 1)
		{
			// Take 'forward' vector of bone matrix and move along by length to find tail
			glm::vec3 tailPosition = position + glm::normalize(glm::vec3(debugBone.Transform[1])) * boneLength;

			for (const auto& childPosition : childPositions)
				lines.emplace_back(tailPosition, childPosition);
		}

		return debugBone.Transform;
	}

	SkeletonViewerPanel::SkeletonViewerPanel(Ref<Skeleton> skeleton)
		: m_Skeleton(skeleton)
	{
		// Compute and cache each bone visualizer's transform
		m_DebugBones.resize(m_Skeleton->GetBoneCount());
		PopulateSkeletonBoneTransforms(m_Skeleton->GetRootNode(), m_Skeleton->GetBoneInfoMap(), m_DebugBones, m_ConnectionLines, glm::mat4(1.0f));
	}

	void SkeletonViewerPanel::OnUpdate(Timestep ts)
	{
		m_Viewport.BeginViewportRender(ts);

		// Draw the skeleton bones
		for (size_t debugBoneIndex = 0; debugBoneIndex < m_DebugBones.size(); debugBoneIndex++)
		{
			const auto& transform = m_DebugBones[debugBoneIndex].Transform;
			const bool selected = m_Viewport.GetSelectedID() == debugBoneIndex;
			SceneRenderer::SubmitModel(transform, EditorResources::BoneMesh, debugBoneIndex, selected);
		}

		m_Viewport.SubmitViewportRender();

		Renderer2D::BeginScene(m_Viewport.GetEditorCamera());
		for (const auto& [p0, p1] : m_ConnectionLines)
			Renderer2D::DrawLineDashed(p0, p1, glm::vec4(1.0f));
		Renderer2D::EndScene();

		m_Viewport.EndViewportRender();
	}

	static void SetupDockspace(ImGuiID dockspace, uint64_t handle)
	{
		// Clear the current dockspace layout
		ImGui::DockBuilderRemoveNodeChildNodes(dockspace);

		const float viewportPanelRatio = 0.60f;

		ImGuiID mainPanel = ImGui::DockBuilderGetNode(dockspace)->ID;

		// Split dockspace
		ImGuiID leftPanel, rightPanel;
		ImGui::DockBuilderSplitNode(mainPanel, ImGuiDir_Left, viewportPanelRatio, &leftPanel, &rightPanel);
		ImGui::DockBuilderGetNode(leftPanel)->LocalFlags |= ImGuiDockNodeFlags_HiddenTabBar;
		ImGui::DockBuilderGetNode(rightPanel)->LocalFlags |= ImGuiDockNodeFlags_HiddenTabBar;
		ImGui::DockBuilderDockWindow(UI::GetFixedWindowNameWithID("Viewport", handle).c_str(), leftPanel);
		ImGui::DockBuilderDockWindow(UI::GetFixedWindowNameWithID("Details", handle).c_str(), rightPanel);
	}

	void SkeletonViewerPanel::OnImGuiRender(bool& open)
	{
		m_Viewport.OnPreImGuiRender();

		const AssetMetadata& metadata = AssetManager::GetMetadata(m_Skeleton->Handle);

		if (m_Focus)
		{
			ImGui::SetNextWindowFocus();
			m_Focus = false;
		}

		const ImGuiStyle& style = ImGui::GetStyle();
		const ImGuiIO& io = ImGui::GetIO();

		ImGuiWindowClass windowClass = UI::CreateDockingRestrictionClass(fmt::format("##SkeletonDockClass{}", m_Skeleton->Handle).c_str());
		UI::CenterAppearingWindow(0.75f);
		const ImGuiID dockspace = UI::BeginDockspaceWindow(Utils::GetViewerWindowName(FILE_ICON_SKELETON, m_Skeleton->Handle, metadata.FilePath, "Skeleton Viewer").c_str(), &open, (m_Viewport.IsHovered() && ImGui::GetIO().KeyAlt) ? ImGuiWindowFlags_NoMove : 0, &windowClass);

		// Setup dockspace
		if (ImGui::IsWindowAppearing())
			SetupDockspace(dockspace, m_Skeleton->Handle);

		ImGui::End();

		ImGui::SetNextWindowClass(&windowClass);
		ImGui::Begin(UI::GetFixedWindowNameWithID("Viewport", m_Skeleton->Handle).c_str());

		m_Viewport.OnImGuiRender();

		const auto hoveredID = m_Viewport.GetHoveredID();
		m_BoneHoveredTime = (hoveredID == m_Viewport.GetPreviousHoveredID()) ? (m_BoneHoveredTime + io.DeltaTime) : 0.0f;

		if (m_BoneHoveredTime >= Preferences::GetData().TooltipHoverDelay * 0.001f && m_Skeleton->IsValidBoneID(hoveredID))
		{
			const BoneNodeData& hoveredBone = m_Skeleton->GetBoneNodeData(hoveredID);
			const DebugBone& hoveredDebugBone = m_DebugBones[hoveredID];

			ImGui::BeginTooltip();

			ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos() + style.FramePadding, ImGui::GetWindowPos() + ImVec2(ImGui::GetWindowSize().x - style.FramePadding.x, 30.0f), ImGui::GetColorU32(ImGuiCol_TextDisabled), style.WindowRounding);

			UI::PushFont(FontType::Bold);
			ImGui::Dummy(ImVec2((ImGui::GetContentRegionAvailWidth() - ImGui::CalcTextSize(hoveredBone.Name.c_str()).x - 50.0f) * 0.5f, 0.0f));
			UI::PopFont();
			ImGui::SameLine();
			ImGui::TextUnformatted(FA_BONE);
			ImGui::SameLine();
			UI::PushFont(FontType::Bold);
			ImGui::TextUnformatted(hoveredBone.Name.c_str());
			UI::PopFont();

			ImGui::Dummy(ImVec2(0.0f, 10.0f));

			if (const BoneNodeData* parent = m_Skeleton->GetBoneParent(hoveredID))
			{
				ImGui::TextDisabled(FA_PERSON_BREASTFEEDING " Parent");
				ImGui::SameLine();
				ImGui::TextUnformatted(parent->Name.c_str());
			}

			if (hoveredBone.Children.empty())
			{
				ImGui::TextDisabled(FA_LEAF);
				ImGui::SameLine();
				ImGui::TextUnformatted("Leaf Bone");
			}
			else
			{
				ImGui::TextDisabled(FA_CHILD);
				ImGui::SameLine();
				ImGui::TextUnformatted(fmt::format("{} children", hoveredBone.Children.size()).c_str());
			}

			ImGui::Separator();

			ImGui::TextDisabled(FA_RULER " Bone Length");
			ImGui::SameLine();
			ImGui::Text("%.2f", hoveredDebugBone.Length);

			const glm::vec3 headPosition = hoveredDebugBone.Transform[3];
			ImGui::TextDisabled(FA_ARROWS_UP_DOWN_LEFT_RIGHT " Head Position");
			ImGui::SameLine();
			ImGui::Text(fmt::format("({:.3f}, {:.3f}, {:.3f})", headPosition.x, headPosition.y, headPosition.z).c_str());

			ImGui::EndTooltip();
		}

		ImGui::End();

		ImGui::SetNextWindowClass(&windowClass);
		ImGui::Begin(UI::GetFixedWindowNameWithID("Details", m_Skeleton->Handle).c_str());
		DrawSkeletonHierarchyNode(m_Skeleton->GetRootNode());
		ImGui::End();
	}

	void SkeletonViewerPanel::DrawSkeletonHierarchyNode(const BoneNodeData& node)
	{
		const auto& boneInfoMap = m_Skeleton->GetBoneInfoMap();
		if (boneInfoMap.find(node.Name) == boneInfoMap.end())
			return;

		const int boneID = boneInfoMap.at(node.Name).id;
		const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | (node.Children.empty() ? ImGuiTreeNodeFlags_Leaf : 0);

		const bool open = UI::SelectableTreeNode(ImGui::GetID(&node), fmt::format(FA_BONE " {}", node.Name).c_str(), flags, boneID == m_Viewport.GetSelectedID(), ImGuiSelectableFlags_AllowItemOverlap);

		if (ImGui::IsItemClicked())
			m_Viewport.SetSelectedID(boneID);

		if (open)
		{
			for (const auto& child : node.Children)
				DrawSkeletonHierarchyNode(child);

			ImGui::TreePop();
		}
	}

	void SkeletonViewerPanel::OnEvent(Event& e)
	{
		m_Viewport.OnEvent(e);
	}

}