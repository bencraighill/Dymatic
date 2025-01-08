#pragma once

#include "Dymatic/Renderer/Texture.h"

namespace Dymatic {

	class ThumbnailManager
	{
	public:
		static void Init();
		static void UpdateProject();

		static void OnUpdate();

		static Ref<Texture2D> GetOrCreateThumbnail(AssetHandle handle);
		static void InvalidateThumbnail(AssetHandle handle);
	};

}