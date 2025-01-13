#pragma once

#include <yaml-cpp/yaml.h>
#include "Dymatic/Utils/YAMLUtils.h"

namespace Dymatic::Utils {

	typedef std::function<void(YAML::Emitter& out, Ref<Editor::EditorPin>, bool input)> SerializePinMethod;
	typedef std::function<void(YAML::Emitter& out, Ref<Editor::EditorNode>)> SerializeNodeMethod;

	static void SerializeEditorGraph(YAML::Emitter& out, Ref<Editor::EditorGraph> editorGraph, const SerializePinMethod& serializePin, const SerializeNodeMethod& serializeNode)
	{
		// Serialize the editor graph
		out << YAML::Key << "Graph" << YAML::Value << YAML::BeginMap;

		// Serialize Nodes
		out << YAML::Key << "Nodes" << YAML::Value << YAML::BeginSeq;
		for (const auto& [nodeID, node] : editorGraph->Nodes)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Node" << YAML::Value << node->ID;

			if (node->Layer != 0)
				out << YAML::Key << "Layer" << YAML::Value << node->Layer;

			if (node->TargetLayer != 0)
				out << YAML::Key << "Target Layer" << YAML::Value << node->TargetLayer;

			out << YAML::Key << "Type" << YAML::Value << editorGraph->GetNodeFunctionName(node);
			out << YAML::Key << "Position" << YAML::Value << node->Position;

			if (node->Size != glm::vec2(0.0f))
				out << YAML::Key << "Size" << YAML::Value << node->Size;

			if (!node->Name.empty())
				out << YAML::Key << "Name" << YAML::Value << node->Name;

			// Comment fields
			if (node->CommentEnabled || node->Type == Editor::NodeType::Comment)
			{
				out << YAML::Key << "Comment" << YAML::Value << node->Comment;
				out << YAML::Key << "Comment Color" << YAML::Value << node->Color;
			}

			if (node->CommentEnabled && node->Type != Editor::NodeType::Comment)
				out << YAML::Key << "Comment Pinned" << YAML::Value << node->CommentPinned;

			// Inputs and Outputs
			out << YAML::Key << "Inputs" << YAML::Value << YAML::BeginSeq;
			for (const auto& input : node->Inputs)
				serializePin(out, input, !editorGraph->IsPinLinked(input->ID));
			out << YAML::EndSeq; // Inputs

			out << YAML::Key << "Outputs" << YAML::Value << YAML::BeginSeq;
			for (const auto& output : node->Outputs)
				serializePin(out, output, false);
			out << YAML::EndSeq; // Outputs

			if (serializeNode)
				serializeNode(out, node);

			out << YAML::EndMap;
		}
		out << YAML::EndSeq; // Nodes

		// Serialize Links
		out << YAML::Key << "Links" << YAML::Value << YAML::BeginSeq;
		for (const auto& [linkID, link] : editorGraph->Links)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Link" << YAML::Value << link.ID;
			out << YAML::Key << "Start" << YAML::Value << link.StartPinID;
			out << YAML::Key << "End" << YAML::Value << link.EndPinID;

			if (link.TargetLayer != 0)
				out << YAML::Key << "Target Layer" << YAML::Value << link.TargetLayer;

			out << YAML::EndMap;
		}
		out << YAML::EndSeq; // Links

		out << YAML::EndMap; // Graph
	}

	typedef std::function<void(Ref<Editor::EditorNode> node, const Ref<Editor::EditorGraph> editorGraph, const YAML::Node& pinListNode, std::vector<Ref<Editor::EditorPin>>& pins)> DeserializePinDataMethod;
	typedef std::function<void(const YAML::Node& nodeNode, Ref<Editor::EditorNode>)> DeserializeNodeMethod;

	static void DeserializeEditorGraph(const YAML::Node& graphNode, Ref<Editor::EditorGraph> editorGraph, const DeserializePinDataMethod& deserializePinData, const DeserializeNodeMethod& deserializeNode)
	{
		// Deserialize the editor graph

		// Deserialize nodes
		if (auto nodesNode = graphNode["Nodes"])
		{
			for (const auto nodeNode : nodesNode)
			{
				const Editor::NodeHandle nodeID = nodeNode["Node"].as<Editor::NodeHandle>();
				const std::string functionName = nodeNode["Type"].as<std::string>();

				if (auto layerNode = nodeNode["Layer"])
					editorGraph->SetNextLayerID(layerNode.as<Editor::LayerHandle>());

				// Spawn the corresponding node with original ID
				editorGraph->SetNextNodeID(nodeID);
				editorGraph->SpawnNodeFromName(functionName);
				Ref<Editor::EditorNode> node = editorGraph->FindNodeInternal(nodeID);

				if (auto targetLayerNode = nodeNode["Target Layer"])
					node->TargetLayer = targetLayerNode.as<Editor::LayerHandle>();

				// Setup other node properties
				node->Position = nodeNode["Position"].as<glm::vec2>();

				if (auto sizeNode = nodeNode["Size"])
					node->Size = sizeNode.as<glm::vec2>();

				if (auto nameNode = nodeNode["Name"])
					node->Name = nameNode.as<std::string>();

				if (auto commentNode = nodeNode["Comment"])
				{
					node->CommentEnabled = true;
					node->Comment = commentNode.as<std::string>();
				}

				if (auto commentColorNode = nodeNode["Comment Color"])
					node->Color = commentColorNode.as<glm::vec3>();

				if (auto commentPinnedNode = nodeNode["Comment Pinned"])
					node->CommentPinned = commentPinnedNode.as<bool>();

				// Match up specified inputs/outputs to existing ones on the node
				deserializePinData(node, editorGraph, nodeNode["Inputs"], node->Inputs);
				deserializePinData(node, editorGraph, nodeNode["Outputs"], node->Outputs);

				if (deserializeNode)
					deserializeNode(nodeNode, node);
			}
		}

		// Deserialize links
		if (auto linksNode = graphNode["Links"])
		{
			for (const auto linkNode : linksNode)
			{
				const Editor::LinkHandle linkID = linkNode["Link"].as<Editor::LinkHandle>();
				const Editor::PinHandle startPinID = linkNode["Start"].as<Editor::PinHandle>();
				const Editor::PinHandle endPinID = linkNode["End"].as<Editor::PinHandle>();

				Editor::LayerHandle targetLayer = 0;
				if (auto targetLayerNode = linkNode["Target Layer"])
					targetLayer = targetLayerNode.as<Editor::LayerHandle>();

				editorGraph->CreateLink(startPinID, endPinID, linkID, targetLayer, false);
			}
		}
	}

}