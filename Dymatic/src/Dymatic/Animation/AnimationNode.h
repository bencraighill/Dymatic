#pragma once

#include "Dymatic/Animation/Animation.h"
#include "Dymatic/Animation/AnimationGraphData.h"

#include "Dymatic/Animation/DelaunayTriangleGenerator.h"

namespace Dymatic {

	class AnimationGraph;

	struct Pose
	{
		std::vector<glm::mat4> BoneMatrices;
		void Copy(const Pose& source);
	};

	enum class AnimationNodeType
	{
		// Pose Nodes
		Default,
		Player,
		Blend,
		BoolBlend,
		IntBlend,
		LayeredBoneBlend,
		Additive,
		TwoBoneIK,
		FABRIK,
		TransformBone,
		ComponentToLocal,
		LocalToComponent,
		Blendspace,
		Blendspace1D,
		StateMachine,

		// Value Nodes
		Parameter,
		Constant,
		Operator,
		BooleanNot,
		TimeRemainingRatio,
	};

	// Base Animation Classes

	class AnimationNode
	{
	public:
		AnimationNode(Ref<AnimationGraph> animationGraph);
		virtual AnimationNodeType GetType() const = 0;
	protected:
		// Note: We use a raw pointer to avoid circular reference dependencies
		AnimationGraph* m_AnimationGraph;

		friend class AnimationGraph;
	};

	class AnimationValueNode : public AnimationNode
	{
	public:
		AnimationValueNode(Ref<AnimationGraph> animationGraph);
		virtual AnimationGraphData GetValue() const = 0;
	};

	class AnimationPoseNode : public AnimationNode
	{
	public:
		AnimationPoseNode(Ref<AnimationGraph> animationGraph);

		// Note: This generic `GetPose` method should always be used as it will utilize the class' pose cache.
		const Pose& GetPose(const float time);
		void UpdatePose(const float time);

	public:
		virtual const Pose& GetPoseInternal() const = 0;
		virtual void UpdatePoseInternal(float time) = 0;
		virtual void Reset() {};
	protected:
		float m_PoseTime = -1.0f;
	};

	class AnimationCachedPoseNode : public AnimationPoseNode
	{
	public:
		AnimationCachedPoseNode(Ref<AnimationGraph> animationGraph);
		virtual const Pose& GetPoseInternal() const override;
	protected:
		Pose m_Pose;
	};

	// Animation Pose Node Implementations

	class AnimationDefaultNode : public AnimationCachedPoseNode
	{
	public:
		AnimationDefaultNode(Ref<AnimationGraph> animationGraph);
		virtual void UpdatePoseInternal(const float time) override;
		inline virtual AnimationNodeType GetType() const override { return AnimationNodeType::Default; }
	};

	class AnimationPlayerNode : public AnimationCachedPoseNode
	{
	public:
		AnimationPlayerNode(Ref<AnimationGraph> animationGraph, Ref<Animation> animation, Ref<AnimationValueNode> playRate);
		virtual void UpdatePoseInternal(const float time) override;
		virtual void Reset() override;
		inline virtual AnimationNodeType GetType() const override { return AnimationNodeType::Player; }

		float GetCurrentTime();
		float GetLength();
	private:
		void CalculateBoneTransform(const BoneNodeData& node, float time);
	public:
		Ref<Animation> AnimationAsset;
		Ref<AnimationValueNode> PlayRate;
	private:
		AnimationGraphValueHandle m_CurrentTime = UUID();
	};

	class AnimationBlendNode : public AnimationCachedPoseNode
	{
	public:
		AnimationBlendNode(Ref<AnimationGraph> animationGraph, Ref<AnimationPoseNode> a, Ref<AnimationPoseNode> b, Ref<AnimationValueNode> weight);
		virtual void UpdatePoseInternal(const float time) override;
		virtual void Reset() override;
		inline virtual AnimationNodeType GetType() const override { return AnimationNodeType::Blend; }
	public:
		virtual const Pose& GetPoseInternal() const override;
	public:
		Ref<AnimationPoseNode> BlendA;
		Ref<AnimationPoseNode> BlendB;
		Ref<AnimationValueNode> Weight;
	};

