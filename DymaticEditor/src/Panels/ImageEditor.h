#pragma once
#include <Dymatic.h>
#include "Dymatic/Core/Base.h"

namespace Dymatic {

	enum ToolMode
	{
		Move,
		Brush,
		Hand
	};

	struct ImageLayer
	{
		Ref<Texture2D> Texture;
		UUID Handle;

		ImageLayer(const uint32_t width, const uint32_t height);
	};

	class ImageEditor
	{
	public:
		ImageEditor();

		void OnEvent(Event& e);
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		bool OnMouseButtonReleased(MouseButtonReleasedEvent& e);
		bool OnMouseScrolled(MouseScrolledEvent& e);

		void OnImGuiRender();
	private:
		void OnUpdate();
		void DispatchImageShader(Ref<Shader> shader);

	private:
		glm::vec2 m_CanvasMin = glm::vec2(0.0f, 0.0f);
		glm::vec2 m_CanvasSize = glm::vec2(0.0f, 0.0f);

		glm::vec2 m_Position = glm::vec2(0.0f);
		glm::vec2 m_MouseOffset;
		float m_Zoom = 1.0f;

		uint32_t m_CanvasWidth = 1024;
		uint32_t m_CanvasHeight = 1024;
		std::vector<ImageLayer> m_Layers;

		Ref<Texture2D> m_BrushBuffer;
		Ref<Texture2D> m_Canvas;
	};
}