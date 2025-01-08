#pragma once
#include "Dymatic/Editor/EditorGraph.h"

#include "Dymatic/Core/Base.h"
#include "Dymatic/Scene/Transform.h"

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

#include "Dymatic/Asset/Asset.h"

namespace Dymatic::Editor {

	enum class AnimationNodeFunction
	{
		None = 0,

		Comment,

		Output,
		Player,
		Blend,
		BoolBlend,
		IntBlend,
		LayeredBoneBlend,
		Additive,
		Parameter,

		TwoBoneIK,
		FABRIK,
		TransformBone,

		ComponentToLocal,
		LocalToComponent,

		Blendspace,
		Blendspace1D,
		BlendspaceAnimation,
		BlendspaceGraph,

		StateMachine,
		State,
		Entry,
		Transition,

		TimeRemainingRatio,

		AND,
		OR,
		NAND,
		NOR,
		XOR,
		NOT,

		Equality,
		Inequality,
		LessThan,
		LessThanOrEqual,
		GreaterThan,
		GreaterThanOrEqual,

		Add,
		Subtract,
		Multiply,
		Divide,
	};

	enum class AnimationPinType : uint32_t
	{
		None = 0,
		
		Pose,

		Bone,
		Bool,
		Int,
		Float,
		Vector2,
		Vector3,
		Vector4,
		Transform,
		
		Animation,
		Player
	};

	struct AnimationPinData
	{
	public:
		union
		{
			bool Bool;

			int Int;
			float Float;
			glm::vec2 Vector2;
			glm::vec3 Vector3;
			glm::vec4 Vector4;
			Dymatic::Transform Transform;

			uint64_t Handle;
		};

		std::string String;

	public:
		AnimationPinData()
		{
			std::memset(this, 0, sizeof(AnimationPinData));
		}

		AnimationPinData(AnimationPinType type)
		{
			switch (type)
			{
			case AnimationPinType::Bool: Bool = false;
			case AnimationPinType::Int: Int = 0;
			case AnimationPinType::Float: Float = 0.0f;
			case AnimationPinType::Vector2: Vector2 = glm::vec2(0.0f);
			case AnimationPinType::Vector3: Vector3 = glm::vec3(0.0f);
			case AnimationPinType::Vector4: Vector4 = glm::vec4(0.0f);
			case AnimationPinType::Transform: Transform = Dymatic::Transform();
			case AnimationPinType::Animation:
			case AnimationPinType::Player:
				Handle = 0;
			}
		}
	};

	struct AnimationPin : public EditorPin
	{
		AnimationPinType Type = AnimationPinType::None;
		AnimationPinData Data;

		AnimationPin() = default;
		AnimationPin(const std::string& name, AnimationPinType type, const bool hidden)
			: EditorPin(name, hidden), Type(type), Data(type)
		{}
	};

	struct AnimationEditorNode : public EditorNode
	{
		AnimationEditorNode(const AnimationNodeFunction function)
			: Function(function) {}

		Ref<AnimationPin> GetInput(uint32_t index) { return As<AnimationPin>(Inputs[index]); }
		Ref<AnimationPin> GetOutput(uint32_t index) { return As<AnimationPin>(Outputs[index]); }

		AnimationNodeFunction Function;
	};

	struct AnimationEditorLayeredBoneBlendNode : public AnimationEditorNode
	{
		AnimationEditorLayeredBoneBlendNode(const AnimationNodeFunction function)
			: AnimationEditorNode(function) {}

		struct FilterData
		{
			std::string BoneName;
			int32_t BlendDepth = 0;
		};

		std::vector<std::vector<FilterData>> BlendPoseFilters;
	};

	struct AnimationEditorGraph : public EditorGraph
	{
	public:
		static Ref<AnimationEditorGraph> Create() { return Ref<AnimationEditorGraph>(); }
		AnimationEditorGraph();

		static const char* GetNodeFunctionName(AnimationNodeFunction function);
		virtual const char* GetNodeFunctionName(Ref<Editor::EditorNode> node) const override;

