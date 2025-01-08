#pragma once

#include "Dymatic/Asset/AssetSerializer.h"
#include "Dymatic/Renderer/Skeleton.h"

namespace Dymatic {

	class SkeletonSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override
		{
			YAML::Emitter out;
			out << YAML::BeginMap << YAML::Key << "Skeleton" << YAML::Value << YAML::BeginMap;

			out << YAML::Key << "Source" << YAML::Value << As<Skeleton>(asset)->GetSourceHandle();

			out << YAML::EndMap; // Skeleton
			out << YAML::EndMap;

			std::ofstream fout(Project::GetAssetFileSystemPath(metadata.FilePath));
			fout << out.c_str();

			return;
		}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset, bool multithreaded) const override
		{
			const std::filesystem::path filepath = AssetManager::GetFileSystemPathString(metadata);
			if (!std::filesystem::exists(filepath))
			{
				DY_CORE_ERROR("Skeleton file '{}' does not exist", metadata.FilePath.string());
				return false;
			}

			YAML::Node data;
			try
			{
				data = YAML::LoadFile(filepath.string());
			}
			catch (YAML::ParserException e)
			{
				DY_CORE_ERROR("Failed to load skeleton file '{}'\n     {}", metadata.FilePath.string(), e.what());
				return false;
			}

			if (auto skeletonNode = data["Skeleton"])
			{
				AssetHandle sourceHandle = skeletonNode["Source"].as<AssetHandle>();

				asset = Skeleton::Create(sourceHandle);

				bool result = As<Skeleton>(asset)->IsLoaded();

				//if (!result)
				//	asset->SetFlag(AssetFlag::Invalid, true);

				return result;
			}

			return false;
		}

		static void SerializeSkeletonNode(const BoneNodeData& node, FileStreamWriter& stream)
		{
			stream.WriteString(node.Name);
			stream.WriteRaw<glm::mat4>(node.Transformation);

			// Write children
			stream.WriteRaw<uint32_t>(node.Children.size());
			for (const auto& child : node.Children)
				SerializeSkeletonNode(child, stream);
		}

		virtual bool SerializeToAssetPack(const AssetMetadata& metadata, const AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const override
		{
			Ref<Skeleton> skeleton = AssetManager::GetAsset<Skeleton>(handle);

			if (!skeleton)
				return false;

			// Skeleton Hierarchy
			const auto& root = skeleton->GetRootNode();
			SerializeSkeletonNode(root, stream);
			
			// Bone Info Map
			const auto& boneInfoMap = skeleton->GetBoneInfoMap();
			stream.WriteRaw<uint32_t>(boneInfoMap.size());
			for (const auto& [name, boneInfo] : boneInfoMap)
			{
				stream.WriteString(name);
				stream.WriteRaw<int>(boneInfo.id);
				stream.WriteRaw<glm::mat4>(boneInfo.offset);
			}

			return true;
		}

		static void DeserializeSkeletonNode(BoneNodeData& node, FileStreamReader& stream)
		{
			stream.ReadString(node.Name);
			stream.ReadRaw<glm::mat4>(node.Transformation);

			// Read Children
			const uint32_t childCount = stream.ReadRaw<uint32_t>();
			node.Children.resize(childCount);
			for (auto& child : node.Children)
				DeserializeSkeletonNode(child, stream);
		}

		virtual bool DeserializeFromAssetPack(const AssetMetadata& metadata, Ref<Asset>& asset, FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const override
		{
			// Skeleton Hierarchy
			BoneNodeData rootNode;
			DeserializeSkeletonNode(rootNode, stream);

			// Bone Info Map
			const uint32_t boneInfoCount = stream.ReadRaw<uint32_t>();
			std::unordered_map<std::string, BoneInfo> boneInfoMap(boneInfoCount);
			for (uint32_t boneInfoIndex = 0; boneInfoIndex < boneInfoCount; boneInfoIndex++)
			{
				std::string name;
				stream.ReadString(name);

				auto& boneInfo = boneInfoMap[name];
				stream.ReadRaw<int>(boneInfo.id);
				stream.ReadRaw<glm::mat4>(boneInfo.offset);
			}

			Ref<Skeleton> skeleton = Skeleton::Create(rootNode, boneInfoMap);
			asset = skeleton;

			return true;
		}
	};

}