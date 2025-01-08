#pragma once
#include "Dymatic/Asset/AssetSerializer.h"
#include "Dymatic/Renderer/Texture.h"

#include "Dymatic/Renderer/ImageLoader.h"

#include <stb_image.h>
#include <queue>

namespace Dymatic {

	class TextureSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override {}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset, bool multithreaded) const override
		{
			const std::string filepath = AssetManager::GetFileSystemPathString(metadata);
			bool result = true;

			if (multithreaded)
			{
				int width, height, channels;
				TextureFormat format = Utils::GetImageFileInfo(TextureFormat::None, filepath.c_str(), &width, &height, &channels, true);

				TextureSpecification spec;
				spec.Width = width;
				spec.Height = height;
				spec.Format = format;
				spec.UseFileChannels = false;

				const size_t dataSize = width * height * channels;
				Buffer buffer = Buffer(dataSize);
				memset(buffer.Data, 1, dataSize);
				Ref<Texture2D> texture = Texture2D::Create(spec, buffer);
				buffer.Release();

				AssetThread::QueueWork(
				{
					[texture, filepath, format, dataSize]()
					{
						// Asset Thread
						int x, y, channels;
						void* data = Utils::LoadImageFile(format, filepath.c_str(), &x, &y, &channels, true);
						texture->StageData(Buffer(data, dataSize));
					},
					[texture]()
					{
						// Main Thread
						texture->UploadData();
					}
				});

				asset = texture;
			}
			else
			{
				asset = Texture2D::Create(AssetManager::GetFileSystemPathString(metadata));
				result = As<Texture2D>(asset)->IsLoaded();
			}

			//if (!result)
			//	asset->SetFlag(AssetFlag::Invalid, true);

			return result;
		}

		virtual bool SerializeToAssetPack(const AssetMetadata& metadata, const AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const override
		{
			// TODO: We should use a library like NVTT3 that utilizes the GPU to load and compress textures extremely optimally for the target platform

			// Locate the asset file and write the raw file data to the asset pack
			Buffer fileData = FileSystem::ReadBytes(AssetManager::GetFileSystemPathString(metadata));
			stream.WriteBuffer(fileData);
			fileData.Release();
			
			return true;
		}

		virtual bool DeserializeFromAssetPack(const AssetMetadata& metadata, Ref<Asset>& asset, FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const override
		{
			ScopedBuffer fileData;
			stream.ReadBuffer(fileData);

			// Note: Here we use the FILE data constructor (so stbi still does decompression at runtime directly in memory)
			Ref<Texture2D> texture = Texture2D::Create(fileData);
			asset = texture;
			return true;
		}
	};

}