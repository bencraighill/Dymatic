#include "Panels/Assets/Utils/GraphAssetPanel.h"

#include "EditorResources.h"
#include "AssetViewerPanelUtils.h"
#include "Panels/Nodes/Utilities/Graph.h"
#include "Panels/Nodes/Utilities/Builders.h"

#include "Fonts.h"

#include <imgui_stdlib.h>
#include <imgui_node_editor.h>

namespace Dymatic {

	namespace ed = ax::NodeEditor;
	namespace util = ax::NodeEditor::Utilities;
	using namespace ax;

	using namespace Editor;

	static const char* s_LinkLayerLabel = FA_ARROW_PROGRESS " Transition";

	GraphAssetPanel::GraphAssetPanel(Ref<Editor::EditorGraph> editorGraph)
		: m_InternalEditorGraph(editorGraph)
	{
		ed::Config config;
		config.SettingsFile = "";
		m_InternalContext = ed::CreateEditor(&config);

		SetupNodes();
	}

	GraphAssetPanel::~GraphAssetPanel()
	{
		ed::DestroyEditor((ax::NodeEditor::EditorContext*)m_InternalContext);
	}

	void GraphAssetPanel::DrawMenuBar()
	{
		ImGui::BeginMenuBar();
		ImGui::MenuItem(FA_FLOPPY_DISK " Save");

		if (ImGui::MenuItem(FA_PERSON_RUNNING " Compile"))
			OnCompile();

		if (ImGui::BeginMenu(FA_EYE " View"))
		{
			ImGui::MenuItem(FA_CIRCLE_DASHED " Hide Unconnected Pins", "", &m_HideUnconnected);
			ImGui::MenuItem(FA_IMAGE_POLAROID " Hide Thumbnails", "", &m_HideThumbnails);
			ImGui::Separator();
			if (ImGui::MenuItem(FA_MAGNIFYING_GLASS_PLUS " Zoom to Content"))
				NavigateToContent();
			if (ImGui::MenuItem(FA_CODE_BRANCH " Show Flow"))
				ShowFlow();

			DrawViewMenu();

			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	void GraphAssetPanel::DrawContextMenus()
	{
		ed::NodeId contextNodeId;
		ed::PinId contextPinId;
		ed::LinkId contextLinkId;

		if (ed::ShowNodeContextMenu(&contextNodeId))
		{
			m_ContextNodeId = contextNodeId.Get();
			ImGui::OpenPopup("Node Context Menu");
		}
		else if (ed::ShowPinContextMenu(&contextPinId))
		{
			m_ContextPinId = contextPinId.Get();
			ImGui::OpenPopup("Pin Context Menu");
		}
		else if (ed::ShowLinkContextMenu(&contextLinkId))
		{
			m_ContextLinkId = contextLinkId.Get();
			ImGui::OpenPopup("Link Context Menu");
		}
		else if (ed::ShowBackgroundContextMenu())
		{
			ImGui::OpenPopup("Create New Node");
			m_NewNodeLinkPinID = 0;
			m_NewLinkPinID = 0;
			m_NewNodePosition = ed::ScreenToCanvas(ImGui::GetMousePos());
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
		if (ImGui::BeginPopup("Node Context Menu"))
		{
			Ref<EditorNode> node = m_InternalEditorGraph->FindNodeInternal(m_ContextNodeId);

			ImGui::TextUnformatted("Node Context Menu");
			ImGui::Separator();
			if (node)
				ImGui::Text("ID: %llu", node->ID);
			else
				ImGui::Text("Unknown node: %llu", m_ContextNodeId);

			ImGui::Separator();
			if (ImGui::MenuItem("Find References"))
			{
				ImGui::SetWindowFocus(FA_MAGNIFYING_GLASS " Find Results");
			}

			if (ImGui::MenuItem("Duplicate"))
				DuplicateNodes();
			if (ImGui::MenuItem("Delete"))
				DeleteNodes();
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("Pin Context Menu"))
		{
			const Ref<EditorPin> pin = m_InternalEditorGraph->FindPinInternal(m_ContextPinId);

			ImGui::TextUnformatted("Pin Context Menu");
			ImGui::Separator();
			if (pin)
			{
				ImGui::Text("ID: %llu", pin->ID);
				if (pin->Node != 0)
					ImGui::Text("Node: %llu", pin->Node);
				else
					ImGui::Text("Node: %s", "<none>");
			}
			else
				ImGui::Text("Unknown pin: %llu", m_ContextPinId);

			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("Link Context Menu"))
		{
			const Link* link = m_InternalEditorGraph->FindLink(m_ContextLinkId);

			ImGui::TextUnformatted("Link Context Menu");
			ImGui::Separator();
			if (link)
			{
				ImGui::Text("ID: %llu", link->ID);
				ImGui::Text("From: %llu", link->StartPinID);
				ImGui::Text("To: %llu", link->EndPinID);
			}
			else
				ImGui::Text("Unknown link: %llu", m_ContextLinkId);
			ImGui::Separator();
			if (ImGui::MenuItem("Delete"))
				ed::DeleteLink(m_ContextLinkId);
			ImGui::EndPopup();
		}
	}

	void GraphAssetPanel::DrawSearch()
	{
		const NodeHandle newNodeHandle = UI::DrawSearchWindow(GetGraphName(), m_SearchTree, m_SearchBuffer, m_ContextSensitive, [this]() { UpdateSearch(); });

		if (newNodeHandle == 0)
			return;

		// A new node was created, let's set it up!
		Ref<EditorNode> newNode = m_InternalEditorGraph->FindNodeInternal(newNodeHandle);

		newNode->Layer = GetCurrentLayerID();

		// Ensure that comment nodes are aligned with their respective selection bounds
		if (newNode->Type == NodeType::Comment && ed::GetSelectedObjectCount() > 0)
		{
			ImVec2 min, max;
			ed::GetSelectionBounds(min, max);
			m_NewNodePosition = min - ImVec2(UI::GraphConstants::CommentPadding, UI::GraphConstants::CommentPadding * 1.5f);
		}

		// Set the node position as required
		SetNodePosition(newNode->ID, m_NewNodePosition);

		if (m_NewNodeLinkPinID == 0)
			return;

		// If we branched off an existing node, connect it if possible
		Ref<EditorPin> otherPin = m_InternalEditorGraph->FindPinInternal(m_NewNodeLinkPinID);

		const auto& pins = otherPin->Kind == PinKind::Input ? newNode->Outputs : newNode->Inputs;

		for (const auto& pin : pins)
		{
			Ref<EditorPin> output = otherPin;
			Ref<EditorPin> input = pin;

			if (output->Kind == PinKind::Input)
				std::swap(output, input);

			if (m_InternalEditorGraph->CanCreateLink(input->ID, output->ID))
			{
				m_InternalEditorGraph->CreateLink(output->ID, input->ID);
				break;
			}
		}
	}

	void GraphAssetPanel::ShowFlow()
	{
		const auto& links = m_InternalEditorGraph->Links;
		for (const auto& [linkId, link] : links)
			ed::Flow(linkId);
	}

	void GraphAssetPanel::NavigateToContent()
	{
		ed::NavigateToContent();
	}

	void GraphAssetPanel::NavigateToNode(Editor::NodeHandle nodeID)
	{
		if (nodeID == 0)
			return;

		const Ref<EditorNode> node = m_InternalEditorGraph->FindNodeInternal(nodeID);

		if (!node)
			return;

		JumpToLayer(node->Layer);
		ed::NavigateTo(node->Position);
	}

	void GraphAssetPanel::DrawNodeGraph()
	{
		ed::SetCurrentEditor((ax::NodeEditor::EditorContext*)m_InternalContext);
		ed::Begin("##GraphNodeEditor", ImGui::IsWindowAppearing() ? ImVec2(500, 500) : ImVec2());

		const ImVec2 cursorTopLeft = ImGui::GetCursorScreenPos();

		// Drag Drop Target
		if (const AssetHandle droppedAsset = UI::ContentBrowserAssetDragDropTarget(AssetType::None))
		{
			if (NodeHandle nodeId = OnAssetDropped(AssetManager::GetMetadata(droppedAsset)))
			{
				ed::Suspend();
				const ImVec2 mousePos = ImGui::GetMousePos();
				ed::Resume();

				SetNodePosition(nodeId, ed::ScreenToCanvas(mousePos));
				m_InternalEditorGraph->FindNodeInternal(nodeId)->Layer = GetCurrentLayerID();
			}
		}

		DrawNodes();
		DrawComments();
		DrawLinks();
		UpdateNodeEvents();

		DrawGraphContents();

		ImGui::SetCursorScreenPos(cursorTopLeft);
		DrawLayerStack();

		ed::Suspend();

		DrawContextMenus();
		DrawSearch();

		ImGui::PopStyleVar();
		ed::Resume();

		m_GridWindowSize = ImGui::GetWindowSize();

		ed::End();

		// Shadows and overlay
		UI::DrawGraphOverlay(GetGraphOverlayText());

		UpdateNodePositions();
	}

	static void DrawTreeNodePin(const Editor::PinHandle pinID, const ed::PinKind kind, const float padding, const ImRect& bounds, const float boundsPadding, const bool canSelect)
	{
		ed::PushStyleVar(ed::StyleVar_PinRadius, boundsPadding);
		ed::BeginPin(pinID, kind);
		ed::PinPivotRect(bounds.Min, bounds.Max);

		if (canSelect)
			ed::PinRect(bounds.Min, bounds.Max);
		else
		{
			const ImVec2 center = (bounds.Min + bounds.Max) * 0.5f;
			const ImVec2 padding = ImVec2(1.0f, 1.0f);
			ed::PinRect(center - padding, center + padding);
		}

		ed::EndPin();
		ed::PopStyleVar();
	}


	void GraphAssetPanel::DrawNodes()
	{
		util::BlueprintNodeBuilder builder((ImTextureID)EditorResources::HeaderBackground->GetRendererID(), EditorResources::HeaderBackground->GetWidth(), EditorResources::HeaderBackground->GetHeight());

		const LayerHandle currentLayer = GetCurrentLayerID();

		// Draw Simple/Blueprint nodes
		for (auto& [nodeId, node] : m_InternalEditorGraph->Nodes)
		{
			if (node->Layer != currentLayer)
				continue;

			if (node->Type != NodeType::Blueprint && node->Type != NodeType::Simple)
				continue;

			const auto isSimple = node->Type == NodeType::Simple;

			builder.Begin(node->ID);
			if (!isSimple)
			{
				builder.Header(GetNodeColor(node));
				ImGui::Spring(0);
				DrawNodeHeader(node);
				ImGui::Spring(1);
				ImGui::Dummy(ImVec2(0, 28));
				ImGui::Spring(0);

				builder.EndHeader();
			}

			auto& inputs = node->Inputs;
			for (uint32_t inputIndex = 0; inputIndex < inputs.size(); inputIndex++)
			{
				auto& input = inputs[inputIndex];
				const bool linked = m_InternalEditorGraph->IsPinLinked(input->ID);

				const bool disabled = !CanDrawPin(node, input, inputIndex);
				if (disabled && !linked)
					continue;

				ImGui::BeginDisabled(disabled);

				if (!input->Hidden && (!m_HideUnconnected || linked))
				{
					float alpha = ImGui::GetStyle().Alpha;
					if (m_NewLinkPinID != 0 && input->ID != m_NewLinkPinID && !m_InternalEditorGraph->CanCreateLink(m_NewLinkPinID, input->ID))
						alpha = alpha * (48.0f / 255.0f);

					builder.Input(input->ID);
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
					DrawPinIcon(input, linked, alpha);
					ImGui::Spring(0);
					auto& pinName = input->Name;
					if (!pinName.empty())
					{
						ImGui::TextUnformatted(pinName.c_str());
						ImGui::Spring(0);
					}

					ImGui::PopStyleVar();
					builder.EndInput();

					if (ImGui::IsItemHovered() && ImGui::IsMouseDown(0))
						m_DraggingPinColor = GetPinColor(input);
				}

				ImGui::EndDisabled();
			}

			if (m_InternalEditorGraph->CanAddPin(node))
			{
				const float lineHeight = ImGui::GetTextLineHeight();
				if (UI::DrawTextIconButton(FA_PLUS, ImVec2(lineHeight, lineHeight)))
					m_InternalEditorGraph->OnAddPin(node);
			}

			if (isSimple)
			{
				builder.Middle();

				ImGui::Spring(1, 0);
				DrawNodeHeaderSimple(node);
				ImGui::Spring(1, 0);
			}

			if (!m_HideThumbnails)
				DrawNodeThumbnail(node);

			auto& outputs = node->Outputs;
			for (uint32_t outputIndex = 0; outputIndex < outputs.size(); outputIndex++)
			{
				auto& output = outputs[outputIndex];
				const bool linked = m_InternalEditorGraph->IsPinLinked(output->ID);

				const bool disabled = !CanDrawPin(node, output, outputIndex);
				if (disabled && !linked)
					continue;

				ImGui::BeginDisabled(disabled);

				if (!output->Hidden && (!m_HideUnconnected || linked))
				{
					float alpha = ImGui::GetStyle().Alpha;
					if (m_NewLinkPinID != 0 && output->ID != m_NewLinkPinID && !m_InternalEditorGraph->CanCreateLink(m_NewLinkPinID, output->ID))
						alpha = alpha * (48.0f / 255.0f);

					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
					builder.Output(output->ID);
					auto& pinName = output->Name;
					if (!pinName.empty())
					{
						ImGui::Spring(0);
						ImGui::TextUnformatted(pinName.c_str());
					}
					ImGui::Spring(0);
					DrawPinIcon(output, linked, alpha);
					ImGui::PopStyleVar();
					builder.EndOutput();

					if (ImGui::IsItemHovered() && ImGui::IsMouseDown(0))
						m_DraggingPinColor = GetPinColor(output);
				}

				ImGui::EndDisabled();
			}

			DrawNodeInnerDecorations(node);
			builder.End();
			DrawNodeOuterDecorations(node);
		}

		// Draw State/Tree Nodes
		for (auto& [nodeId, node] : m_InternalEditorGraph->Nodes)
		{
			if (node->Layer != currentLayer)
				continue;

			if (node->Type != NodeType::Tree)
				continue;

			const float rounding = 5.0f;
			const float padding = 12.0f;

			const auto pinBackground = ed::GetStyle().Colors[ed::StyleColor_NodeBg];

			ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(73, 73, 73, 200));
			ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(32, 32, 32, 200));
			ed::PushStyleColor(ed::StyleColor_PinRect, ImColor(60, 180, 255, 150));
			ed::PushStyleColor(ed::StyleColor_PinRectBorder, ImColor(60, 180, 255, 150));

			ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(0, 0, 0, 0));
			ed::PushStyleVar(ed::StyleVar_NodeRounding, rounding);
			ed::PushStyleVar(ed::StyleVar_SourceDirection, ImVec2(0.0f, 1.0f));
			ed::PushStyleVar(ed::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
			ed::PushStyleVar(ed::StyleVar_LinkStrength, 0.0f);
			ed::PushStyleVar(ed::StyleVar_PinBorderWidth, 1.0f);
			ed::PushStyleVar(ed::StyleVar_PinRadius, 5.0f);
			ed::PushStyleVar(ed::StyleVar_PinLinkOffset, 10.0f);
			ed::BeginNode(node->ID);

			ImDrawList* drawList = ImGui::GetWindowDrawList();

			ImGui::BeginVertical(node->ID);

			ImGui::BeginHorizontal("content_frame");
			ImGui::Spring(1, padding);

			const float boundsPadding = 15.0f;
			ImGui::BeginVertical("content", ImVec2(0.0f, 0.0f));
			ImGui::Dummy(ImVec2(0.0f, boundsPadding));

			ImGui::Dummy(ImVec2(160, 0));

			drawList->ChannelsSplit(2);
			drawList->ChannelsSetCurrent(1);

			ImGui::BeginHorizontal("header");
			ImGui::Spring(1);
			DrawNodeHeader(node);
			ImGui::Spring(1);
			ImGui::EndHorizontal();
			ImGui::EndVertical();

			drawList->ChannelsSetCurrent(0);

			ImGui::Spring(1, padding);
			ImGui::EndHorizontal();
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			ImGui::EndVertical();

			const ImRect bounds = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
			const ImRect outerBounds = ImRect(bounds.Min, bounds.Max + ImVec2(0.0f, 7.0f));
			const ImRect innerBounds = ImRect(bounds.Min + ImVec2(boundsPadding, boundsPadding), bounds.Max - ImVec2(boundsPadding, 5.0f));

			// Draw and merge channels to ensure box appears below text field
			drawList->AddRectFilled(innerBounds.Min, innerBounds.Max, ImColor(40, 40, 40), rounding);
			drawList->ChannelsMerge();

			const bool isActiveOutput = node->Outputs.empty() ? false : (node->Outputs[0]->ID == m_NewLinkPinID);
			
			if (!node->Inputs.empty())
			{
				ed::PushStyleVar(ed::StyleVar_PinArrowSize, 10.0f);
				ed::PushStyleVar(ed::StyleVar_PinArrowWidth, 10.0f);
				const PinHandle pinID = node->Inputs[0]->ID;
				DrawTreeNodePin(pinID, ed::PinKind::Input, padding, outerBounds, 0.0f, m_DraggingPin && !isActiveOutput);
				ed::PopStyleVar(2);
			}

			if (!node->Outputs.empty())
			{
				const PinHandle pinID = node->Outputs[0]->ID;
				DrawTreeNodePin(pinID, ed::PinKind::Output, padding, innerBounds, boundsPadding, !m_DraggingPin || isActiveOutput);
			}

			DrawNodeInnerDecorations(node);
			ed::EndNode();
			DrawNodeOuterDecorations(node);

			ed::PopStyleVar(8);
			ed::PopStyleColor(4);
		}

		for (auto& [nodeId, node] : m_InternalEditorGraph->Nodes)
		{
			if (node->Layer != currentLayer)
				continue;

			if (node->Type != NodeType::Point)
				continue;

			ed::PushStyleColor(NodeEditor::StyleColor_NodeBg, GetNodeColor(node));
			ed::BeginNode(node->ID);
			DrawNodeInnerDecorations(node);
			ed::EndNode();
			UpdateNodeSelection(node);
			ed::PopStyleColor();
		}

		// Handle double click events
		if (NodeHandle nodeID = ed::GetDoubleClickedNode().Get())
		{
			const Ref<EditorNode> node = m_InternalEditorGraph->FindNodeInternal(nodeID);
			if (node->TargetLayer != 0)
				MoveForwardLayer(node->TargetLayer, GetLayerStackLabel(node), node);

			OnNodeDoubleClicked(node);
		}

		if (NodeHandle nodeID = ed::GetHoveredNode().Get())
		{
			if (UI::IsHoveredTooltipTimer())
			{
				const Ref<EditorNode> node = m_InternalEditorGraph->FindNodeInternal(nodeID);

				ed::Suspend();
				OnNodeTooltip(node);
				ed::Resume();
			}
		}
	}

	void GraphAssetPanel::DrawNodeInnerDecorations(Ref<Editor::EditorNode> node)
	{
		// Error Message
		if (m_Errors.find(node->ID) != m_Errors.end())
		{
			if (node->Type == NodeType::Point)
			{
				const auto& position = node->Position;
				UI::DrawErrorMessage(position.x - 50.0f, position.x + 50.0f, position.y + 40.0f);
			}
			else
				UI::DrawNodeErrorMessage(node->ID);
		}
	}

	void GraphAssetPanel::DrawNodeOuterDecorations(Ref<Editor::EditorNode> node)
	{
		UpdateNodeSelection(node);

		// Draw shadows beneath nodes
		UI::DrawNodeShadow(node->ID);

		// Comments for Blueprint Nodes (Based off code for comments)
		UI::DrawNodeComment(node->ID, node->CommentEnabled, node->CommentPinned, node->Comment, node->Color);
	}

	void GraphAssetPanel::UpdateNodeSelection(Ref<Editor::EditorNode> node)
	{
		if (ed::IsNodeSelected(node->ID))
			m_SelectedNodeID = node->ID;
	}

	void GraphAssetPanel::DrawComments()
	{
		const LayerHandle currentLayer = GetCurrentLayerID();

		for (auto& [nodeId, node] : m_InternalEditorGraph->Nodes)
		{
			if (node->Layer != currentLayer)
				continue;

			if (node->Type != NodeType::Comment)
				continue;

			UI::DrawCommentNode(node->ID, node->Comment, node->Size, node->Color);

			if (ed::IsNodeSelected(node->ID))
				m_SelectedNodeID = node->ID;
		}
	}

	void GraphAssetPanel::DrawLinks()
	{
		for (const auto& [linkId, link] : m_InternalEditorGraph->Links)
			ed::Link(link.ID, link.StartPinID, link.EndPinID, GetLinkColor(link), 2.0f);

		if (LinkHandle linkID = ed::GetDoubleClickedLink().Get())
		{
			const auto& link = m_InternalEditorGraph->Links.at(linkID);
			if (link.TargetLayer != 0)
				MoveForwardLayer(link.TargetLayer, s_LinkLayerLabel, nullptr);
		}
	}

	void GraphAssetPanel::UpdateNodeEvents()
	{
		const ImVec4 linkColor = m_DraggingPinColor;

		ed::BeginCreate(linkColor, 10.0f);
		ed::EndCreate();

		m_DraggingPin = false;

		if (ed::BeginCreate(linkColor, 2.0f))
		{
			ed::PinId startPinId = 0, endPinId = 0;
			if (ed::QueryNewLink(&startPinId, &endPinId))
			{
				if (startPinId.Get() == 0)
					DY_ASSERT(false);

				m_DraggingPin = true;

				Ref<EditorPin> startPin = m_InternalEditorGraph->FindPinInternal(startPinId.Get());
				Ref<EditorPin> endPin = m_InternalEditorGraph->FindPinInternal(endPinId.Get());

				m_NewLinkPinID = startPin ? startPin->ID : endPin->ID;

				if (startPin->Kind == PinKind::Input)
				{
					std::swap(startPin, endPin);
					std::swap(startPinId, endPinId);
				}

				if (startPin && endPin)
				{
					if (endPin == startPin)
						ed::RejectNewItem(UI::GraphConstants::BadLinkColor, 1.0f);
					else if (endPin->Kind == startPin->Kind)
					{
						UI::DrawLabel(FA_XMARK " Incompatible Pin Kind", UI::GraphConstants::BadLinkLabelColor);
						ed::RejectNewItem(UI::GraphConstants::BadLinkColor, 1.0f);
					}
					else if (endPin->Node == startPin->Node)
					{
						UI::DrawLabel(FA_XMARK " Cannot connect to self", UI::GraphConstants::BadLinkLabelColor);
						ed::RejectNewItem(UI::GraphConstants::BadLinkColor, 1.0f);
					}
					else if (!m_InternalEditorGraph->AreTypesCompatible(endPin->ID, startPin->ID))
					{
						UI::DrawLabel(FA_XMARK " Incompatible Pin Type", UI::GraphConstants::BadLinkLabelColor);
						ed::RejectNewItem(UI::GraphConstants::BadLinkColor, 1.0f);
					}
					else
					{
						UI::DrawLabel(FA_PLUS " Create Link", UI::GraphConstants::GoodLinkLabelColor);
						if (ed::AcceptNewItem(UI::GraphConstants::GoodLinkColor, 4.0f))
							m_InternalEditorGraph->CreateLink(startPin->ID, endPin->ID);
					}
				}
			}

			ed::PinId pinId = 0;
			if (ed::QueryNewNode(&pinId))
			{
				m_DraggingPin = true;

				m_NewLinkPinID = pinId.Get();
				if (m_NewLinkPinID != 0)
					UI::DrawLabel(FA_PLUS " Create Node", UI::GraphConstants::GoodLinkLabelColor);

				if (ed::AcceptNewItem())
				{
					m_NewNodeLinkPinID = pinId.Get();
					m_NewLinkPinID = 0;
					ed::Suspend();
					ImGui::OpenPopup("Create New Node");
					ed::Resume();
					m_NewNodePosition = ed::ScreenToCanvas(ImGui::GetMousePos());
				}
			}
		}
		else
			m_NewLinkPinID = 0;

		ed::EndCreate();

		if (ed::BeginDelete())
		{
			ed::LinkId linkId = 0;
			while (ed::QueryDeletedLink(&linkId))
			{
				ed::AcceptDeletedItem();
				m_InternalEditorGraph->DeleteLink(linkId.Get());
			}

			ed::NodeId nodeId = 0;
			while (ed::QueryDeletedNode(&nodeId))
			{
				if (!m_InternalEditorGraph->CanDeleteNode(nodeId.Get()))
					continue;

				if (ed::AcceptDeletedItem())
					m_InternalEditorGraph->DeleteNode(nodeId.Get());
			}
		}

		ed::EndDelete();
	}

	static bool DrawLayerStackBreadcrumb(const char* label, ImDrawList* drawList, const float scale)
	{
		bool result = false;

		const ImGuiStyle& style = ImGui::GetStyle();

		if (ImGui::Button("##LayerBreadcrumb", (ImGui::CalcTextSize(label) + style.FramePadding * 2.0f) * scale))
			result = true;
		ed::Suspend();
		drawList->AddText(ed::CanvasToScreen(ImGui::GetItemRectMin()) + style.FramePadding, ImGui::GetColorU32(ImGuiCol_Text), label);
		ed::Resume();

		return result;
	}

	static void DrawLayerStackArrow(ImDrawList* drawList, const float scale)
	{
		const ImGuiStyle& style = ImGui::GetStyle();

		ImGui::Dummy((ImGui::CalcTextSize(FA_CHEVRON_RIGHT) + style.FramePadding * 2.0f) * scale);
		ed::Suspend();
		drawList->AddText(ed::CanvasToScreen(ImGui::GetItemRectMin()) + style.FramePadding, ImGui::GetColorU32(ImGuiCol_TextDisabled), FA_CHEVRON_RIGHT);
		ed::Resume();
	}

	void GraphAssetPanel::DrawLayerStack()
	{
		if (!DoesDrawLayerStack())
			return;

		// Get Window position
		ed::Suspend();
		const ImVec2 windowPos = ImGui::GetWindowPos();
		ImDrawList* drawList = ImGui::GetForegroundDrawList();
		ed::Resume();

		// Begin drawing in upper left
		ImGui::SetCursorPos(ed::ScreenToCanvas(windowPos + ImVec2(15, 15)));

		const float scale = ed::GetCurrentZoom();
		const ImGuiStyle& style = ImGui::GetStyle();
		ImGui::PushStyleColor(ImGuiCol_Button, {});
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, style.FrameRounding * scale);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());

