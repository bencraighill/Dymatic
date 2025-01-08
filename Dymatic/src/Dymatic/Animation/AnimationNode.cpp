#include "dypch.h"
#include "Dymatic/Animation/AnimationNode.h"

#include "Dymatic/Animation/AnimationGraph.h"
#include "Dymatic/Animation/AnimationCore.h"

#include <glm/gtx/matrix_decompose.hpp>
#include <numeric>

namespace Dymatic {

	namespace Utils {

		static void LocalToComponentSpace(const std::unordered_map<std::string, BoneInfo>& boneInfoMap, const BoneNodeData& node, const Pose& inPose, Pose& outPose, const glm::mat4& parentTransform)
		{
			if (boneInfoMap.find(node.Name) == boneInfoMap.end())
				return;

			const uint32_t index = boneInfoMap.at(node.Name).id;
			const glm::mat4 componentTransformation = parentTransform * inPose.BoneMatrices[index];

			outPose.BoneMatrices[index] = componentTransformation;

			for (const auto& child : node.Children)
				LocalToComponentSpace(boneInfoMap, child, inPose, outPose, componentTransformation);
		}

		static void ComponentToLocalSpace(const std::unordered_map<std::string, BoneInfo>& boneInfoMap, const BoneNodeData& node, const Pose& inPose, Pose& outPose, const glm::mat4& parentTransform)
		{
			if (boneInfoMap.find(node.Name) == boneInfoMap.end())
				return;

			const uint32_t index = boneInfoMap.at(node.Name).id;

			const glm::mat4 componentTransformation = inPose.BoneMatrices[index];
			const glm::mat4 localTransformation = glm::inverse(parentTransform) * componentTransformation;

			outPose.BoneMatrices[index] = localTransformation;

			for (const auto& child : node.Children)
				ComponentToLocalSpace(boneInfoMap, child, inPose, outPose, componentTransformation);
		}

		static void BlendPoses(Pose& result, const Pose& a, const Pose& b, const float weight)
		{
			// Directly copy poses for pure weights
			// NOTE: This is here as a minor optimization, AVOID calling this method if you can avoid a blend altogether
			if (weight == 0.0f)
				return result.Copy(a);

			if (weight == 1.0f)
				return result.Copy(b);

			// TODO: Consider shipping matrix work off to the GPU!
			const auto& transformationA = a.BoneMatrices;
			const auto& transformationB = b.BoneMatrices;

			for (size_t i = 0; i < transformationA.size(); i++)
			{
				glm::vec3 translationA, scaleA;
				glm::quat rotationA;
				glm::decompose(transformationA[i], scaleA, rotationA, translationA, glm::vec3(0.0f), glm::vec4(0.0f));

				glm::vec3 translationB, scaleB;
				glm::quat rotationB;
				glm::decompose(transformationB[i], scaleB, rotationB, translationB, glm::vec3(0.0f), glm::vec4(0.0f));

				auto& transform = result.BoneMatrices[i];

				transform = glm::translate(glm::mat4(1.0f), glm::mix(translationA, translationB, weight));
				transform *= glm::toMat4(glm::normalize(glm::slerp(rotationA, rotationB, weight)));
				transform = glm::scale(transform, glm::mix(scaleA, scaleB, weight));
			}
		}

		static void BlendPoses(Pose& result, const Pose& a, const Pose& b, const Pose& c, const float weightA, const float weightB, const float weightC)
		{
			const float totalWeight = weightA + weightB + weightC;
			if (totalWeight <= 0.0f)
				return;

			// Check if we are at a given point (can directly copy if so)
			if (weightB == 0.0f && weightC == 0.0f)
				return result.Copy(a);

			if (weightA == 0.0f && weightC == 0.0f)
				return result.Copy(b);

			if (weightA == 0.0f && weightB == 0.0f)
				return result.Copy(c);

			// Calculate normalized weight values
			const float normWeightA = weightA / totalWeight;
			const float normWeightB = weightB / totalWeight;
			const float normWeightC = weightC / totalWeight;

			// TODO: Consider shipping matrix work off to the GPU!
			const auto& transformationA = a.BoneMatrices;
			const auto& transformationB = b.BoneMatrices;
			const auto& transformationC = c.BoneMatrices;

			for (size_t i = 0; i < transformationA.size(); i++)
			{
				glm::vec3 translationA, scaleA, translationB, scaleB, translationC, scaleC;
				glm::quat rotationA, rotationB, rotationC;

				glm::decompose(transformationA[i], scaleA, rotationA, translationA, glm::vec3(0.0f), glm::vec4(0.0f));
				glm::decompose(transformationB[i], scaleB, rotationB, translationB, glm::vec3(0.0f), glm::vec4(0.0f));
				glm::decompose(transformationC[i], scaleC, rotationC, translationC, glm::vec3(0.0f), glm::vec4(0.0f));

				// Blend translation and scale
				const glm::vec3 blendedTranslation = translationA * normWeightA + translationB * normWeightB + translationC * normWeightC;
				const glm::vec3 blendedScale = scaleA * normWeightA + scaleB * normWeightB + scaleC * normWeightC;
				
				// Blend rotation
				glm::quat blendedRotation = glm::normalize(glm::slerp(rotationA, rotationB, normWeightB / (normWeightA + normWeightB)));
				blendedRotation = glm::normalize(glm::slerp(blendedRotation, rotationC, normWeightC));

				auto& transform = result.BoneMatrices[i];
				transform = glm::translate(glm::mat4(1.0f), blendedTranslation);
				transform *= glm::toMat4(blendedRotation);
				transform = glm::scale(transform, blendedScale);
			}
		}

		static glm::vec3 GetTransformPosition(const glm::mat4& transform)
		{
			return transform[3];
		}
	
	}

	void Pose::Copy(const Pose& source)
	{
		std::memcpy(BoneMatrices.data(), source.BoneMatrices.data(), source.BoneMatrices.size() * sizeof(glm::mat4));
	}

	AnimationNode::AnimationNode(Ref<AnimationGraph> animationGraph)
		: m_AnimationGraph(animationGraph.get())
	{
		animationGraph->m_RegisteredNodes.emplace_back(this);
	}

	AnimationPoseNode::AnimationPoseNode(Ref<AnimationGraph> animationGraph)
		: AnimationNode(animationGraph)
	{}

	const Pose& AnimationPoseNode::GetPose(const float time)
	{
		UpdatePose(time);
		return GetPoseInternal();
	}

	void AnimationPoseNode::UpdatePose(const float time)
	{
		// Check if cache is invalid and invoke the runtime node update process if so
		if (time == m_PoseTime)
			return;

		m_PoseTime = time;
		UpdatePoseInternal(time);
	}

	AnimationCachedPoseNode::AnimationCachedPoseNode(Ref<AnimationGraph> animationGraph)
		: AnimationPoseNode(animationGraph)
	{
		m_Pose.BoneMatrices = std::vector<glm::mat4>(animationGraph->GetSkeleton()->GetBoneCount());
	}

	const Pose& AnimationCachedPoseNode::GetPoseInternal() const
	{
		// Default behavior is just to use pose buffer but other nodes may override this behavior
		// as a 'pass-through' optimization where copying to a separate buffer is unnecessary (e.g. blending with weight 0.0/1.0)
		return m_Pose;
	}

