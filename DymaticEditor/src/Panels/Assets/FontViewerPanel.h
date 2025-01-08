#pragma once
#include "Panels/EditorPanel.h"

#include "Dymatic/Renderer/Font.h"

#include "Dymatic/Renderer/Framebuffer.h"
#include "Dymatic/Renderer/Renderer.h"
#include "Dymatic/Renderer/Renderer2D.h"

namespace Dymatic {

	class FontViewerPanel : public EditorPanel
	{
	public:
		FontViewerPanel(const AssetHandle handle);

		virtual void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender(bool& open) override;

		inline virtual void Focus() override { m_Focus = true; }

	private:
		Ref<Font> m_Font;

		// Render Resources
		Ref<Framebuffer> m_Framebuffer = nullptr;
		SceneCamera m_Camera;
		glm::mat4 m_CameraTransform;
		glm::vec2 m_ViewportSize = glm::vec2(512.0f, 512.0f);
		bool m_Invalidate = false;
		std::unordered_map<uint32_t, Ref<Texture2D>> m_GlyphThumbnails;

		// Panel properties
		bool m_Focus = false;
		float m_Scale = 1.0f;
		int m_TabBarIndex = 0;
		std::string m_PreviewInputBuffer;
	};

}