	class AnimationLayeredBoneBlendNode : public AnimationCachedPoseNode
	{
	public:
		typedef std::vector<std::vector<float>> BlendWeightMultiplierMatrix;

		struct BranchFilter
		{
			std::string BoneName;
			int32_t BlendDepth = 0;

			// Cached Value
			int BoneID;

			BranchFilter() = default;
			BranchFilter(const std::string& boneName, const int32_t blendDepth)
				: BoneName(boneName), BlendDepth(blendDepth) {}
		};

		struct BlendPose
		{
			Ref<AnimationPoseNode> AnimationPose;
			Ref<AnimationValueNode> BlendWeight;

			std::vector<BranchFilter> BranchFilters;
		};

	public:
		AnimationLayeredBoneBlendNode(Ref<AnimationGraph> animationGraph, const Ref<AnimationPoseNode> basePose, const std::vector<BlendPose>& blendPoses);
		virtual void UpdatePoseInternal(const float time) override;
		virtual void Reset() override;
		// Note: Only the 'Triggers' from the highest weighted animation should be passed through (when we add these)
		inline virtual AnimationNodeType GetType() const override { return AnimationNodeType::LayeredBoneBlend; }
	public:
		Ref<AnimationPoseNode> BasePose;
		std::vector<BlendPose> BlendPoses;
	private:
		BlendWeightMultiplierMatrix m_BlendWeightMultipliers;
	};

	class AnimationAdditiveNode : public AnimationCachedPoseNode
	{
	public:
		AnimationAdditiveNode(Ref<AnimationGraph> animationGraph, Ref<AnimationPoseNode> base, Ref<AnimationPoseNode> additive, Ref<AnimationValueNode> alpha);
		virtual void UpdatePoseInternal(const float time) override;
		virtual void Reset() override;
		inline virtual AnimationNodeType GetType() const override { return AnimationNodeType::Additive; }
	public:
		Ref<AnimationPoseNode> Base;
		Ref<AnimationPoseNode> Additive;
		Ref<AnimationValueNode> Alpha;
	};

	class AnimationBoolBlendNode : public AnimationCachedPoseNode
	{
	public:
		AnimationBoolBlendNode(Ref<AnimationGraph> animationGraph, Ref<AnimationValueNode> flag, Ref<AnimationPoseNode> truePose, Ref<AnimationPoseNode> falsePose, Ref<AnimationValueNode> trueBlendTime, Ref<AnimationValueNode> falseBlendTime, const bool resetOnBlend);
		virtual void UpdatePoseInternal(const float time) override;
		virtual void Reset() override;
		inline virtual AnimationNodeType GetType() const override { return AnimationNodeType::BoolBlend; }
		virtual const Pose& GetPoseInternal() const override;
	public:
		Ref<AnimationValueNode> Flag;
		Ref<AnimationPoseNode> TruePose;
		Ref<AnimationPoseNode> FalsePose;
		Ref<AnimationValueNode> TrueBlendTime;
		Ref<AnimationValueNode> FalseBlendTime;
		bool ResetOnBlend;
	private:
		AnimationGraphValueHandle m_PreviousFlag = UUID();
		AnimationGraphValueHandle m_CurrentBlendTime = UUID();
		AnimationGraphValueHandle m_TotalBlendTime = UUID();
	};

