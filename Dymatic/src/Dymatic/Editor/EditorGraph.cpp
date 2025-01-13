#include "dypch.h"
#include "Dymatic/Editor/EditorGraph.h"

#include <stack>
#include <unordered_set>
#include <unordered_map>

namespace Dymatic::Editor {

	void EditorGraph::DeleteNode(NodeHandle id)
	{
		if (!CanDeleteNode(id))
			return;

		// Delete any link connected to the node
		auto it = Links.begin();
		while (it != Links.end())
		{
			const auto& [key, link] = *it;

			Ref<EditorPin> startPin = FindPinInternal(link.StartPinID);
			Ref<EditorPin> endPin = FindPinInternal(link.EndPinID);

			if (!startPin || !endPin || startPin->Node == id || endPin->Node == id)
				it = Links.erase(it);
			else
				++it;
		}

		// Delete the actual node
		if (Nodes.find(id) == Nodes.end())
			return;

		Nodes.erase(id);
	}

	bool EditorGraph::DoesNodeExist(NodeHandle id)
	{
		return Nodes.find(id) != Nodes.end();
	}

	// Note: This will access only the first link found if multiple exist
	Link* EditorGraph::GetPinLink(PinHandle id)
	{
		if (id == 0)
			return nullptr;

		for (auto& [linkId, link] : Links)
			if (link.StartPinID == id || link.EndPinID == id)
				return &link;

		return nullptr;
	}

	bool EditorGraph::IsPinLinked(PinHandle id)
	{
		return (bool)GetPinLink(id);
	}

	void EditorGraph::CheckLinkSafety(PinHandle startPinId, PinHandle endPinId)
	{
		if (startPinId == 0 || endPinId == 0)
			return;

		LinkHandle linkToDelete = 0;
		for (const auto& [linkId, link] : Links)
		{
			if ((link.EndPinID == endPinId || link.StartPinID == endPinId))
			{
				linkToDelete = link.ID;
				break;
			}
		}

		DeleteLink(linkToDelete);
	}

	void EditorGraph::CreateLink(Ref<EditorPin> a, Ref<EditorPin> b, LinkHandle linkID, LayerHandle targetLayer, const bool setup)
	{
		if (!a || !b)
			return;

		if (!CanCreateLink(a->ID, b->ID))
			return;

		if (a->Kind == PinKind::Input)
			std::swap(a, b);

		if (PerformLinkCheck(a, b))
			CheckLinkSafety(a->ID, b->ID);

		Link link = Link(a->ID, b->ID);

		if (linkID != 0)
			link.ID = linkID;

		if (targetLayer)
			link.TargetLayer = targetLayer;

		Links[link.ID] = link;

		if (setup)
			OnCreateLink(link.ID, a, b);
	}

	void EditorGraph::CreateLink(PinHandle startPinId, PinHandle endPinId, LinkHandle linkID, LayerHandle targetLayer, const bool setup)
	{
		const Ref<EditorPin> a = FindPinInternal(startPinId);
		const Ref<EditorPin> b = FindPinInternal(endPinId);
		CreateLink(a, b, linkID, targetLayer, setup);
	}

	void EditorGraph::DeleteLink(LinkHandle id)
	{
		if (id == 0 || Links.find(id) == Links.end())
			return;

		Links.erase(id);
	}

	Link* EditorGraph::FindLink(LinkHandle id)
	{
		if (Links.find(id) == Links.end())
			return nullptr;

		return &Links.at(id);
	}

	PinHandle EditorGraph::AddDynamicPin(const NodeHandle id, const std::string& name, const PinKind kind)
	{
		Ref<EditorNode> node = FindNodeInternal(id);

		if (!node)
			return 0;

		auto& pins = (kind == PinKind::Input) ? node->Inputs : node->Outputs;

		Ref<EditorPin> pin = CreateNewPin(name);
		pins.emplace_back(pin);

		BuildNode(node);
		return pins.back()->ID;
	}

