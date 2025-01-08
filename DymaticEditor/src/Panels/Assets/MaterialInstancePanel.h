#pragma once
#include "Panels/EditorPanel.h"

#include "Dymatic/Renderer/MaterialAsset.h"
#include "Panels/Assets/Utils/MaterialPanelViewport.h"

namespace Dymatic {

	class MaterialInstancePanel : public EditorPanel
	{
	public:
		MaterialInstancePanel(Ref<MaterialInstance> materialInstance);

		virtual void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender(bool& open) override;

		virtual void Focus() override;

	private:
		bool m_Focused = false;
		Ref<MaterialInstance> m_MaterialInstance;
		MaterialPanelViewport m_Viewport;
	};

}