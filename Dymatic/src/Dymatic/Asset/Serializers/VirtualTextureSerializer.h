#pragma once

#include "Dymatic/Renderer/Texture.h"
#include "Dymatic/Asset/AssetSerializer.h"

namespace Dymatic {

	class VirtualTextureSerializer : public AssetSerializer
	{
	public:
		static Ref<Texture2D> CreateVirtualTexture(const TextureSpecification& specification)
		{
			Buffer data = Buffer(specification.Width * specification.Height * Utils::GetDymaticTextureFormatBPP(specification.Format));
			data.ZeroInitialize();
			Ref<Texture2D> texture = Texture2D::Create(specification, data);
			data.Release();

			return texture;
		}

		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override
		{
			const Ref<Texture2D> texture = As<Texture2D>(asset);
			auto& specification = texture->GetSpecification();

			YAML::Emitter out;
			out << YAML::BeginMap << YAML::Key << "Virtual Texture";
			out << YAML::BeginMap; // Virtual Texture

			out << YAML::Key << "Format" << Utils::TextureFormatToString(specification.Format);
			out << YAML::Key << "Width" << specification.Width;
			out << YAML::Key << "Height" << specification.Height;

			out << YAML::EndMap; // Virtual Texture
			out << YAML::EndMap;

			std::ofstream fout(Project::GetAssetFileSystemPath(metadata.FilePath));
			fout << out.c_str();
		}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset, bool multithreaded) const override
		{
			YAML::Node data;
			if (!Utils::TryLoadYAMLFromFile(metadata, data))
				return false;

			auto virtualTextureNode = data["Virtual Texture"];
			if (!virtualTextureNode)
				return false;

			TextureSpecification specification;
			specification.Format = Utils::TextureFormatFromString(virtualTextureNode["Format"].as<std::string>());
			specification.Width = virtualTextureNode["Width"].as<uint32_t>();
			specification.Height = virtualTextureNode["Height"].as<uint32_t>();

			asset = CreateVirtualTexture(specification);
			return true;
		}

		virtual bool SerializeToAssetPack(const AssetMetadata& metadata, const AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const override
		{
			const Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(handle);

			if (!texture)
				return false;

			auto& specifiation = texture->GetSpecification();

			stream.WriteRaw<uint8_t>((uint8_t)specifiation.Format);
			stream.WriteRaw<uint32_t>(specifiation.Width);
			stream.WriteRaw<uint32_t>(specifiation.Height);

			return true;
		}

		virtual bool DeserializeFromAssetPack(const AssetMetadata& metadata, Ref<Asset>& asset, FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const override
		{
			TextureSpecification specification;
			specification.Format = (TextureFormat)stream.ReadRaw<uint8_t>();
			stream.ReadRaw<uint32_t>(specification.Width);
			stream.ReadRaw<uint32_t>(specification.Height);

			asset = CreateVirtualTexture(specification);
			return true;
		}
	};

}