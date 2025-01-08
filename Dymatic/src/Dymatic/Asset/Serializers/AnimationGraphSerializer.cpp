#include "dypch.h"
#include "Dymatic/Asset/Serializers/AnimationGraphSerializer.h"

#include "Dymatic/Animation/Editor/AnimationGraphCompiler.h"

#include "Dymatic/Asset/Serializers/Utils/EditorGraphSerializer.h"
#include "Dymatic/Asset/Serializers/Utils/SerializerUtils.h"

#include "Dymatic/Project/Project.h"

namespace Dymatic {


	namespace Utils {

		static const char* AnimationPinTypeToString(const Editor::AnimationPinType type)
		{
			switch (type)
			{
			case Editor::AnimationPinType::Pose:		return "Pose";
			case Editor::AnimationPinType::Bone:		return "Bone";
			case Editor::AnimationPinType::Bool:		return "Bool";
			case Editor::AnimationPinType::Int:			return "Int";
			case Editor::AnimationPinType::Float:		return "Float";
			case Editor::AnimationPinType::Vector2:		return "Vector2";
			case Editor::AnimationPinType::Vector3:		return "Vector3";
			case Editor::AnimationPinType::Vector4:		return "Vector4";
			case Editor::AnimationPinType::Transform:	return "Transform";
			case Editor::AnimationPinType::Animation:	return "Animation";
			case Editor::AnimationPinType::Player:		return "Player";
			}

			DY_CORE_ASSERT(false, "Unknown animation pin data type");
			return nullptr;
		}

		static Editor::AnimationPinType AnimationPinTypeFromString(const std::string& string)
		{
			if (string == "Pose")		return Editor::AnimationPinType::Pose;
			if (string == "Bone")		return Editor::AnimationPinType::Bone;
			if (string == "Bool")		return Editor::AnimationPinType::Bool;
			if (string == "Int")		return Editor::AnimationPinType::Int;
			if (string == "Float")		return Editor::AnimationPinType::Float;
			if (string == "Vector2")	return Editor::AnimationPinType::Vector2;
			if (string == "Vector3")	return Editor::AnimationPinType::Vector3;
			if (string == "Vector4")	return Editor::AnimationPinType::Vector4;
			if (string == "Transform")	return Editor::AnimationPinType::Transform;
			if (string == "Animation")	return Editor::AnimationPinType::Animation;
			if (string == "Player")		return Editor::AnimationPinType::Player;

			DY_CORE_ASSERT(false, "Unknown animation pin data type string");
			return Editor::AnimationPinType::None;
		}

		static void SerializeAnimationPin(YAML::Emitter& out, const Ref<Editor::EditorPin> internalPin, const bool includeData)
		{
			const Ref<Editor::AnimationPin> pin = As<Editor::AnimationPin>(internalPin);

			out << YAML::BeginMap;

			out << YAML::Key << "Pin" << YAML::Value << pin->ID;

			if (!pin->Name.empty())
				out << YAML::Key << "Name" << YAML::Value << pin->Name;

			out << YAML::Key << "Type" << YAML::Value << AnimationPinTypeToString(pin->Type);

			if (includeData && pin->Type != Editor::AnimationPinType::Pose)
			{
				out << YAML::Key << "Data" << YAML::Value;

				switch (pin->Type)
				{
				case Editor::AnimationPinType::Bone:				out << pin->Data.String;	break;
				case Editor::AnimationPinType::Bool:				out << pin->Data.Bool;		break;
				case Editor::AnimationPinType::Int:					out << pin->Data.Int;		break;
				case Editor::AnimationPinType::Float:				out << pin->Data.Float;		break;
				case Editor::AnimationPinType::Vector2:				out << pin->Data.Vector2;	break;
				case Editor::AnimationPinType::Vector3:				out << pin->Data.Vector3;	break;
				case Editor::AnimationPinType::Vector4:				out << pin->Data.Vector4;	break;
				case Editor::AnimationPinType::Transform:			out << pin->Data.Transform;	break;
				case Editor::AnimationPinType::Animation:			out << pin->Data.Handle;	break;
				case Editor::AnimationPinType::Player:				out << pin->Data.Handle;	break;
				}
			}

			out << YAML::EndMap;
		}

		static Editor::AnimationPinData GetAnimationPinData(const Editor::AnimationPinType type, const YAML::Node& node)
		{
			Editor::AnimationPinData data;

			if (!node)
				return data;

			switch (type)
			{
			case Editor::AnimationPinType::Bone:		data.String = node.as<std::string>();			break;
			case Editor::AnimationPinType::Bool:		data.Bool = node.as<bool>();					break;
			case Editor::AnimationPinType::Int:			data.Int = node.as<int>();						break;
			case Editor::AnimationPinType::Float:		data.Float = node.as<float>();					break;
			case Editor::AnimationPinType::Vector2:		data.Vector2 = node.as<glm::vec2>();			break;
			case Editor::AnimationPinType::Vector3:		data.Vector3 = node.as<glm::vec3>();			break;
			case Editor::AnimationPinType::Vector4:		data.Vector4 = node.as<glm::vec4>();			break;
			case Editor::AnimationPinType::Transform:	data.Transform = node.as<Transform>();			break;
			case Editor::AnimationPinType::Animation:	data.Handle = node.as<AssetHandle>();			break;
			case Editor::AnimationPinType::Player:		data.Handle = node.as<Editor::NodeHandle>();	break;
			}

			return data;
		}