	class AnimationIntBlendNode : public AnimationCachedPoseNode
	{
	public:
		struct BlendPose
		{
			Ref<AnimationPoseNode> AnimationPose;
			Ref<AnimationValueNode> BlendTime;
 		};
	public:
		// Note: Only the 'Triggers' from the highest weighted animation should be passed through
		AnimationIntBlendNode(Ref<AnimationGraph> animationGraph, Ref<AnimationValueNode> value, const std::vector<BlendPose>& blendPoses, const bool resetOnBlend);
		virtual void UpdatePoseInternal(const float time) override;
		virtual void Reset() override;
		inline virtual AnimationNodeType GetType() const override { return AnimationNodeType::IntBlend; }
		virtual const Pose& GetPoseInternal() const override;
	private:
		const uint32_t GetActiveBlendIndex() const;
	public:
		Ref<AnimationValueNode> Value;
		std::vector<BlendPose> BlendPoses;
		bool ResetOnBlend;
	private:
		AnimationGraphValueHandle m_PreviousPose = UUID();
		AnimationGraphValueHandle m_CurrentBlendTime = UUID();
		AnimationGraphValueHandle m_TotalBlendTime = UUID();
	};

	class AnimationLocalToComponentSpaceNode : public AnimationCachedPoseNode
	{
	public:
		AnimationLocalToComponentSpaceNode(Ref<AnimationGraph> animationGraph, Ref<AnimationPoseNode> local);
		virtual void UpdatePoseInternal(const float time) override;
		virtual void Reset() override;
		inline virtual AnimationNodeType GetType() const override { return AnimationNodeType::LocalToComponent; }
	public:
		Ref<AnimationPoseNode> Local;
	};

	class AnimationComponentToLocalSpaceNode : public AnimationCachedPoseNode
	{
	public:
		AnimationComponentToLocalSpaceNode(Ref<AnimationGraph> animationGraph, Ref<AnimationPoseNode> component);
		virtual void UpdatePoseInternal(const float time) override;
		virtual void Reset() override;
		inline virtual AnimationNodeType GetType() const override { return AnimationNodeType::ComponentToLocal; }
	public:
		Ref<AnimationPoseNode> Component;
	};

	class AnimationTwoBoneIKNode : public AnimationCachedPoseNode
	{
	public:
		AnimationTwoBoneIKNode(Ref<AnimationGraph> animationGraph, Ref<AnimationPoseNode> componentPose, const std::string& targetBone, Ref<AnimationValueNode> effectorLocation, Ref<AnimationValueNode> jointTargetLocation, const bool allowStretching, const float startStretchRatio, const float maxStretchScale);
		virtual void UpdatePoseInternal(const float time) override;
		virtual void Reset() override;
		inline virtual AnimationNodeType GetType() const override { return AnimationNodeType::TwoBoneIK; }
	public:
		Ref<AnimationPoseNode> ComponentPose;
		Ref<AnimationValueNode> EffectorLocation;
		Ref<AnimationValueNode> JointTargetLocation;

		std::string TargetBone;
		bool AllowStretching = false;
		float StartStretchRatio = 1.0f;
		float MaxStretchScale = 1.2f;
	private:
		// Cached data
		int m_EndBoneID, m_JointBoneID, m_RootBoneID;

		// Note: We usually avoid storing pointer caches like this (prefer IDs) but this is necessary for optimal skeleton hierarchy traversal
		// This should be fine as this data is tied to the lifetime of the graph skeleton which is tied to the lifetime of the graph itself
		const BoneNodeData* m_EndBoneData = nullptr;
	};

	class AnimationFABRIKNode : public AnimationCachedPoseNode
	{
	public:
		enum class RotationSource
		{
			KeepLocalSpaceRotation,
			CopyFromTarget,
			KeepComponentSpaceRotation
		};

	public:
		AnimationFABRIKNode(Ref<AnimationGraph> animationGraph, Ref<AnimationPoseNode> componentPose, const std::string& rootBone, const std::string& tipBone, Ref<AnimationValueNode> effectorTransform, RotationSource effectorRotationSource, float precision, int32_t maxIterations);
		virtual void UpdatePoseInternal(const float time) override;
		virtual void Reset() override;
		inline virtual AnimationNodeType GetType() const override { return AnimationNodeType::FABRIK; }
	public:
		Ref<AnimationPoseNode> ComponentPose;
		Ref<AnimationValueNode> EffectorTransform;

