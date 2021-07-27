#pragma once
#include <Dymatic.h>
#include "Dymatic/Core/Base.h"

namespace Dymatic {

	struct ImageLayer
	{
		ImageLayer(std::string name, int id, int width, int height)
			: Name(name), ID(id), Texture(Texture2D::Create(width, height))
		{
		}

		std::string Name;
		int ID;
		Ref<Texture2D> Texture;
	};

	class ImageEditor
	{
	public:
		ImageEditor();

		void OnImGuiRender();

		void OnEvent(Event& e);
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		bool OnMouseScrolled(MouseScrolledEvent& e);

		bool& GetImageEditorVisible() { return m_ImageEditorVisible; }
	private:
		int GetNextLayerID() { m_NextLayerID++; return m_NextLayerID; }

		void DrawLine(void* originalData, void* newData, int dataSize, glm::vec2 p0, glm::vec2 p1, glm::vec4 color);
		void SetBrushData(Ref<Texture2D> brushTexture);
	private:
		bool m_ImageEditorVisible = false;

		int m_NextLayerID = 0;

		int m_DocumentWidth = 300;
		int m_DocumentHeight = 300;
		std::vector<ImageLayer> m_Layers = { ImageLayer("Base Layer", GetNextLayerID(), m_DocumentWidth, m_DocumentHeight) };

		Ref<Texture2D> m_Brush = Texture2D::Create("assets/BrushMask.png");
		char* m_BrushData;
		int m_BrushDataSize;

		glm::vec2 m_HoveredPixel = { 0, 0 };
		glm::vec2 m_PreviouslyHoveredPixel = { 0, 0 };

		glm::vec2 m_Position = glm::vec2(0, 0);

		glm::vec2 m_PreviousMousePosition = glm::vec2(0, 0);

		std::vector<int> m_ChangePixels;

		float m_BrushSize = 0.5f;

		std::vector<glm::vec2> m_EditedPixels;
		char* m_BrushPath;
	};

}