	// Default (no-animation) Bind Pose
	AnimationDefaultNode::AnimationDefaultNode(Ref<AnimationGraph> animationGraph)
		: AnimationCachedPoseNode(animationGraph)
	{
		const Ref<Skeleton> skeleton = animationGraph->GetSkeleton();
		const auto& boneInfoMap = skeleton->GetBoneInfoMap();
		const auto& rootNode = skeleton->GetRootNode();

		// Apply inverse offset (bind pose) matrix
		for (const auto& [name, boneInfo] : boneInfoMap)
			m_Pose.BoneMatrices[boneInfo.id] = glm::inverse(boneInfo.offset);

		// Covert to local space
		Utils::ComponentToLocalSpace(boneInfoMap, rootNode, m_Pose, m_Pose, glm::mat4(1.0f));
	}

	void AnimationDefaultNode::UpdatePoseInternal(float time)
	{
	}

	// User Authored Animation Player
	AnimationPlayerNode::AnimationPlayerNode(Ref<AnimationGraph> animationGraph, Ref<Animation> animation, Ref<AnimationValueNode> playRate)
		: AnimationCachedPoseNode(animationGraph), AnimationAsset(animation), PlayRate(playRate)
	{}

	void AnimationPlayerNode::UpdatePoseInternal(float time)
	{
		// Convert real-time to animation space time
		auto& currentTime = m_AnimationGraph->GetInstanceValue(m_CurrentTime).Float;
		const float playRate = PlayRate ? PlayRate->GetValue().Float : 1.0f;
		currentTime = std::fmod(currentTime + m_AnimationGraph->GetInstanceDeltaTime() * AnimationAsset->GetTicksPerSecond() * playRate, AnimationAsset->GetDuration());

		// Recursively populate pose bone matrix buffer with global bone transforms
		CalculateBoneTransform(m_AnimationGraph->GetSkeleton()->GetRootNode(), currentTime);
	}

	void AnimationPlayerNode::Reset()
	{
		m_AnimationGraph->GetInstanceValue(m_CurrentTime).Float = 0.0f;
	}

	void AnimationPlayerNode::CalculateBoneTransform(const BoneNodeData& node, float time)
	{
		const std::string& nodeName = node.Name;
		Ref<Bone> Bone = AnimationAsset->FindBone(nodeName);
		const glm::mat4& localAnimationTransform = Bone ? Bone->GetLocalTransform(time) : node.Transformation;

		const auto& boneInfoMap = m_AnimationGraph->GetSkeleton()->GetBoneInfoMap();
		if (boneInfoMap.find(nodeName) != boneInfoMap.end())
		{
			const uint32_t index = boneInfoMap.at(nodeName).id;
			m_Pose.BoneMatrices[index] = localAnimationTransform;
		}

		for (const auto& child : node.Children)
			CalculateBoneTransform(child, time);
	}

	float AnimationPlayerNode::GetCurrentTime()
	{
		return m_AnimationGraph->GetInstanceValue(m_CurrentTime).Float;
	}

	float AnimationPlayerNode::GetLength()
	{
		return AnimationAsset->GetDuration();
	}

	// Animation Blend by Factor
	AnimationBlendNode::AnimationBlendNode(Ref<AnimationGraph> animationGraph, Ref<AnimationPoseNode> a, Ref<AnimationPoseNode> b, Ref<AnimationValueNode> weight)
		: AnimationCachedPoseNode(animationGraph), BlendA(a), BlendB(b), Weight(weight)
	{}

	void AnimationBlendNode::UpdatePoseInternal(float time)
	{
		const float weight = Weight->GetValue().Float;

		// Early exit for pass-through optimization
		if (weight == 0.0f)
		{
			BlendA->UpdatePose(time);
			return;
		}

		if (weight == 1.0f)
		{
			BlendB->UpdatePose(time);
			return;
		}

		Utils::BlendPoses(m_Pose, BlendA->GetPose(time), BlendB->GetPose(time), weight);
	}

	void AnimationBlendNode::Reset()
	{
		BlendA->Reset();
		BlendB->Reset();
	}

	const Pose& AnimationBlendNode::GetPoseInternal() const
	{
		const float weight = Weight->GetValue().Float;

		if (weight == 0.0f)
			return BlendA->GetPoseInternal();

		if (weight == 1.0f)
			return BlendB->GetPoseInternal();

		return m_Pose;
	}

	static void ComputeLayeredBlendWeights(const BoneNodeData& node, int32_t maxBlendDepth, int32_t currentBlendDepth, uint32_t poseIndex, AnimationLayeredBoneBlendNode::BlendWeightMultiplierMatrix& blendWeightMultipliers, const std::unordered_map<std::string, BoneInfo>& boneInfoMap)
	{
		auto& blendWeightMultiplier = blendWeightMultipliers[boneInfoMap.at(node.Name).id][poseIndex];

		// Note: Negative resets weight for index (magnitude of negative is used to determine falloff speed)
		if (currentBlendDepth >= 0)
			blendWeightMultiplier = currentBlendDepth == 0 ? 1.0f : glm::max((1.0f - ((float)currentBlendDepth / (float)maxBlendDepth)), blendWeightMultiplier);
		else
			blendWeightMultiplier *= currentBlendDepth == -1 ? 0.0f : ((float)(currentBlendDepth + 1) / (float)(maxBlendDepth + 1));

		if (currentBlendDepth > 0)
			currentBlendDepth--;
		else if (currentBlendDepth < -1)
			currentBlendDepth++;

		for (const auto& child : node.Children)
			ComputeLayeredBlendWeights(child, maxBlendDepth, currentBlendDepth, poseIndex, blendWeightMultipliers, boneInfoMap);
	}

	AnimationLayeredBoneBlendNode::AnimationLayeredBoneBlendNode(Ref<AnimationGraph> animationGraph, const Ref<AnimationPoseNode> basePose, const std::vector<BlendPose>& blendPoses)
		: AnimationCachedPoseNode(animationGraph), BasePose(basePose), BlendPoses(blendPoses)
	{
		const Ref<Skeleton> skeleton = animationGraph->GetSkeleton();
		const auto& boneInfoMap = skeleton->GetBoneInfoMap();

		// Cache bone IDs for quick lookups
		for (auto& pose : BlendPoses)
		{
			for (auto& filter : pose.BranchFilters)
				filter.BoneID = boneInfoMap.at(filter.BoneName).id;

			// Sort by boneID (index) so that filters higher in the hierarchy get applied first (so lower level hierarchy is not overwritten)
			std::sort(pose.BranchFilters.begin(), pose.BranchFilters.end(), [](const BranchFilter& a, const BranchFilter& b)
			{
				return a.BoneID < b.BoneID;
			});
		}

		// Precompute blend weight multipliers (vector ordered by boneID/index then pose index)
		m_BlendWeightMultipliers = BlendWeightMultiplierMatrix(skeleton->GetBoneCount(), std::vector<float>(BlendPoses.size(), 0.0f));

		for (uint32_t poseIndex = 0; poseIndex < BlendPoses.size(); poseIndex++)
			for (const auto& filter : BlendPoses[poseIndex].BranchFilters)
				ComputeLayeredBlendWeights(skeleton->GetBoneNodeData(filter.BoneID), filter.BlendDepth, filter.BlendDepth, poseIndex, m_BlendWeightMultipliers, boneInfoMap);
	}

