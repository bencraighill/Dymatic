#include "dypch.h"
#include "Dymatic/Animation/AnimationGraph.h"

namespace Dymatic {

	AnimationGraph::AnimationGraph(Ref<Skeleton> skeleton, Ref<Editor::AnimationEditorGraph> editorGraph)
		: m_Skeleton(skeleton), m_EditorGraph(editorGraph)
	{}

	const std::vector<glm::mat4>& AnimationGraph::GetFinalBoneMatrices(float time, const Ref<AnimationGraphInstanceData> activeData)
	{
		m_ActiveData = activeData;
		return m_Output->GetPose(time).BoneMatrices;
	}

	void AnimationGraph::ResetNodes(const Ref<AnimationGraphInstanceData> activeData)
	{
		m_ActiveData = activeData;
		m_Output->Reset();
	}

	AnimationGraph& AnimationGraph::operator=(const AnimationGraph& other)
	{
		if (this == &other)
			return *this;

		m_Skeleton = other.m_Skeleton;
		m_Output = other.m_Output;
		m_DefaultParameters = other.m_DefaultParameters;
		m_ActiveData = other.m_ActiveData;
		m_RegisteredNodes = other.m_RegisteredNodes;
		m_CompileID = other.m_CompileID;

		for (const auto& node : m_RegisteredNodes)
			node->m_AnimationGraph = this;

		return *this;
	}

}