		static void DeserializeAnimationPinData(Ref<Editor::EditorNode> node, const Ref<Editor::EditorGraph> editorGraphInternal, const YAML::Node& pinListNode, std::vector<Ref<Editor::EditorPin>>& pins)
		{
			const Editor::AnimationNodeFunction function = As<Editor::AnimationEditorNode>(node)->Function;
			const Ref<Editor::AnimationEditorGraph> editorGraph = As<Editor::AnimationEditorGraph>(editorGraphInternal);

			// Match up pin data where it still aligns and defaults otherwise (in case the engine has compiler/node changes)
			size_t pinIndex = 0;
			for (const auto pinNode : pinListNode)
			{
				const Editor::PinHandle pinID = pinNode["Pin"].as<Editor::PinHandle>();

				const auto nameNode = pinNode["Name"];
				const std::string name = nameNode ? nameNode.as<std::string>() : std::string();

				// Extract type and data
				const Editor::AnimationPinType type = Utils::AnimationPinTypeFromString(pinNode["Type"].as<std::string>());
				const auto dataNode = pinNode["Data"];

				if (function == Editor::AnimationNodeFunction::LayeredBoneBlend)
				{
					const Ref<Editor::AnimationPin> pin = pinIndex >= pins.size() ? editorGraph->AddPin(pins, name, type) : As<Editor::AnimationPin>(pins[pinIndex]);
					pin->Name = name;
					pin->ID = pinID;
					pin->Type = type;
					pin->Data = Utils::GetAnimationPinData(type, dataNode);

					pinIndex++;
					continue;
				}

				// Parameters will just accept the pin type (as this can vary)
				if (function == Editor::AnimationNodeFunction::Parameter)
				{
					const Ref<Editor::AnimationPin> pin = As<Editor::AnimationPin>(pins[pinIndex]);
					pin->ID = pinID;
					pin->Type = type;
					pin->Data = Utils::GetAnimationPinData(type, dataNode);
					continue;
				}

				if (name.empty())
				{
					// If no name is provided use index to match
					const Ref<Editor::AnimationPin> pin = As<Editor::AnimationPin>(pins[pinIndex]);
					if (pin->Name.empty() && pin->Type == type)
					{
						pin->ID = pinID;
						pin->Data = Utils::GetAnimationPinData(type, dataNode);
					}
				}
				else
				{
					// If a name is provided match using the name
					for (const auto& internalPin : pins)
					{
						const Ref<Editor::AnimationPin> pin = As<Editor::AnimationPin>(internalPin);
						if (pin->Name == name && pin->Type == type)
						{
							pin->ID = pinID;
							pin->Data = Utils::GetAnimationPinData(type, dataNode);
							break;
						}
					}
				}

				pinIndex++;
			}
		}
	}

	void AnimationGraphSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<AnimationGraph> animationGraph = As<AnimationGraph>(asset);

		YAML::Emitter out;
		out << YAML::BeginMap << YAML::Key << "Animation Graph" << YAML::Value << YAML::BeginMap;

		const Ref<Skeleton> skeleton = animationGraph->GetSkeleton();
		out << YAML::Key << "Skeleton" << YAML::Value << (skeleton ? skeleton->Handle : 0);

		if (const Ref<Editor::AnimationEditorGraph> editorGraph = animationGraph->m_EditorGraph)
		{
			editorGraph->RemoveUnusedLayers();
			Utils::SerializeEditorGraph(out, editorGraph, &Utils::SerializeAnimationPin, [](YAML::Emitter& out, Ref<Editor::EditorNode> internalNode)
			{
				if (As<Editor::AnimationEditorNode>(internalNode)->Function != Editor::AnimationNodeFunction::LayeredBoneBlend)
					return;

				Ref<Editor::AnimationEditorLayeredBoneBlendNode> layeredBoneBlendNode = As<Editor::AnimationEditorLayeredBoneBlendNode>(internalNode);

				out << YAML::Key << "Blend Pose Filters" << YAML::Value;
				out << YAML::BeginSeq; // Blend Pose Filters
				for (const auto& filters : layeredBoneBlendNode->BlendPoseFilters)
				{
					out << YAML::BeginSeq; // Filters

					for (const auto& filter : filters)
					{
						out << YAML::BeginMap; // Filter
						out << YAML::Key << "Bone Name" << YAML::Value << filter.BoneName;
						out << YAML::Key << "Blend Depth" << YAML::Value << filter.BlendDepth;
						out << YAML::EndMap;
					}

					out << YAML::EndSeq; // Filters
				}
				out << YAML::EndSeq; // Blend Pose Filters
			});
		}

		out << YAML::EndMap; // Animation Graph
		out << YAML::EndMap;

		std::ofstream fout(Project::GetAssetFileSystemPath(metadata.FilePath));
		fout << out.c_str();

