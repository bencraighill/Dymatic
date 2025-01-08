#include "dypch.h"
#include "Dymatic/Animation/Editor/AnimationGraphCompiler.h"

#include "Dymatic/Core/Version.h"
#include "Dymatic/Asset/AssetManager.h"

#include <stack>

namespace Dymatic::Editor {

	namespace Utils {
	
		AnimationGraphData EditorToRuntimeGraphData(const AnimationPinType type, const AnimationPinData& data)
		{
			switch (type)
			{
			case AnimationPinType::Bool:		return AnimationGraphData(data.Bool);
			case AnimationPinType::Int:			return AnimationGraphData(data.Int);
			case AnimationPinType::Float:		return AnimationGraphData(data.Float);
			case AnimationPinType::Vector2:		return AnimationGraphData(data.Vector2);
			case AnimationPinType::Vector3:		return AnimationGraphData(data.Vector3);
			case AnimationPinType::Vector4:		return AnimationGraphData(data.Vector4);
			case AnimationPinType::Transform:	return AnimationGraphData(data.Transform);
			case AnimationPinType::Animation:	return AnimationGraphData(data.Handle);
			case AnimationPinType::Player:		return AnimationGraphData(data.Handle);
			}

			DY_CORE_ASSERT(false);
		}

		static AnimationOperatorNode::OperatorType GetOperatorType(const AnimationNodeFunction function)
		{
			switch (function)
			{
			case AnimationNodeFunction::AND:				return AnimationOperatorNode::OperatorType::AND;
			case AnimationNodeFunction::OR:					return AnimationOperatorNode::OperatorType::OR;
			case AnimationNodeFunction::NAND:				return AnimationOperatorNode::OperatorType::NAND;
			case AnimationNodeFunction::NOR:				return AnimationOperatorNode::OperatorType::NOR;
			case AnimationNodeFunction::XOR:				return AnimationOperatorNode::OperatorType::XOR;

			case AnimationNodeFunction::Equality:			return AnimationOperatorNode::OperatorType::Equality;
			case AnimationNodeFunction::Inequality:			return AnimationOperatorNode::OperatorType::Inequality;
			case AnimationNodeFunction::LessThan:			return AnimationOperatorNode::OperatorType::LessThan;
			case AnimationNodeFunction::LessThanOrEqual:	return AnimationOperatorNode::OperatorType::LessThanOrEqual;
			case AnimationNodeFunction::GreaterThan:		return AnimationOperatorNode::OperatorType::GreaterThan;
			case AnimationNodeFunction::GreaterThanOrEqual:	return AnimationOperatorNode::OperatorType::GreaterThanOrEqual;

			case AnimationNodeFunction::Add:				return AnimationOperatorNode::OperatorType::Add;
			case AnimationNodeFunction::Subtract:			return AnimationOperatorNode::OperatorType::Subtract;
			case AnimationNodeFunction::Multiply:			return AnimationOperatorNode::OperatorType::Multiply;
			case AnimationNodeFunction::Divide:				return AnimationOperatorNode::OperatorType::Divide;
			}

			return AnimationOperatorNode::OperatorType::None;
		}

	}

	AnimationGraphCompiler::AnimationGraphCompiler(Ref<AnimationEditorGraph> editorGraph)
		: m_EditorGraph(editorGraph)
	{}

	void AnimationGraphCompiler::Compile()
	{
		// Reset
		m_CompilerResult = CompilerResult();
		m_DefaultNode = nullptr;
		m_AnimationNodes = {};
		m_ProcessingNodes = {};

		// Compile
		CompileGraph();

		// Evaluate Result
		const std::string animationGraphName = AssetManager::GetMetadata(TargetHandle).FilePath.stem().string();
		if (m_CompilerResult.Success)
			m_CompilerResult.Add(fmt::format("Compilation of '{}' completed [Animation Graph] - Dymatic Animation Nodes Version " DY_VERSION_STRING, animationGraphName));
		else
		{
			m_CompilerResult.Add(fmt::format("Compilation of '{}' failed [Animation Graph] - {} Error(s) {} Warnings(s) - Dymatic Animation Nodes Version " DY_VERSION_STRING, animationGraphName, m_CompilerResult.ErrorCount, m_CompilerResult.WarningCount));
			m_AnimationGraph = nullptr;
		}
	}

