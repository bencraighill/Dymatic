#pragma once
#include "Panels/Assets/Utils/GraphAssetPanel.h"

#include "Dymatic/Renderer/MaterialAsset.h"
#include "Dymatic/Editor/Material/MaterialCompiler.h"
#include "Panels/Assets/Utils/MaterialPanelViewport.h"

namespace Dymatic {

	class MaterialPanel : public GraphAssetPanel
	{
	public:
		MaterialPanel(Ref<MaterialSource> material);
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

		virtual void OnNodeDoubleClicked(const Ref<Editor::EditorNode> internalNode) override;

		void DrawGraphPropertiesPanel();
		void DrawDetailsPanel();
		void DrawCompilerOutputPanel();

		inline virtual bool DoesDrawLayerStack() const override { return false; }
		inline virtual std::string GetLayerStackLabel(const Ref<Editor::EditorNode> internalNode) const override { return std::string(); }

		virtual bool CanDrawPin(const Ref<Editor::EditorNode> node, const Ref<Editor::EditorPin> internalPin, const uint32_t pinIndex) override;

		inline virtual const char* GetGraphName() const override { return GetStaticGraphName(); }
		inline static const char* GetStaticGraphName() { return "Material Graph"; }

		inline virtual const char* GetGraphOverlayText() const override { return GetStaticGraphOverlayText(); }
		inline static const char* GetStaticGraphOverlayText() { return "MATERIAL"; }

	private:
		Ref<MaterialSource> m_Material;

		MaterialPanelViewport m_Viewport;
		Editor::MaterialCompiler m_Compiler;
	};

}