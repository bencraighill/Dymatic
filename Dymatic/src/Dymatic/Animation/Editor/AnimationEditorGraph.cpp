#include "dypch.h"
#include "Dymatic/Animation/Editor/AnimationEditorGraph.h"

namespace Dymatic::Editor {

	AnimationEditorGraph::AnimationEditorGraph()
	{
	}

	const char* AnimationEditorGraph::GetNodeFunctionName(AnimationNodeFunction function)
	{
		switch (function)
		{
		case AnimationNodeFunction::None: return "None";
		case AnimationNodeFunction::Comment: return "Comment";
		case AnimationNodeFunction::Output: return "Output";
		case AnimationNodeFunction::Player: return "Player";
		case AnimationNodeFunction::Blend: return "Blend";
		case AnimationNodeFunction::BoolBlend: return "Blend by Bool";
		case AnimationNodeFunction::IntBlend: return "Blend by Int";
		case AnimationNodeFunction::LayeredBoneBlend: return "Layered Blend per Bone";
		case AnimationNodeFunction::Additive: return "Additive";
		case AnimationNodeFunction::Parameter: return "Parameter";
		case AnimationNodeFunction::TwoBoneIK: return "Two Bone IK";
		case AnimationNodeFunction::FABRIK: return "FABRIK";
		case AnimationNodeFunction::TransformBone: return "Transform Bone";
		case AnimationNodeFunction::LocalToComponent: return "Local To Component";
		case AnimationNodeFunction::ComponentToLocal: return "Component To Local";
		case AnimationNodeFunction::Blendspace: return "Blendspace";
		case AnimationNodeFunction::Blendspace1D: return "Blendspace 1D";
		case AnimationNodeFunction::BlendspaceAnimation: return "Blendspace Animation";
		case AnimationNodeFunction::BlendspaceGraph: return "Blendspace Graph";
		case AnimationNodeFunction::StateMachine: return "State Machine";
		case AnimationNodeFunction::State: return "State";
		case AnimationNodeFunction::Entry: return "Entry";
		case AnimationNodeFunction::Transition: return "Transition";
		case AnimationNodeFunction::TimeRemainingRatio: return "Time Remaining Ratio";
		case AnimationNodeFunction::AND: return "AND";
		case AnimationNodeFunction::OR: return "OR";
		case AnimationNodeFunction::NAND: return "NAND";
		case AnimationNodeFunction::NOR: return "NOR";
		case AnimationNodeFunction::XOR: return "XOR";
		case AnimationNodeFunction::NOT: return "NOT";
		case AnimationNodeFunction::Equality: return "Equality";
		case AnimationNodeFunction::Inequality: return "Inequality";
		case AnimationNodeFunction::LessThan: return "Less Than";
		case AnimationNodeFunction::LessThanOrEqual: return "Less Than Or Equal";
		case AnimationNodeFunction::GreaterThan: return "Greater Than";
		case AnimationNodeFunction::GreaterThanOrEqual: return "Greater Than Or Equal";
		case AnimationNodeFunction::Add: return "Add";
		case AnimationNodeFunction::Subtract: return "Subtract";
		case AnimationNodeFunction::Multiply: return "Multiply";
		case AnimationNodeFunction::Divide: return "Divide";
		}
	}

	const char* AnimationEditorGraph::GetNodeFunctionName(Ref<Editor::EditorNode> node) const
	{
		return GetNodeFunctionName(As<AnimationEditorNode>(node)->Function);
	}

	Ref<AnimationEditorNode> AnimationEditorGraph::CreateNode(const AnimationNodeFunction function)
	{
		return CreateNode<AnimationEditorNode>(function);
	}

	template<typename T>
	Ref<T> AnimationEditorGraph::CreateNode(const AnimationNodeFunction function)
	{
		Ref<T> node = CreateRef<T>(function);
		CreateNodeInternal(node);
		return node;
	}

