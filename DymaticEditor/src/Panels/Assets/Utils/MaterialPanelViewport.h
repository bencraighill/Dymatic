#pragma once

#include "Panels/Assets/Utils/AssetPanelViewport.h"
#include "Dymatic/Renderer/MaterialAsset.h"

namespace Dymatic {

	class MaterialPanelViewport : public AssetPanelViewport
	{
	public:
		MaterialPanelViewport();
		void BeginViewportRender(Timestep ts, const Ref<MaterialAsset> material);
	};

}