		return;
	}

	bool AnimationGraphSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset, bool multithreaded) const
	{
		YAML::Node data;
		if (!Utils::TryLoadYAMLFromFile(metadata, data))
			return false;

		auto animationGraphNode = data["Animation Graph"];
		if (!animationGraphNode)
			return false;

		Ref<Editor::AnimationEditorGraph> editorGraph = CreateRef<Editor::AnimationEditorGraph>();

		// Setup the editor graph
		if (auto graphNode = animationGraphNode["Graph"])
		{
			Utils::DeserializeEditorGraph(graphNode, editorGraph, &Utils::DeserializeAnimationPinData, [](const YAML::Node& nodeNode, Ref<Editor::EditorNode> internalNode)
			{
				if (As<Editor::AnimationEditorNode>(internalNode)->Function != Editor::AnimationNodeFunction::LayeredBoneBlend)
					return;

				const Ref<Editor::AnimationEditorLayeredBoneBlendNode> layeredBoneBlendNode = As<Editor::AnimationEditorLayeredBoneBlendNode>(internalNode);

				if (auto blendPoseFiltersNode = nodeNode["Blend Pose Filters"])
				{
					for (const auto& filtersNode : blendPoseFiltersNode)
					{
						auto& filters = layeredBoneBlendNode->BlendPoseFilters.emplace_back();

						for (const auto& filterNode : filtersNode)
						{
							auto& filter = filters.emplace_back();
							filter.BoneName = filterNode["Bone Name"].as<std::string>();
							filter.BlendDepth = filterNode["Blend Depth"].as<int32_t>();
						}
					}
				}

				// Resize to match pins
				const uint32_t pinBlendCount = (layeredBoneBlendNode->Inputs.size() - 1) / 2;
				if (layeredBoneBlendNode->BlendPoseFilters.size() != pinBlendCount)
					layeredBoneBlendNode->BlendPoseFilters.resize(pinBlendCount);
			});
		}
		else
		{
			editorGraph->SpawnOutputNode();
		}

		// Compile the editor graph
		Editor::AnimationGraphCompiler compiler(editorGraph);
		compiler.TargetHandle = metadata.Handle;

		if (auto skeletonNode = animationGraphNode["Skeleton"])
			compiler.TargetSkeleton = skeletonNode.as<AssetHandle>();

		compiler.Compile();
		Ref<AnimationGraph> animationGraph = compiler.GetAnimationGraph();

		if (!animationGraph)
		{
			// Create an empty animation graph if compilation failed
			const Ref<Skeleton> skeleton = AssetManager::GetAsset<Skeleton>(compiler.TargetSkeleton);
			animationGraph = AnimationGraph::Create(skeleton, editorGraph);
		}

		asset = animationGraph;
		return true;
	}

	namespace Utils {

		static void SerializeAnimationGraphData(FileStreamWriter& stream, const AnimationGraphData& data)
		{
			stream.WriteRaw<uint8_t>((uint8_t)data.Type);

			switch (data.Type)
			{
			case AnimationGraphDataType::Bool:		stream.WriteRaw<bool>(data.Bool);				break;
			case AnimationGraphDataType::Int:		stream.WriteRaw<int>(data.Int);					break;
			case AnimationGraphDataType::Float:		stream.WriteRaw<float>(data.Float);				break;
			case AnimationGraphDataType::Vector2:	stream.WriteRaw<glm::vec2>(data.Vector2);		break;
			case AnimationGraphDataType::Vector3:	stream.WriteRaw<glm::vec3>(data.Vector3);		break;
			case AnimationGraphDataType::Vector4:	stream.WriteRaw<glm::vec4>(data.Vector4);		break;
			case AnimationGraphDataType::Transform:	stream.WriteRaw<Transform>(data.Transform);		break;
			case AnimationGraphDataType::Handle:	stream.WriteRaw<AssetHandle>(data.Handle);		break;
			}
		}

		static void DeserializeAnimationGraphData(FileStreamReader& stream, AnimationGraphData& data)
		{
			const AnimationGraphDataType type = (AnimationGraphDataType)stream.ReadRaw<uint8_t>();
			data.Type = type;

			switch (type)
			{
			case AnimationGraphDataType::Bool:		stream.ReadRaw<bool>(data.Bool);			break;
			case AnimationGraphDataType::Int:		stream.ReadRaw<int>(data.Int);				break;
			case AnimationGraphDataType::Float:		stream.ReadRaw<float>(data.Float);			break;
			case AnimationGraphDataType::Vector2:	stream.ReadRaw<glm::vec2>(data.Vector2);	break;
			case AnimationGraphDataType::Vector3:	stream.ReadRaw<glm::vec3>(data.Vector3);	break;
			case AnimationGraphDataType::Vector4:	stream.ReadRaw<glm::vec4>(data.Vector4);	break;
			case AnimationGraphDataType::Transform:	stream.ReadRaw<Transform>(data.Transform);	break;
			case AnimationGraphDataType::Handle:	stream.ReadRaw<AssetHandle>(data.Handle);	break;
			}
		}

		static void SerializeAnimationNodeHierarchy(FileStreamWriter& stream, Ref<AnimationNode> node, std::unordered_map<AnimationNode*, uint64_t>& nodeIDMap)
		{
			const bool exists = nodeIDMap.find(node.get()) != nodeIDMap.end();

			if (!exists)
				nodeIDMap[node.get()] = UUID();

			stream.WriteRaw<uint64_t>(nodeIDMap.at(node.get()));

			if (exists)
				return;

			const AnimationNodeType type = node->GetType();
			stream.WriteRaw<uint8_t>((uint8_t)type);

			// Pose Nodes
			if (type == AnimationNodeType::Player)
			{
				const Ref<AnimationPlayerNode> playerNode = As<AnimationPlayerNode>(node);
				stream.WriteRaw<AssetHandle>(playerNode->AnimationAsset->Handle);

				const bool hasPlayRate = (bool)playerNode->PlayRate;
				stream.WriteRaw<bool>(hasPlayRate);
				if (hasPlayRate)
					SerializeAnimationNodeHierarchy(stream, playerNode->PlayRate, nodeIDMap);
			}
			else if (type == AnimationNodeType::Blend)
			{
				const Ref<AnimationBlendNode> blendNode = As<AnimationBlendNode>(node);
				SerializeAnimationNodeHierarchy(stream, blendNode->BlendA, nodeIDMap);
				SerializeAnimationNodeHierarchy(stream, blendNode->BlendB, nodeIDMap);
				SerializeAnimationNodeHierarchy(stream, blendNode->Weight, nodeIDMap);
			}
			else if (type == AnimationNodeType::LayeredBoneBlend)
			{
				const Ref<AnimationLayeredBoneBlendNode> blendNode = As<AnimationLayeredBoneBlendNode>(node);
				SerializeAnimationNodeHierarchy(stream, blendNode->BasePose, nodeIDMap);

				const auto& blendPoses = blendNode->BlendPoses;
				stream.WriteRaw<uint32_t>(blendPoses.size());
				for (const auto& blendPose : blendPoses)
				{
					SerializeAnimationNodeHierarchy(stream, blendPose.AnimationPose, nodeIDMap);
					SerializeAnimationNodeHierarchy(stream, blendPose.BlendWeight, nodeIDMap);
					
					const auto& filters = blendPose.BranchFilters;
					stream.WriteRaw<uint32_t>(filters.size());
					for (const auto& filter : filters)
					{
						stream.WriteString(filter.BoneName);
						stream.WriteRaw<int32_t>(filter.BlendDepth);
					}
				}
			}
			else if (type == AnimationNodeType::Additive)
			{
				const Ref<AnimationAdditiveNode> additiveNode = As<AnimationAdditiveNode>(node);
				SerializeAnimationNodeHierarchy(stream, additiveNode->Base, nodeIDMap);
				SerializeAnimationNodeHierarchy(stream, additiveNode->Additive, nodeIDMap);
				SerializeAnimationNodeHierarchy(stream, additiveNode->Alpha, nodeIDMap);
			}
			else if (type == AnimationNodeType::BoolBlend)
			{
				const Ref<AnimationBoolBlendNode> boolBlendNode = As<AnimationBoolBlendNode>(node);
				SerializeAnimationNodeHierarchy(stream, boolBlendNode->Flag, nodeIDMap);
				SerializeAnimationNodeHierarchy(stream, boolBlendNode->TruePose, nodeIDMap);
				SerializeAnimationNodeHierarchy(stream, boolBlendNode->FalsePose, nodeIDMap);
				SerializeAnimationNodeHierarchy(stream, boolBlendNode->TrueBlendTime, nodeIDMap);
				SerializeAnimationNodeHierarchy(stream, boolBlendNode->FalseBlendTime, nodeIDMap);
				stream.WriteRaw<bool>(boolBlendNode->ResetOnBlend);
			}
			else if (type == AnimationNodeType::IntBlend)
			{
				const Ref<AnimationIntBlendNode> intBlendNode = As<AnimationIntBlendNode>(node);
				SerializeAnimationNodeHierarchy(stream, intBlendNode->Value, nodeIDMap);
				stream.WriteRaw<bool>(intBlendNode->ResetOnBlend);

				const auto& blendPoses = intBlendNode->BlendPoses;
				stream.WriteRaw<uint32_t>(blendPoses.size());
				for (const auto& blendPose : blendPoses)
				{
					SerializeAnimationNodeHierarchy(stream, blendPose.AnimationPose, nodeIDMap);
					SerializeAnimationNodeHierarchy(stream, blendPose.BlendTime, nodeIDMap);
				}
			}
			else if (type == AnimationNodeType::TwoBoneIK)
			{
				const Ref<AnimationTwoBoneIKNode> twoBoneIKNode = As<AnimationTwoBoneIKNode>(node);
				SerializeAnimationNodeHierarchy(stream, twoBoneIKNode->ComponentPose, nodeIDMap);
				SerializeAnimationNodeHierarchy(stream, twoBoneIKNode->EffectorLocation, nodeIDMap);
				SerializeAnimationNodeHierarchy(stream, twoBoneIKNode->JointTargetLocation, nodeIDMap);
				stream.WriteString(twoBoneIKNode->TargetBone);
				stream.WriteRaw<bool>(twoBoneIKNode->AllowStretching);
				stream.WriteRaw<float>(twoBoneIKNode->StartStretchRatio);
				stream.WriteRaw<float>(twoBoneIKNode->MaxStretchScale);
			}
			else if (type == AnimationNodeType::FABRIK)
			{
				const Ref<AnimationFABRIKNode> fabrikNode = As<AnimationFABRIKNode>(node);
				SerializeAnimationNodeHierarchy(stream, fabrikNode->ComponentPose, nodeIDMap);
				SerializeAnimationNodeHierarchy(stream, fabrikNode->EffectorTransform, nodeIDMap);
				stream.WriteString(fabrikNode->RootBone);
				stream.WriteString(fabrikNode->TipBone);
				stream.WriteRaw<uint8_t>((uint8_t)fabrikNode->EffectorRotationSource);
				stream.WriteRaw<float>(fabrikNode->Precision);
				stream.WriteRaw<int32_t>(fabrikNode->MaxIterations);
			}
			else if (type == AnimationNodeType::TransformBone)
			{
				const Ref<AnimationTransformBoneNode> transformBoneNode = As<AnimationTransformBoneNode>(node);
				SerializeAnimationNodeHierarchy(stream, transformBoneNode->BasePose, nodeIDMap);
				stream.WriteString(transformBoneNode->TargetBone);
				SerializeAnimationNodeHierarchy(stream, transformBoneNode->Translation, nodeIDMap);
				stream.WriteRaw<uint8_t>((uint8_t)transformBoneNode->TranslationMode);
				SerializeAnimationNodeHierarchy(stream, transformBoneNode->Rotation, nodeIDMap);
				stream.WriteRaw<uint8_t>((uint8_t)transformBoneNode->RotationMode);
				SerializeAnimationNodeHierarchy(stream, transformBoneNode->Scale, nodeIDMap);
				stream.WriteRaw<uint8_t>((uint8_t)transformBoneNode->ScaleMode);
				SerializeAnimationNodeHierarchy(stream, transformBoneNode->Alpha, nodeIDMap);
			}
			else if (type == AnimationNodeType::ComponentToLocal)
			{
				const Ref<AnimationComponentToLocalSpaceNode> componentToLocalNode = As<AnimationComponentToLocalSpaceNode>(node);
				SerializeAnimationNodeHierarchy(stream, componentToLocalNode->Component, nodeIDMap);
			}
			else if (type == AnimationNodeType::LocalToComponent)
			{
				const Ref<AnimationLocalToComponentSpaceNode> localToComponentNode = As<AnimationLocalToComponentSpaceNode>(node);
				SerializeAnimationNodeHierarchy(stream, localToComponentNode->Local, nodeIDMap);
			}
			else if (type == AnimationNodeType::Blendspace)
			{
				const Ref<AnimationBlendSpaceNode> blendSpaceNode = As<AnimationBlendSpaceNode>(node);

				SerializeAnimationNodeHierarchy(stream, blendSpaceNode->X, nodeIDMap);
				SerializeAnimationNodeHierarchy(stream, blendSpaceNode->Y, nodeIDMap);

				const auto& points = blendSpaceNode->Points;
				stream.WriteRaw<uint32_t>(points.size());
				for (const auto& point : points)
				{
					stream.WriteRaw<glm::vec2>(point.Point);
					SerializeAnimationNodeHierarchy(stream, point.Output, nodeIDMap);
				}
			}
			else if (type == AnimationNodeType::Blendspace1D)
			{
				const Ref<AnimationBlendSpace1DNode> blendSpaceNode = As<AnimationBlendSpace1DNode>(node);

				SerializeAnimationNodeHierarchy(stream, blendSpaceNode->X, nodeIDMap);

				const auto& points = blendSpaceNode->Points;
				stream.WriteRaw<uint32_t>(points.size());
				for (const auto& point : points)
				{
					stream.WriteRaw<float>(point.Point);
					SerializeAnimationNodeHierarchy(stream, point.Output, nodeIDMap);
				}
			}
			else if (type == AnimationNodeType::StateMachine)
			{
				const Ref<AnimationStateMachineNode> stateMachineNode = As<AnimationStateMachineNode>(node);

				stream.WriteRaw<uint32_t>(stateMachineNode->MaxTransitionsPerFrame);
				stream.WriteRaw<bool>(stateMachineNode->SkipFirstUpdateTransition);

				const auto& states = stateMachineNode->States;
				stream.WriteRaw<uint32_t>(states.size());
				for (const auto& state : states)
				{
					SerializeAnimationNodeHierarchy(stream, state.Output, nodeIDMap);

					stream.WriteRaw<uint32_t>(state.Transitions.size());
					for (const auto& transition : state.Transitions)
					{
						stream.WriteRaw<AnimationStateMachineNode::StateHandle>(transition.Target);
						SerializeAnimationNodeHierarchy(stream, transition.Transition, nodeIDMap);
						SerializeAnimationNodeHierarchy(stream, transition.Duration, nodeIDMap);
					}
				}
			}

			// Values Nodes
			else if (type == AnimationNodeType::Parameter)
			{
				const Ref<AnimationParameterNode> parameterNode = As<AnimationParameterNode>(node);
				stream.WriteString(parameterNode->Name);
			}
			else if (type == AnimationNodeType::Constant)
			{
				const Ref<AnimationConstantNode> constantNode = As<AnimationConstantNode>(node);
				SerializeAnimationGraphData(stream, constantNode->Constant);
			}
			else if (type == AnimationNodeType::Operator)
			{
				const Ref<AnimationOperatorNode> operatorNode = As<AnimationOperatorNode>(node);
				stream.WriteRaw<uint8_t>((uint8_t)operatorNode->Operator);
				SerializeAnimationNodeHierarchy(stream, operatorNode->A, nodeIDMap);
				SerializeAnimationNodeHierarchy(stream, operatorNode->B, nodeIDMap);
			}
			else if (type == AnimationNodeType::BooleanNot)
			{
				const Ref<AnimationBooleanNotNode> notNode = As<AnimationBooleanNotNode>(node);
				SerializeAnimationNodeHierarchy(stream, notNode->A, nodeIDMap);
			}
			else if (type == AnimationNodeType::TimeRemainingRatio)
			{
				const Ref<AnimationTimeRemainingRatioNode> timeRemainingRatioNode = As<AnimationTimeRemainingRatioNode>(node);
				SerializeAnimationNodeHierarchy(stream, timeRemainingRatioNode->Player, nodeIDMap);
			}
		}

		static Ref<AnimationNode> DeserializeAnimationNode(FileStreamReader& stream, Ref<AnimationGraph> animationGraph, std::unordered_map<uint64_t, Ref<AnimationNode>>& nodeIDMap);

		static Ref<AnimationPoseNode> DeserializeAnimationPoseNode(FileStreamReader& stream, Ref<AnimationGraph> animationGraph, std::unordered_map<uint64_t, Ref<AnimationNode>>& nodeIDMap)
		{
			return As<AnimationPoseNode>(DeserializeAnimationNode(stream, animationGraph, nodeIDMap));
		}

		static Ref<AnimationValueNode> DeserializeAnimationValueNode(FileStreamReader& stream, Ref<AnimationGraph> animationGraph, std::unordered_map<uint64_t, Ref<AnimationNode>>& nodeIDMap)
		{
			return As<AnimationValueNode>(DeserializeAnimationNode(stream, animationGraph, nodeIDMap));
		}

		static Ref<AnimationNode> DeserializeAnimationNode(FileStreamReader& stream, Ref<AnimationGraph> animationGraph, std::unordered_map<uint64_t, Ref<AnimationNode>>& nodeIDMap)
		{
			const uint64_t nodeID = stream.ReadRaw<uint64_t>();

			if (nodeIDMap.find(nodeID) != nodeIDMap.end())
				return nodeIDMap.at(nodeID);

			Ref<AnimationNode> node = nullptr;

			const AnimationNodeType type = (AnimationNodeType)stream.ReadRaw<uint8_t>();

			// Pose Nodes
			if (type == AnimationNodeType::Default)
				return CreateRef<AnimationDefaultNode>(animationGraph);
			else if (type == AnimationNodeType::Player)
			{
				Ref<Animation> animation = AssetManager::GetAsset<Animation>(stream.ReadRaw<AssetHandle>());
				
				const bool hasPlayRate = stream.ReadRaw<bool>();
				Ref<AnimationValueNode> playRate = hasPlayRate ? DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap) : nullptr;
				node = CreateRef<AnimationPlayerNode>(animationGraph, animation, playRate);
			}
			else if (type == AnimationNodeType::Blend)
			{
				const Ref<AnimationPoseNode> a = DeserializeAnimationPoseNode(stream, animationGraph, nodeIDMap);
				const Ref<AnimationPoseNode> b = DeserializeAnimationPoseNode(stream, animationGraph, nodeIDMap);
				const Ref<AnimationValueNode> weight = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);
				node = CreateRef<AnimationBlendNode>(animationGraph, a, b, weight);
			}
			else if (type == AnimationNodeType::LayeredBoneBlend)
			{
				const Ref<AnimationPoseNode> basePose = DeserializeAnimationPoseNode(stream, animationGraph, nodeIDMap);

				std::vector<AnimationLayeredBoneBlendNode::BlendPose> blendPoses;
				const uint32_t blendPoseCount = stream.ReadRaw<uint32_t>();
				blendPoses.reserve(blendPoseCount);
				for (uint32_t blendPoseIndex = 0; blendPoseIndex < blendPoseCount; blendPoseIndex++)
				{
					auto& blendPose = blendPoses.emplace_back();
					blendPose.AnimationPose = DeserializeAnimationPoseNode(stream, animationGraph, nodeIDMap);
					blendPose.BlendWeight = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);

					const uint32_t filterCount = stream.ReadRaw<uint32_t>();
					blendPose.BranchFilters.reserve(filterCount);
					for (uint32_t filterIndex = 0; filterIndex < filterCount; filterIndex++)
					{
						auto& filter = blendPose.BranchFilters.emplace_back();
						stream.ReadString(filter.BoneName);
						stream.ReadRaw<int32_t>(filter.BlendDepth);
					}
				}

				node = CreateRef<AnimationLayeredBoneBlendNode>(animationGraph, basePose, blendPoses);
			}
			else if (type == AnimationNodeType::Additive)
			{
				const Ref<AnimationPoseNode> base = DeserializeAnimationPoseNode(stream, animationGraph, nodeIDMap);
				const Ref<AnimationPoseNode> additive = DeserializeAnimationPoseNode(stream, animationGraph, nodeIDMap);
				const Ref<AnimationValueNode> alpha = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);
				node = CreateRef<AnimationAdditiveNode>(animationGraph, base, additive, alpha);
			}
			else if (type == AnimationNodeType::BoolBlend)
			{
				const Ref<AnimationValueNode> flag = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);
				const Ref<AnimationPoseNode> truePose = DeserializeAnimationPoseNode(stream, animationGraph, nodeIDMap);
				const Ref<AnimationPoseNode> falsePose = DeserializeAnimationPoseNode(stream, animationGraph, nodeIDMap);
				const Ref<AnimationValueNode> trueBlendTime = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);
				const Ref<AnimationValueNode> falseBlendTime = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);
				const bool resetOnBlend = stream.ReadRaw<bool>();
				node = CreateRef<AnimationBoolBlendNode>(animationGraph, flag, truePose, falsePose, trueBlendTime, falseBlendTime, resetOnBlend);
			}
			else if (type == AnimationNodeType::IntBlend)
			{
				const Ref<AnimationValueNode> value = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);
				const bool resetOnBlend = stream.ReadRaw<bool>();

				const uint32_t blendPoseCount = stream.ReadRaw<uint32_t>();
				std::vector<AnimationIntBlendNode::BlendPose> blendPoses;
				blendPoses.reserve(blendPoseCount);
				for (uint32_t blendPoseIndex = 0; blendPoseIndex < blendPoseCount; blendPoseIndex++)
				{
					auto& blendPose = blendPoses.emplace_back();
					blendPose.AnimationPose = DeserializeAnimationPoseNode(stream, animationGraph, nodeIDMap);
					blendPose.BlendTime = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);
				}

				node = CreateRef<AnimationIntBlendNode>(animationGraph, value, blendPoses, resetOnBlend);
			}
			else if (type == AnimationNodeType::TwoBoneIK)
			{
				const Ref<AnimationPoseNode> componentPose = DeserializeAnimationPoseNode(stream, animationGraph, nodeIDMap);
				const Ref<AnimationValueNode> effectorLocation = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);
				const Ref<AnimationValueNode> jointTargetLocation = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);

				std::string targetBone;
				stream.ReadString(targetBone);
				const bool allowStretching = stream.ReadRaw<bool>();
				const bool startStretchRatio = stream.ReadRaw<float>();
				const bool maxStretchScale = stream.ReadRaw<float>();

				node = CreateRef<AnimationTwoBoneIKNode>(animationGraph, componentPose, targetBone, effectorLocation, jointTargetLocation, allowStretching, startStretchRatio, maxStretchScale);
			}
			else if (type == AnimationNodeType::FABRIK)
			{
				const Ref<AnimationPoseNode> componentPose = DeserializeAnimationPoseNode(stream, animationGraph, nodeIDMap);
				const Ref<AnimationValueNode> effectorTransform = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);

				std::string rootBone, tipBone;
				stream.ReadString(rootBone);
				stream.ReadString(tipBone);
				const AnimationFABRIKNode::RotationSource effectorRotationSource = (AnimationFABRIKNode::RotationSource)stream.ReadRaw<uint8_t>();
				const float precision = stream.ReadRaw<float>();
				const int32_t maxIterations = stream.ReadRaw<int32_t>();

				node = CreateRef<AnimationFABRIKNode>(animationGraph, componentPose, rootBone, tipBone, effectorTransform, effectorRotationSource, precision, maxIterations);
			}

			else if (type == AnimationNodeType::TransformBone)
			{
				const Ref<AnimationPoseNode> basePose = DeserializeAnimationPoseNode(stream, animationGraph, nodeIDMap);

				std::string targetBone;
				stream.ReadString(targetBone);

				const Ref<AnimationValueNode> translation = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);
				const AnimationTransformBoneNode::TransformMode translationMode = (AnimationTransformBoneNode::TransformMode)stream.ReadRaw<uint8_t>();
				const Ref<AnimationValueNode> rotation = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);
				const AnimationTransformBoneNode::TransformMode rotationMode = (AnimationTransformBoneNode::TransformMode)stream.ReadRaw<uint8_t>();
				const Ref<AnimationValueNode> scale = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);
				const AnimationTransformBoneNode::TransformMode scaleMode = (AnimationTransformBoneNode::TransformMode)stream.ReadRaw<uint8_t>();
				const Ref<AnimationValueNode> alpha = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);

				node = CreateRef<AnimationTransformBoneNode>(animationGraph, basePose, targetBone, translation, rotation, scale, translationMode, rotationMode, scaleMode, alpha);
			}
			else if (type == AnimationNodeType::ComponentToLocal)
			{
				const Ref<AnimationPoseNode> component = DeserializeAnimationPoseNode(stream, animationGraph, nodeIDMap);
				node = CreateRef<AnimationComponentToLocalSpaceNode>(animationGraph, component);
			}
			else if (type == AnimationNodeType::LocalToComponent)
			{
				const Ref<AnimationPoseNode> local = DeserializeAnimationPoseNode(stream, animationGraph, nodeIDMap);
				node = CreateRef<AnimationLocalToComponentSpaceNode>(animationGraph, local);
			}
			else if (type == AnimationNodeType::Blendspace)
			{
				const Ref<AnimationValueNode> x = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);
				const Ref<AnimationValueNode> y = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);

				std::vector<AnimationBlendSpaceNode::BlendSpacePoint> points;
				const uint32_t pointCount = stream.ReadRaw<uint32_t>();
				points.reserve(pointCount);
				for (uint32_t pointIndex = 0; pointIndex < pointCount; pointIndex++)
				{
					auto& point = points.emplace_back();
					stream.ReadRaw<glm::vec2>(point.Point);
					point.Output = DeserializeAnimationPoseNode(stream, animationGraph, nodeIDMap);
				}

				node = CreateRef<AnimationBlendSpaceNode>(animationGraph, points, x, y);
			}
			else if (type == AnimationNodeType::Blendspace1D)
			{
				const Ref<AnimationValueNode> x = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);

				std::vector<AnimationBlendSpace1DNode::BlendSpacePoint> points;
				const uint32_t pointCount = stream.ReadRaw<uint32_t>();
				points.reserve(pointCount);
				for (uint32_t pointIndex = 0; pointIndex < pointCount; pointIndex++)
				{
					auto& point = points.emplace_back();
					stream.ReadRaw<float>(point.Point);
					point.Output = DeserializeAnimationPoseNode(stream, animationGraph, nodeIDMap);
				}

				node = CreateRef<AnimationBlendSpace1DNode>(animationGraph, points, x);
			}
			else if (type == AnimationNodeType::StateMachine)
			{
				const uint32_t maxTransitionsPerFrame = stream.ReadRaw<uint32_t>();
				const bool skipFirstUpdateTransition = stream.ReadRaw<bool>();

				const uint32_t stateCount = stream.ReadRaw<uint32_t>();
				std::vector<AnimationStateMachineNode::State> states;
				states.reserve(stateCount);
				for (uint32_t stateIndex = 0; stateIndex < stateCount; stateIndex++)
				{
					auto& state = states.emplace_back();
					state.Output = DeserializeAnimationPoseNode(stream, animationGraph, nodeIDMap);

					const uint32_t transitionCount = stream.ReadRaw<uint32_t>();
					state.Transitions.reserve(transitionCount);
					for (uint32_t transitionIndex = 0; transitionIndex < transitionCount; transitionIndex++)
					{
						auto& transition = state.Transitions.emplace_back();
						transition.Target = stream.ReadRaw<AnimationStateMachineNode::StateHandle>();
						transition.Transition = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);
						transition.Duration = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);
					}
				}

				node = CreateRef<AnimationStateMachineNode>(animationGraph, states, maxTransitionsPerFrame, skipFirstUpdateTransition);
			}

			// Value Nodes

			else if (type == AnimationNodeType::Constant)
			{
				AnimationGraphData constant;
				DeserializeAnimationGraphData(stream, constant);
				node = CreateRef<AnimationConstantNode>(animationGraph, constant);
			}
			else if (type == AnimationNodeType::Parameter)
			{
				std::string name;
				stream.ReadString(name);
				node = CreateRef<AnimationParameterNode>(animationGraph, name);
			}
			else if (type == AnimationNodeType::Operator)
			{
				const AnimationOperatorNode::OperatorType operatorType = (AnimationOperatorNode::OperatorType)stream.ReadRaw<uint8_t>();
				const Ref<AnimationValueNode> a = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);
				const Ref<AnimationValueNode> b = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);
				node = CreateRef<AnimationOperatorNode>(animationGraph, operatorType, a, b);
			}
			else if (type == AnimationNodeType::BooleanNot)
			{
				const Ref<AnimationValueNode> a = DeserializeAnimationValueNode(stream, animationGraph, nodeIDMap);
				node = CreateRef<AnimationBooleanNotNode>(animationGraph, a);
			}
			else if (type == AnimationNodeType::TimeRemainingRatio)
			{
				const Ref<AnimationPlayerNode> player = As<AnimationPlayerNode>(DeserializeAnimationPoseNode(stream, animationGraph, nodeIDMap));
				node = CreateRef<AnimationTimeRemainingRatioNode>(animationGraph, player);
			}
			else
			{
				// We failed to recognize the node. This will crash the runtime!
				DY_CORE_VERIFY(false);
			}

			nodeIDMap[nodeID] = node;
			return node;
		}

	}

	bool AnimationGraphSerializer::SerializeToAssetPack(const AssetMetadata& metadata, const AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		const Ref<AnimationGraph> animationGraph = AssetManager::GetAsset<AnimationGraph>(handle);

		if (!animationGraph)
		{
			DY_CORE_WARN("Animation Graph {} is not initialized! Excluding from asset pack...", handle);
			return false;
		}

		// Skeleton
		const Ref<Skeleton> skeleton = animationGraph->GetSkeleton();

		if (!skeleton)
		{
			DY_CORE_WARN("Animation Graph {} does not have a valid skeleton. Excluding from asset pack...", handle);
			return false;
		}

		stream.WriteRaw<AssetHandle>(skeleton->Handle);

		// Parameters
		const auto& defaultParameters = animationGraph->GetDefaultParameters().Parameters;
		stream.WriteRaw<uint32_t>(defaultParameters.size());
		for (const auto& [name, data] : defaultParameters)
		{
			stream.WriteString(name);
			Utils::SerializeAnimationGraphData(stream, data);
		}

		// Runtime Graph
		std::unordered_map<AnimationNode*, uint64_t> nodeIDMap;
		Utils::SerializeAnimationNodeHierarchy(stream, animationGraph->GetOutputNode(), nodeIDMap);

		return true;
	}

	bool AnimationGraphSerializer::DeserializeFromAssetPack(const AssetMetadata& metadata, Ref<Asset>& asset, FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		// Skeleton
		const Ref<Skeleton> skeleton = AssetManager::GetAsset<Skeleton>(stream.ReadRaw<AssetHandle>());

		if (!skeleton)
		{
			DY_CORE_ERROR("Failed to load skeleton for animation graph during asset pack deserialization!");
			return false;
		}

		// Create New Graph
		Ref<AnimationGraph> animationGraph = AnimationGraph::Create(skeleton, nullptr);

		// Parameters
		const uint32_t defaultParametersCount = stream.ReadRaw<uint32_t>();
		auto& defaultParameters = animationGraph->m_DefaultParameters.Parameters;
		defaultParameters.reserve(defaultParametersCount);
		for (uint32_t defaultParameterIndex = 0; defaultParameterIndex < defaultParametersCount; defaultParameterIndex++)
		{
			std::string name;
			stream.ReadString(name);

			Utils::DeserializeAnimationGraphData(stream, defaultParameters[name]);
		}

		// Runtime Graph
		std::unordered_map<uint64_t, Ref<AnimationNode>> nodeIDMap;
		const Ref<AnimationPoseNode> output = Utils::DeserializeAnimationPoseNode(stream, animationGraph, nodeIDMap);
		animationGraph->SetOutputNode(output);

		asset = animationGraph;
		return true;
	}

}