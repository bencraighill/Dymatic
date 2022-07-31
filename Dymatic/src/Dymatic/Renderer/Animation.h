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

	struct AssimpNodeData
	{
		glm::mat4 transformation;
		std::string name;
		int childrenCount;
		std::vector<AssimpNodeData> children;
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
		inline const AssimpNodeData& GetRootNode() { return m_RootNode; }
		inline const std::map<std::string, BoneInfo>& GetBoneIDMap() { return m_BoneInfoMap; }

		inline bool IsLoaded() { return m_IsLoaded; }

	private:
		void ReadMissingBones(const aiAnimation* animation, Ref<Model> model);
		void ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src);

	private:
		float m_Duration;
		int m_TicksPerSecond;
		std::vector<Ref<Bone>> m_Bones;
		AssimpNodeData m_RootNode;
		std::map<std::string, BoneInfo> m_BoneInfoMap;

		bool m_IsLoaded = false;
	};

}