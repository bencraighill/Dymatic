#pragma once
#include "Dymatic/Editor/EditorNode.h"

#include "Dymatic/Core/Base.h"
#include "Dymatic/Core/UUID.h"

#include <glm/glm.hpp>
#include <string>

namespace Dymatic {
	class GraphAssetPanel;
}

namespace Dymatic::Editor {

	typedef uint64_t NodeHandle;
	typedef uint64_t PinHandle;
	typedef uint64_t LinkHandle;
	typedef uint64_t LayerHandle;

	static constexpr glm::vec2 DefaultCommentSize = glm::vec2(300.0f, 100.0f);

	struct EditorPin
	{
		PinHandle ID = UUID();

		NodeHandle Node = 0;
		std::string Name;
		PinKind Kind;
		bool Hidden = false;
		bool Editable = true;

		EditorPin(const std::string& name, const bool hidden)
			: Name(name), Hidden(hidden)
		{}
	};

	struct EditorNode
	{
		NodeHandle ID = UUID();
		LayerHandle Layer = 0;
		LayerHandle TargetLayer = 0;

		std::vector<Ref<EditorPin>> Inputs;
		std::vector<Ref<EditorPin>> Outputs;
		NodeType Type = NodeType::Blueprint;

		glm::vec2 Position = glm::vec2();
		glm::vec2 Size = glm::vec2(0.0f);

		// Node Comment Data
		bool CommentEnabled = false;
		bool CommentPinned = false;
		std::string Comment;

		// Use by comments/variable fields
		// Note: Name should NOT be used for normal nodes (use `Function` field to distinguish those), but rather just for parameter/variable fields
		std::string Name;
		glm::vec3 Color = glm::vec3(1.0f);
	};

	struct Link
	{
		LinkHandle ID = UUID();

		PinHandle StartPinID;
		PinHandle EndPinID;
		LayerHandle TargetLayer = 0;

		Link() = default;

		Link(const PinHandle startPinId, PinHandle endPinId)
			: StartPinID(startPinId), EndPinID(endPinId)
		{}
	};

	class EditorGraph
	{
	public:
		virtual const char* GetNodeFunctionName(Ref<EditorNode> node) const = 0;

		virtual NodeHandle SpawnNodeFromName(const std::string& name) = 0;

		virtual bool CanDeleteNode(NodeHandle id) const = 0;
		void DeleteNode(NodeHandle id);

		bool DoesNodeExist(NodeHandle id);

		Link* GetPinLink(PinHandle id);
		bool IsPinLinked(PinHandle id);
		void CheckLinkSafety(PinHandle startPinId, PinHandle endPinId);
		void CreateLink(Ref<EditorPin> startPin, Ref<EditorPin> endPin, LinkHandle linkID = 0, LayerHandle targetLayer = 0, const bool setup = true);
		void CreateLink(PinHandle startPinId, PinHandle endPinId, LinkHandle linkID = 0, LayerHandle targetLayer = 0, const bool setup = true);
		void DeleteLink(LinkHandle id);

		virtual bool PerformLinkCheck(Ref<EditorPin> a, Ref<EditorPin> b) = 0;

		virtual bool AreTypesCompatible(PinHandle a, PinHandle b) const = 0;
		virtual bool CanCreateLink(PinHandle a, PinHandle b) const = 0;
		Link* FindLink(LinkHandle id);

		virtual void OnCreateLink(LinkHandle linkID, Ref<EditorPin> a, Ref<EditorPin> b) = 0;
		virtual bool CanAddPin(Ref<EditorNode> node) const = 0;
		virtual void OnAddPin(Ref<EditorNode> node) = 0;

		// These should be used for dynamic pins (i.e. created/removed by editor interfaces).
		// Note: The respective editor/interface must setup all type/data values on derived pin object
		PinHandle AddDynamicPin(const NodeHandle id, const std::string& name, const PinKind kind);
		void RemovePin(PinHandle id);

		void RemoveUnusedLayers();

	public:
		static void BuildNode(Ref<EditorNode> node);
		static void SetupCommentNode(Ref<EditorNode> node, const glm::vec2& size);

		inline void SetNextNodeID(const NodeHandle id) { m_NextNodeID = id; }
		inline void SetNextLayerID(const LayerHandle id) { m_NextLayerID = id; }

		Ref<EditorNode> FindNodeInternal(const NodeHandle id) const;
		Ref<EditorPin> FindPinInternal(const PinHandle id) const;

		virtual Ref<EditorPin> CreateNewPin(const std::string& name) = 0;

	protected:
		void CreateNodeInternal(Ref<EditorNode> node);

	public:
		// TODO:  Have a pin-node lookup hash table to speed up 'FindPin' operations.
		// WARNING: When we add the above hash table we need to ensure the material serializer updates this when modifying pin values (maybe some form of regenerate function)

		std::unordered_map<NodeHandle, Ref<EditorNode>> Nodes;
		std::unordered_map<LinkHandle, Link> Links;

		friend class Dymatic::GraphAssetPanel;

	protected:
		NodeHandle m_NextNodeID = 0;
		LayerHandle m_NextLayerID = 0;
	};

}