	void AnimationGraphCompiler::CompileGraph()
	{
		if (!TargetSkeleton)
		{
			m_CompilerResult.Add("No target skeleton was specified for output animation graph : Aborting graph compilation.", CompilerMessageType::Error);
			return;
		}

		Ref<Skeleton> skeleton = AssetManager::GetAsset<Skeleton>(TargetSkeleton);

		if (!skeleton)
		{
			m_CompilerResult.Add("Failed to load target skeleton specified for animation graph : Aborting graph compilation.", CompilerMessageType::Error);
			return;
		}

		m_CompilerResult.Add("Build Started [Animation Graph] - Dymatic Animation Nodes Version " DY_VERSION_STRING);
		m_CompilerResult.Add("Initializing Pre Compile Link Checks...");

		// TODO: Check that all links between pins are valid. We probably need a more optimal way to access pins before doing this (use a hash map)!

		// Verify root node integrity
		Ref<AnimationEditorNode> outputNode = FindNodeOnLayer(0, AnimationNodeFunction::Output);

		if (!outputNode)
		{
			m_CompilerResult.Add("No output node found in default graph! Critical animation graph compilation failure. Aborting...", CompilerMessageType::Error);
			return;
		}

		if (!m_EditorGraph->IsPinLinked(outputNode->Inputs[0]->ID))
			m_CompilerResult.Add("No pose was connected to the output node during compilation!", CompilerMessageType::Warning, outputNode->ID);

		m_CompilerResult.Add("Initializing Asset Checks...");

		// Verify animation assets
		for (const auto& [nodeId, node] : m_EditorGraph->Nodes)
		{
			Ref<AnimationEditorNode> animationNode = As<AnimationEditorNode>(node);

			if (animationNode->Function != AnimationNodeFunction::Player && animationNode->Function != AnimationNodeFunction::BlendspaceAnimation)
				continue;

			if (m_EditorGraph->IsPinLinked(animationNode->ID))
				continue;

			const AssetHandle handle = animationNode->GetInput(0)->Data.Handle;

			if (handle == 0)
				m_CompilerResult.Add("Animation asset was not specified for player node!", CompilerMessageType::Error, node->ID);
			else if (!AssetManager::DoesAssetExist(handle) || AssetManager::GetMetadata(handle).Type != AssetType::Animation)
				m_CompilerResult.Add("Player node asset does not exist or is not of type animation!", CompilerMessageType::Error, node->ID);
		}

		// Create the new asset (Note: We will terminate this reference after compilation if the build fails)
		m_AnimationGraph = AnimationGraph::Create(skeleton, m_EditorGraph);

		m_CompilerResult.Add("Initializing Node Tree Parse...");

		// Recursively traverse the animation graph
		const NodeHandle previousNodeID = GetConnectedNode(outputNode->Inputs[0]->ID);
		const Ref<AnimationPoseNode> previousNode = GenerateRuntimeNode(previousNodeID);

		// The engine will automatically insert a local-space to component-space node as it is assumed that all graphs are in local space by default.
		// TODO: This should be optimized to check if we are already in component-space so the end user doesn't have to unnecessarily convert to local just for use to convert back (very expensive)
		const Ref<AnimationPoseNode> output = CreateRef<AnimationLocalToComponentSpaceNode>(m_AnimationGraph, previousNode);

		m_AnimationGraph->SetOutputNode(output);
		m_AnimationGraph->m_CompileID = UUID();
	}

