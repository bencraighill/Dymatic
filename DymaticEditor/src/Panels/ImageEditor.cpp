#include "ImageEditor.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "../TextSymbols.h"

#include "Dymatic/Math/Math.h"

#include <stdio.h>
#include <stdlib.h>

namespace Dymatic {

	ImageEditor::ImageEditor()
	{
		int dataSize = m_DocumentWidth * m_DocumentHeight * 4;
		char* data = new char[dataSize];
		for (int i = 0; i < dataSize; i++)
		{
			data[i] = 255;
		}
		m_Layers[0].Texture->SetData(data, dataSize);
		delete[] data;

		SetBrushData(m_Brush);

		m_BrushPath = new char[m_DocumentWidth * m_DocumentHeight * 4];
		for (int i = 0; i < m_DocumentWidth * m_DocumentHeight * 4; i++)
		{
			if (std::fmod(i + 2, 4) == 1.0f)
			{
				m_BrushPath[i] = 255.0f;
			}
			else
			{
				m_BrushPath[i] = 0.0f;
			}
		}
	}

	void ImageEditor::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);

		dispatcher.Dispatch<KeyPressedEvent>(DY_BIND_EVENT_FN(ImageEditor::OnKeyPressed));
		dispatcher.Dispatch<MouseButtonPressedEvent>(DY_BIND_EVENT_FN(ImageEditor::OnMouseButtonPressed));
		dispatcher.Dispatch<MouseScrolledEvent>(DY_BIND_EVENT_FN(ImageEditor::OnMouseScrolled));
	}

	bool ImageEditor::OnKeyPressed(KeyPressedEvent& e)
	{
		return false;
	}

	bool ImageEditor::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		return false;
	}

	bool ImageEditor::OnMouseScrolled(MouseScrolledEvent& e)
	{
		m_BrushSize += e.GetYOffset();
		return false;
	}

	void ImageEditor::OnImGuiRender()
	{
		if (m_ImageEditorVisible)
		{
			DY_CORE_INFO(m_BrushSize);

			if (Input::IsMouseButtonPressed(Mouse::ButtonMiddle))
			{
				m_Position += Input::GetMousePosition() - m_PreviousMousePosition;
			}
			m_PreviousMousePosition = Input::GetMousePosition();

			ImGui::Begin((std::string(CHARACTER_WINDOW_ICON_IMAGE_EDITOR) + " Image Editor").c_str(), &m_ImageEditorVisible);
			if (false)
			{
				auto drawList = ImGui::GetWindowDrawList();
				m_HoveredPixel = glm::vec2(ImGui::GetMousePos().x - ImGui::GetWindowPos().x, m_DocumentHeight - (ImGui::GetMousePos().y - ImGui::GetWindowPos().y));


				for (int i = 0; i < m_Layers.size(); i++)
				{
					auto& texture = m_Layers[i].Texture;

					drawList->AddImage(reinterpret_cast<void*>(m_Layers[i].Texture->GetRendererID()), ImGui::GetWindowPos() + ImVec2(m_Position.x, m_Position.y), ImGui::GetWindowPos() + ImVec2(m_DocumentWidth, m_DocumentHeight) + ImVec2(m_Position.x, m_Position.y), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

					if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
					{
						int width = texture->GetWidth();
						int height = texture->GetHeight();
						int bpp = 4;
						int dataSize = width * height * bpp;
						char* originalData = new char[dataSize];
						char* newData = new char[dataSize];
						texture->GetData(originalData, dataSize);
						strcpy(newData, originalData);

						DrawLine(m_BrushPath, m_BrushPath, dataSize, ((false ? false : (m_HoveredPixel == m_PreviouslyHoveredPixel)) ? glm::vec2(m_PreviouslyHoveredPixel.x + 1, m_PreviouslyHoveredPixel.y) : m_PreviouslyHoveredPixel) - glm::vec2(m_Position.x, m_Position.y * -1), m_HoveredPixel - glm::vec2(m_Position.x, m_Position.y * -1), glm::vec4(140.0f, 200.0f, 255.0f, 255.0f));

						texture->SetData(m_BrushPath, dataSize);
						delete[] originalData;
						delete[] newData;
					}
				}

				m_PreviouslyHoveredPixel = m_HoveredPixel;
			}
			ImGui::End();
		}
	}

	void ImageEditor::DrawLine(void* originalData, void* newData, int dataSize, glm::vec2 p0, glm::vec2 p1, glm::vec4 color)
	{
		//if (p0.x < 0) { p0.x = 0; }
		//if (p0.y < 0) { p0.y = 0; }
		//if (p0.x > m_DocumentWidth) { p0.x = m_DocumentWidth; }
		//if (p0.y > m_DocumentWidth) { p0.y = m_DocumentWidth; }
		//
		//if (p1.x < 0) { p1.x = 0; }
		//if (p1.y < 0) { p1.y = 0; }
		//if (p1.x > m_DocumentWidth) { p1.x = m_DocumentWidth; }
		//if (p1.y > m_DocumentWidth) { p1.y = m_DocumentWidth; }

		char* inData = reinterpret_cast<char*>(originalData);
		char* outData = reinterpret_cast<char*>(newData);
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
			bool found = false;
			for (int x = 0; x < m_ChangePixels.size(); x++)
			{
				if (m_ChangePixels[x] == index)
				{
					found = true;
				}
			}
			if (!found)
			{
				m_ChangePixels.push_back(index);
				if ((index > 0 && index < dataSize / 4))
				{
					int centreX = fmod(index, (m_DocumentWidth));
					centreX += m_Position.x;
					int centreY = (index - centreX) / (m_DocumentWidth);
					centreY -= m_Position.y;

					for (int i = 0; i < dataSize; i += 4)
					{
						int x =  fmod(i / 4, (m_DocumentWidth));
						x += m_Position.x;
						int y = (i / 4 - x) / (m_DocumentWidth);
						y -= m_Position.y;

						float distance = std::sqrt(std::pow((x - centreX), 2) + std::pow((y - centreY), 2));

						if (distance < m_BrushSize)
						{
							bool success = true;
							for (int z = 0; z < m_EditedPixels.size(); z++)
							{
								if (m_EditedPixels[z] == glm::vec2(x - m_Position.x, y + m_Position.y))
								{
									success = false;
									break;
								}
							}

							float alpha = 1.0f - (distance / m_BrushSize);
							if (success)
							{
								outData[i + 1] += alpha * 250.0f;
								if (outData[i + 1] < 0)
								{
									outData[i + 1] = 0;
								}
								if (outData[i + 1] > 250.0f)
								{
									outData[i + 1] = 250;
								}
								//outData[i + 3] = 1 - (1 - color.a) * (1 - inData[i + 3]);
								//outData[i + 0] = color.r * color.a / outData[i + 3] + outData[i + 0] * outData[i + 3] * (1 - color.a) / outData[i + 3];
								//outData[i + 1] = color.g * color.a / outData[i + 3] + outData[i + 1] * outData[i + 3] * (1 - color.a) / outData[i + 3];
								//outData[i + 2] = color.b * color.a / outData[i + 3] + outData[i + 2] * outData[i + 3] * (1 - color.a) / outData[i + 3];

								//outData[i + 0] = (color.r * (alpha)) + (inData[i + 0] * (1.0f - (alpha)));
								//outData[i + 1] = (color.g * (alpha)) + (inData[i + 1] * (1.0f - (alpha)));
								//outData[i + 2] = (color.b * (alpha)) + (inData[i + 2] * (1.0f - (alpha)));
								//outData[i + 3] = (color.a * (alpha)) + (inData[i + 3] * (1.0f - (alpha)));

								//m_EditedPixels.push_back(glm::vec2(x - m_Position.x, y + m_Position.y));
							}
						}
					}

					//float factor = 1.0f;
					//for (int y = 0; y < 4; y++)
					//{
					//	factor * -1.0f;
					//	int copyIndex = (index + (m_Brush->GetWidth() / 2 - 1) - ((m_Brush->GetHeight() / 2 - 1) * m_DocumentWidth)) * 4;//(m_HoveredPixel.x + m_HoveredPixel.y * m_DocumentWidth) * 4;
					//	for (int i = 1; i < m_BrushDataSize;)
					//	{
					//		if (fmod(i + 1, 4) == 1.0f)
					//		{
					//			//returnData[copyIndex - 4] = LerpValue * 255.0f / 1.0f * 0;
					//			//returnData[copyIndex - 3] = LerpValue * 255.0f / 1.0f * 1;
					//			//returnData[copyIndex - 2] = LerpValue * 255.0f / 1.0f * 0;
					//			//returnData[copyIndex - 1] = LerpValue * 255.0f / 2.0f;
					//
					//			if (copyIndex > 0 && copyIndex < dataSize)
					//			{
					//				float LerpValue = ((float)m_BrushData[i - 0]);/*-1, -0 -3 - 2*/
					//				//if (LerpValue < 0) { LerpValue *= -1; }
					//				//returnData[copyIndex - 0] = color.r * (LerpValue) + returnData[copyIndex - 0] * (1.0f - LerpValue); 0;
					//				//returnData[copyIndex - 3] = color.g * (LerpValue) + returnData[copyIndex - 3] * (1.0f - LerpValue);
					//				//returnData[copyIndex - 2] = color.b * (LerpValue) + returnData[copyIndex - 2] * (1.0f - LerpValue); 0;
					//
					//				//returnData[copyIndex - 0] = color.r * (LerpValue)+returnData[copyIndex - 0] * (1.0f - LerpValue);
					//				//returnData[copyIndex - 3] = color.g * (LerpValue)+returnData[copyIndex - 3] * (1.0f - LerpValue);
					//				//returnData[copyIndex - 2] = color.b * (LerpValue)+returnData[copyIndex - 2] * (1.0f - LerpValue);
					//
					//				//DY_CORE_INFO(LerpValue);
					//
					//				//if (LerpValue * -255.0f > m_BrushSize || LerpValue * 255.0f > m_BrushSize)
					//				//{
					//				//	returnData[copyIndex - 3] = color.r;
					//				//	returnData[copyIndex - 2] = color.g;
					//				//	returnData[copyIndex - 1] = color.b;
					//				//}
					//
					//				//if (LerpValue * 255.0f > m_BrushSize)
					//				//{
					//				//	returnData[copyIndex - 3] = color.r;
					//				//	returnData[copyIndex - 2] = color.g;
					//				//	returnData[copyIndex - 1] = color.b;
					//				//}
					//
					//
					//				//returnData[copyIndex - 3] = (color.r * LerpValue) + (returnData[copyIndex - 3] * (1.0f - LerpValue));
					//				//returnData[copyIndex - 2] = (color.g * LerpValue) + (returnData[copyIndex - 2] * (1.0f - LerpValue));
					//				//returnData[copyIndex - 1] = (color.b * LerpValue) + (returnData[copyIndex - 1] * (1.0f - LerpValue));
					//				//
					//				//returnData[copyIndex - 3] = LerpValue;
					//				//returnData[copyIndex - 2] = LerpValue;
					//				//returnData[copyIndex - 1] = LerpValue;
					//
					//				returnData[copyIndex - 2] = LerpValue + (float)(1.0f - LerpValue) * (float)returnData[copyIndex - 2];
					//				returnData[copyIndex - 3] = LerpValue + (float)(1.0f - LerpValue) * (float)returnData[copyIndex - 3];
					//				returnData[copyIndex - 1] = LerpValue + (float)(1.0f - LerpValue) * (float)returnData[copyIndex - 1];
					//
					//				//returnData[copyIndex + 1] = (LerpValue * (color.r));
					//				//returnData[copyIndex + 2] = (LerpValue * (color.g));
					//				//returnData[copyIndex + 3] = (LerpValue * (color.b));
					//
					//				//returnData[copyIndex - 3] = LerpValue * color.r + (1.0f - LerpValue) * returnData[copyIndex - 3];
					//				//returnData[copyIndex - 2] = LerpValue * color.g + (1.0f - LerpValue) * returnData[copyIndex - 2];
					//				//returnData[copyIndex - 1] = LerpValue * color.b + (1.0f - LerpValue) * returnData[copyIndex - 1];
					//
					//				//static float brushOpacity = 0.5f;
					//				//
					//				//if (LerpValue * -1 > 0.0f)
					//				//{
					//				//	returnData[copyIndex + 1] = color.r * brushOpacity + returnData[copyIndex + 1] * 0.5f;
					//				//	returnData[copyIndex + 2] = color.g * brushOpacity + returnData[copyIndex + 2] * 0.5f;
					//				//	returnData[copyIndex + 3] = color.b * brushOpacity + returnData[copyIndex + 3] * 0.5f;
					//				//}
					//				//
					//				//if (LerpValue > 0.0f)
					//				//{
					//				//	returnData[copyIndex + 1] = color.r * brushOpacity + returnData[copyIndex + 1] * 0.5f;
					//				//	returnData[copyIndex + 2] = color.g * brushOpacity + returnData[copyIndex + 2] * 0.5f;
					//				//	returnData[copyIndex + 3] = color.b * brushOpacity + returnData[copyIndex + 3] * 0.5f;
					//				//}
					//
					//				//if (LerpValue * -1 > 0.0f)
					//				//{
					//				//	returnData[copyIndex + 1] = color.r * 
					//				//	returnData[copyIndex + 2] = color.g * 
					//				//	returnData[copyIndex + 3] = color.b * 
					//				//}
					//				//
					//				//if (LerpValue > 0.0f)
					//				//{
					//				//	returnData[copyIndex + 1] = color.r ;
					//				//	returnData[copyIndex + 2] = color.g ;
					//				//	returnData[copyIndex + 3] = color.b ;
					//				//}
					//
					//				//returnData[copyIndex - 3] = LerpValue * 255.0f;
					//				//returnData[copyIndex - 2] = LerpValue * 255.0f;
					//				//returnData[copyIndex - 1] = LerpValue * 255.0f;
					//			}
					//
					//			//returnData[copyIndex - 4] += LerpValue * color.r;
					//			//returnData[copyIndex - 3] += LerpValue * color.g;
					//			//returnData[copyIndex - 2] += LerpValue * color.b;
					//
					//			//returnData[copyIndex - 1] = color.a * LerpValue + returnData[copyIndex - 1] * (1.0f - LerpValue);
					//		}
					//		//returnData[copyIndex] = brushData[i];
					//
					//		if (fmod(i, m_Brush->GetWidth() * 4) == 1.0f)
					//		{
					//			copyIndex -= m_Brush->GetWidth() * 4;
					//			copyIndex += m_DocumentWidth * 4;
					//		}
					//
					//		copyIndex++;
					//		i++;
					//	}
					//}
				}
			}
			const int errorIsTooBig = error >= dLong;
			index += offset[errorIsTooBig];
			error += abs_d[errorIsTooBig];

		}
	}

	void ImageEditor::SetBrushData(Ref<Texture2D> brushTexture)
	{
		//delete[] m_BrushData;

		int dataSize = brushTexture->GetWidth() * brushTexture->GetHeight() * 4;
		m_BrushData = new char[dataSize];
		m_BrushDataSize = dataSize;
		brushTexture->GetData(m_BrushData, dataSize);
	}

}