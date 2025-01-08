#pragma once
#include "Dymatic/Asset/AssetSerializer.h"
#include "Dymatic/Video/VideoReader.h"

#include "Dymatic/Asset/Serializers/Utils/SerializerUtils.h"

namespace Dymatic {

	class VideoPlayerSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override
		{
			const Ref<VideoReader> player = As<VideoReader>(asset);

			const Ref<Video> source = player->GetSource();
			const Ref<Subtitle> subtitles = player->GetSubtitles();
			const Ref<Texture2D> target = player->GetTarget();

			YAML::Emitter out;
			out << YAML::BeginMap << YAML::Key << "Video Player" << YAML::Value << YAML::BeginMap;

			out << YAML::Key << "Video" << YAML::Value << (source ? source->Handle : 0);
			out << YAML::Key << "Subtitles" << YAML::Value << (subtitles ? subtitles->Handle : 0);
			out << YAML::Key << "Target" << YAML::Value << (target ? target->Handle : 0);

			out << YAML::EndMap; // Video Player
			out << YAML::EndMap;

			std::ofstream fout(Project::GetAssetFileSystemPath(metadata.FilePath));
			fout << out.c_str();
		}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset, bool multithreaded) const override
		{
			YAML::Node data;
			if (!Utils::TryLoadYAMLFromFile(metadata, data))
				return false;

			auto videoPlayerNode = data["Video Player"];
			if (!videoPlayerNode)
				return false;

			VideoReaderSpecification specification;
			specification.VideoStream = AssetManager::GetAsset<Video>(videoPlayerNode["Video"].as<AssetHandle>());
			specification.SubtitleStream = AssetManager::GetAsset<Subtitle>(videoPlayerNode["Subtitles"].as<AssetHandle>());
			specification.Target = AssetManager::GetAsset<Texture2D>(videoPlayerNode["Target"].as<AssetHandle>());
			const Ref<VideoReader> player = CreateRef<VideoReader>(specification);
			
			asset = player;
			return true;
		}

		virtual bool SerializeToAssetPack(const AssetMetadata& metadata, const AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const override
		{
			const Ref<VideoReader> player = AssetManager::GetAsset<VideoReader>(handle);

			if (!player)
			{
				DY_CORE_WARN("Failed to load Video Player {}. Excluding from asset pack...", handle);
				return false;
			}

			const Ref<Video> source = player->GetSource();
			const Ref<Subtitle> subtitles = player->GetSubtitles();
			const Ref<Texture2D> target = player->GetTarget();

			stream.WriteRaw<AssetHandle>(source ? source->Handle : 0);
			stream.WriteRaw<AssetHandle>(subtitles ? subtitles->Handle : 0);
			stream.WriteRaw<AssetHandle>(target ? target->Handle : 0);

			return true;
		}

		virtual bool DeserializeFromAssetPack(const AssetMetadata& metadata, Ref<Asset>& asset, FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const override
		{
			VideoReaderSpecification specification;
			specification.VideoStream = AssetManager::GetAsset<Video>(stream.ReadRaw<AssetHandle>());
			specification.SubtitleStream = AssetManager::GetAsset<Subtitle>(stream.ReadRaw<AssetHandle>());
			specification.Target = AssetManager::GetAsset<Texture2D>(stream.ReadRaw<AssetHandle>());
			const Ref<VideoReader> player = CreateRef<VideoReader>(specification);

			asset = player;
			return true;
		}
	};

}