	Ref<AnimationPoseNode> AnimationGraphCompiler::GenerateRuntimeNode(NodeHandle nodeID)
	{
		// If we already have this node path generated it should be reused
		if (m_AnimationNodes.find(nodeID) != m_AnimationNodes.end())
			return m_AnimationNodes.at(nodeID);

		// Insert a 'default' (bind pose) node at the end of empty node chains
		// Note: All chain ends link to the same node to avoid repeated calculations
		if (nodeID == 0)
			return GetDefaultNode();

		// Check for cyclical dependencies
		if (m_ProcessingNodes.find(nodeID) != m_ProcessingNodes.end())
		{
			m_CompilerResult.Add("Cyclical node dependency detected!", CompilerMessageType::Error, nodeID);
			return nullptr;
		}

		m_ProcessingNodes.insert(nodeID);

		// Gather editor/input node details
		Ref<AnimationEditorNode> editorNode = m_EditorGraph->FindNode(nodeID);
		const AnimationNodeFunction nodeFunction = editorNode->Function;

		// Verify that all bones are valid
		for (const auto& pin : editorNode->Inputs)
		{
			Ref<AnimationPin> input = As<AnimationPin>(pin);

			if (input->Type == AnimationPinType::Bone && !DoesSkeletonContainBone(input->Data.String))
			{
				m_CompilerResult.Add(fmt::format("Bone specified for input '{}' of node '{}' does not exist on target skeleton {}", input->Name, AnimationEditorGraph::GetNodeFunctionName(editorNode->Function), m_AnimationGraph->m_Skeleton->Handle), CompilerMessageType::Error, nodeID);
				return GetDefaultNode();
			}
		}

		// Generate corresponding runtime/output node
		if (nodeFunction == AnimationNodeFunction::Player)
		{
			const AssetHandle animationHandle = editorNode->GetInput(0)->Data.Handle;
			const Ref<Animation> animation = AssetManager::GetAsset<Animation>(animationHandle);
			const Ref<AnimationPin> playRatePin = editorNode->GetInput(1);
			const Ref<AnimationValueNode> playRate = playRatePin->Data.Float == 1.0f ? nullptr : GenerateRuntimeValueNode(playRatePin);
			m_AnimationNodes[nodeID] = CreateRef<AnimationPlayerNode>(m_AnimationGraph, animation, playRate);
		}
		else if (nodeFunction == AnimationNodeFunction::Blend)
		{
			Ref<AnimationPoseNode> a = GenerateRuntimeNode(GetConnectedNode(editorNode->GetInput(0)->ID));
			Ref<AnimationPoseNode> b = GenerateRuntimeNode(GetConnectedNode(editorNode->GetInput(1)->ID));
			Ref<AnimationValueNode> weight = GenerateRuntimeValueNode(editorNode->GetInput(2));
			m_AnimationNodes[nodeID] = CreateRef<AnimationBlendNode>(m_AnimationGraph, a, b, weight);
		}
		else if (nodeFunction == AnimationNodeFunction::LayeredBoneBlend)
		{
			const Ref<AnimationEditorLayeredBoneBlendNode> editorLayeredBlendNode = As<AnimationEditorLayeredBoneBlendNode>(editorNode);

			// Validate bone names
			for (const auto& filters : editorLayeredBlendNode->BlendPoseFilters)
			{
				for (const auto& filter : filters)
				{
					if (!DoesSkeletonContainBone(filter.BoneName))
					{
						m_CompilerResult.Add(fmt::format("Bone '{}' specified for Layered Blend filter does not exist on target skeleton {}", filter.BoneName, m_AnimationGraph->m_Skeleton->Handle), CompilerMessageType::Error, nodeID);
						return GetDefaultNode();
					}
				}
			}

			const Ref<AnimationPoseNode> basePose = GenerateRuntimeNode(GetConnectedNode(editorNode->GetInput(0)->ID));

			std::vector<AnimationLayeredBoneBlendNode::BlendPose> blendPoses;
			const uint32_t pinCount = editorNode->Inputs.size();
			blendPoses.reserve((pinCount - 1) / 2);

			for (uint32_t pinIndex = 1; pinIndex < pinCount; pinIndex += 2)
			{
				auto& blendPose = blendPoses.emplace_back();
				blendPose.AnimationPose = GenerateRuntimeNode(GetConnectedNode(editorLayeredBlendNode->GetInput(pinIndex)->ID));
				blendPose.BlendWeight = GenerateRuntimeValueNode(editorLayeredBlendNode->GetInput(pinIndex + 1));

				const auto& editorFilters = editorLayeredBlendNode->BlendPoseFilters[(pinIndex - 1) / 2];
				blendPose.BranchFilters.reserve(editorFilters.size());

				for (const auto& editorFilter : editorFilters)
					blendPose.BranchFilters.emplace_back(editorFilter.BoneName, editorFilter.BlendDepth);
			}

			m_AnimationNodes[nodeID] = CreateRef<AnimationLayeredBoneBlendNode>(m_AnimationGraph, basePose, blendPoses);
		}
		else if (nodeFunction == AnimationNodeFunction::Additive)
		{
			Ref<AnimationPoseNode> base = GenerateRuntimeNode(GetConnectedNode(editorNode->GetInput(0)->ID));
			Ref<AnimationPoseNode> additive = GenerateRuntimeNode(GetConnectedNode(editorNode->GetInput(1)->ID));
			Ref<AnimationValueNode> alpha = GenerateRuntimeValueNode(editorNode->GetInput(2));
			m_AnimationNodes[nodeID] = CreateRef<AnimationAdditiveNode>(m_AnimationGraph, base, additive, alpha);
		}
		else if (nodeFunction == AnimationNodeFunction::BoolBlend)
		{
			const Ref<AnimationValueNode> flag = GenerateRuntimeValueNode(editorNode->GetInput(0));
			const bool resetOnBlend = editorNode->GetInput(0)->Data.Bool;
			const Ref<AnimationPoseNode> falsePose = GenerateRuntimeNode(GetConnectedNode(editorNode->GetInput(2)->ID));
			const Ref<AnimationPoseNode> truePose = GenerateRuntimeNode(GetConnectedNode(editorNode->GetInput(3)->ID));
			const Ref<AnimationValueNode> falseBlendTime = GenerateRuntimeValueNode(editorNode->GetInput(4));
			const Ref<AnimationValueNode> trueBlendTime = GenerateRuntimeValueNode(editorNode->GetInput(5));
			m_AnimationNodes[nodeID] = CreateRef<AnimationBoolBlendNode>(m_AnimationGraph, flag, truePose, falsePose, trueBlendTime, falseBlendTime, resetOnBlend);
		}
		else if (nodeFunction == AnimationNodeFunction::IntBlend)
		{
			const Ref<AnimationValueNode> value = GenerateRuntimeValueNode(editorNode->GetInput(0));
			const bool resetOnBlend = editorNode->GetInput(1)->Data.Bool;

			std::vector<AnimationIntBlendNode::BlendPose> blendPoses;
			const uint32_t pinCount = editorNode->Inputs.size();
			blendPoses.reserve((pinCount - 2) / 2);

			for (uint32_t pinIndex = 2; pinIndex < pinCount; pinIndex += 2)
			{
				auto& blendPose = blendPoses.emplace_back();
				blendPose.AnimationPose = GenerateRuntimeNode(GetConnectedNode(editorNode->GetInput(pinIndex)->ID));
				blendPose.BlendTime = GenerateRuntimeValueNode(editorNode->GetInput(pinIndex + 1));
			}

			m_AnimationNodes[nodeID] = CreateRef<AnimationIntBlendNode>(m_AnimationGraph, value, blendPoses, resetOnBlend);
		}
		else if (nodeFunction == AnimationNodeFunction::LocalToComponent)
		{
			Ref<AnimationPoseNode> local = GenerateRuntimeNode(GetConnectedNode(editorNode->GetInput(0)->ID));
			m_AnimationNodes[nodeID] = CreateRef<AnimationLocalToComponentSpaceNode>(m_AnimationGraph, local);
		}
		else if (nodeFunction == AnimationNodeFunction::ComponentToLocal)
		{
			Ref<AnimationPoseNode> component = GenerateRuntimeNode(GetConnectedNode(editorNode->GetInput(0)->ID));
			m_AnimationNodes[nodeID] = CreateRef<AnimationComponentToLocalSpaceNode>(m_AnimationGraph, component);
		}
		else if (nodeFunction == AnimationNodeFunction::TwoBoneIK)
		{
			Ref<AnimationValueNode> effectorLocation = GenerateRuntimeValueNode(editorNode->GetInput(0));
			Ref<AnimationValueNode> jointTargetLocation = GenerateRuntimeValueNode(editorNode->GetInput(1));
			Ref<AnimationPoseNode> componentPose = GenerateRuntimeNode(GetConnectedNode(editorNode->GetInput(2)->ID));
			const bool allowStretching = editorNode->GetInput(4)->Data.Bool;
			const float startStretchRatio = editorNode->GetInput(5)->Data.Float;
			const float maxStretchScale = editorNode->GetInput(6)->Data.Float;
			const std::string& targetBone = editorNode->GetInput(7)->Data.String;
			m_AnimationNodes[nodeID] = CreateRef<AnimationTwoBoneIKNode>(m_AnimationGraph, componentPose, targetBone, effectorLocation, jointTargetLocation, allowStretching, startStretchRatio, maxStretchScale);
		}
		else if (nodeFunction == AnimationNodeFunction::FABRIK)
		{
			Ref<AnimationValueNode> effectorTransform = GenerateRuntimeValueNode(editorNode->GetInput(0));
			Ref<AnimationPoseNode> componentPose = GenerateRuntimeNode(GetConnectedNode(editorNode->GetInput(1)->ID));
			const float precision = editorNode->GetInput(3)->Data.Float;
			const int maxIterations = editorNode->GetInput(4)->Data.Int;
			const AnimationFABRIKNode::RotationSource rotationSource = (AnimationFABRIKNode::RotationSource)editorNode->GetInput(5)->Data.Int;
			const std::string& rootBone = editorNode->GetInput(6)->Data.String;
			const std::string& tipBone = editorNode->GetInput(7)->Data.String;
			m_AnimationNodes[nodeID] = CreateRef<AnimationFABRIKNode>(m_AnimationGraph, componentPose, rootBone, tipBone, effectorTransform, rotationSource, precision, maxIterations);
		}
		else if (nodeFunction == AnimationNodeFunction::TransformBone)
		{
			const Ref<AnimationPoseNode> basePose = GenerateRuntimeNode(GetConnectedNode(editorNode->GetInput(0)->ID));
			const std::string& targetBone = editorNode->GetInput(1)->Data.String;
			const Ref<AnimationValueNode> translation = GenerateRuntimeValueNode(editorNode->GetInput(2));
			const AnimationTransformBoneNode::TransformMode translationMode = (AnimationTransformBoneNode::TransformMode)editorNode->GetInput(3)->Data.Int;
			const Ref<AnimationValueNode> rotation = GenerateRuntimeValueNode(editorNode->GetInput(4));
			const AnimationTransformBoneNode::TransformMode rotationMode = (AnimationTransformBoneNode::TransformMode)editorNode->GetInput(5)->Data.Int;
			const Ref<AnimationValueNode> scale = GenerateRuntimeValueNode(editorNode->GetInput(6));
			const AnimationTransformBoneNode::TransformMode scaleMode = (AnimationTransformBoneNode::TransformMode)editorNode->GetInput(7)->Data.Int;
			const Ref<AnimationValueNode> alpha = GenerateRuntimeValueNode(editorNode->GetInput(8));
			m_AnimationNodes[nodeID] = CreateRef<AnimationTransformBoneNode>(m_AnimationGraph, basePose, targetBone, translation, rotation, scale, translationMode, rotationMode, scaleMode, alpha);
		}
		else if (nodeFunction == AnimationNodeFunction::Blendspace)
		{
			std::vector<AnimationBlendSpaceNode::BlendSpacePoint> points;
			const glm::vec2 scale = glm::vec2(editorNode->GetInput(2)->Data.Float, editorNode->GetInput(3)->Data.Float);

			GenerateBlendSpaceGraph(editorNode->TargetLayer, [&points, &scale](const Ref<AnimationEditorNode> editorNode, const Ref<AnimationPoseNode> node)
			{
				// Note: We flip the Y coordinate and divide by the grid scale
				points.emplace_back(glm::vec2(editorNode->Position.x, -editorNode->Position.y) * scale / 32.0f, node);
			});

			const Ref<AnimationValueNode> x = GenerateRuntimeValueNode(editorNode->GetInput(0));
			const Ref<AnimationValueNode> y = GenerateRuntimeValueNode(editorNode->GetInput(1));
			m_AnimationNodes[nodeID] = CreateRef<AnimationBlendSpaceNode>(m_AnimationGraph, points, x, y);
		}
		else if (nodeFunction == AnimationNodeFunction::Blendspace1D)
		{
			std::vector<AnimationBlendSpace1DNode::BlendSpacePoint> points;
			const float scale = editorNode->GetInput(1)->Data.Float;

			GenerateBlendSpaceGraph(editorNode->TargetLayer, [&points, &scale](const Ref<AnimationEditorNode> editorNode, const Ref<AnimationPoseNode> node)
			{
				points.emplace_back(editorNode->Position.x * scale / 32.0f, node);
			});

			const Ref<AnimationValueNode> x = GenerateRuntimeValueNode(editorNode->GetInput(0));
			m_AnimationNodes[nodeID] = CreateRef<AnimationBlendSpace1DNode>(m_AnimationGraph, points, x);
		}
		else if (nodeFunction == AnimationNodeFunction::StateMachine)
		{
			const LayerHandle layer = editorNode->TargetLayer;

			// Find entry point to state machine
			const Ref<AnimationEditorNode> entryNode = FindNodeOnLayer(layer, AnimationNodeFunction::Entry);

			if (!entryNode)
				m_CompilerResult.Add("Failed to find entry point node for state machine!", CompilerMessageType::Error, editorNode->ID);

			const NodeHandle entryHandle = GetConnectedNode(entryNode->Outputs[0]->ID);

			if (!entryHandle)
				m_AnimationNodes[nodeID] = GetDefaultNode();
			else
			{
				std::stack<NodeHandle> nodesToSearch;
				std::unordered_set<NodeHandle> includedNodes;
				std::unordered_map<NodeHandle, AnimationStateMachineNode::StateHandle> nodeStateHandle;
				std::vector<AnimationStateMachineNode::State> states;

				nodesToSearch.push(entryHandle);
				includedNodes.insert(entryHandle);

				nodeStateHandle[entryHandle] = 0;
				states.emplace_back();

				while (!nodesToSearch.empty())
				{
					const NodeHandle nodeID = nodesToSearch.top();
					nodesToSearch.pop();

					// Find output node on state's target layer
					const Ref<AnimationEditorNode> stateNode = m_EditorGraph->FindNode(nodeID);
					const Ref<AnimationEditorNode> outputNode = FindNodeOnLayer(stateNode->TargetLayer, AnimationNodeFunction::Output);

					// Add the internal graph of this state to the animation stack
					const size_t stateIndex = nodeStateHandle.at(nodeID);
					states[stateIndex].Output = GenerateRuntimeNode(GetConnectedNode(outputNode->Inputs[0]->ID));

					// Explore surrounding links and generate transitions
					for (const auto& [linkID, link] : m_EditorGraph->Links)
					{
						if (link.StartPinID != stateNode->Outputs[0]->ID)
							continue;

						const Ref<AnimationEditorNode> transitionNode = FindNodeOnLayer(link.TargetLayer, AnimationNodeFunction::Transition);

						// Add transition
						const NodeHandle otherNodeID = m_EditorGraph->FindPin(link.EndPinID)->Node;

						// Mark connected state node to be searched (if we haven't already)
						if (includedNodes.find(otherNodeID) == includedNodes.end())
						{
							includedNodes.insert(otherNodeID);
							nodesToSearch.push(otherNodeID);
							nodeStateHandle[otherNodeID] = states.size();
							states.emplace_back();
						}

						// Create the transition represented by the given link
						auto& transition = states[stateIndex].Transitions.emplace_back();
						transition.Target = nodeStateHandle.at(otherNodeID);
						transition.Transition = GenerateRuntimeValueNode(As<AnimationPin>(transitionNode->Inputs[0]));
						transition.Duration = GenerateRuntimeValueNode(As<AnimationPin>(transitionNode->Inputs[1]));
					}
				}

				const uint32_t maxTransitionsPerFrame = editorNode->GetInput(0)->Data.Int;
				const bool skipFirstUpdateTransition = editorNode->GetInput(1)->Data.Bool;
				m_AnimationNodes[nodeID] = CreateRef<AnimationStateMachineNode>(m_AnimationGraph, states, maxTransitionsPerFrame, skipFirstUpdateTransition);
			}
		}
		else
		{
			// Node function does not exist (or the compiler does not know how to deal with it). To avoid a runtime crash we insert a default node here.
			DY_CORE_WARN("Node function for node '{}' does not exist", editorNode->ID);
			m_AnimationNodes[nodeID] = GetDefaultNode();
		}

		// Mark the node as no longer being processed
		m_ProcessingNodes.erase(nodeID);

		return As<AnimationPoseNode>(m_AnimationNodes.at(nodeID));
	}

