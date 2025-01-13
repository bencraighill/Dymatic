#pragma once
#include "Panels/EditorPanel.h"

#include "Dymatic/Renderer/Skeleton.h"
#include "Panels/Assets/Utils/AssetPanelViewport.h"

namespace Dymatic {

	class SkeletonViewerPanel : public EditorPanel
	{
	public:
		struct DebugBone
		{
			glm::mat4 Transform;
			float Length;
		};

	public:
		SkeletonViewerPanel(Ref<Skeleton> skeleton);

		virtual void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender(bool& open) override;

		virtual void OnEvent(Event& e) override;
		inline virtual void Focus() override { m_Focus = true; }

	private:
		void DrawSkeletonHierarchyNode(const BoneNodeData& node);

	private:
		Ref<Skeleton> m_Skeleton;
		AssetPanelViewport m_Viewport;
		bool m_Focus = false;
		
		float m_BoneHoveredTime = 0.0f;

		// Rendering cache
		std::vector<DebugBone> m_DebugBones;
		std::vector<std::pair<glm::vec3, glm::vec3>> m_ConnectionLines;
	};

}