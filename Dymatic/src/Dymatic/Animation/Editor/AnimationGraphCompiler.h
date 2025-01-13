#pragma once

#include "Dymatic/Animation/AnimationGraph.h"
#include "Dymatic/Editor/Compiler.h"

namespace Dymatic::Editor {

	class AnimationGraphCompiler
	{
	public:
		AnimationGraphCompiler(Ref<AnimationEditorGraph> editorGraph);
		void Compile();

		inline Ref<AnimationGraph> GetAnimationGraph() const { return m_AnimationGraph; }
		inline const CompilerResult& GetCompilerResult() const { return m_CompilerResult; }

	private:
		void CompileGraph();

		Ref<AnimationPoseNode> GenerateRuntimeNode(NodeHandle nodeID);
		Ref<AnimationValueNode> GenerateRuntimeValueNode(Ref<AnimationPin> pin);
		PinHandle GetConnectedPin(PinHandle pinID);
		NodeHandle GetConnectedNode(PinHandle pinID);
		Ref<AnimationPoseNode> GetDefaultNode();

		bool DoesSkeletonContainBone(const std::string& boneName);

		Ref<AnimationEditorNode> FindNodeOnLayer(const LayerHandle layer, const AnimationNodeFunction function);
		void GenerateBlendSpaceGraph(const LayerHandle layer, const std::function<void(Ref<AnimationEditorNode>, Ref<AnimationPoseNode>)>& pointCallback);

	public:
		AssetHandle TargetHandle;
		AssetHandle TargetSkeleton = 0;

	private:
		// Inputs
		Ref<AnimationEditorGraph> m_EditorGraph;

		// Outputs
		Ref<AnimationGraph> m_AnimationGraph;
		CompilerResult m_CompilerResult;

		// Internal
		Ref<AnimationDefaultNode> m_DefaultNode;
		std::unordered_map<uint64_t, Ref<AnimationPoseNode>> m_AnimationNodes;
		std::unordered_set<uint64_t> m_ProcessingNodes;
	};

}