	Ref<AnimationValueNode> AnimationGraphCompiler::GenerateRuntimeValueNode(Ref<AnimationPin> pin)
	{
		const bool linked = m_EditorGraph->IsPinLinked(pin->ID);

		if (!linked)
			return CreateRef<AnimationConstantNode>(m_AnimationGraph, Utils::EditorToRuntimeGraphData(pin->Type, pin->Data));

		Ref<AnimationEditorNode> node = m_EditorGraph->FindNode(GetConnectedNode(pin->ID));

		if (node->Function == AnimationNodeFunction::Parameter)
		{
			Ref<AnimationPin> defaultPin = node->GetInput(0);
			m_AnimationGraph->m_DefaultParameters.Parameters[node->Name] = Utils::EditorToRuntimeGraphData(defaultPin->Type, defaultPin->Data);
			return CreateRef<AnimationParameterNode>(m_AnimationGraph, node->Name);
		}
		else if (Utils::GetOperatorType(node->Function) != AnimationOperatorNode::OperatorType::None)
		{
			const Ref<AnimationValueNode> a = GenerateRuntimeValueNode(node->GetInput(0));
			const Ref<AnimationValueNode> b = GenerateRuntimeValueNode(node->GetInput(1));
			return CreateRef<AnimationOperatorNode>(m_AnimationGraph, Utils::GetOperatorType(node->Function), a, b);
		}
		else if (node->Function == AnimationNodeFunction::NOT)
		{
			const Ref<AnimationValueNode> a = GenerateRuntimeValueNode(node->GetInput(0));
			return CreateRef<AnimationBooleanNotNode>(m_AnimationGraph, a);
		}
		else if (node->Function == AnimationNodeFunction::TimeRemainingRatio)
		{
			const NodeHandle targetID = node->GetInput(0)->Data.Handle;

			if (!m_EditorGraph->DoesNodeExist(targetID) || m_EditorGraph->FindNode(targetID)->Function != AnimationNodeFunction::Player)
				m_CompilerResult.Add(fmt::format("Referenced node {} does not exist on any graph layer!", targetID), CompilerMessageType::Error, node->ID);

			return CreateRef<AnimationTimeRemainingRatioNode>(m_AnimationGraph, As<AnimationPlayerNode>(GenerateRuntimeNode(targetID)));
		}

		// Unreachable (In theory... time will tell how robust that theory is. Regardless here's a cheeky assert and default return)
		DY_CORE_WARN("Node function for node '{}' does not exist", node->ID);
		DY_CORE_ASSERT(false);
		return CreateRef<AnimationConstantNode>(m_AnimationGraph, Utils::EditorToRuntimeGraphData(pin->Type, pin->Data));
	}

