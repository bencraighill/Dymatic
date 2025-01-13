#pragma once
#include "Dymatic/Asset/Asset.h"

namespace Dymatic {

	class PhysicsMaterial : public Asset
	{
	public:
		PhysicsMaterial() = default;

		static AssetType GetStaticType() { return AssetType::PhysicsMaterial; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
	};

}