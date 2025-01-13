#pragma once
#include "Panels/EditorPanel.h"

#include "Dymatic/Renderer/Texture.h"

namespace Dymatic {

	class TextureViewerPanel : public EditorPanel
	{
	public:
		TextureViewerPanel(AssetHandle handle);

		virtual void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender(bool& open) override;

		virtual void Focus() override;

	private:
		Ref<Texture2D> m_Texture;
		
		bool m_Focused = false;
		float m_Zoom = 1.0f;
		bool m_RedChannel = true, m_GreenChannel = true, m_BlueChannel = true, m_AlphaChannel = true;
	};

}