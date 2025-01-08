#pragma once
#include "Dymatic/Video/MediaStream.h"

namespace Dymatic {

	class Subtitle : public MediaStream
	{
	public:
		Subtitle(const std::filesystem::path& filepath, size_t baseOffset = 0, size_t size = 0)
			: MediaStream(filepath, baseOffset, size) {}

		static AssetType GetStaticType() { return AssetType::Subtitle; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
	};

}