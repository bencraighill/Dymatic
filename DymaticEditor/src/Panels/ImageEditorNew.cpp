#include "ImageEditorNew.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "Dymatic/Math/Math.h"
#include <glm/glm.hpp>

namespace Dymatic {


	ImageEditorNew::ImageEditorNew()
	{
		int dataSize = m_DocumentWidth * m_DocumentHeight * 4;
		char* data = new char[dataSize];
		for (int i = 0; i < dataSize; i++)
		{
			data[i] = 255;
		}
		m_Layers[0].Texture->SetData(data, dataSize);

		char* dataStroke = new char[dataSize];
		for (int i = 0; i < dataSize; i++)
		{
			dataStroke[i] = 0.0f;
		}
		m_Layers[0].StrokeTextureBuffer->SetData(dataStroke, dataSize);

		delete[] data;
		delete[] dataStroke;

		auto brushDataSize = 25 * 25 * 4;
		m_BrushData = new char[brushDataSize];
		m_Brush->GetData(m_BrushData, brushDataSize);
	}

	void ImageEditorNew::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);

		dispatcher.Dispatch<KeyPressedEvent>(DY_BIND_EVENT_FN(ImageEditorNew::OnKeyPressed));
		dispatcher.Dispatch<MouseButtonPressedEvent>(DY_BIND_EVENT_FN(ImageEditorNew::OnMouseButtonPressed));
		dispatcher.Dispatch<MouseButtonReleasedEvent>(DY_BIND_EVENT_FN(ImageEditorNew::OnMouseButtonReleased));
		dispatcher.Dispatch<MouseScrolledEvent>(DY_BIND_EVENT_FN(ImageEditorNew::OnMouseScrolled));
	}

	bool ImageEditorNew::OnKeyPressed(KeyPressedEvent& e)
	{
		return false;
	}

	bool ImageEditorNew::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		return false;
	}

	bool ImageEditorNew::OnMouseButtonReleased(MouseButtonReleasedEvent& e)
	{
		switch (e.GetMouseButton())
		{
		case (Mouse::ButtonLeft): {
			auto width = m_Layers[0].StrokeTextureBuffer->GetWidth();
			auto height = m_Layers[0].StrokeTextureBuffer->GetHeight();
			auto dataSize = width * height * 4;
			char* data = new char[dataSize];
			char* strokeData = new char[dataSize];
			m_Layers[0].Texture->GetData(data, dataSize);
			m_Layers[0].StrokeTextureBuffer->GetData(strokeData, dataSize);
			for (int i = 0; i < dataSize; i++)
			{
				data[i] += strokeData[i];
				strokeData[i] = 0.0f;
			}
			m_Layers[0].StrokeTextureBuffer;

			m_Layers[0].Texture->SetData(data, dataSize);
			m_Layers[0].StrokeTextureBuffer->SetData(strokeData, dataSize);

			delete[] data;
			delete[] strokeData;
		}
		}
		return false;
	}

	bool ImageEditorNew::OnMouseScrolled(MouseScrolledEvent& e)
	{
		return false;
	}

	void ImageEditorNew::OnImGuiRender()
	{
		ImGui::Begin("Image Editor");

		auto drawList = ImGui::GetWindowDrawList();
		m_HoveredPixel = glm::vec2(ImGui::GetMousePos().x - ImGui::GetWindowPos().x, m_DocumentHeight - (ImGui::GetMousePos().y - ImGui::GetWindowPos().y));
		for (int i = 0; i < m_Layers.size(); i++)
		{
			auto& texture = m_Layers[i].StrokeTextureBuffer;
			drawList->AddImage((ImTextureID)m_Layers[i].Texture->GetRendererID(), ImGui::GetWindowPos() + ImVec2(m_Position.x, m_Position.y), ImGui::GetWindowPos() + ImVec2(m_DocumentWidth, m_DocumentHeight) + ImVec2(m_Position.x, m_Position.y), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

			if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
			{
				int width = texture->GetWidth();
				int height = texture->GetHeight();
				int bpp = 4;
				int dataSize = width * height * bpp;
				char* originalData = new char[dataSize];
				char* newData = new char[dataSize];
				texture->GetData(originalData, dataSize);

				for (int i = 0; i < dataSize; i++)
				{
					newData[i] = originalData[i];
				}

				//DrawLine(originalData, (m_DocumentWidth * m_DocumentHeight * 4), glm::vec2(5, 5), glm::vec2(100, 100), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
				drawline(originalData, newData, dataSize, m_PreviouslyHoveredPixel, m_HoveredPixel);

				m_Layers[i].StrokeTextureBuffer->SetData(newData, dataSize);
				delete[] originalData;
				delete[] newData;
			}
		}
		m_PreviouslyHoveredPixel = m_HoveredPixel;
		ImGui::End();
	}

	void ImageEditorNew::DrawLine(void* originalData, int dataSize, glm::vec2 p0, glm::vec2 p1, glm::vec4 color)
	{
		if (p0.x < 0) { p0.x = 0; }
		if (p0.y < 0) { p0.y = 0; }
		if (p0.x > m_DocumentWidth) { p0.x = m_DocumentWidth; }
		if (p0.y > m_DocumentWidth) { p0.y = m_DocumentWidth; }

		if (p1.x < 0) { p1.x = 0; }
		if (p1.y < 0) { p1.y = 0; }
		if (p1.x > m_DocumentWidth) { p1.x = m_DocumentWidth; }
		if (p1.y > m_DocumentWidth) { p1.y = m_DocumentWidth; }

		char* inData = reinterpret_cast<char*>(originalData);
		int dx = p1.x - p0.x;
		int dy = p1.y - p0.y;

		int dLong = std::abs(dx);
		int dShort = std::abs(dy);

		int offsetLong = dx > 0 ? 1 : -1;
		int offsetShort = dy > 0 ? m_DocumentWidth : -m_DocumentWidth; //Could also possibly be height

		if (dLong < dShort)
		{
			std::swap(dShort, dLong);
			std::swap(offsetShort, offsetLong);
		}

		int error = dLong / 2;
		int index = p0.y * m_DocumentWidth + p0.x;
		const int offset[] = { offsetLong, offsetLong + offsetShort };
		const int abs_d[] = { dShort, dShort - dLong };
		for (int i = 0; i < dLong; i++)
		{
			inData[i * 4 * 4 * 4] = 0.0f;
			//if ((index > 0 && index < dataSize / 4))
			//{
			//	int centreX = fmod(index, (m_DocumentWidth));
			//	centreX += m_Position.x;
			//	int centreY = (index - centreX) / (m_DocumentWidth);
			//	centreY -= m_Position.y;
			//
			//	for (int i = 0; i < dataSize; i += 4)
			//	{
			//		int x = fmod(i / 4, (m_DocumentWidth));
			//		x += m_Position.x;
			//		int y = (i / 4 - x) / (m_DocumentWidth);
			//		y -= m_Position.y;
			//
			//		float distance = std::sqrt(std::pow((x - centreX), 2) + std::pow((y - centreY), 2));
			//
			//		if (distance < 1.0f)
			//		{
			//			float alpha = 1.0f - (distance / 1.0f);
			//			
			//			inData[i] = 0.0f;
			//		}
			//	}
			//}
			const int errorIsTooBig = error >= dLong;
			index += offset[errorIsTooBig];
			error += abs_d[errorIsTooBig];

		}
	}

	void ImageEditorNew::drawline(void* originalData, void* newData, int dataSize, glm::vec2 p0, glm::vec2 p1)
	{
		char* inData = reinterpret_cast<char*>(originalData);
		char* outData = reinterpret_cast<char*>(newData);

		int dx = p1.x - p0.x;
		int dy = p1.y - p0.y;

		int dLong = abs(dx);
		int dShort = abs(dy);

		int offsetLong = dx > 0 ? 1 : -1;
		int offsetShort = dy > 0 ? m_DocumentWidth : -m_DocumentWidth; // Could be height

		if (dLong < dShort)
		{
			std::swap(dShort, dLong);
			std::swap(offsetShort, offsetLong);
		}

		int error = dLong / 2;
		int index = p0.y * m_DocumentWidth + p0.x; // Could be height
		const int offset[] = { offsetLong, offsetLong + offsetShort };
		const int abs_d[] = { dShort, dShort - dLong };
		for (int i = 0; i <= dLong; ++i)
		{
			if (index >= 0 && index < dataSize / 4) {
				//buffer[index] = 255;  // or a call to your painting method

				int yIndex = -((25 / 2) + ((m_DocumentWidth * (25 - 1)) / 2));
				for (int y = 0; y < (25 * 25); y++)
				{
					if ((index)+(yIndex) < dataSize / 4 && (index)+(yIndex) > 0)
					{
						// Working SOLUTION
						outData[(index * 4) + (yIndex * 4) + 0] = glm::clamp(((float)outData[(index * 4) + (yIndex * 4) + 0] + (glm::clamp(glm::abs((float)m_BrushData[y * 4 + 3] - 1.0f), 0.0f, 127.0f))), 0.0f, 127.0f);
						outData[(index * 4) + (yIndex * 4) + 1] = glm::clamp(((float)outData[(index * 4) + (yIndex * 4) + 1] + (glm::clamp(glm::abs((float)m_BrushData[y * 4 + 3] - 1.0f), 0.0f, 127.0f))), 0.0f, 127.0f);
						outData[(index * 4) + (yIndex * 4) + 2] = glm::clamp(((float)outData[(index * 4) + (yIndex * 4) + 2] + (glm::clamp(glm::abs((float)m_BrushData[y * 4 + 3] - 1.0f), 0.0f, 127.0f))), 0.0f, 127.0f);
						outData[(index * 4) + (yIndex * 4) + 3] = glm::clamp(((float)outData[(index * 4) + (yIndex * 4) + 3] + (glm::clamp(glm::abs((float)m_BrushData[y * 4 + 3] - 1.0f), 0.0f, 127.0f))), 0.0f, 127.0f);

						yIndex++;
						if (std::fmod(y + 2, 25) == 1)
						{
							yIndex += (m_DocumentWidth - 25);
						}
					}
				}

				const int errorIsTooBig = error >= dLong;
				index += offset[errorIsTooBig];
				error += abs_d[errorIsTooBig];

			}
		}
	}
}