	NodeHandle AnimationEditorGraph::SpawnNodeFromName(const std::string& name)
	{
		if (name == "Comment") return SpawnCommentNode();
		if (name == "Output") return SpawnOutputNode(false);
		if (name == "Player") return SpawnPlayerNode(0);
		if (name == "Blend") return SpawnBlendNode();
		if (name == "Blend by Bool") return SpawnBoolBlendNode();
		if (name == "Blend by Int") return SpawnIntBlendNode();
		if (name == "Layered Blend per Bone") return SpawnLayeredBoneBlendNode();
		if (name == "Additive") return SpawnAdditiveNode();
		if (name == "Parameter") return SpawnParameterNode(Editor::AnimationPinType::Float);
		if (name == "Two Bone IK") return SpawnTwoBoneIKNode();
		if (name == "FABRIK") return SpawnFABRIKNode();
		if (name == "Transform Bone") return SpawnTransformBoneNode();
		if (name == "Local To Component") return SpawnLocalToComponentSpaceNode();
		if (name == "Component To Local") return SpawnComponentToLocalSpaceNode();
		if (name == "Blendspace") return SpawnBlendspaceNode();
		if (name == "Blendspace 1D") return SpawnBlendspace1DNode();
		if (name == "Blendspace Animation") return SpawnBlendspaceAnimationNode(0);
		if (name == "Blendspace Graph") return SpawnBlendspaceGraphNode();
		if (name == "State Machine") return SpawnStateMachineNode();
		if (name == "State") return SpawnStateNode();
		if (name == "Entry") return SpawnEntryNode();
		if (name == "Transition") return SpawnTransitionNode();
		if (name == "Time Remaining Ratio") return SpawnTimeRemainingRatioNode();
		if (name == "AND") return SpawnANDNode();
		if (name == "OR") return SpawnORNode();
		if (name == "NAND") return SpawnNANDNode();
		if (name == "NOR") return SpawnNORNode();
		if (name == "XOR") return SpawnXORNode();
		if (name == "NOT") return SpawnNOTNode();
		if (name == "Equality") return SpawnEqualityNode();
		if (name == "Inequality") return SpawnInequalityNode();
		if (name == "Less Than") return SpawnLessThanNode();
		if (name == "Less Than Or Equal") return SpawnLessThanOrEqualNode();
		if (name == "Greater Than") return SpawnGreaterThanNode();
		if (name == "Greater Than Or Equal") return SpawnGreaterThanOrEqualNode();
		if (name == "Add") return SpawnAddNode();
		if (name == "Subtract") return SpawnSubtractNode();
		if (name == "Multiply") return SpawnMultiplyNode();
		if (name == "Divide") return SpawnDivideNode();

		return 0;
	}

	Ref<AnimationEditorNode> AnimationEditorGraph::FindNode(const NodeHandle id) const
	{
		return As<AnimationEditorNode>(FindNodeInternal(id));
	}

	Ref<AnimationPin> AnimationEditorGraph::FindPin(const PinHandle id) const
	{
		return As<AnimationPin>(FindPinInternal(id));
	}

	bool AnimationEditorGraph::CanDeleteNode(NodeHandle id) const
	{
		Ref<AnimationEditorNode> node = FindNode(id);
		return node && node->Function != AnimationNodeFunction::Output && node->Function != AnimationNodeFunction::Entry && node->Function != AnimationNodeFunction::Transition;
	}

	Ref<AnimationPin> AnimationEditorGraph::AddPin(std::vector<Ref<EditorPin>>& target, const std::string& name, AnimationPinType type, const bool hidden)
	{
		Ref<AnimationPin> node = CreateRef<AnimationPin>(name, type, hidden);
		target.emplace_back(node);
		return node;
	}

