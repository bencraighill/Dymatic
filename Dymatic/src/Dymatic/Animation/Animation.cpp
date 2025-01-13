#include "dypch.h"
#include "Dymatic/Animation/Animation.h"

#include "Dymatic/Asset/AssetManager.h"

#include "Dymatic/Renderer/Utils/AssimpGLMHelpers.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Dymatic {

	Animation::Animation(const AssetHandle sourceHandle, uint32_t animationIndex, Ref<Skeleton> skeleton)
		: m_SourceHandle(sourceHandle), m_AnimationIndex(animationIndex), m_Skeleton(skeleton)
	{
		const std::filesystem::path filepath = AssetManager::GetFileSystemPathString(AssetManager::GetMetadata(sourceHandle));

		Assimp::Importer importer;
		importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 100.0f);
		const aiScene* scene = importer.ReadFile(filepath.string(), aiProcess_GlobalScale | aiProcess_Triangulate);
		
		DY_CORE_ASSERT(scene && scene->mRootNode);
		
		aiAnimation* animation = scene->mAnimations[0];
		m_Duration = animation->mDuration;
		m_TicksPerSecond = animation->mTicksPerSecond;

		// Note: We can use this for root motion?
		aiMatrix4x4 globalTransformation = scene->mRootNode->mTransformation;
		globalTransformation = globalTransformation.Inverse();

		//ReadHeirarchyData(m_RootNode, scene->mRootNode);
		ReadMissingBones(animation);

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

	Animation::Animation(Ref<Skeleton> skeleton, const float duration, const uint32_t ticksPerSecond, const std::unordered_map<std::string, Ref<Bone>>& bones)
		: m_Skeleton(skeleton), m_Duration(duration), m_TicksPerSecond(ticksPerSecond), m_Bones(bones)
	{
		m_IsLoaded = true;
	}

	Ref<Bone> Animation::FindBone(const std::string& name)
	{
		if (m_Bones.find(name) == m_Bones.end())
			return nullptr;

		return m_Bones.at(name);
	}

	void Animation::ReadMissingBones(const aiAnimation* animation)
	{
		const auto& boneInfoMap = m_Skeleton->GetBoneInfoMap();

		// Reading channels (bones engaged in an animation and their key frames)
		for (uint32_t i = 0; i < animation->mNumChannels; i++)
		{
			const aiNodeAnim* channel = animation->mChannels[i];
			const std::string boneName = channel->mNodeName.data;

			// Only add bones if they exist in the skeleton
			if (boneInfoMap.find(boneName) != boneInfoMap.end())
				m_Bones[boneName] = Bone::Create(channel->mNodeName.data, boneInfoMap.at(channel->mNodeName.data).id, channel);
			else
				DY_CORE_WARN("Bone '{}' in animation does not exist on target skeleton {}!", channel->mNodeName.data, m_Skeleton->Handle);
		}
	}

#if 0
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
#endif

}