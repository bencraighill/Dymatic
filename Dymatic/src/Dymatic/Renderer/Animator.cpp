#include "dypch.h"
#include "Dymatic/Renderer/Animator.h"

#include <glm/gtx/matrix_decompose.hpp>

namespace Dymatic {

	Animator::Animator(Ref<Animation> animation)
	{
		m_CurrentTime = 0.0;
		m_CurrentAnimation = animation;

		m_FinalBoneMatrices.reserve(100);

		for (size_t i = 0; i < 100; i++)
			m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
	}

	Animator::Animator()
	{
		m_CurrentTime = 0.0;
		m_CurrentAnimation = nullptr;

		m_FinalBoneMatrices.reserve(100);

		for (size_t i = 0; i < 100; i++)
			m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
	}

	void Animator::UpdateAnimation(float dt)
	{
		m_DeltaTime = dt;
		if (m_CurrentAnimation && !m_IsPaused)
		{
			m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
			m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
			CalculateBoneTransform(m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
		}
	}

	void Animator::SetAnimation(Ref<Animation> animation)
	{
		m_CurrentAnimation = animation;
		m_CurrentTime = 0.0f;
	}

	void Animator::CalculateBoneTransform(const BoneNodeData& node, const glm::mat4& parentTransform)
	{
		const std::string& nodeName = node.Name;

		Ref<Bone> Bone = m_CurrentAnimation->FindBone(nodeName);

		if (Bone)
			Bone->Update(m_CurrentTime);
		
		glm::mat4 nodeTransform = Bone ? Bone->GetLocalTransform() : node.Transformation;

		{
			float weight = m_CurrentTime / m_CurrentAnimation->GetDuration();

			glm::vec3 translation1, scale1;
			glm::quat rotation1;
			glm::decompose(Bone ? Bone->GetLocalTransform() : node.Transformation, scale1, rotation1, translation1, glm::vec3(0.0f), glm::vec4(0.0f));

			glm::vec3 translation2, scale2;
			glm::quat rotation2;
			glm::decompose(node.Transformation, scale2, rotation2, translation2, glm::vec3(0.0f), glm::vec4(0.0f));

			glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::mix(translation1, translation2, weight));
			transform *= glm::mat4_cast(glm::slerp(rotation1, rotation2, weight));
			transform = glm::scale(transform, glm::mix(scale1, scale2, weight));

			nodeTransform = transform;
		}

		glm::mat4 globalTransformation = parentTransform * nodeTransform;
		
		auto& boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
		if (boneInfoMap.find(nodeName) != boneInfoMap.end())
		{
			const uint32_t index = boneInfoMap[nodeName].id;
			const glm::mat4& offset = boneInfoMap[nodeName].offset;
			m_FinalBoneMatrices[index] = globalTransformation * offset;
		}

		for (auto& child : node.Children)
			CalculateBoneTransform(child, globalTransformation);
	}

}