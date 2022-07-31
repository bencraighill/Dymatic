#include "dypch.h"
#include "Dymatic/Renderer/Animation.h"

#include "Dymatic/Renderer/AssimpGLMHelpers.h"

namespace Dymatic {

	Animation::Animation(const std::string& animationPath, Ref<Model> model)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);
		assert(scene && scene->mRootNode);
		auto animation = scene->mAnimations[0];
		m_Duration = animation->mDuration;
		m_TicksPerSecond = animation->mTicksPerSecond;
		aiMatrix4x4 globalTransformation = scene->mRootNode->mTransformation;
		globalTransformation = globalTransformation.Inverse();
		ReadHeirarchyData(m_RootNode, scene->mRootNode);
		ReadMissingBones(animation, model);

		m_IsLoaded = true;
	}

	Ref<Bone> Animation::FindBone(const std::string& name)
	{
		auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
			[&](const Ref<Bone> Bone)
			{
				return Bone->GetBoneName() == name;
			}
		);
		if (iter == m_Bones.end()) return nullptr;
		else return *iter;
	}

	void Animation::ReadMissingBones(const aiAnimation* animation, Ref<Model> model)
	{
		int size = animation->mNumChannels;

		auto& boneInfoMap = model->GetBoneInfoMap();
		int& boneCount = model->GetBoneCount();

		//reading channels(bones engaged in an animation and their keyframes)
		for (int i = 0; i < size; i++)
		{
			auto channel = animation->mChannels[i];
			std::string boneName = channel->mNodeName.data;

			if (boneInfoMap.find(boneName) == boneInfoMap.end())
			{
				boneInfoMap[boneName].id = boneCount;
				boneCount++;
			}
			m_Bones.push_back(Bone::Create(channel->mNodeName.data, boneInfoMap[channel->mNodeName.data].id, channel));
		}

		m_BoneInfoMap = boneInfoMap;
	}

	void Animation::ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src)
	{
		DY_CORE_ASSERT(src);

		dest.name = src->mName.data;
		dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
		dest.childrenCount = src->mNumChildren;

		for (int i = 0; i < src->mNumChildren; i++)
		{
			AssimpNodeData newData;
			ReadHeirarchyData(newData, src->mChildren[i]);
			dest.children.push_back(newData);
		}
	}

}