	NodeHandle AnimationEditorGraph::SpawnCommentNode(const glm::vec2 size)
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::Comment);
		EditorGraph::SetupCommentNode(node, size);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnOutputNode(const bool comment)
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::Output);
		AddPin(node->Inputs, "Output Pose", AnimationPinType::Pose);

		if (comment)
		{
			node->Comment = "The output node of the animation graph.";
			node->CommentEnabled = true;
			node->CommentPinned = true;
		}

		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnPlayerNode(AssetHandle handle)
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::Player);
		AddPin(node->Inputs, std::string(), AnimationPinType::Animation, true)->Data.Handle = handle;
		AddPin(node->Inputs, "Play Rate", AnimationPinType::Float)->Data.Float = 1.0f;
		AddPin(node->Outputs, std::string(), AnimationPinType::Pose);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnBlendNode()
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::Blend);
		AddPin(node->Inputs, "A", AnimationPinType::Pose);
		AddPin(node->Inputs, "B", AnimationPinType::Pose);
		AddPin(node->Inputs, "Weight", AnimationPinType::Float);
		AddPin(node->Outputs, std::string(), AnimationPinType::Pose);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnParameterNode(AnimationPinType type)
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::Parameter);
		node->Name = "Parameter";
		AddPin(node->Inputs, "Default", type, true);
		AddPin(node->Outputs, std::string(), type);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnTwoBoneIKNode()
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::TwoBoneIK);
		AddPin(node->Inputs, "Effector Location", AnimationPinType::Vector3);
		AddPin(node->Inputs, "Joint Target Location", AnimationPinType::Vector3);
		AddPin(node->Inputs, "Component Pose", AnimationPinType::Pose);
		AddPin(node->Inputs, "Alpha", AnimationPinType::Float)->Data.Float = 1.0f;
		AddPin(node->Inputs, "Allow Stretching", AnimationPinType::Bool, true)->Data.Bool = false;
		AddPin(node->Inputs, "Start Stretch Ratio", AnimationPinType::Float, true)->Data.Float = 1.0f;
		AddPin(node->Inputs, "Max Stretch Scale", AnimationPinType::Float, true)->Data.Float = 1.2f;
		AddPin(node->Inputs, "Target Bone", AnimationPinType::Bone, true);
		AddPin(node->Outputs, std::string(), AnimationPinType::Pose);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnFABRIKNode()
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::FABRIK);
		AddPin(node->Inputs, "Effector Transform", AnimationPinType::Transform);
		AddPin(node->Inputs, "Component Pose", AnimationPinType::Pose);
		AddPin(node->Inputs, "Alpha", AnimationPinType::Float)->Data.Float = 1.0f;
		AddPin(node->Inputs, "Precision", AnimationPinType::Float, true)->Data.Float = 1.0f;
		AddPin(node->Inputs, "Max Iterations", AnimationPinType::Int, true)->Data.Int = 10;
		AddPin(node->Inputs, "Tip Rotation Source", AnimationPinType::Int, true)->Data.Int = 0;
		AddPin(node->Inputs, "Root Bone", AnimationPinType::Bone, true);
		AddPin(node->Inputs, "Tip Bone", AnimationPinType::Bone, true);
		AddPin(node->Outputs, std::string(), AnimationPinType::Pose);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnTransformBoneNode()
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::TransformBone);
		AddPin(node->Inputs, "Pose", AnimationPinType::Pose);
		AddPin(node->Inputs, "Target Bone", AnimationPinType::Bone, true);
		AddPin(node->Inputs, "Translation", AnimationPinType::Vector3);
		AddPin(node->Inputs, "Translation Mode", AnimationPinType::Int, true);
		AddPin(node->Inputs, "Rotation", AnimationPinType::Vector3);
		AddPin(node->Inputs, "Rotation Mode", AnimationPinType::Int, true);
		AddPin(node->Inputs, "Scale", AnimationPinType::Vector3)->Data.Vector3 = glm::vec3(1.0f);
		AddPin(node->Inputs, "Scale Mode", AnimationPinType::Int, true);
		AddPin(node->Inputs, "Alpha", AnimationPinType::Float)->Data.Float = 1.0f;
		AddPin(node->Outputs, std::string(), AnimationPinType::Pose);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnLocalToComponentSpaceNode()
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::LocalToComponent);
		AddPin(node->Inputs, std::string(), AnimationPinType::Pose);
		AddPin(node->Outputs, std::string(), AnimationPinType::Pose);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnComponentToLocalSpaceNode()
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::ComponentToLocal);
		AddPin(node->Inputs, std::string(), AnimationPinType::Pose);
		AddPin(node->Outputs, std::string(), AnimationPinType::Pose);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnBoolBlendNode()
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::BoolBlend);
		AddPin(node->Inputs, "Bool", AnimationPinType::Bool);
		AddPin(node->Inputs, "Reset on Blend", AnimationPinType::Bool, true);
		AddPin(node->Inputs, "False", AnimationPinType::Pose);
		AddPin(node->Inputs, "True", AnimationPinType::Pose);
		AddPin(node->Inputs, "False Blend Time", AnimationPinType::Float);
		AddPin(node->Inputs, "True Blend Time", AnimationPinType::Float);
		AddPin(node->Outputs, std::string(), AnimationPinType::Pose);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnIntBlendNode()
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::IntBlend);
		AddPin(node->Inputs, "Int", AnimationPinType::Int);
		AddPin(node->Inputs, "Reset on Blend", AnimationPinType::Bool, true);
		OnAddPin(node);
		OnAddPin(node);
		AddPin(node->Outputs, std::string(), AnimationPinType::Pose);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnLayeredBoneBlendNode()
	{
		Ref<AnimationEditorNode> node = CreateNode<AnimationEditorLayeredBoneBlendNode>(AnimationNodeFunction::LayeredBoneBlend);
		AddPin(node->Inputs, "Base", AnimationPinType::Pose);
		AddPin(node->Inputs, "Blend Pose 0", AnimationPinType::Pose);
		AddPin(node->Inputs, "Blend Weight 0", AnimationPinType::Float);
		AddPin(node->Outputs, std::string(), AnimationPinType::Pose);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnAdditiveNode()
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::Additive);
		AddPin(node->Inputs, "Base", AnimationPinType::Pose);
		AddPin(node->Inputs, "Additive", AnimationPinType::Pose);
		AddPin(node->Inputs, "Alpha", AnimationPinType::Float);
		AddPin(node->Outputs, std::string(), AnimationPinType::Pose);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnBlendspaceNode()
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::Blendspace);
		node->TargetLayer = UUID();
		AddPin(node->Inputs, "X", AnimationPinType::Float);
		AddPin(node->Inputs, "Y", AnimationPinType::Float);
		AddPin(node->Inputs, "Scale X", AnimationPinType::Float, true)->Data.Float = 1.0f;
		AddPin(node->Inputs, "Scale Y", AnimationPinType::Float, true)->Data.Float = 1.0f;
		AddPin(node->Outputs, std::string(), AnimationPinType::Pose);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnBlendspace1DNode()
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::Blendspace1D);
		node->TargetLayer = UUID();
		AddPin(node->Inputs, "X", AnimationPinType::Float);
		AddPin(node->Inputs, "Scale X", AnimationPinType::Float, true)->Data.Float = 1.0f;
		AddPin(node->Outputs, std::string(), AnimationPinType::Pose);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnBlendspaceAnimationNode(AssetHandle handle)
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::BlendspaceAnimation);
		node->Type = NodeType::Point;
		AddPin(node->Inputs, "Animation", AnimationPinType::Animation, true)->Data.Handle = handle;
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnBlendspaceGraphNode()
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::BlendspaceGraph);
		node->Type = NodeType::Point;
		node->TargetLayer = UUID();
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;

		// Setup internal graph
		SetNextLayerID(node->TargetLayer);
		SpawnOutputNode(false);

		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnStateMachineNode()
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::StateMachine);
		node->TargetLayer = UUID();
		AddPin(node->Inputs, "Max Transitions Per Frame", AnimationPinType::Int, true)->Data.Int = 3;
		AddPin(node->Inputs, "Skip First Update Transition", AnimationPinType::Bool, true)->Data.Bool = true;
		AddPin(node->Outputs, "State", AnimationPinType::Pose);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;

		// Setup internal graph
		SetNextLayerID(node->TargetLayer);
		SpawnEntryNode();

		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnStateNode()
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::State);
		node->Name = "New State";
		node->Type = NodeType::Tree;
		node->TargetLayer = UUID();
		AddPin(node->Inputs, std::string(), AnimationPinType::Pose);
		AddPin(node->Outputs, std::string(), AnimationPinType::Pose);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;

		// Setup internal graph
		SetNextLayerID(node->TargetLayer);
		SpawnOutputNode(false);

		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnEntryNode()
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::Entry);
		node->Type = NodeType::Tree;
		AddPin(node->Outputs, std::string(), AnimationPinType::Pose);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnTransitionNode()
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::Transition);
		AddPin(node->Inputs, "Transition", AnimationPinType::Bool);
		AddPin(node->Inputs, "Duration", AnimationPinType::Float)->Data.Float = 0.2f;
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnTimeRemainingRatioNode()
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::TimeRemainingRatio);
		AddPin(node->Inputs, "Target Player", AnimationPinType::Player, true);
		AddPin(node->Outputs, "Time Remaining (Ratio)", AnimationPinType::Float);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnBooleanOperatorNode(const AnimationNodeFunction function)
	{
		Ref<AnimationEditorNode> node = CreateNode(function);
		node->Type = NodeType::Simple;
		AddPin(node->Inputs, "A", AnimationPinType::Bool);
		AddPin(node->Inputs, "B", AnimationPinType::Bool);
		AddPin(node->Outputs, std::string(), AnimationPinType::Bool);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnANDNode()
	{
		return SpawnBooleanOperatorNode(AnimationNodeFunction::AND);
	}

	NodeHandle AnimationEditorGraph::SpawnORNode()
	{
		return SpawnBooleanOperatorNode(AnimationNodeFunction::OR);
	}

	NodeHandle AnimationEditorGraph::SpawnNANDNode()
	{
		return SpawnBooleanOperatorNode(AnimationNodeFunction::NAND);
	}

	NodeHandle AnimationEditorGraph::SpawnNORNode()
	{
		return SpawnBooleanOperatorNode(AnimationNodeFunction::NOR);
	}

	NodeHandle AnimationEditorGraph::SpawnXORNode()
	{
		return SpawnBooleanOperatorNode(AnimationNodeFunction::XOR);
	}

	NodeHandle AnimationEditorGraph::SpawnNOTNode()
	{
		Ref<AnimationEditorNode> node = CreateNode(AnimationNodeFunction::NOT);
		node->Type = NodeType::Simple;
		AddPin(node->Inputs, std::string(), AnimationPinType::Bool);
		AddPin(node->Outputs, std::string(), AnimationPinType::Bool);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnFloatComparatorNode(const AnimationNodeFunction function)
	{
		Ref<AnimationEditorNode> node = CreateNode(function);
		node->Type = NodeType::Simple;
		AddPin(node->Inputs, "A", AnimationPinType::Float);
		AddPin(node->Inputs, "B", AnimationPinType::Float);
		AddPin(node->Outputs, std::string(), AnimationPinType::Bool);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnEqualityNode()
	{
		return SpawnFloatComparatorNode(AnimationNodeFunction::Equality);
	}

	NodeHandle AnimationEditorGraph::SpawnInequalityNode()
	{
		return SpawnFloatComparatorNode(AnimationNodeFunction::Inequality);
	}

	NodeHandle AnimationEditorGraph::SpawnLessThanNode()
	{
		return SpawnFloatComparatorNode(AnimationNodeFunction::LessThan);
	}

	NodeHandle AnimationEditorGraph::SpawnLessThanOrEqualNode()
	{
		return SpawnFloatComparatorNode(AnimationNodeFunction::LessThanOrEqual);
	}

	NodeHandle AnimationEditorGraph::SpawnGreaterThanNode()
	{
		return SpawnFloatComparatorNode(AnimationNodeFunction::GreaterThan);
	}

	NodeHandle AnimationEditorGraph::SpawnGreaterThanOrEqualNode()
	{
		return SpawnFloatComparatorNode(AnimationNodeFunction::GreaterThanOrEqual);
	}

	NodeHandle AnimationEditorGraph::SpawnFloatOperatorNode(const AnimationNodeFunction function)
	{
		Ref<AnimationEditorNode> node = CreateNode(function);
		node->Type = NodeType::Simple;
		AddPin(node->Inputs, "A", AnimationPinType::Float);
		AddPin(node->Inputs, "B", AnimationPinType::Float);
		AddPin(node->Outputs, std::string(), AnimationPinType::Float);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle AnimationEditorGraph::SpawnAddNode()
	{
		return SpawnFloatOperatorNode(AnimationNodeFunction::Add);
	}

	NodeHandle AnimationEditorGraph::SpawnSubtractNode()
	{
		return SpawnFloatOperatorNode(AnimationNodeFunction::Subtract);
	}

	NodeHandle AnimationEditorGraph::SpawnMultiplyNode()
	{
		return SpawnFloatOperatorNode(AnimationNodeFunction::Multiply);
	}

	NodeHandle AnimationEditorGraph::SpawnDivideNode()
	{
		return SpawnFloatOperatorNode(AnimationNodeFunction::Divide);
	}
	
	bool AnimationEditorGraph::PerformLinkCheck(Ref<EditorPin> a, Ref<EditorPin> b)
	{
		const AnimationNodeFunction function = FindNode(a->Node)->Function;

		if (function != AnimationNodeFunction::State && function != AnimationNodeFunction::Entry)
			return true;

		LinkHandle linkToDelete = 0;
		for (const auto& [linkID, link] : Links)
		{
			if (link.StartPinID == a->ID && link.EndPinID == b->ID)
			{
				linkToDelete = linkID;
				break;
			}
		}

		if (linkToDelete != 0)
			DeleteLink(linkToDelete);

		return false;
	}


	void AnimationEditorGraph::OnCreateLink(LinkHandle linkID, Ref<EditorPin> a, Ref<EditorPin> b)
	{
		if (FindNode(a->Node)->Function != AnimationNodeFunction::State)
			return;

		// Link has associated layer with transition
		Link& link = Links[linkID];
		link.TargetLayer = UUID();

		SetNextLayerID(link.TargetLayer);
		SpawnTransitionNode();
	}

	bool AnimationEditorGraph::CanAddPin(Ref<EditorNode> node) const
	{
		const AnimationNodeFunction function = As<AnimationEditorNode>(node)->Function;
		return function == AnimationNodeFunction::LayeredBoneBlend || function == AnimationNodeFunction::IntBlend;
	}

	void AnimationEditorGraph::OnAddPin(Ref<EditorNode> internalNode)
	{
		const Ref<AnimationEditorNode> node = As<AnimationEditorNode>(internalNode);
		const AnimationNodeFunction function = node->Function;

		if (function == AnimationNodeFunction::LayeredBoneBlend)
		{
			const uint32_t index = (node->Inputs.size()) / 2;
			AddPin(node->Inputs, fmt::format("Blend Pose {}", index), AnimationPinType::Pose);
			AddPin(node->Inputs, fmt::format("Blend Weight {}", index), AnimationPinType::Float);
		}
		else if (function == AnimationNodeFunction::IntBlend)
		{
			const uint32_t index = (node->Inputs.size()) / 2 - 1;
			AddPin(node->Inputs, fmt::format("Blend Pose {}", index), AnimationPinType::Pose);
			AddPin(node->Inputs, fmt::format("Blend Time {}", index), AnimationPinType::Float);
		}
	}

	bool AnimationEditorGraph::AreTypesCompatible(AnimationPinType a, AnimationPinType b)
	{
		return a == b;
	}

	bool AnimationEditorGraph::AreTypesCompatible(PinHandle a, PinHandle b) const
	{
		Ref<AnimationPin> pinA = FindPin(a);
		Ref<AnimationPin> pinB = FindPin(b);

		if (pinA && pinB)
			return AreTypesCompatible(pinA->Type, pinB->Type);

		return false;
	}

	bool AnimationEditorGraph::CanCreateLink(Ref<AnimationPin> a, Ref<AnimationPin> b)
	{
		return (a && b) && (a->ID != b->ID) && (a->Kind != b->Kind) && AreTypesCompatible(a->Type, b->Type) && (a->Node != b->Node);
	}

	bool AnimationEditorGraph::CanCreateLink(PinHandle a, PinHandle b) const
	{
		Ref<AnimationPin> pinA = FindPin(a);
		Ref<AnimationPin> pinB = FindPin(b);

		if (pinA && pinB)
			return CanCreateLink(pinA, pinB);

		return false;
	}

	Ref<EditorPin> AnimationEditorGraph::CreateNewPin(const std::string& name)
	{
		return CreateRef<AnimationPin>(name, AnimationPinType::Pose, false);
	}

}