		// Draw stack contents
		if (DrawLayerStackBreadcrumb(FA_HASHNODE " Output", drawList, scale) && !m_LayerStack.empty())
		{
			const auto& previousLayer = m_LayerStack.front();
			ed::NavigateTo(previousLayer.SavedMinBounds, previousLayer.SavedMaxBounds, 0.0f);
			m_LayerStack.clear();
		}

		uint32_t layerIndex = 0;
		for (const auto& layer : m_LayerStack)
		{
			ImGui::PushID(layer.Layer);

			ImGui::SameLine();
			DrawLayerStackArrow(drawList, scale);
			ImGui::SameLine();

			if (DrawLayerStackBreadcrumb(layer.Name.c_str(), drawList, scale) && layerIndex < m_LayerStack.size() - 1)
			{
				// Restore the cached bounds before jumping
				const auto& previousLayer = m_LayerStack[layerIndex + 1];
				ed::NavigateTo(previousLayer.SavedMinBounds, previousLayer.SavedMaxBounds, 0.0f);
				m_SelectedNodeID = 0;

				// Remove all breadcrumbs after the new active layer
				m_LayerStack.erase(m_LayerStack.begin() + layerIndex + 1, m_LayerStack.end());
				ImGui::PopID();
				break;
			}

			ImGui::PopID();

			layerIndex++;
		}

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor();

	}

	void GraphAssetPanel::SetNodePosition(NodeHandle id, glm::vec2 position)
	{
		Ref<EditorNode> node = m_InternalEditorGraph->FindNodeInternal(id);

		if (!node)
			return;

		node->Position = position;
		ed::SetNodePosition(id, position);
	}

	void GraphAssetPanel::DuplicateNodes()
	{
	}

	void GraphAssetPanel::DeleteNodes()
	{
		std::vector<ed::NodeId> selectedNodes(ed::GetSelectedObjectCount());
		int nodeCount = ed::GetSelectedNodes(selectedNodes.data(), selectedNodes.size());

		for (uint32_t i = 0; i < nodeCount; i++)
			m_InternalEditorGraph->DeleteNode(selectedNodes[i].Get());
	}

	LayerHandle GraphAssetPanel::GetCurrentLayerID()
	{
		return m_LayerStack.empty() ? 0 : m_LayerStack.back().Layer;
	}

	Ref<EditorNode> GraphAssetPanel::GetCurrentLayerContext()
	{
		return m_LayerStack.empty() ? nullptr : m_LayerStack.back().Context;
	}

	void GraphAssetPanel::MoveForwardLayer(Editor::LayerHandle layer, const std::string& name, const Ref<EditorNode> context)
	{
		ImVec2 min, max;
		ed::GetViewRect(min, max);
		m_LayerStack.push_back({ layer, name, min, max, context });

		const ImVec2 halfSize = ImGui::GetWindowSize() * 0.5f;
		ed::NavigateTo(-halfSize, halfSize, 0.0f);

		ResetContext();
	}

	// More expensive operation to completely reconstruct the layer stack (required for discontinuous jumps to a completely arbitrary point in the graph)
	void GraphAssetPanel::JumpToLayer(Editor::LayerHandle layer)
	{
		m_LayerStack.clear();

		const glm::vec2 halfBounds = m_GridWindowSize * 0.5f;

		while (layer != 0)
		{
			bool found = false;
			for (const auto& [nodeID, node] : m_InternalEditorGraph->Nodes)
			{
				if (node->TargetLayer == layer)
				{
					m_LayerStack.push_front({ layer, GetLayerStackLabel(node), -halfBounds, halfBounds, node });
					layer = node->Layer;

					found = true;
					break;
				}
			}

			if (found)
				continue;

			for (const auto& [linkID, link] : m_InternalEditorGraph->Links)
			{
				if (link.TargetLayer == layer)
				{
					m_LayerStack.push_front({ layer, s_LinkLayerLabel, -halfBounds, halfBounds });
					layer = m_InternalEditorGraph->FindNodeInternal(m_InternalEditorGraph->FindPinInternal(link.StartPinID)->Node)->Layer;
					break;
				}
			}

			if (!found)
				break;
		}

		ResetContext();
	}

	const glm::vec4& GraphAssetPanel::GetLinkColor(const Editor::Link& link)
	{
		if (m_LinkColors.find(link.ID) == m_LinkColors.end())
		{
			Ref<EditorPin> startPin = m_InternalEditorGraph->FindPinInternal(link.StartPinID);
			m_LinkColors[link.ID] = GetPinColor(startPin);
		}

		return m_LinkColors.at(link.ID);
	}

	void GraphAssetPanel::OnCompile()
	{
		const CompilerResult& compilerResult = Compile();

		ShowFlow();

		m_Errors.clear();
		for (const auto& message : compilerResult.Messages)
			if (message.Type == CompilerMessageType::Error && message.Handle != 0)
				m_Errors.insert(message.Handle);
	}

	void GraphAssetPanel::SetupNodes()
	{
		ed::SetCurrentEditor((ax::NodeEditor::EditorContext*)m_InternalContext);

		for (const auto& [nodeID, node] : m_InternalEditorGraph->Nodes)
			ed::SetNodePosition(nodeID, node->Position);
	}

	void GraphAssetPanel::UpdateNodePositions()
	{
		for (const auto& [nodeID, node] : m_InternalEditorGraph->Nodes)
		{
			node->Position = ed::GetNodePosition(nodeID);
		}
	}

	void GraphAssetPanel::ResetContext()
	{
		m_SelectedNodeID = 0;
	}

}