	void AnimationLayeredBoneBlendNode::UpdatePoseInternal(const float time)
	{
		// Based on logic from `Utils::BlendPoses`
		const uint32_t boneCount = m_AnimationGraph->GetSkeleton()->GetBoneCount();
		const uint32_t poseCount = BlendPoses.size();

		const auto& basePose = BasePose->GetPose(time);

		for (uint32_t boneIndex = 0; boneIndex < boneCount; boneIndex++)
		{
			const auto& blendWeightMultipliers = m_BlendWeightMultipliers[boneIndex];

			float totalWeight = 0.0f;
			for (uint32_t poseIndex = 0; poseIndex < poseCount; poseIndex++)
				totalWeight += BlendPoses[poseIndex].BlendWeight->GetValue().Float * blendWeightMultipliers[poseIndex];

			if (totalWeight < 0.0f)
				return;

			const bool useBasePose = totalWeight < 1.0f;

			float accumulatedWeight;
			glm::vec3 blendedTranslation, blendedScale;
			glm::quat blendedRotation;

			if (useBasePose)
			{
				const float baseWeight = 1.0f - totalWeight;
				glm::decompose(basePose.BoneMatrices[boneIndex], blendedScale, blendedRotation, blendedTranslation, glm::vec3(0.0f), glm::vec4(0.0f));
				blendedTranslation *= baseWeight;
				blendedScale *= baseWeight;
				accumulatedWeight = baseWeight;
			}
			else
			{
				accumulatedWeight = 0.0f;
				blendedTranslation = glm::vec3(0.0f);
				blendedRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
				blendedScale = glm::vec3(0.0f);
			}

			if (totalWeight > 0.0f)
			{
				for (uint32_t poseIndex = 0; poseIndex < poseCount; poseIndex++)
				{
					// Normalize pose weight if it exceeds 1, otherwise leave untouched (as remaining weight will be filled by base pose)
					const auto& blendPose = BlendPoses[poseIndex];
					const float poseWeight = (BlendPoses[poseIndex].BlendWeight->GetValue().Float * blendWeightMultipliers[poseIndex]) / std::max(1.0f, totalWeight);

					if (poseWeight > 0.0f)
					{
						accumulatedWeight += poseWeight;

						glm::vec3 translation, scale;
						glm::quat rotation;

						glm::decompose(blendPose.AnimationPose->GetPose(time).BoneMatrices[boneIndex], scale, rotation, translation, glm::vec3(0.0f), glm::vec4(0.0f));

						blendedTranslation += translation * poseWeight;
						blendedScale += scale * poseWeight;

						blendedRotation = glm::slerp(blendedRotation, rotation, poseWeight / accumulatedWeight);
					}
				}
			}

			auto& transform = m_Pose.BoneMatrices[boneIndex];
			transform = glm::translate(glm::mat4(1.0f), blendedTranslation);
			transform *= glm::toMat4(glm::normalize(blendedRotation));
			transform = glm::scale(transform, blendedScale);
		}
	}

	void AnimationLayeredBoneBlendNode::Reset()
	{
		BasePose->Reset();

		for (const auto& pose : BlendPoses)
			pose.AnimationPose->Reset();
	}

	AnimationAdditiveNode::AnimationAdditiveNode(Ref<AnimationGraph> animationGraph, Ref<AnimationPoseNode> base, Ref<AnimationPoseNode> additive, Ref<AnimationValueNode> alpha)
		: AnimationCachedPoseNode(animationGraph), Base(base), Additive(additive), Alpha(alpha) {}

	void AnimationAdditiveNode::UpdatePoseInternal(const float time)
	{
		const auto& base = Base->GetPose(time);
		const auto& additive = Additive->GetPose(time);
		const float alpha = Alpha->GetValue().Float;

		for (size_t boneIndex = 0; boneIndex < m_Pose.BoneMatrices.size(); boneIndex++)
		{
			glm::vec3 baseTranslation, baseScale, additiveTranslation, additiveScale;
			glm::quat baseRotation, additiveRotation;

			glm::decompose(base.BoneMatrices[boneIndex], baseScale, baseRotation, baseTranslation, glm::vec3(0.0f), glm::vec4(0.0f));
			glm::decompose(additive.BoneMatrices[boneIndex], additiveScale, additiveRotation, additiveTranslation, glm::vec3(0.0f), glm::vec4(0.0f));

			auto& transform = m_Pose.BoneMatrices[boneIndex];
			transform = glm::translate(glm::mat4(1.0f), baseTranslation + additiveTranslation * alpha);
			transform *= glm::toMat4(glm::normalize(glm::slerp(baseRotation, baseRotation * additiveRotation, alpha)));
			transform = glm::scale(transform, baseScale + additiveScale * alpha);
		}
	}

	void AnimationAdditiveNode::Reset()
	{
		Base->Reset();
		Additive->Reset();
	}

	AnimationBoolBlendNode::AnimationBoolBlendNode(Ref<AnimationGraph> animationGraph, Ref<AnimationValueNode> flag, Ref<AnimationPoseNode> truePose, Ref<AnimationPoseNode> falsePose, Ref<AnimationValueNode> trueBlendTime, Ref<AnimationValueNode> falseBlendTime, const bool resetOnBlend)
		: AnimationCachedPoseNode(animationGraph), Flag(flag), TruePose(truePose), FalsePose(falsePose), TrueBlendTime(trueBlendTime), FalseBlendTime(falseBlendTime), ResetOnBlend(resetOnBlend) {}

	void AnimationBoolBlendNode::UpdatePoseInternal(const float time)
	{
		const bool currentFlag = Flag->GetValue().Bool;
		auto& previousFlag = m_AnimationGraph->GetInstanceValue(m_PreviousFlag).Bool;
		auto& totalBlendTime = m_AnimationGraph->GetInstanceValue(m_TotalBlendTime).Float;

		if (currentFlag == previousFlag)
		{
			totalBlendTime = 0.0f;
			(currentFlag ? TruePose : FalsePose)->UpdatePoseInternal(time);
			return;
		}

		auto& currentBlendTime = m_AnimationGraph->GetInstanceValue(m_CurrentBlendTime).Float;
		
		if (totalBlendTime == 0.0f)
		{
			currentBlendTime = 0.0f;
			totalBlendTime = (currentFlag ? TrueBlendTime : FalseBlendTime)->GetValue().Float;

			if (ResetOnBlend)
				(currentFlag ? TruePose : FalsePose)->Reset();
		}

		currentBlendTime += m_AnimationGraph->GetInstanceDeltaTime();

		if (currentBlendTime >= totalBlendTime)
		{
			totalBlendTime = 0.0f;
			previousFlag = currentFlag;
		}

		Utils::BlendPoses(
			m_Pose,
			(currentFlag ? FalsePose : TruePose)->GetPose(time),
			(currentFlag ? TruePose : FalsePose)->GetPose(time),
			currentBlendTime / totalBlendTime
		);
	}

	void AnimationBoolBlendNode::Reset()
	{
		m_AnimationGraph->GetInstanceValue(m_PreviousFlag).Bool = Flag->GetValue().Bool;
		m_AnimationGraph->GetInstanceValue(m_CurrentBlendTime).Float = 0.0f;
		m_AnimationGraph->GetInstanceValue(m_TotalBlendTime).Float = 0.0f;

		TruePose->Reset();
		FalsePose->Reset();
	}

