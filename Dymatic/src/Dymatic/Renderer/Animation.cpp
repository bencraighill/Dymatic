#include "dypch.h"
#include "Dymatic/Renderer/Animation.h"

#include "Dymatic/Renderer/AssimpGLMHelpers.h"

namespace Dymatic {

	Animation::Animation(const std::string& animationPath, Ref<Model> model)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);
		DY_CORE_ASSERT(scene && scene->mRootNode);
		aiAnimation* animation = scene->mAnimations[0];
		m_Duration = animation->mDuration;
		m_TicksPerSecond = animation->mTicksPerSecond;
		aiMatrix4x4 globalTransformation = scene->mRootNode->mTransformation;
		globalTransformation = globalTransformation.Inverse();
		ReadHeirarchyData(m_RootNode, scene->mRootNode);
		ReadMissingBones(animation, model);

		// Decode mesh channels
		for (uint32_t i = 0; i < animation->mNumMeshChannels; i++)
		{
			aiMeshAnim* meshChannel = animation->mMeshChannels[i];
			DY_CORE_INFO("Decoding mesh channel '{}'", meshChannel->mName.data);
			for (uint32_t j = 0; j < meshChannel->mNumKeys; j++)
			{
				aiMeshKey key = meshChannel->mKeys[j];
				DY_CORE_INFO("	Time: {}, Value: {}", key.mTime, key.mValue);
			}
		}

		m_IsLoaded = true;
	}

	Ref<Bone> Animation::FindBone(const std::string& name)
	{
		for (auto& bone : m_Bones)
		{
			if (bone->GetBoneName() == name)
				return bone;
		}
		return nullptr;
	}

	void Animation::ReadMissingBones(const aiAnimation* animation, Ref<Model> model)
	{
		uint32_t size = animation->mNumChannels;

		auto& boneInfoMap = model->GetBoneInfoMap();
		uint32_t& boneCount = model->GetBoneCount();

		//reading channels(bones engaged in an animation and their key frames)
		for (uint32_t i = 0; i < size; i++)
		{
			const aiNodeAnim* channel = animation->mChannels[i];
			const std::string boneName = channel->mNodeName.data;
			

			if (boneInfoMap.find(boneName) == boneInfoMap.end())
			{
				boneInfoMap[boneName].id = boneCount;
				boneCount++;
			}
			m_Bones.push_back(Bone::Create(channel->mNodeName.data, boneInfoMap[channel->mNodeName.data].id, channel));
		}

		m_BoneInfoMap = boneInfoMap;
	}

	void Animation::ReadHeirarchyData(BoneNodeData& dest, const aiNode* src)
	{
		DY_CORE_ASSERT(src);

		dest.Name = src->mName.data;
		dest.Transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);

		dest.Children.resize(src->mNumChildren);
		for (uint32_t i = 0; i < src->mNumChildren; i++)
		{
			BoneNodeData newData;
			ReadHeirarchyData(newData, src->mChildren[i]);
			dest.Children[i] = newData;
		}
	}

}