		std::string RootBone;
		std::string TipBone;

		RotationSource EffectorRotationSource = RotationSource::KeepLocalSpaceRotation;
		float Precision = 1.0f;
		int32_t MaxIterations = 10;
	private:
		// Cached data
		std::vector<int> m_ChainBoneIDs;
		std::vector<int> m_ParentChain;
	};

	class AnimationTransformBoneNode : public AnimationCachedPoseNode
	{
	public:
		enum class TransformMode
		{
			Ignore,
			ReplaceExisting,
			AddToExisting
		};

	public:
		AnimationTransformBoneNode(Ref<AnimationGraph> animationGraph, Ref<AnimationPoseNode> basePose, const std::string& targetBone, Ref<AnimationValueNode> translation, Ref<AnimationValueNode> rotation, Ref<AnimationValueNode> scale, const TransformMode translationMode, const TransformMode rotationMode, const TransformMode scaleMode, Ref<AnimationValueNode> alpha);
		virtual void UpdatePoseInternal(const float time) override;
		virtual void Reset() override;
		inline virtual AnimationNodeType GetType() const override { return AnimationNodeType::FABRIK; }
	public:
		Ref<AnimationPoseNode> BasePose;
		std::string TargetBone;
		Ref<AnimationValueNode> Translation, Rotation, Scale;
		TransformMode TranslationMode, RotationMode, ScaleMode;
		Ref<AnimationValueNode> Alpha;
	private:
		int m_TargetBoneID;
	};

	class AnimationBlendSpace1DNode : public AnimationCachedPoseNode
	{
	public:
		struct BlendSpacePoint
		{
			float Point;
			Ref<AnimationPoseNode> Output;

			BlendSpacePoint() = default;
			BlendSpacePoint(const float point, const Ref<AnimationPoseNode> output)
				: Point(point), Output(output) {}
		};
	public:
		AnimationBlendSpace1DNode(Ref<AnimationGraph> animationGraph, const std::vector<BlendSpacePoint>& points, const Ref<AnimationValueNode> x);
		virtual void UpdatePoseInternal(const float time) override;
		virtual const Pose& GetPoseInternal() const override;
		virtual void Reset() override;
		inline virtual AnimationNodeType GetType() const override { return AnimationNodeType::Blendspace1D; }
	private:
		bool CheckAndUpdateSegment(const float point, const uint32_t startPointIndex, const float time);
	public:
		std::vector<BlendSpacePoint> Points;
		Ref<AnimationValueNode> X;
	private:
		AnimationGraphValueHandle m_PreviousSegmentIndex = UUID();
	};

	class AnimationBlendSpaceNode : public AnimationCachedPoseNode
	{
	public:
		struct BlendSpacePoint
		{
			glm::vec2 Point;
			Ref<AnimationPoseNode> Output;

			BlendSpacePoint() = default;
			BlendSpacePoint(const glm::vec2& point, const Ref<AnimationPoseNode> output)
				: Point(point), Output(output) {}
		};
	public:
		AnimationBlendSpaceNode(Ref<AnimationGraph> animationGraph, const std::vector<BlendSpacePoint>& points, const Ref<AnimationValueNode> x, const Ref<AnimationValueNode> y);
		virtual void UpdatePoseInternal(const float time) override;
		virtual void Reset() override;
		inline virtual AnimationNodeType GetType() const override { return AnimationNodeType::Blendspace; }
	private:
		bool CheckAndUpdateTriangle(const glm::vec2& point, const uint32_t triangleIndex, const float time);
	public:
		std::vector<BlendSpacePoint> Points;
		Ref<AnimationValueNode> X;
		Ref<AnimationValueNode> Y;
	private:
		// Cache previous triangle we were in and check that first
		std::vector<DelaunayTriangleResult> m_Triangles;
		std::vector<DelaunayEdgeResult> m_OuterEdges;
		AnimationGraphValueHandle m_PreviousTriangleIndex = UUID();
	};

