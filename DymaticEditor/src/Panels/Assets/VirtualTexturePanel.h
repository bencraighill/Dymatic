#pragma once
#include "Panels/EditorPanel.h"

#include "Dymatic/Renderer/Texture.h"

namespace Dymatic {

	class VirtualTexturePanel : public EditorPanel
	{
	public:
		VirtualTexturePanel(Ref<Texture2D> virtualTexture);

		virtual void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender(bool& open) override;
		inline virtual void Focus() override { m_Focused = true; }

	private:
		void OnModified();
	private:
		Ref<Texture2D> m_VirtualTexture;
		TextureSpecification m_VirtualSpecification;

		bool m_Focused = false;
	};

}