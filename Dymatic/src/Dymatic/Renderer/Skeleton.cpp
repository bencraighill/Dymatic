#include "dypch.h"
#include "Dymatic/Renderer/Skeleton.h"

#include "Dymatic/Asset/AssetManager.h"

#include "Dymatic/Renderer/Utils/AssimpGLMHelpers.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Dymatic {

	static size_t CountHeirarchy(const BoneNodeData& node)
	{
		size_t count = 1;

		for (const auto& child : node.Children)
		{
			count += CountHeirarchy(child);
		}

		return count;
	}

	Skeleton::Skeleton(AssetHandle sourceHandle)
		: m_SourceHandle(sourceHandle)
	{
		const std::filesystem::path filepath = AssetManager::GetFileSystemPathString(AssetManager::GetMetadata(sourceHandle));

		// Load the skeleton using assimp
		Assimp::Importer importer;
		importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 100.0f);

		// Set this importer flags to avoid garbage bones being inserted (assimp will sometimes add these!)
		importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
		importer.SetPropertyBool(AI_CONFIG_IMPORT_REMOVE_EMPTY_BONES, false);
		
		const aiScene* scene = importer.ReadFile(filepath.string(), aiProcess_GlobalScale | aiProcess_Triangulate);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			DY_CORE_ERROR("Failed to load skeleton: {}", importer.GetErrorString());
			return;
		}

		// Populate the info map with bone details from the mesh
		for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
		{
			const aiMesh* mesh = scene->mMeshes[meshIndex];
			for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++)
			{
				const aiBone* bone = mesh->mBones[boneIndex];
				const std::string boneName = bone->mName.C_Str();

				// Skip processing bones that already have been added (if multiple meshes share a skeleton)
				if (m_BoneInfoMap.find(boneName) != m_BoneInfoMap.end())
					continue;

				BoneInfo newBoneInfo;
				newBoneInfo.id = m_BoneInfoMap.size();
				newBoneInfo.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(bone->mOffsetMatrix);
				m_BoneInfoMap[boneName] = newBoneInfo;
			}
		}

		ReadHeirarchyData(scene->mRootNode);
		GenerateBoneNodeDataIDMap(m_RootNode);

		DY_CORE_INFO("Successfully loaded skeleton with {} bones from '{}'", CountHeirarchy(m_RootNode), filepath.string());

		m_IsLoaded = true;
	}

	Skeleton::Skeleton(const BoneNodeData& rootNode, const std::unordered_map<std::string, BoneInfo>& boneInfoMap)
		: m_RootNode(rootNode), m_BoneInfoMap(boneInfoMap)
	{
		GenerateBoneNodeDataIDMap(m_RootNode);
		m_IsLoaded = true;
	}

	bool Skeleton::IsValidBoneID(const int id) const
	{
		return m_BoneNodeDataIDMap.find(id) != m_BoneNodeDataIDMap.end();
	}

	bool Skeleton::IsValidBoneName(const std::string& name) const
	{
		return m_BoneInfoMap.find(name) != m_BoneInfoMap.end();
	}

	int Skeleton::GetBoneID(const std::string& boneName) const
	{
		if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
			return -1;

		return m_BoneInfoMap.at(boneName).id;
	}

	int Skeleton::GetBoneParentID(int id) const
	{
		if (m_BoneParentIDMap.find(id) == m_BoneParentIDMap.end())
			return -1;

		return m_BoneParentIDMap.at(id);
	}

	const BoneNodeData* Skeleton::GetBoneParent(int id)
	{
		int parentID = GetBoneParentID(id);

		if (parentID == -1)
			return nullptr;

		return m_BoneNodeDataIDMap.at(parentID);
	}

	void Skeleton::ReadHeirarchyData(const aiNode* src, BoneNodeData* parent)
	{
		DY_CORE_ASSERT(src);

		const std::string name = src->mName.data;

		// Check if we actually have a bone
		BoneNodeData* nextParent = parent;
		if (m_BoneInfoMap.find(name) != m_BoneInfoMap.end())
		{
			BoneNodeData& dest = parent ? parent->Children.emplace_back() : m_RootNode;
			dest.Name = name;
			dest.Transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);

			nextParent = &dest;
		}

		for (uint32_t i = 0; i < src->mNumChildren; i++)
			ReadHeirarchyData(src->mChildren[i], nextParent);
	}

	void Skeleton::GenerateBoneNodeDataIDMap(const BoneNodeData& node)
	{
		if (m_BoneInfoMap.find(node.Name) == m_BoneInfoMap.end())
			return;

		m_BoneNodeDataIDMap[m_BoneInfoMap.at(node.Name).id] = &node;

		for (const auto& child : node.Children)
		{
			m_BoneParentIDMap[m_BoneInfoMap.at(child.Name).id] = m_BoneInfoMap.at(node.Name).id;
			GenerateBoneNodeDataIDMap(child);
		}
	}

}