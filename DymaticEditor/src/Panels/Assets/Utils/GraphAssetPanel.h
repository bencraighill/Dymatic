#pragma once
#include "Dymatic/Editor/EditorGraph.h"

#include "Panels/EditorPanel.h"
#include "Dymatic/Core/Base.h"

#include "Dymatic/Editor/Compiler.h"
#include "Panels/Nodes/Utilities/Search.h"

#include <unordered_map>
#include <set>
#include <deque>

namespace Dymatic {

	class GraphAssetPanel : public EditorPanel
	{
	public:
		GraphAssetPanel(Ref<Editor::EditorGraph> editorGraph);
		~GraphAssetPanel();

		inline virtual void Focus() override { m_Focus = true; }

	protected:
		virtual const CompilerResult& Compile() = 0;
		virtual Editor::NodeHandle OnAssetDropped(const AssetMetadata& metadata) = 0;

		void DrawMenuBar();
		void DrawContextMenus();
		void DrawSearch();
		virtual void UpdateSearch() = 0;

		void ShowFlow();
		void NavigateToContent();
		void NavigateToNode(Editor::NodeHandle node);

		void DrawNodeGraph();
		void DrawNodes();
		void DrawNodeInnerDecorations(Ref<Editor::EditorNode> node);
		void DrawNodeOuterDecorations(Ref<Editor::EditorNode> node);
		void UpdateNodeSelection(Ref<Editor::EditorNode> node);
		void DrawComments();
		void DrawLinks();
		virtual void DrawGraphContents() {}
		virtual void DrawViewMenu() {}
		void DrawLayerStack();
		void UpdateNodeEvents();

		virtual glm::vec4 GetNodeColor(const Ref<Editor::EditorNode> internalNode) const = 0;
		virtual void DrawNodeHeader(const Ref<Editor::EditorNode> internalNode) = 0;
		virtual void DrawNodeHeaderSimple(const Ref<Editor::EditorNode> internalNode) const = 0;
		virtual void DrawNodeThumbnail(const Ref<Editor::EditorNode> internalNode) const = 0;
		virtual void DrawPinIcon(const Ref<Editor::EditorPin> internalPin, const bool linked, const float alpha) const = 0;
		virtual void OnNodeDoubleClicked(const Ref<Editor::EditorNode> internalNode) = 0;
		virtual void OnNodeTooltip(const Ref<Editor::EditorNode> internalNode) {}
		virtual bool DoesDrawLayerStack() const = 0;
		virtual std::string GetLayerStackLabel(const Ref<Editor::EditorNode> internalNode) const = 0;

		void SetNodePosition(Editor::NodeHandle id, glm::vec2 position);
		void DuplicateNodes();
		void DeleteNodes();

		Editor::LayerHandle GetCurrentLayerID();
		Ref<Editor::EditorNode> GetCurrentLayerContext();
		void MoveForwardLayer(Editor::LayerHandle layer, const std::string& name, const Ref<Editor::EditorNode> context);
		void JumpToLayer(Editor::LayerHandle layer);

		const glm::vec4& GetLinkColor(const Editor::Link& link);
		virtual const glm::vec4 GetPinColor(const Ref<Editor::EditorPin> internalPin) const = 0;

		inline virtual bool CanDrawPin(const Ref<Editor::EditorNode> node, const Ref<Editor::EditorPin> internalPin, const uint32_t pinIndex) { return true; }

		virtual const char* GetGraphName() const = 0;
		virtual const char* GetGraphOverlayText() const = 0;

	private:
		void OnCompile();
		void SetupNodes();
		void UpdateNodePositions();

		void ResetContext();

	protected:
		Ref<Editor::EditorGraph> m_InternalEditorGraph;
		bool m_Focus = false;

		bool m_HideUnconnected = false;
		bool m_HideThumbnails = false;
		bool m_ContextSensitive = true;

		std::string m_SearchBuffer;
		NodeSearchTree m_SearchTree;

		// State
		struct LayerStackItem
		{
			Editor::LayerHandle Layer;
			std::string Name;
			glm::vec2 SavedMinBounds;
			glm::vec2 SavedMaxBounds;
			Ref<Editor::EditorNode> Context = nullptr;
		};

		std::deque<LayerStackItem> m_LayerStack;
		glm::vec2 m_GridWindowSize;

		bool m_CreateNewNode = false;
		bool m_DraggingPin = false;
		Editor::NodeHandle m_SelectedNodeID = 0;
		Editor::PinHandle m_NewLinkPinID = 0;
		Editor::PinHandle m_NewNodeLinkPinID = 0;
		glm::vec4 m_DraggingPinColor;

		// Context Menus
		Editor::NodeHandle m_ContextNodeId;
		Editor::PinHandle m_ContextPinId;
		Editor::LinkHandle m_ContextLinkId;
		glm::vec2 m_NewNodePosition;

		// Editor side data
		std::unordered_map<Editor::LinkHandle, glm::vec4> m_LinkColors;
		std::set<Editor::NodeHandle> m_Errors;

		void* m_InternalContext;
	};

}