	const Pose& AnimationBoolBlendNode::GetPoseInternal() const
	{
		if (m_AnimationGraph->GetInstanceValue(m_CurrentBlendTime).Float < m_AnimationGraph->GetInstanceValue(m_TotalBlendTime).Float)
			return m_Pose;

		return Flag->GetValue().Bool ? TruePose->GetPoseInternal() : FalsePose->GetPoseInternal();
	}

	AnimationIntBlendNode::AnimationIntBlendNode(Ref<AnimationGraph> animationGraph, Ref<AnimationValueNode> value, const std::vector<BlendPose>& blendPoses, const bool resetOnBlend)
		: AnimationCachedPoseNode(animationGraph), Value(value), BlendPoses(blendPoses), ResetOnBlend(resetOnBlend) {}

	void AnimationIntBlendNode::UpdatePoseInternal(const float time)
	{
		const uint32_t activePose = GetActiveBlendIndex();
		auto& previousPose = m_AnimationGraph->GetInstanceValue(m_PreviousPose).UInt;

		if (activePose == previousPose)
		{
			BlendPoses[activePose].AnimationPose->UpdatePose(time);
			return;
		}

		auto& currentBlendTime = m_AnimationGraph->GetInstanceValue(m_CurrentBlendTime).Float;
		auto& totalBlendTime = m_AnimationGraph->GetInstanceValue(m_TotalBlendTime).Float;

		if (totalBlendTime == 0.0f)
		{
			currentBlendTime = 0.0f;

			auto& blendPose = BlendPoses[activePose];
			totalBlendTime = blendPose.BlendTime->GetValue().Float;

			if (ResetOnBlend)
				blendPose.AnimationPose->Reset();
		}

		currentBlendTime += m_AnimationGraph->GetInstanceDeltaTime();

		if (currentBlendTime >= totalBlendTime)
		{
			// Check if blend has completed
			currentBlendTime = 0.0f;
			totalBlendTime = 0.0f;
			previousPose = activePose;
			return;
		}

		// Otherwise ensure blending occurs
		Utils::BlendPoses(
			m_Pose,
			BlendPoses[previousPose].AnimationPose->GetPose(time),
			BlendPoses[activePose].AnimationPose->GetPose(time),
			currentBlendTime / totalBlendTime
		);
	}

	void AnimationIntBlendNode::Reset()
	{
		m_AnimationGraph->GetInstanceValue(m_PreviousPose).UInt = GetActiveBlendIndex();
		m_AnimationGraph->GetInstanceValue(m_CurrentBlendTime).Float = 0.0f;
		m_AnimationGraph->GetInstanceValue(m_TotalBlendTime).Float = 0.0f;

		for (const auto& pose : BlendPoses)
			pose.AnimationPose->Reset();
	}

	const Pose& AnimationIntBlendNode::GetPoseInternal() const
	{
		const uint32_t activeIndex = GetActiveBlendIndex();
		const bool blending = m_AnimationGraph->GetInstanceValue(m_PreviousPose).UInt != activeIndex;
		return blending ? m_Pose : BlendPoses[activeIndex].AnimationPose->GetPoseInternal();
	}

	const uint32_t AnimationIntBlendNode::GetActiveBlendIndex() const
	{
		// Note: We clamp using signed integers so negatives get correctly mapped to the front
		return glm::clamp<int>(Value->GetValue().Int, 0, BlendPoses.size() - 1);
	}

	AnimationLocalToComponentSpaceNode::AnimationLocalToComponentSpaceNode(Ref<AnimationGraph> animationGraph, Ref<AnimationPoseNode> local)
		: AnimationCachedPoseNode(animationGraph), Local(local)
	{}

	void AnimationLocalToComponentSpaceNode::UpdatePoseInternal(const float time)
	{
		// Recursively update the pose bone matrix buffer with parent heirarchies
		Ref<Skeleton> skeleton = m_AnimationGraph->GetSkeleton();
		const auto& boneInfoMap = skeleton->GetBoneInfoMap();
		const auto& rootNode = skeleton->GetRootNode();
		Utils::LocalToComponentSpace(boneInfoMap, rootNode, Local->GetPose(time), m_Pose, glm::mat4(1.0f));
	}

	void AnimationLocalToComponentSpaceNode::Reset()
	{
		Local->Reset();
	}

	AnimationComponentToLocalSpaceNode::AnimationComponentToLocalSpaceNode(Ref<AnimationGraph> animationGraph, Ref<AnimationPoseNode> component)
		: AnimationCachedPoseNode(animationGraph), Component(component)
	{}

	void AnimationComponentToLocalSpaceNode::UpdatePoseInternal(const float time)
	{
		// Recursively update the pose bone matrix buffer reverting parent transforms
		Ref<Skeleton> skeleton = m_AnimationGraph->GetSkeleton();
		const auto& boneInfoMap = skeleton->GetBoneInfoMap();
		const auto& rootNode = skeleton->GetRootNode();
		Utils::ComponentToLocalSpace(boneInfoMap, rootNode, Component->GetPose(time), m_Pose, glm::mat4(1.0f));
	}

	void AnimationComponentToLocalSpaceNode::Reset()
	{
		Component->Reset();
	}

	static bool FindParents(const BoneNodeData& node, const std::string& name, const BoneNodeData*& target, const BoneNodeData*& parent, const BoneNodeData*& grandparent, const BoneNodeData* currentParent = nullptr)
	{
		// Check if we have found target node
		if (node.Name == name)
		{
			target = &node;
			parent = currentParent;
			return true;
		}

		for (auto& child : node.Children)
		{
			if (FindParents(child, name, target, parent, grandparent, &node))
			{
				// If we found a parent, set the grandparent
				if (parent && !grandparent)
					grandparent = currentParent;

				return true;
			}
		}

		return false;
	}

	AnimationTwoBoneIKNode::AnimationTwoBoneIKNode(Ref<AnimationGraph> animationGraph, Ref<AnimationPoseNode> componentPose, const std::string& targetBone, Ref<AnimationValueNode> effectorLocation, Ref<AnimationValueNode> jointTargetLocation, const bool allowStretching, const float startStretchRatio, const float maxStretchScale)
		: AnimationCachedPoseNode(animationGraph), ComponentPose(componentPose), TargetBone(targetBone), EffectorLocation(effectorLocation), JointTargetLocation(jointTargetLocation), AllowStretching(allowStretching), StartStretchRatio(startStretchRatio), MaxStretchScale(maxStretchScale)
	{
		// Find two parent bones.
		Ref<Skeleton> skeleton = m_AnimationGraph->GetSkeleton();
		const auto& boneInfoMap = skeleton->GetBoneInfoMap();

		const BoneNodeData* parent = nullptr;
		const BoneNodeData* grandparent = nullptr;
		const bool success = FindParents(skeleton->GetRootNode(), TargetBone, m_EndBoneData, parent, grandparent);

		DY_CORE_VERIFY(success && parent && grandparent, "Specified target bone does not have two successive parent bones for IK to be performed");

		// Specified bone is end, parent of that is joint and second parent is root
		m_EndBoneID = boneInfoMap.at(TargetBone).id;
		m_JointBoneID = boneInfoMap.at(parent->Name).id;
		m_RootBoneID = boneInfoMap.at(grandparent->Name).id;
	}

