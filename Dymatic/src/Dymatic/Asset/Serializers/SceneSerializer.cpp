#include "dypch.h"
#include "SceneSerializer.h"

#include "Dymatic/Asset/AssetManager.h"
#include "Dymatic/Asset/Serializers/EntityRegistrySerializer.h"
#include "Dymatic/Project/Project.h"

#include <fstream>

namespace Dymatic {

	void SceneSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<Scene> scene = As<Scene>(asset);
		
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << "Untitled";

		out << YAML::Key << "Settings" << YAML::Value << YAML::BeginMap;;
		{
			out << YAML::Key << "Gravity" << YAML::Value << scene->m_Gravity;
		}
		out << YAML::EndMap;

		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		
		EntityRegistrySerializer::SerializeRegistry(out, scene);
		
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(Project::GetAssetDirectory() / metadata.FilePath);
		fout << out.c_str();
	}

	bool SceneSerializer::SerializeToAssetPack(const AssetMetadata& metadata, const AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		Ref<Scene> scene = AssetManager::GetAsset<Scene>(handle);

		if (!scene)
			return false;

		// Scene Settings
		stream.WriteRaw<glm::vec3>(scene->m_Gravity);

		// Entity Registry
		EntityRegistrySerializer::SerializeRegistry(scene, stream);

		return true;
	}

	bool SceneSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset, bool multithreaded) const
	{
		asset = nullptr;

		YAML::Node data;
		try
		{
			data = YAML::LoadFile((Project::GetAssetDirectory() / metadata.FilePath).string());
		}
		catch (YAML::ParserException e)
		{
			DY_CORE_ERROR("Failed to load scene file '{}'\n     {}", metadata.FilePath, e.what());
			return false;
		}

		if (!data["Scene"])
			return false;

		Ref<Scene> scene = Scene::Create();
		asset = scene;

		std::string sceneName = data["Scene"].as<std::string>();
		DY_CORE_TRACE("Deserializing scene '{}'", sceneName);

		auto settings = data["Settings"];
		if (settings)
		{
			auto gravity = settings["Gravity"];
			if (gravity)
				scene->m_Gravity = gravity.as<glm::vec3>();
		}

		auto entities = data["Entities"];
		if (entities)
			EntityRegistrySerializer::DeserializeRegistry(entities, scene);

		return true;
	}

	bool SceneSerializer::DeserializeFromAssetPack(const AssetMetadata& metadata, Ref<Asset>& asset, FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		Ref<Scene> scene = Scene::Create();
		asset = scene;

		// Scene Settings
		stream.ReadRaw<glm::vec3>(scene->m_Gravity);

		// Entity Registry
		EntityRegistrySerializer::DeserializeRegistry(stream, scene);

		return true;
	}

}
