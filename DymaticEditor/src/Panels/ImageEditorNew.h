#pragma once
#include <Dymatic.h>
#include "Dymatic/Core/Base.h"

namespace Dymatic {

	struct ImageLayerNew
	{
		ImageLayerNew(std::string name, unsigned int id, int width, int height)
			: Name(name), ID(id), Texture(Texture2D::Create(width, height)), StrokeTextureBuffer(Texture2D::Create(width, height))
		{
		}

		std::string Name;
		unsigned int ID;
		Ref<Texture2D> Texture;
		Ref<Texture2D> StrokeTextureBuffer;
	};

	class ImageEditorNew
	{
	public:
		ImageEditorNew();

		void OnEvent(Event& e);
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		bool OnMouseButtonReleased(MouseButtonReleasedEvent& e);
		bool OnMouseScrolled(MouseScrolledEvent& e);

		void OnImGuiRender();
	private:
		int GetNextLayerID() { m_NextLayerID++; return m_NextLayerID; }
		void DrawLine(void* originalData, int dataSize, glm::vec2 p0, glm::vec2 p1, glm::vec4 color);
		void drawline(void* originalData, void* newData, int dataSize, glm::vec2 p0, glm::vec2 p1);
	private:
		unsigned int m_NextLayerID = 0;

		std::vector<ImageLayerNew> m_Layers = { ImageLayerNew("Base Layer", GetNextLayerID(), 300, 300) };

		Ref<Texture2D> m_Brush = Texture2D::Create("assets/BrushMask.png");
		char* m_BrushData;

		int m_DocumentWidth = 300, m_DocumentHeight = 300;
		glm::vec2 m_Position = glm::vec2(0, 0);

		glm::vec2 m_HoveredPixel = { 0, 0 };
		glm::vec2 m_PreviouslyHoveredPixel = { 0, 0 };
	};
}