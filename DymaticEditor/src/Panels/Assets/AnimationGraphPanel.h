#pragma once
#include "Panels/Assets/Utils/GraphAssetPanel.h"

#include "Dymatic/Animation/AnimationGraph.h"
#include "Dymatic/Animation/Editor/AnimationGraphCompiler.h"
#include "Panels/Assets/Utils/AssetPanelViewport.h"

namespace Dymatic {

	class AnimationGraphPanel : public GraphAssetPanel
	{
	public:
		AnimationGraphPanel(Ref<AnimationGraph> animationGraph);
		virtual void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender(bool& open) override;

	private:
		virtual const CompilerResult& Compile() override;
		virtual Editor::NodeHandle OnAssetDropped(const AssetMetadata& metadata) override;
		virtual glm::vec4 GetNodeColor(const Ref<Editor::EditorNode> internalNode) const override;
		virtual void DrawNodeHeader(const Ref<Editor::EditorNode> internalNode) override;
		virtual void DrawNodeHeaderSimple(const Ref<Editor::EditorNode> internalNode) const override;
		virtual void DrawNodeThumbnail(const Ref<Editor::EditorNode> internalNode) const override;

		virtual void DrawPinIcon(const Ref<Editor::EditorPin> internalPin, const bool linked, const float alpha) const override;
		virtual void UpdateSearch() override;
		virtual const glm::vec4 GetPinColor(const Ref<Editor::EditorPin> internalPin) const override;

		virtual void DrawGraphContents() override;
		virtual void DrawViewMenu() override;
		virtual void OnNodeTooltip(const Ref<Editor::EditorNode> internalNode) override;
		virtual void OnNodeDoubleClicked(const Ref<Editor::EditorNode> internalNode) override;

		void DrawGraphPropertiesPanel();
		void DrawDetailsPanel();
		void NodeSelectionInput(Editor::NodeHandle& handle, const Editor::AnimationNodeFunction function);
		void DrawNodeDefaultValueInput(const Editor::AnimationNodeFunction nodeFunction, const std::string& name, const Editor::AnimationPinType type, Editor::AnimationPinData& data);

		inline virtual bool DoesDrawLayerStack() const override { return true; }
		virtual std::string GetLayerStackLabel(const Ref<Editor::EditorNode> internalNode) const override;

		inline virtual const char* GetGraphName() const override { return GetStaticGraphName(); }
		inline static const char* GetStaticGraphName() { return "Animation Graph"; }

		inline virtual const char* GetGraphOverlayText() const override { return GetStaticGraphOverlayText(); }
		inline static const char* GetStaticGraphOverlayText() { return "ANIMATION"; }

	private:
		Ref<AnimationGraph> m_AnimationGraph;
		Editor::AnimationGraphCompiler m_Compiler;

		AssetPanelViewport m_Viewport;

		bool m_ShowPreview = true;
		bool m_ShowBlendspaceTriangulation = true;
		bool m_ShowBlendspaceOuterEdge = true;
		bool m_ShowBlendspaceLabels = true;
	};

}