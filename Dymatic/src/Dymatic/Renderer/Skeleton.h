#pragma once

#include "Dymatic/Asset/Asset.h"

#include "Dymatic/Animation/AnimationData.h"

#include <unordered_map>

struct aiNode;

namespace Dymatic {

	struct BoneNodeData
	{
		glm::mat4 Transformation;
		std::string Name;
		std::vector<BoneNodeData> Children;
	};

	class Skeleton : public Asset
	{
	public:
		static Ref<Skeleton> Create(AssetHandle sourceHandle) { return CreateRef<Skeleton>(sourceHandle); }
		static Ref<Skeleton> Create(const BoneNodeData& rootNode, const std::unordered_map<std::string, BoneInfo>& boneInfoMap) { return CreateRef<Skeleton>(rootNode, boneInfoMap); }
		
		static AssetType GetStaticType() { return AssetType::Skeleton; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
		
	public:
		Skeleton(AssetHandle sourceHandle);
		Skeleton(const BoneNodeData& rootNode, const std::unordered_map<std::string, BoneInfo>& boneInfoMap);

		inline AssetHandle GetSourceHandle() const { return m_SourceHandle; }
		inline const std::unordered_map<std::string, BoneInfo>& GetBoneInfoMap() const { return m_BoneInfoMap; }
		inline uint32_t GetBoneCount() const { return m_BoneInfoMap.size(); }
		inline const BoneNodeData& GetRootNode() const { return m_RootNode; }

		bool IsValidBoneID(const int id) const;
		bool IsValidBoneName(const std::string& name) const;

		inline const BoneNodeData& GetBoneNodeData(int id) const { return *m_BoneNodeDataIDMap.at(id); }
		int GetBoneID(const std::string& boneName) const;
		int GetBoneParentID(int id) const;
		const BoneNodeData* GetBoneParent(int id);

		inline bool IsLoaded() const { return m_IsLoaded; }

	private:
		void ReadHeirarchyData(const aiNode* src, BoneNodeData* parent = nullptr);
		void GenerateBoneNodeDataIDMap(const BoneNodeData& node);

	private:
		AssetHandle m_SourceHandle;
		bool m_IsLoaded = false;

		// Note: The bone info map is NOT guaranteed to have a node that exists in the hierarchy as it only contains bones
		// actively used by meshes in the imported scene
		BoneNodeData m_RootNode;
		std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;

		// Hash map with quick access to all BoneNodeData pointers based upon ID.
		// Note: Memory lifetime is tied to asset so should be fine.
		std::unordered_map<int, const BoneNodeData*> m_BoneNodeDataIDMap;
		std::unordered_map<int, int> m_BoneParentIDMap;
	};
	
}