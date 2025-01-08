#include "Thumbnails/ThumbnailManager.h"

#include "Thumbnails/ThumbnailGenerators.h"

#include "Dymatic/Project/Project.h"
#include "Dymatic/Core/Filesystem.h"
#include "Dymatic/Core/FileStream.h"

#include <deque>

namespace Dymatic {

	static const std::filesystem::path s_ThumbnailCacheDirectory = "Thumbnail";
	static std::deque<AssetHandle> s_ThumbnailGenerationQueue;
	static std::unordered_map<AssetHandle, Ref<Texture2D>> s_ThumbnailMap;
	static std::unordered_map<AssetType, Scope<ThumbnailGenerator>> s_ThumbnailGenerators;

	namespace Utils {
	
		static std::filesystem::path GetThumbnailCachePath(AssetHandle handle)
		{
			return Project::GetCacheDirectory() / s_ThumbnailCacheDirectory / fmt::format("{}.thumb", handle);
		}
		
	}

	void ThumbnailManager::Init()
	{
		s_ThumbnailGenerators[AssetType::Texture] = CreateScope<TextureThumbnailGenerator>();
		s_ThumbnailGenerators[AssetType::Font] = CreateScope<FontThumbnailGenerator>();
		s_ThumbnailGenerators[AssetType::Video] = CreateScope<VideoThumbnailGenerator>();
		s_ThumbnailGenerators[AssetType::Material] = CreateScope<MaterialThumbnailGenerator>();
		s_ThumbnailGenerators[AssetType::Mesh] = CreateScope<MeshThumbnailGenerator>();
	}

	void ThumbnailManager::UpdateProject()
	{
		// Create the cache directory
		FileSystem::CreateDirectory(Project::GetCacheDirectory() / s_ThumbnailCacheDirectory);

		// Clear the current thumbnail memory cache
		s_ThumbnailMap.clear();
		s_ThumbnailGenerationQueue.clear();
	}

	void ThumbnailManager::OnUpdate()
	{
		// Generate one thumbnail per frame from the queue
		if (s_ThumbnailGenerationQueue.empty())
			return;

		// Pop off the handle of the asset we are creating a thumbnail for
		AssetHandle handle = s_ThumbnailGenerationQueue.front();
		s_ThumbnailGenerationQueue.pop_front();

		DY_CORE_TRACE("Generating thumbnail for asset '{}'", handle);

		const auto& metadata = AssetManager::GetMetadata(handle);
		const AssetType assetType = metadata.Type;

		// Check if we have a generator for this type of asset
		if (s_ThumbnailGenerators.find(assetType) == s_ThumbnailGenerators.end())
			return;

		// Check if we can hit the cache, and load if so
		const std::filesystem::path cachePath = Utils::GetThumbnailCachePath(handle);
		if (FileSystem::Exists(cachePath) && std::filesystem::file_size(cachePath) != 0)
		{
			// Read in the cache data
			FileStreamReader reader(cachePath);

			// Check if the thumbnail is still valid
			const uint64_t lastWriteTime = std::chrono::system_clock::to_time_t(std::chrono::time_point_cast<std::chrono::system_clock::duration>(std::filesystem::last_write_time(AssetManager::GetFileSystemPathString(metadata)) - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()));
			uint64_t thumbnailTimestamp;
			reader.ReadRaw<uint64_t>(thumbnailTimestamp);

			if (lastWriteTime <= thumbnailTimestamp)
			{
				DY_CORE_INFO("Requesting thumbnail cache for asset '{}'", handle);

				TextureSpecification specification;

				reader.ReadRaw<uint32_t>(specification.Width);
				reader.ReadRaw<uint32_t>(specification.Height);

				Buffer imageData;
				reader.ReadBuffer(imageData);

				// Create the texture
				s_ThumbnailMap[handle] = Texture2D::Create(specification, imageData);

				return;
			}
		}

		// Otherwise we will attempt to generate the thumbnail.
		DY_CORE_INFO("Generating thumbnail for asset '{}'", handle);
		Ref<Texture2D> thumbnail = s_ThumbnailGenerators[assetType]->GenerateThumbnail(handle);
		if (thumbnail)
		{
			DY_CORE_INFO("Caching thumbnail for asset '{}'", handle);

			// Save the thumbnail to the cache directory
			FileStreamWriter writer(cachePath);
			writer.WriteRaw<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
			writer.WriteRaw<uint32_t>(thumbnail->GetWidth());
			writer.WriteRaw<uint32_t>(thumbnail->GetHeight());
			writer.WriteBuffer(thumbnail->GetData());

			// Store the thumbnail in memory for immediate usage.
			s_ThumbnailMap[handle] = thumbnail;
		}
	}

	Ref<Texture2D> ThumbnailManager::GetOrCreateThumbnail(AssetHandle handle)
	{
		if (s_ThumbnailMap.find(handle) != s_ThumbnailMap.end())
			return s_ThumbnailMap.at(handle);

		// Ensure we actually have a generator for the thumbnail
		if (s_ThumbnailGenerators.find(AssetManager::GetMetadata(handle).Type) == s_ThumbnailGenerators.end())
			return nullptr;

		// Only push the thumbnail to be generated if it hasn't been queued yet
		if (std::find(s_ThumbnailGenerationQueue.begin(), s_ThumbnailGenerationQueue.end(), handle) == s_ThumbnailGenerationQueue.end())
		{
			DY_CORE_INFO("Queueing thumbnail generation for asset '{}'", handle);
			s_ThumbnailGenerationQueue.push_back(handle);
		}
		
		return nullptr;
	}

	void ThumbnailManager::InvalidateThumbnail(AssetHandle handle)
	{
		// Delete the cache on disk if it exists
		const std::filesystem::path cachePath = Utils::GetThumbnailCachePath(handle);
		if (FileSystem::Exists(cachePath))
			FileSystem::DeleteFile(cachePath);

		// Erase any in-memory reference to the thumbnail
		if (s_ThumbnailMap.find(handle) != s_ThumbnailMap.end())
			s_ThumbnailMap.erase(handle);
	}

}