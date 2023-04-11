#pragma once

#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include "Dymatic/Renderer/Animation.h"
#include "Dymatic/Renderer/Bone.h"

namespace Dymatic {

	class Animator
	{
	public:
		static Ref<Animator> Create() { return CreateRef<Animator>(); }
		static Ref<Animator> Create(Ref<Animation> animation) { return CreateRef<Animator>(animation); }
		
		Animator();
		Animator(Ref<Animation> animation);

		void UpdateAnimation(float deltaTime);
		void SetAnimation(Ref<Animation> animation);
		void CalculateBoneTransform(const BoneNodeData& node, const glm::mat4& parentTransform);
		std::vector<glm::mat4>& GetFinalBoneMatrices() { return m_FinalBoneMatrices; }

		inline bool HasAnimation() { return m_CurrentAnimation != nullptr; }
		inline Ref<Animation> GetAnimation() { return m_CurrentAnimation; }

		inline float& GetAnimationTime() { return m_CurrentTime; }
		inline float GetAnimationDuration() { return m_CurrentAnimation->GetDuration(); }
		inline bool& GetIsPaused() { return m_IsPaused; }

	private:
		std::vector<glm::mat4> m_FinalBoneMatrices;
		Ref<Animation> m_CurrentAnimation = nullptr;
		float m_CurrentTime;
		float m_DeltaTime;
		bool m_IsPaused = false;

	};

}