	class AnimationStateMachineNode : public AnimationCachedPoseNode
	{
	public:
		typedef uint64_t StateHandle;

		struct Transition
		{
			StateHandle Target;
			Ref<AnimationValueNode> Transition;
			Ref<AnimationValueNode> Duration;
		};

		struct State
		{
			Ref<AnimationPoseNode> Output;
			std::vector<Transition> Transitions;
		};
	public:
		AnimationStateMachineNode(Ref<AnimationGraph> animationGraph, const std::vector<State>& states, const uint32_t maxTransitionsPerFrame, const bool skipFirstUpdateTransition);
		virtual const Pose& GetPoseInternal() const override;
		virtual void Reset() override;
		inline virtual AnimationNodeType GetType() const override { return AnimationNodeType::StateMachine; }
	protected:
		virtual void UpdatePoseInternal(float time) override;
	public:
		uint32_t MaxTransitionsPerFrame = 3;
		bool SkipFirstUpdateTransition = true;
		std::vector<State> States;
	private:
		AnimationGraphValueHandle m_ActiveState = UUID();
		AnimationGraphValueHandle m_PreviousState = UUID();
		AnimationGraphValueHandle m_CurrentBlendTime = UUID();
		AnimationGraphValueHandle m_TotalBlendTime = UUID();
	};

	// Animation Value Node Implementations

	class AnimationConstantNode : public AnimationValueNode
	{
	public:
		AnimationConstantNode(Ref<AnimationGraph> animationGraph, const AnimationGraphData& constant);
		inline virtual AnimationNodeType GetType() const override { return AnimationNodeType::Constant; }
		inline virtual AnimationGraphData GetValue() const override { return Constant; }
	public:
		AnimationGraphData Constant;
	};

	class AnimationParameterNode : public AnimationValueNode
	{
	public:
		AnimationParameterNode(Ref<AnimationGraph> animationGraph, const std::string& name);
		inline virtual AnimationNodeType GetType() const override { return AnimationNodeType::Parameter; }
		virtual AnimationGraphData GetValue() const override;
	public:
		std::string Name;
	};

	class AnimationOperatorNode : public AnimationValueNode
	{
	public:
		enum class OperatorType
		{
			None,

			// Boolean Operators
			AND,
			OR,
			NAND,
			NOR,
			XOR,

			// Numeric Comparators
			Equality,
			Inequality,
			LessThan,
			LessThanOrEqual,
			GreaterThan,
			GreaterThanOrEqual,

			// Numeric Operators
			Add,
			Subtract,
			Multiply,
			Divide,
		};
	public:
		AnimationOperatorNode(Ref<AnimationGraph> animationGraph, const OperatorType operatorType, Ref<AnimationValueNode> a, Ref<AnimationValueNode> b);
		inline virtual AnimationNodeType GetType() const override { return AnimationNodeType::Operator; }
		virtual AnimationGraphData GetValue() const override;
	public:
		Ref<AnimationValueNode> A;
		Ref<AnimationValueNode> B;
		OperatorType Operator;
	};

	class AnimationBooleanNotNode : public AnimationValueNode
	{
	public:
		AnimationBooleanNotNode(Ref<AnimationGraph> animationGraph, Ref<AnimationValueNode> a);
		inline virtual AnimationNodeType GetType() const override { return AnimationNodeType::BooleanNot; }
		virtual AnimationGraphData GetValue() const override;
	public:
		Ref<AnimationValueNode> A;
	};

	class AnimationTimeRemainingRatioNode : public AnimationValueNode
	{
	public:
		AnimationTimeRemainingRatioNode(Ref<AnimationGraph> animationGraph, Ref<AnimationPlayerNode> player);
		inline virtual AnimationNodeType GetType() const override { return AnimationNodeType::TimeRemainingRatio; }
		virtual AnimationGraphData GetValue() const override;
	public:
		Ref<AnimationPlayerNode> Player;
	};

}