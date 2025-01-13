#include "dypch.h"
#include "Dymatic/Asset/Serializers/PrefabSerializer.h"

#include "Dymatic/Asset/AssetManager.h"
#include "Dymatic/Asset/Serializers/EntityRegistrySerializer.h"

#include "Dymatic/Scene/Prefab.h"
#include "Dymatic/Project/Project.h"

namespace Dymatic {

	void PrefabSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<Prefab> prefab = As<Prefab>(asset);

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Prefab" << YAML::Value << prefab->Handle;

		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

		EntityRegistrySerializer::SerializeRegistry(out, prefab);

		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(Project::GetAssetDirectory() / metadata.FilePath);
		fout << out.c_str();
	}
	
	bool PrefabSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset, bool multithreaded) const
	{
		asset = nullptr;

		YAML::Node data;
		try
		{
			data = YAML::LoadFile((Project::GetAssetDirectory() / metadata.FilePath).string());
		}
		catch (YAML::ParserException e)
		{
			DY_CORE_ERROR("Failed to load prefab file '{}'\n     {}", metadata.FilePath, e.what());
			return false;
		}

		auto prefabNode = data["Prefab"];
		if (!prefabNode)
			return false;

		Ref<Prefab> prefab = Prefab::Create();
		asset = prefab;
		
		DY_CORE_TRACE("Deserializing prefab '{}'", prefabNode.as<uint64_t>());

		auto entities = data["Entities"];
		if (entities)
			EntityRegistrySerializer::DeserializeRegistry(entities, prefab);

		return true;
	}

	bool PrefabSerializer::SerializeToAssetPack(const AssetMetadata& metadata, const AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(handle);

		if (!prefab)
			return false;

		// Entity Registry
		EntityRegistrySerializer::SerializeRegistry(prefab, stream);

		return true;
	}
	
	bool PrefabSerializer::DeserializeFromAssetPack(const AssetMetadata& metadata, Ref<Asset>& asset, FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		Ref<Prefab> prefab = Prefab::Create();
		asset = prefab;

		// Entity Registry
		EntityRegistrySerializer::DeserializeRegistry(stream, prefab);

		return true;
	}

}