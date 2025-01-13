#include "dypch.h"
#include "Dymatic/Animation/AnimationGraphPlayer.h"

namespace Dymatic {

	AnimationGraphPlayer::AnimationGraphPlayer(Ref<Model> model, Ref<AnimationGraph> animationGraph)
		: m_Model(model), m_AnimationGraph(animationGraph)
	{
		m_FinalBoneMatrices = CreateRef<BoneMatrixList>();
		m_BlendShapeWeights = CreateRef<BlendShapeWeightList>(m_Model->GetBlendShapes().size(), 0.0f);
		CheckCompilation();
	}

	void AnimationGraphPlayer::CheckCompilation()
	{
		const UUID compileID = m_AnimationGraph->GetCompileID();
		if (compileID == m_CompileID)
			return;

		m_CompileID = compileID;
		Ref<AnimationGraphInstanceData> originalData = m_Data;
		m_Data = CreateRef<AnimationGraphInstanceData>(m_AnimationGraph->GetDefaultParameters());

		// Copy any parameters that existed previously to the new set (if their type remains the same)
		if (originalData)
		{
			for (const auto& [name, parameter] : originalData->Parameters.Parameters)
				if (m_Data->Parameters.Parameters.find(name) != m_Data->Parameters.Parameters.end() && m_Data->Parameters.Parameters.at(name).Type == originalData->Parameters.Parameters.at(name).Type)
					m_Data->Parameters.Parameters[name] = originalData->Parameters.Parameters.at(name);
		}

		// Trigger a reset call so everything will be setup again
		m_AnimationGraph->ResetNodes(m_Data);
	}

	void AnimationGraphPlayer::OnUpdate(Timestep ts)
	{
		if (m_Paused)
			return;

		if (!m_AnimationGraph || !m_AnimationGraph->GetOutputNode())
			return;

		// Check if a compilation update occurred
		CheckCompilation();

		// Query the animation graph asset for up to date bone matrices
		m_CurrentTime += ts;
		m_Data->DeltaTime = ts;
		const auto& finalBoneMatrices = m_AnimationGraph->GetFinalBoneMatrices(m_CurrentTime, m_Data);

		if (finalBoneMatrices.size() != m_FinalBoneMatrices->size())
		{
			m_FinalBoneMatrices->resize(finalBoneMatrices.size());
			m_GlobalBoneMatrices.resize(finalBoneMatrices.size());
		}

		// Copy the bone matrices data to the player's local buffer
		std::memcpy(m_GlobalBoneMatrices.data(), finalBoneMatrices.data(), finalBoneMatrices.size() * sizeof(glm::mat4));

		// Apply the required offset matrix
		const auto& boneInfoMap = m_AnimationGraph->GetSkeleton()->GetBoneInfoMap();
		for (const auto& [name, boneInfo] : boneInfoMap)
			m_FinalBoneMatrices->at(boneInfo.id) = m_GlobalBoneMatrices[boneInfo.id] * boneInfo.offset;
	}

	bool AnimationGraphPlayer::SetParameter(const std::string& name, const AnimationGraphData& value)
	{
		// Ensure parameter exists
		if (!DoesParameterExist(name))
			return false;

		// Check the type (ensuring it is the same)
		if (m_Data->Parameters.Parameters.at(name).Type != value.Type)
			return false;

		// Set the animation data (ensuring the type remains the same)
		m_Data->Parameters.Parameters[name] = value;
	}

	bool AnimationGraphPlayer::DoesParameterExist(const std::string& name)
	{
		if (!m_Data)
			m_Data = CreateRef<AnimationGraphInstanceData>(m_AnimationGraph->GetDefaultParameters());

		return m_Data->Parameters.Parameters.find(name) != m_Data->Parameters.Parameters.end();
	}

}