		virtual NodeHandle SpawnNodeFromName(const std::string& name) override;

		Ref<AnimationEditorNode> FindNode(const NodeHandle id) const;
		Ref<AnimationPin> FindPin(const PinHandle id) const;
		virtual bool CanDeleteNode(NodeHandle id) const override;
		Ref<AnimationPin> AddPin(std::vector<Ref<EditorPin>>& target, const std::string& name, AnimationPinType type, const bool hidden = false);

		virtual bool PerformLinkCheck(Ref<EditorPin> a, Ref<EditorPin> b) override;
		virtual void OnCreateLink(LinkHandle linkID, Ref<EditorPin> a, Ref<EditorPin> b) override;
		virtual bool CanAddPin(Ref<EditorNode> node) const override;
		virtual void OnAddPin(Ref<EditorNode> node) override;

		static bool AreTypesCompatible(AnimationPinType a, AnimationPinType b);
		virtual bool AreTypesCompatible(PinHandle a, PinHandle b) const override;
		static bool CanCreateLink(Ref<AnimationPin> a, Ref<AnimationPin> b);
		virtual bool CanCreateLink(PinHandle a, PinHandle b) const override;

		// Main Pose Logic
		NodeHandle SpawnCommentNode(const glm::vec2 size = DefaultCommentSize);
		NodeHandle SpawnOutputNode(const bool comment = true);
		NodeHandle SpawnPlayerNode(AssetHandle handle);
		NodeHandle SpawnBlendNode();
		NodeHandle SpawnBoolBlendNode();
		NodeHandle SpawnIntBlendNode();
		NodeHandle SpawnLayeredBoneBlendNode();
		NodeHandle SpawnAdditiveNode();
		NodeHandle SpawnParameterNode(AnimationPinType type);
		NodeHandle SpawnTwoBoneIKNode();
		NodeHandle SpawnFABRIKNode();
		NodeHandle SpawnTransformBoneNode();
		NodeHandle SpawnLocalToComponentSpaceNode();
		NodeHandle SpawnComponentToLocalSpaceNode();
		NodeHandle SpawnBlendspaceNode();
		NodeHandle SpawnBlendspace1DNode();
		NodeHandle SpawnBlendspaceAnimationNode(AssetHandle handle);
		NodeHandle SpawnBlendspaceGraphNode();
		NodeHandle SpawnStateMachineNode();
		NodeHandle SpawnStateNode();
		NodeHandle SpawnEntryNode();
		NodeHandle SpawnTransitionNode();

		// Queries
		NodeHandle SpawnTimeRemainingRatioNode();

		// Value Operations
		NodeHandle SpawnBooleanOperatorNode(const AnimationNodeFunction function);
		NodeHandle SpawnANDNode();
		NodeHandle SpawnORNode();
		NodeHandle SpawnNANDNode();
		NodeHandle SpawnNORNode();
		NodeHandle SpawnXORNode();
		NodeHandle SpawnNOTNode();

		NodeHandle SpawnFloatComparatorNode(const AnimationNodeFunction function);
		NodeHandle SpawnEqualityNode();
		NodeHandle SpawnInequalityNode();
		NodeHandle SpawnLessThanNode();
		NodeHandle SpawnLessThanOrEqualNode();
		NodeHandle SpawnGreaterThanNode();
		NodeHandle SpawnGreaterThanOrEqualNode();

		NodeHandle SpawnFloatOperatorNode(const AnimationNodeFunction function);
		NodeHandle SpawnAddNode();
		NodeHandle SpawnSubtractNode();
		NodeHandle SpawnMultiplyNode();
		NodeHandle SpawnDivideNode();

	private:
		Ref<AnimationEditorNode> CreateNode(const AnimationNodeFunction function);

		template<typename T>
		Ref<T> CreateNode(const AnimationNodeFunction function);

	private:
		virtual Ref<EditorPin> CreateNewPin(const std::string& name) override;
	};

}
