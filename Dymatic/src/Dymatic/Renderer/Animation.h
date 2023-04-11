#pragma once

#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <assimp/scene.h>

#include <functional>

#include "Dymatic/Renderer/Bone.h"
#include "Dymatic/Renderer/AnimationData.h"
#include "Dymatic/Renderer/Model.h"

namespace Dymatic {

	struct BoneNodeData
	{
		glm::mat4 Transformation;
		std::string Name;
		std::vector<BoneNodeData> Children;
	};

	class Animation
	{
	public:
		static Ref<Animation> Create() { return CreateRef<Animation>(); }
		static Ref<Animation> Create(const std::string& animationPath, Ref<Model> model) { return CreateRef<Animation>(animationPath, model); }

		Animation() = default;
		Animation(const std::string& animationPath, Ref<Model> model);
		~Animation() {}

		Ref<Bone> FindBone(const std::string& name);

		inline float GetTicksPerSecond() { return m_TicksPerSecond; }
		inline float GetDuration() { return m_Duration; }
		inline const BoneNodeData& GetRootNode() { return m_RootNode; }
		inline std::map<std::string, BoneInfo>& GetBoneIDMap() { return m_BoneInfoMap; }

		inline bool IsLoaded() { return m_IsLoaded; }

	private:
		void ReadMissingBones(const aiAnimation* animation, Ref<Model> model);
		void ReadHeirarchyData(BoneNodeData& dest, const aiNode* src);

	private:
		float m_Duration;
		uint32_t m_TicksPerSecond;
		std::vector<Ref<Bone>> m_Bones;
		BoneNodeData m_RootNode;
		std::map<std::string, BoneInfo> m_BoneInfoMap;

		bool m_IsLoaded = false;
	};

}