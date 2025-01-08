#pragma once
#include "Dymatic/Core/UUID.h"
#include "Dymatic/Asset/AssetHandle.h"

namespace Dymatic {

	enum class AssetType : uint16_t
	{
		None = 0,
		Scene = 1,
		Prefab = 2,
		MeshSource = 3,
		Mesh = 4,
		Material = 5,
		Texture = 6,
		EnvironmentMap = 7,
		VirtualTexture = 8,
		Font = 9,
		Audio = 10,
		ParticleSystem = 11,
		Skeleton = 12,
		Animation = 13,
		AnimationGraph = 14,
		PhysicsMaterial = 15,
		Video = 16,
		Subtitle = 17,
		VideoPlayer = 18,
		ASSET_TYPE_SIZE
	};

	class Asset
	{
	public:
		AssetHandle Handle;
		virtual AssetType GetAssetType() const = 0;
	};
	
}