	PinHandle AnimationGraphCompiler::GetConnectedPin(PinHandle pinID)
	{
		const auto& links = m_EditorGraph->Links;
		for (const auto& [linkID, link] : links)
		{
			if (link.StartPinID == pinID)
				return link.EndPinID;

			if (link.EndPinID == pinID)
				return link.StartPinID;
		}

		return 0;
	}

	NodeHandle AnimationGraphCompiler::GetConnectedNode(PinHandle pinID)
	{
		const PinHandle otherPinID = GetConnectedPin(pinID);

		if (otherPinID == 0)
			return 0;

		Ref<AnimationPin> otherPin = m_EditorGraph->FindPin(otherPinID);

		if (!otherPin)
			return 0;

		return otherPin->Node;
	}

	Ref<AnimationPoseNode> AnimationGraphCompiler::GetDefaultNode()
	{
		if (!m_DefaultNode)
			m_DefaultNode = CreateRef<AnimationDefaultNode>(m_AnimationGraph);

		return m_DefaultNode;
	}

	bool AnimationGraphCompiler::DoesSkeletonContainBone(const std::string& boneName)
	{
		const auto& boneInfoMap = m_AnimationGraph->GetSkeleton()->GetBoneInfoMap();
		return boneInfoMap.find(boneName) != boneInfoMap.end();
	}