	void AnimationTwoBoneIKNode::UpdatePoseInternal(const float time)
	{
		const auto& pose = ComponentPose->GetPose(time);

		// Copy the pose for modification
		std::memcpy(m_Pose.BoneMatrices.data(), pose.BoneMatrices.data(), pose.BoneMatrices.size() * sizeof(glm::mat4));

		// DY_CORE_INFO(glm::vec3((m_Pose.BoneMatrices[m_RootBoneID] * glm::inverse(m_AnimationGraph->GetSkeleton()->GetBoneInfoMap().at(TargetBone).offset))[3]));

		// TODO: We can add support for this operation to be done in local space where we only move the root, joint and end bones to component space and back (opposite to current setup).
		// This would be a lot more optimal than the current approach! (although we should support both situations)
		
		// Prior to update move child bones to component space
		const auto& boneInfoMap = m_AnimationGraph->GetSkeleton()->GetBoneInfoMap();
		Utils::ComponentToLocalSpace(boneInfoMap, *m_EndBoneData, m_Pose, m_Pose, glm::mat4(1.0f));

		// Use animation core IK solver
		SolveTwoBoneIK(m_Pose.BoneMatrices[m_RootBoneID], m_Pose.BoneMatrices[m_JointBoneID], m_Pose.BoneMatrices[m_EndBoneID], JointTargetLocation->GetValue().Vector3, EffectorLocation->GetValue().Vector3, AllowStretching, StartStretchRatio, MaxStretchScale);

		// Move child bones back to component space where the new parent transforms propagate down the skeletal hierarchy
		Utils::LocalToComponentSpace(boneInfoMap, *m_EndBoneData, m_Pose, m_Pose, glm::mat4(1.0f));
	}

	void AnimationTwoBoneIKNode::Reset()
	{
		ComponentPose->Reset();
	}

	static bool CreateBoneIDChain(std::vector<int>& chain, std::vector<int>& parentChain, const std::unordered_map<std::string, BoneInfo>& boneInfoMap, const BoneNodeData& node, const std::string& rootBone, const std::string& tipBone, bool& rootFound)
	{
		bool include = false;
		if (node.Name == tipBone)
			include = true;
		else
		{
			for (const auto& child : node.Children)
			{
				if (CreateBoneIDChain(chain, parentChain, boneInfoMap, child, rootBone, tipBone, rootFound))
				{
					include = true;
					break;
				}
			}
		}

		if (!include)
			return false;

		(rootFound ? parentChain : chain).push_back(boneInfoMap.at(node.Name).id);

		if (node.Name == rootBone)
			rootFound = true;

		return true;
	}

	AnimationFABRIKNode::AnimationFABRIKNode(Ref<AnimationGraph> animationGraph, Ref<AnimationPoseNode> componentPose, const std::string& rootBone, const std::string& tipBone, Ref<AnimationValueNode> effectorTransform, RotationSource effectorRotationSource, float precision, int32_t maxIterations)
		: AnimationCachedPoseNode(animationGraph), ComponentPose(componentPose), RootBone(rootBone), TipBone(tipBone), EffectorTransform(effectorTransform), EffectorRotationSource(effectorRotationSource), Precision(precision), MaxIterations(maxIterations)
	{
		// Cache the bone IDs in the chain using a DFS algorithm
		Ref<Skeleton> skeleton = animationGraph->GetSkeleton();
		const auto& boneInfoMap = skeleton->GetBoneInfoMap();
		const auto& rootNode = skeleton->GetRootNode();

		bool rootFound = false;
		CreateBoneIDChain(m_ChainBoneIDs, m_ParentChain, boneInfoMap, rootNode, RootBone, TipBone, rootFound);
		std::reverse(m_ChainBoneIDs.begin(), m_ChainBoneIDs.end());
		std::reverse(m_ParentChain.begin(), m_ParentChain.end());
	}

	void AnimationFABRIKNode::UpdatePoseInternal(const float time)
	{
		// Note: FABRIK calculations assume the node is in component space

		const auto& originalPose = ComponentPose->GetPose(time);
		std::memcpy(m_Pose.BoneMatrices.data(), originalPose.BoneMatrices.data(), originalPose.BoneMatrices.size() * sizeof(glm::mat4));

		// Prior to update convert all nodes that require calculations to component space (and leave others in local space)
		glm::mat4 parentTransform = glm::mat4(1.0f);
		for (const auto& id : m_ParentChain)
			parentTransform = parentTransform * m_Pose.BoneMatrices[id];

		{
			glm::mat4 chainComponentTransform = parentTransform;
			for (const auto& id : m_ChainBoneIDs)
			{
				chainComponentTransform = chainComponentTransform * m_Pose.BoneMatrices[id];
				m_Pose.BoneMatrices[id] = chainComponentTransform;
			}
		}

		// Generate the FABRIK chain
		const size_t chainSize = m_ChainBoneIDs.size();
		std::vector<FABRIKChainLink> chain;
		chain.reserve(chainSize);

		// Start with the root bone
		const auto rootBoneID = m_ChainBoneIDs[0];
		chain.push_back({ Utils::GetTransformPosition(m_Pose.BoneMatrices[rootBoneID]), 0.0f, rootBoneID });

		float maximumReach = 0.0f;

		// Go through all remaining transforms
		for (size_t transformIndex = 1; transformIndex < chainSize; transformIndex++)
		{
			// Calculate the combined length of this segment of skeleton
			const auto currentBoneID = m_ChainBoneIDs[transformIndex];
			const auto parentBoneID = m_ChainBoneIDs[transformIndex - 1];
			glm::vec3 currentPosition = Utils::GetTransformPosition(m_Pose.BoneMatrices[currentBoneID]);
			const float boneLength = glm::distance(currentPosition, Utils::GetTransformPosition(m_Pose.BoneMatrices[parentBoneID]));

			if (!glm::epsilonEqual(boneLength, 0.0f, glm::epsilon<float>()))
			{
				chain.push_back({ currentPosition, boneLength, currentBoneID });
				maximumReach += boneLength;
			}
			else
			{
				// Mark this transform as a zero length child of the last link
				// It will inherit position and delta rotation from parent link
				DY_CORE_VERIFY(false);
				// TODO
			}
		}

		const int32_t numChainLinks = chain.size();
		const auto& effectorTransform = EffectorTransform->GetValue().Transform;
		const glm::vec3 effectorPosition = effectorTransform.Translation;
		const bool boneLocationUpdated = SolveFabrik(chain, effectorPosition, maximumReach, Precision, MaxIterations);

		// If we moved some bones update the transforms
		if (boneLocationUpdated)
		{
			// FABRIK Algorithm - re-orientation of bone local axes after translation calculation
			// Note: This section of the algorithm does not handle the tip bone
			for (size_t linkIndex = 0; linkIndex < numChainLinks - 1; linkIndex++)
			{
				const FABRIKChainLink& currentLink = chain[linkIndex];
				const FABRIKChainLink& childLink = chain[linkIndex + 1];
			
				// Calculate pre-translation vector between this bone and child
				const glm::vec3 oldDir = glm::normalize(Utils::GetTransformPosition(m_Pose.BoneMatrices[childLink.BoneIndex]) - Utils::GetTransformPosition(m_Pose.BoneMatrices[currentLink.BoneIndex]));
			
				// Get vector from the post-translation bone to its child
				const glm::vec3 newDir = glm::normalize(childLink.Position - currentLink.Position);
			
				// Calculate axis of rotation from pre-translation vector to post-translation vector
				const glm::quat deltaRotation = glm::rotation(oldDir, newDir);

				// WARNING: This does not account for any form of bone scaling!
				glm::mat4& currentBoneMatrix = m_Pose.BoneMatrices[currentLink.BoneIndex];
				currentBoneMatrix = glm::translate(glm::mat4(1.0f), currentLink.Position)
					* glm::toMat4(deltaRotation * glm::quat_cast(currentBoneMatrix));
			
				// Update zero length children if any 
				// TODO
			}
		}

		// Update position of tip
		const FABRIKChainLink& chainTip = chain[numChainLinks - 1];
		glm::mat4& tipBoneMatrix = m_Pose.BoneMatrices[chainTip.BoneIndex];
		const glm::vec3& tipPosition = chainTip.Position;

		// Handle tip bone's rotation separately
		if (EffectorRotationSource == RotationSource::CopyFromTarget)
		{
			// Directly set the rotation of the tip bone to match the effector's rotation
			const glm::quat targetRotation = effectorTransform.Rotation;
			tipBoneMatrix = glm::translate(glm::mat4(1.0f), tipPosition) * glm::toMat4(targetRotation);
		}
		else if (EffectorRotationSource == RotationSource::KeepComponentSpaceRotation)
		{
			// Orientation does not need to be updated at all, just set translation
			tipBoneMatrix[3] = glm::vec4(tipPosition, 1.0f);
		}

		// After FABRIK process has completed covert nodes on chain path back to local space
		{
			glm::mat4 chainComponentTransform = parentTransform;
			for (const auto& id : m_ChainBoneIDs)
			{
				const glm::mat4 nextTransform = m_Pose.BoneMatrices.at(id);
				m_Pose.BoneMatrices[id] = glm::inverse(chainComponentTransform) * m_Pose.BoneMatrices.at(id);
				chainComponentTransform = nextTransform;
			}
		}

		// Unlike Unreal implementation, since we convert back to local space we wait to after this conversion to update if we
		// are keeping the local space transform.
		if (EffectorRotationSource == RotationSource::KeepLocalSpaceRotation)
		{
			tipBoneMatrix = originalPose.BoneMatrices[chainTip.BoneIndex];
		}
	}