	void EditorGraph::RemovePin(PinHandle id)
	{
		Ref<EditorPin> pin = FindPinInternal(id);

		if (!pin)
			return;

		Ref<EditorNode> node = FindNodeInternal(pin->Node);
		auto& pins = (pin->Kind == PinKind::Input ? node->Inputs : node->Outputs);

		// Remove the pin itself
		pins.erase(std::remove_if(pins.begin(), pins.end(), [id](Ref<EditorPin> pin)
		{
			return pin->ID == id;
		}), pins.end());

		// Remove any link referencing this pin
		for (auto it = Links.begin(); it != Links.end(); ) 
		{
			if (it->second.StartPinID == id || it->second.EndPinID == id)
				it = Links.erase(it);
			else
				++it;
		}
	}

	// Very expensive operation, do not call often! I don't even want to know the time complexity of this...
	void EditorGraph::RemoveUnusedLayers()
	{
		std::unordered_set<LayerHandle> usedLayers;
		std::stack<LayerHandle> layersToSearch;
		layersToSearch.push(0);
		usedLayers.insert(0);

		// Compute all links layers
		std::unordered_map<LayerHandle, std::vector<LinkHandle>> layerLinks;
		for (const auto& [linkID, link] : Links)
			layerLinks[FindNodeInternal(FindPinInternal(link.StartPinID)->Node)->Layer].emplace_back(linkID);


		// Traverse all accessible layers starting from the root/default layer
		while (!layersToSearch.empty())
		{
			LayerHandle layer = layersToSearch.top();
			layersToSearch.pop();

			for (const auto& [nodeID, node] : Nodes)
			{
				if (node->Layer == layer && usedLayers.find(node->TargetLayer) == usedLayers.end())
				{
					usedLayers.insert(node->TargetLayer);
					layersToSearch.push(node->TargetLayer);
				}
			}

			if (layerLinks.find(layer) != layerLinks.end())
			{
				for (const auto& linkID : layerLinks.at(layer))
				{
					const LayerHandle targetLayer = FindLink(linkID)->TargetLayer;

					if (usedLayers.find(targetLayer) == usedLayers.end())
					{
						usedLayers.insert(targetLayer);
						layersToSearch.push(targetLayer);
					}
				}
			}
		}

		// Delete any node that belongs to a layer that is inaccessible
		std::vector<NodeHandle> nodesToRemove;
		for (const auto& [nodeID, node] : Nodes)
			if (usedLayers.find(node->Layer) == usedLayers.end())
				nodesToRemove.emplace_back(node->ID);

		for (const auto& nodeID : nodesToRemove)
			DeleteNode(nodeID);
	}

	void EditorGraph::BuildNode(Ref<EditorNode> node)
	{
		for (auto& input : node->Inputs)
		{
			input->Node = node->ID;
			input->Kind = PinKind::Input;
		}

		for (auto& output : node->Outputs)
		{
			output->Node = node->ID;
			output->Kind = PinKind::Output;
		}
	}

	void EditorGraph::SetupCommentNode(Ref<EditorNode> node, const glm::vec2& size)
	{
		node->Type = NodeType::Comment;
		node->Comment = "Comment";
		node->Size = size;
	}

	Ref<EditorNode> EditorGraph::FindNodeInternal(const NodeHandle id) const
	{
		if (Nodes.find(id) == Nodes.end())
			return nullptr;

		return Nodes.at(id);
	}

	Ref<EditorPin> EditorGraph::FindPinInternal(const PinHandle id) const
	{
		for (auto& [nodeId, node] : Nodes)
		{
			for (auto& input : node->Inputs)
				if (input->ID == id)
					return input;

			for (auto& output : node->Outputs)
				if (output->ID == id)
					return output;
		}

		return nullptr;
	}

	void EditorGraph::CreateNodeInternal(Ref<EditorNode> node)
	{
		if (m_NextNodeID != 0)
		{
			node->ID = m_NextNodeID;
			m_NextNodeID = 0;
		}

		if (m_NextLayerID != 0)
		{
			node->Layer = m_NextLayerID;
			m_NextLayerID = 0;
		}
	}

}