	Ref<AnimationEditorNode> AnimationGraphCompiler::FindNodeOnLayer(const LayerHandle layer, const AnimationNodeFunction function)
	{
		for (const auto& [nodeId, node] : m_EditorGraph->Nodes)
		{
			Ref<AnimationEditorNode> animationNode = As<AnimationEditorNode>(node);
			if (animationNode->Layer == layer && animationNode->Function == function)
				return animationNode;
		}

		return nullptr;
	}

	void AnimationGraphCompiler::GenerateBlendSpaceGraph(const LayerHandle layer, const std::function<void(Ref<AnimationEditorNode>, Ref<AnimationPoseNode>)>& pointCallback)
	{
		for (const auto& [nodeID, internalNode] : m_EditorGraph->Nodes)
		{
			if (internalNode->Layer != layer)
				continue;

			if (internalNode->Type != NodeType::Point)
				continue;

			const Ref<AnimationEditorNode> node = As<AnimationEditorNode>(internalNode);

			if (node->Function == AnimationNodeFunction::BlendspaceAnimation)
			{
				const AssetHandle animationHandle = node->GetInput(0)->Data.Handle;
				const Ref<Animation> animation = AssetManager::GetAsset<Animation>(animationHandle);
				pointCallback(node, CreateRef<AnimationPlayerNode>(m_AnimationGraph, animation, nullptr));
			}
			else if (node->Function == AnimationNodeFunction::BlendspaceGraph)
			{
				const Ref<AnimationEditorNode> outputNode = FindNodeOnLayer(node->TargetLayer, AnimationNodeFunction::Output);
				const NodeHandle previousNodeID = GetConnectedNode(outputNode->Inputs[0]->ID);
				pointCallback(node, GenerateRuntimeNode(previousNodeID));
			}
		}
	}

}