	void AnimationFABRIKNode::Reset()
	{
		ComponentPose->Reset();
	}

	AnimationTransformBoneNode::AnimationTransformBoneNode(Ref<AnimationGraph> animationGraph, Ref<AnimationPoseNode> basePose, const std::string& targetBone, Ref<AnimationValueNode> translation, Ref<AnimationValueNode> rotation, Ref<AnimationValueNode> scale, const TransformMode translationMode, const TransformMode rotationMode, const TransformMode scaleMode, Ref<AnimationValueNode> alpha)
		: AnimationCachedPoseNode(animationGraph), BasePose(basePose), TargetBone(targetBone), Translation(translation), Rotation(rotation), Scale(scale), TranslationMode(translationMode), RotationMode(rotationMode), ScaleMode(scaleMode), Alpha(alpha)
	{
		m_TargetBoneID = m_AnimationGraph->GetSkeleton()->GetBoneID(TargetBone);
	}

	void AnimationTransformBoneNode::UpdatePoseInternal(const float time)
	{
		m_Pose.Copy(BasePose->GetPose(time));

		if (m_TargetBoneID == -1 || (TranslationMode == TransformMode::Ignore && RotationMode == TransformMode::Ignore && ScaleMode == TransformMode::Ignore))
			return;

		glm::vec3 translation, scale;
		glm::quat rotation;

		const float alpha = Alpha->GetValue().Float;
		auto& transform = m_Pose.BoneMatrices[m_TargetBoneID];
		glm::decompose(transform, scale, rotation, translation, glm::vec3(0.0f), glm::vec4(1.0f));

		if (TranslationMode != TransformMode::Ignore)
			translation = TranslationMode == TransformMode::ReplaceExisting ? glm::mix(translation, Translation->GetValue().Vector3, alpha) : translation + Translation->GetValue().Vector3 * alpha;

		if (RotationMode != TransformMode::Ignore)
			rotation = glm::quat(glm::radians(RotationMode == TransformMode::ReplaceExisting ? glm::mix(glm::degrees(glm::eulerAngles(rotation)), Rotation->GetValue().Vector3, alpha) : glm::degrees(glm::eulerAngles(rotation)) + Rotation->GetValue().Vector3 * alpha));

		if (ScaleMode != TransformMode::Ignore)
			scale = ScaleMode == TransformMode::ReplaceExisting ? glm::mix(scale, Scale->GetValue().Vector3, alpha) : scale + Scale->GetValue().Vector3 * alpha;

		transform = glm::translate(glm::mat4(1.0f), translation);
		transform *= glm::toMat4(glm::normalize(rotation));
		transform = glm::scale(transform, scale);
	}

	void AnimationTransformBoneNode::Reset()
	{
		BasePose->Reset();
	}

	AnimationBlendSpace1DNode::AnimationBlendSpace1DNode(Ref<AnimationGraph> animationGraph, const std::vector<BlendSpacePoint>& points, const Ref<AnimationValueNode> x)
		: AnimationCachedPoseNode(animationGraph), Points(points), X(x)
	{
		// Sort all points (we implicitly have edges between these points)
		std::sort(Points.begin(), Points.end(), [](const BlendSpacePoint& a, const BlendSpacePoint& b)
		{
			return a.Point < b.Point;
		});
	}

	void AnimationBlendSpace1DNode::UpdatePoseInternal(const float time)
	{
		if (Points.empty())
			return;

		// Early exit if we are outside point bounds
		const float point = X->GetValue().Float;

		const auto& startPoint = Points[0];
		if (point <= startPoint.Point)
			return startPoint.Output->UpdatePose(time);

		const auto& endPoint = Points[Points.size() - 1];
		if (point >= endPoint.Point)
			return endPoint.Output->UpdatePose(time);

		auto& previousSegmentIndex = m_AnimationGraph->GetInstanceValue(m_PreviousSegmentIndex).UInt;

		// Check the same segment we were in previously first
		if (CheckAndUpdateSegment(point, previousSegmentIndex, time))
			return;

		// Check all remaining segments
		for (uint32_t segmentIndex = 0; segmentIndex < Points.size() - 1; segmentIndex++)
		{
			if (segmentIndex == previousSegmentIndex)
				continue;

			if (CheckAndUpdateSegment(point, segmentIndex, time))
			{
				previousSegmentIndex = segmentIndex;
				return;
			}
		}

		// Unreachable return
		DY_CORE_ASSERT(false);
	}

	const Pose& AnimationBlendSpace1DNode::GetPoseInternal() const
	{
		if (Points.empty())
			return m_Pose;

		const float point = X->GetValue().Float;

		if (point <= Points[0].Point)
			return Points[0].Output->GetPoseInternal();

		const size_t lastIndex = Points.size() - 1;
		if (point >= Points[lastIndex].Point)
			return Points[lastIndex].Output->GetPoseInternal();

		return m_Pose;
	}

