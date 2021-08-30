#pragma once
#include <Dymatic.h>
#include "Dymatic/Core/Base.h"

namespace Dymatic {

	enum Tool
	{
		Move,
		Brush,
		Hand
	};

	struct ImageLayer
	{
		ImageLayer(unsigned int id, std::string name, int width, int height)
			: ID(id), Name(name), Texture(Texture2D::Create(width, height))
		{
			auto dataSize = width * height * 4;
			unsigned char* data = new unsigned char[dataSize];
			for (int i = 0; i < dataSize; i++)
			{
				if (fmod(i + 1, 4) == 0) data[i] = 0.0f;
				else data[i] = 255.0f;
			}
			Texture->SetData(data, dataSize);
			delete[] data;
		}

		std::string Name;
		unsigned int ID;
		Ref<Texture2D> Texture;
		bool Visible = true;
		int Opacity = 100;
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

		bool& GetImageEditorVisible() { return m_ImageEditorVisible; }
	private:
		unsigned int GetNextLayerID() { m_NextLayerID++; return m_NextLayerID; }
		void DrawLine(unsigned char* data, int dataSize, glm::vec2 p0, glm::vec2 p1);

		ImageLayer* GetLayerById(unsigned int id);
		int GetLayerIndexById(unsigned int id);

		void FindBoundingBox(unsigned char* data, uint32_t size, glm::vec2* min, glm::vec2* max);
		glm::vec2 ConvertIndexToVector(unsigned int index, unsigned int width) { return { std::fmod(index, width), index / width }; }
		unsigned int ConvertVectorToIndex(glm::vec2 vector, unsigned int width) { return vector.x + vector.y * width; }

		void ExportImage(std::string);
	private:
		bool m_ImageEditorVisible = false;

		unsigned int m_NextLayerID = 0;

		glm::vec2 m_MousePosition = glm::vec2(0.0f, 0.0f);
		glm::vec2 m_PreviousMousePos = glm::vec2(0.0f, 0.0f);
		glm::vec2 m_Position = glm::vec2(0.0f, 0.0f);
		float m_Zoom = 1.0f;

		Tool m_Tool = Move;

		std::vector<ImageLayer> m_Layers = { ImageLayer(GetNextLayerID(), "Base Layer", 300, 300) };
		ImageLayer* m_SelectedLayer = nullptr;

		Ref<Texture2D> m_CheckerboardTexture = Texture2D::Create("assets/textures/Checkerboard.png");

		Ref<Texture2D> m_Brush = Texture2D::Create("assets/BrushMask.png");
		unsigned char* m_BrushData;
		//Ref<Texture2D> m_StrokeTextureBuffer = Texture2D::Create(300, 300);
		unsigned char* m_StrokeBuffer = nullptr;
		unsigned char* m_LayerBuffer = nullptr;

		int m_DocumentWidth = 300, m_DocumentHeight = 300;

		glm::vec2 m_HoveredPixel = { 0, 0 };
		glm::vec2 m_PreviouslyHoveredPixel = { 0, 0 };

		glm::vec4 m_BrushColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		int m_BrushOpacity = 100;
	};
}