	void AnimationBlendSpace1DNode::Reset()
	{
		m_AnimationGraph->GetInstanceValue(m_PreviousSegmentIndex).UInt = 0;

		for (const auto& point : Points)
			point.Output->Reset();
	}

	bool AnimationBlendSpace1DNode::CheckAndUpdateSegment(const float point, const uint32_t startPointIndex, const float time)
	{
		const auto& startPoint = Points[startPointIndex];
		const auto& endPoint = Points[startPointIndex + 1];

		if (point < startPoint.Point || point > endPoint.Point)
			return false;

		Utils::BlendPoses(
			m_Pose,
			startPoint.Output->GetPose(time),
			endPoint.Output->GetPose(time),
			(point - startPoint.Point) / (endPoint.Point - startPoint.Point)
		);

		return true;
	}

	AnimationBlendSpaceNode::AnimationBlendSpaceNode(Ref<AnimationGraph> animationGraph, const std::vector<BlendSpacePoint>& points, const Ref<AnimationValueNode> x, const Ref<AnimationValueNode> y)
		: AnimationCachedPoseNode(animationGraph), Points(points), X(x), Y(y)
	{
		if (Points.size() < 3)
			return;

		// Use a Delaunay Triangulation algorithm on vertex graph
		// TODO: We should probably serialize this for runtime to avoid computing this during distribution
		std::vector<DelaunayVertex> vertices;
		vertices.reserve(points.size());

		for (const auto& point : Points)
			vertices.emplace_back(point.Point);

		DelaunaryTriangulate(vertices, m_Triangles);

		if (m_Triangles.empty())
		{
			// Handle edge case where all points are collinear so no valid triangle data was generated
			// We will build a list of edges ourselves

			std::vector<uint32_t> indexList(vertices.size());
			std::iota(indexList.begin(), indexList.end(), 0);

			// ALL Points are collinear so we can sort using x component (and fall back to y if equal)
			std::sort(indexList.begin(), indexList.end(), [&vertices](const uint32_t indexA, const uint32_t indexB)
			{
				const auto& a = vertices[indexA];
				const auto& b = vertices[indexB];
				return (a.x == b.x) ? (a.y < b.y) : (a.x < b.x);
			});

			// Populate outer edge list with sorted edges
			m_OuterEdges.clear();
			const size_t lineCount = vertices.size() - 1;
			m_OuterEdges.reserve(lineCount);
			for (size_t i = 0; i < lineCount; i++)
				m_OuterEdges.emplace_back(indexList[i], indexList[i + 1]);
		}
		else
		{
			// Build a cache of all perimeter edges around the triangulated region
			DelaunaryTraceOutsideEdges(m_Triangles, m_OuterEdges);
		}
	}

	void AnimationBlendSpaceNode::UpdatePoseInternal(const float time)
	{
		const glm::vec2 point = glm::vec2(X->GetValue().Float, Y->GetValue().Float);

		if (Points.empty())
			return;

		// Handle degenerate triangles cases
		if (Points.size() == 1)
		{
			// Only single animation specified, no blending needed
			// Note: Pass-through optimization is not used here and we do a full pose copy. Who in their right mind would use a one node blendspace?
			m_Pose.Copy(Points[0].Output->GetPose(time));
		}
		else if (Points.size() == 2)
		{
			// Linear blend directly between two poses
			const auto& pointA = Points[0];
			const auto& pointB = Points[1];

			const glm::vec2 delta = pointB.Point - pointA.Point;
			float projectionFactor = glm::dot(point - pointA.Point, delta) / glm::dot(delta, delta);

			if (projectionFactor <= 0.0f)
			{
				m_Pose.Copy(pointA.Output->GetPose(time));
				return;
			}

			if (projectionFactor >= 1.0f)
			{
				m_Pose.Copy(pointB.Output->GetPose(time));
				return;
			}

			Utils::BlendPoses(m_Pose, pointA.Output->GetPose(time), pointB.Output->GetPose(time), projectionFactor);
			return;
		}

		// Check if we in the same triangle as previously
		auto& previousTriangleIndex = m_AnimationGraph->GetInstanceValue(m_PreviousTriangleIndex).UInt;
		
		if (previousTriangleIndex != -1 && CheckAndUpdateTriangle(point, previousTriangleIndex, time))
			return;

		// Otherwise, attempt to locate which triangulation unit we are in
		for (uint32_t triangleIndex = 0; triangleIndex < m_Triangles.size(); triangleIndex++)
		{
			if (triangleIndex == previousTriangleIndex)
				continue;

			if (CheckAndUpdateTriangle(point, triangleIndex, time))
			{
				previousTriangleIndex = triangleIndex;
				return;
			}
		}

		// If we are outside all triangles, identify the closest edge of the convex structure and project onto it to determine the linear blend weight
		previousTriangleIndex = -1;

		float minDistance = std::numeric_limits<float>::max();
		uint32_t closestEdgeStart = -1;
		uint32_t closestEdgeEnd = -1;
		float closestProjectionFactor;

		for (const auto& edge : m_OuterEdges)
		{
			const glm::vec2& startPoint = Points[edge.V0].Point;
			const glm::vec2& endPoint = Points[edge.V1].Point;
			const glm::vec2 edgeVector = endPoint - startPoint;

			// Project the point onto the edge vector and ensure we stay within the edge bounds
			float projectionFactor = glm::dot(point - startPoint, edgeVector) / glm::dot(edgeVector, edgeVector);
			projectionFactor = glm::clamp(projectionFactor, 0.0f, 1.0f);

			// Calculate the projected point and the distance to the query point
			const glm::vec2 projectedPoint = startPoint + projectionFactor * edgeVector;
			float distance = glm::distance(point, projectedPoint);

			if (distance < minDistance)
			{
				minDistance = distance;
				closestEdgeStart = edge.V0;
				closestEdgeEnd = edge.V1;
				closestProjectionFactor = projectionFactor;
			}
		}

		if (closestEdgeStart == -1 || closestEdgeEnd == -1)
			return;

		// Blend using closest edge
		Utils::BlendPoses(
			m_Pose,
			Points[closestEdgeStart].Output->GetPose(time),
			Points[closestEdgeEnd].Output->GetPose(time),
			closestProjectionFactor
		);
	}

	void AnimationBlendSpaceNode::Reset()
	{
		m_AnimationGraph->GetInstanceValue(m_PreviousTriangleIndex).UInt = -1;

		for (const auto& point : Points)
			point.Output->Reset();
	}

	bool AnimationBlendSpaceNode::CheckAndUpdateTriangle(const glm::vec2& point, const uint32_t triangleIndex, const float time)
	{
		const auto& triangle = m_Triangles[triangleIndex];

		const auto& point0 = Points[triangle.V0];
		const auto& point1 = Points[triangle.V1];
		const auto& point2 = Points[triangle.V2];

		// Calculate barycentric coordinates (and early exit if we are outside the triangle)
		const glm::vec2 v0 = point1.Point - point0.Point;
		const glm::vec2 v1 = point2.Point - point0.Point;
		const glm::vec2 v2 = point - point0.Point;

		const float d00 = glm::dot(v0, v0);
		const float d01 = glm::dot(v0, v1);
		const float d11 = glm::dot(v1, v1);
		const float d20 = glm::dot(v2, v0);
		const float d21 = glm::dot(v2, v1);
		const float denom = d00 * d11 - d01 * d01;

		const float v = (d11 * d20 - d01 * d21) / denom;
		if (v < 0.0f)
			return false;

		const float w = (d00 * d21 - d01 * d20) / denom;
		if (w < 0.0f)
			return false;

		const float u = 1.0f - v - w;
		if (u < 0.0f)
			return false;

		// We have located the triangle we are in, let's blend the nodes!
		Utils::BlendPoses(
			m_Pose,
			point0.Output->GetPose(time),
			point1.Output->GetPose(time),
			point2.Output->GetPose(time),
			u, v, w
		);

		return true;
	}

	AnimationStateMachineNode::AnimationStateMachineNode(Ref<AnimationGraph> animationGraph, const std::vector<State>& states, const uint32_t maxTransitionsPerFrame, const bool skipFirstUpdateTransition)
		: AnimationCachedPoseNode(animationGraph), States(states), MaxTransitionsPerFrame(maxTransitionsPerFrame), SkipFirstUpdateTransition(skipFirstUpdateTransition)
	{}

	const Pose& AnimationStateMachineNode::GetPoseInternal() const
	{
		const bool blending = m_AnimationGraph->GetInstanceValue(m_CurrentBlendTime).Float < m_AnimationGraph->GetInstanceValue(m_TotalBlendTime).Float;
		return blending ? m_Pose : States[m_AnimationGraph->GetInstanceValue(m_ActiveState).Handle].Output->GetPoseInternal();
	}

	void AnimationStateMachineNode::Reset()
	{
		m_AnimationGraph->GetInstanceValue(m_ActiveState).Handle = 0;
		m_AnimationGraph->GetInstanceValue(m_PreviousState).Handle = 0;
		m_AnimationGraph->GetInstanceValue(m_CurrentBlendTime).Float = 0.0f;
		m_AnimationGraph->GetInstanceValue(m_TotalBlendTime).Float = 0.0f;

		for (const auto& state : States)
			state.Output->Reset();
	}

	void AnimationStateMachineNode::UpdatePoseInternal(float time)
	{
		auto& activeState = m_AnimationGraph->GetInstanceValue(m_ActiveState).Handle;
		auto& previousState = m_AnimationGraph->GetInstanceValue(m_PreviousState).Handle;
		auto& currentBlendTime = m_AnimationGraph->GetInstanceValue(m_CurrentBlendTime).Float;
		auto& totalBlendTime = m_AnimationGraph->GetInstanceValue(m_TotalBlendTime).Float;

		for (uint32_t transitionIndex = 0; transitionIndex < MaxTransitionsPerFrame; transitionIndex++)
		{
			const auto& transitions = States[activeState].Transitions;

			bool transitioned = false;
			for (const auto& transition : transitions)
			{
				if (transition.Transition->GetValue().Bool)
				{
					previousState = activeState;
					activeState = transition.Target;
					currentBlendTime = 0.0f;
					totalBlendTime = transition.Duration->GetValue().Float;

					// Reset the new active state when we transition
					States[activeState].Output->Reset();

					transitioned = true;
					break;
				}
			}

			if (!transitioned)
				break;
		}

		currentBlendTime += m_AnimationGraph->GetInstanceDeltaTime();

		if (currentBlendTime < totalBlendTime && activeState != previousState)
		{
			Utils::BlendPoses(m_Pose, States[previousState].Output->GetPose(time), States[activeState].Output->GetPose(time), currentBlendTime / totalBlendTime);
		}
		else
		{
			// Trigger an update for our active state
			States[activeState].Output->UpdatePose(time);
		}
	}

	AnimationValueNode::AnimationValueNode(Ref<AnimationGraph> animationGraph)
		: AnimationNode(animationGraph)
	{}

	AnimationConstantNode::AnimationConstantNode(Ref<AnimationGraph> animationGraph, const AnimationGraphData& constant)
		: AnimationValueNode(animationGraph), Constant(constant)
	{}

	AnimationParameterNode::AnimationParameterNode(Ref<AnimationGraph> animationGraph, const std::string& name)
		: AnimationValueNode(animationGraph), Name(name)
	{}

	AnimationGraphData AnimationParameterNode::GetValue() const
	{
		const auto& activeParameters = m_AnimationGraph->GetActiveData();
		const auto& parameters = activeParameters ? activeParameters->Parameters.Parameters : m_AnimationGraph->GetDefaultParameters().Parameters;

		if (parameters.find(Name) == parameters.end())
		{
			DY_CORE_ASSERT(false);
			DY_CORE_WARN("Parameter '{}' was not found on animation graph", Name);
			return AnimationGraphData();
		}

		return parameters.at(Name);
	}

	AnimationOperatorNode::AnimationOperatorNode(Ref<AnimationGraph> animationGraph, const OperatorType operatorType, Ref<AnimationValueNode> a, Ref<AnimationValueNode> b)
		: AnimationValueNode(animationGraph), Operator(operatorType), A(a), B(b)
	{}

	AnimationGraphData AnimationOperatorNode::GetValue() const
	{
		const auto& a = A->GetValue();
		const auto& b = B->GetValue();

		switch (Operator)
		{
		case OperatorType::AND: return AnimationGraphData(a.Bool && b.Bool);
		case OperatorType::OR: return AnimationGraphData(a.Bool || b.Bool);
		case OperatorType::NAND: return AnimationGraphData(!(a.Bool && b.Bool));
		case OperatorType::NOR: return AnimationGraphData(!(a.Bool || b.Bool));
		case OperatorType::XOR: return AnimationGraphData(a.Bool ^ b.Bool);

		case OperatorType::Equality: return AnimationGraphData(a.Float == b.Float);
		case OperatorType::Inequality: return AnimationGraphData(a.Float != b.Float);
		case OperatorType::LessThan: return AnimationGraphData(a.Float < b.Float);
		case OperatorType::LessThanOrEqual: return AnimationGraphData(a.Float <= b.Float);
		case OperatorType::GreaterThan: return AnimationGraphData(a.Float > b.Float);
		case OperatorType::GreaterThanOrEqual: return AnimationGraphData(a.Float >= b.Float);

		case OperatorType::Add: return AnimationGraphData(a.Float + b.Float);
		case OperatorType::Subtract: return AnimationGraphData(a.Float - b.Float);
		case OperatorType::Multiply: return AnimationGraphData(a.Float * b.Float);
		case OperatorType::Divide: return AnimationGraphData(a.Float / b.Float);
		}

		return AnimationGraphData();
	}

	AnimationBooleanNotNode::AnimationBooleanNotNode(Ref<AnimationGraph> animationGraph, Ref<AnimationValueNode> a)
		: AnimationValueNode(animationGraph), A(a)
	{}

	AnimationGraphData AnimationBooleanNotNode::GetValue() const
	{
		return AnimationGraphData(!A->GetValue().Bool);
	}

	AnimationTimeRemainingRatioNode::AnimationTimeRemainingRatioNode(Ref<AnimationGraph> animationGraph, Ref<AnimationPlayerNode> player)
		: AnimationValueNode(animationGraph), Player(player)
	{}

	AnimationGraphData AnimationTimeRemainingRatioNode::GetValue() const
	{
		return 1.0f - (Player->GetCurrentTime() / Player->GetLength());
	}

}