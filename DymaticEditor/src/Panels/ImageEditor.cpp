#include "ImageEditor.h"

#include "EditorResources.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

#include "Dymatic/Math/Math.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/compatibility.hpp>

#include "Settings/Preferences.h"
#include "TextSymbols.h"

#define INT_CEILING_DIVISION(dividend, divisor) ((dividend + divisor - 1) / divisor)

namespace Dymatic {

	struct ImageEditorData
	{
		static constexpr uint32_t ImageDispatchLocalSize = 8;

		struct ImageEditorGPUData
		{
			glm::vec4 BrushColor = glm::vec4(1.0f);
			glm::vec2 MousePosition;
			glm::vec2 PreviousMousePosition;
			glm::vec2 PreviousDrawPosition;
			glm::uvec2 CanvasSize;
			float BrushRadius = 20.0f;
			float Zoom;
		};
		ImageEditorGPUData ImageEditorGPUBuffer;

		Ref<Shader> DrawBrushShader;
		Ref<Shader> ApplyBrushShader;
		Ref<Shader> ClearShader;

		Ref<Shader> CompositeGridShader;
		Ref<Shader> CompositeLayerShader;
	};

	static ImageEditorData s_Data;

	ImageLayer::ImageLayer(const uint32_t width, const uint32_t height)
	{
		TextureSpecification spec;
		spec.SamplerFilter = TextureFilter::Nearest;
		spec.Width = width;
		spec.Height = height;

		const size_t bufferSize = width * height * Utils::GetDymaticTextureFormatBPP(spec.Format);
		ScopedBuffer buffer = ScopedBuffer(bufferSize);
		buffer.ZeroInitialize();

		Texture = Texture2D::Create(spec, buffer);
	}

	ImageEditor::ImageEditor()
	{
		m_Layers.push_back({ m_CanvasWidth, m_CanvasHeight });

		TextureSpecification spec;
		spec.SamplerFilter = TextureFilter::Nearest;
		spec.Width = m_CanvasWidth;
		spec.Height = m_CanvasHeight;

		spec.Format = TextureFormat::R8;
		size_t bufferSize = m_CanvasWidth * m_CanvasHeight * Utils::GetDymaticTextureFormatBPP(spec.Format);
		ScopedBuffer buffer = ScopedBuffer(bufferSize);
		buffer.ZeroInitialize();
		m_BrushBuffer = Texture2D::Create(spec, buffer);
		
		spec.Format = TextureFormat::RGBA8;
		bufferSize = m_CanvasWidth * m_CanvasHeight * Utils::GetDymaticTextureFormatBPP(spec.Format);
		ScopedBuffer canvasBuffer = ScopedBuffer(bufferSize);
		canvasBuffer.ZeroInitialize();
		m_Canvas = Texture2D::Create(spec, canvasBuffer);

		// Init Data
		s_Data.DrawBrushShader = Shader::Create("Resources/Shaders/Editor/ImageEditor/Editor_DrawBrush.glsl");
		s_Data.ApplyBrushShader = Shader::Create("Resources/Shaders/Editor/ImageEditor/Editor_ApplyBrush.glsl");
		s_Data.ClearShader = Shader::Create("Resources/Shaders/Editor/ImageEditor/Editor_Clear.glsl");
		s_Data.CompositeGridShader = Shader::Create("Resources/Shaders/Editor/ImageEditor/Editor_CompositeGrid.glsl");
		s_Data.CompositeLayerShader = Shader::Create("Resources/Shaders/Editor/ImageEditor/Editor_CompositeLayer.glsl");
	}

	void ImageEditor::OnEvent(Event& e)
	{
		if (Preferences::GetEditorWindowVisible(Preferences::EditorWindow::ImageEditor))
		{
			EventDispatcher dispatcher(e);

			dispatcher.Dispatch<KeyPressedEvent>(DY_BIND_EVENT_FN(ImageEditor::OnKeyPressed));
			dispatcher.Dispatch<MouseButtonPressedEvent>(DY_BIND_EVENT_FN(ImageEditor::OnMouseButtonPressed));
			dispatcher.Dispatch<MouseButtonReleasedEvent>(DY_BIND_EVENT_FN(ImageEditor::OnMouseButtonReleased));
			dispatcher.Dispatch<MouseScrolledEvent>(DY_BIND_EVENT_FN(ImageEditor::OnMouseScrolled));
		}
	}

	bool ImageEditor::OnKeyPressed(KeyPressedEvent& e)
	{
		return false;
	}

	bool ImageEditor::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		return false;
	}

	bool ImageEditor::OnMouseButtonReleased(MouseButtonReleasedEvent& e)
	{
		return false;
	}

	bool ImageEditor::OnMouseScrolled(MouseScrolledEvent& e)
	{
		if (ImGui::GetIO().KeyAlt)
		{
			auto delta = e.GetYOffset() * 0.1f;
			m_Zoom += delta;
			m_Position -= m_MouseOffset * delta;
		}
		
		return false;
	}

	void ImageEditor::OnUpdate()
	{
		// Handle Moving
		if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
			m_Position += glm::vec2(ImGui::GetIO().MousePos - ImGui::GetIO().MousePosPrev);

		// Upload data to editor uniform buffer
		s_Data.ImageEditorGPUBuffer.Zoom = m_Zoom;
		s_Data.ImageEditorGPUBuffer.PreviousMousePosition = s_Data.ImageEditorGPUBuffer.MousePosition;
		s_Data.ImageEditorGPUBuffer.MousePosition = (ImGui::GetMousePos() - m_CanvasMin) / m_Zoom;
		s_Data.ImageEditorGPUBuffer.MousePosition.y = (m_CanvasSize.y / m_Zoom) - s_Data.ImageEditorGPUBuffer.MousePosition.y;
		s_Data.ImageEditorGPUBuffer.CanvasSize = glm::uvec2(m_CanvasWidth, m_CanvasHeight);

		const bool beginDraw = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
		if (beginDraw)
		{
			m_BrushBuffer->BindTexture();
			DispatchImageShader(s_Data.ClearShader);
			s_Data.ImageEditorGPUBuffer.PreviousDrawPosition = s_Data.ImageEditorGPUBuffer.MousePosition + 0.01f;
		}

		Renderer::SetEditorScratchBufferData(&s_Data.ImageEditorGPUBuffer, sizeof(ImageEditorData::ImageEditorGPUData));
		
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && s_Data.ImageEditorGPUBuffer.MousePosition.x != FLT_MAX)
		{
			if (beginDraw || glm::distance(s_Data.ImageEditorGPUBuffer.PreviousDrawPosition, s_Data.ImageEditorGPUBuffer.MousePosition) > s_Data.ImageEditorGPUBuffer.BrushRadius)
			{
				m_BrushBuffer->BindTexture();
				DispatchImageShader(s_Data.DrawBrushShader);
				s_Data.ImageEditorGPUBuffer.PreviousDrawPosition = s_Data.ImageEditorGPUBuffer.MousePosition;
			}
		}

		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			m_BrushBuffer->Bind(0);
			m_Layers[0].Texture->BindTexture();
			DispatchImageShader(s_Data.ApplyBrushShader);
		}

		// Composite Layers
		m_Canvas->BindTexture();
		DispatchImageShader(s_Data.ClearShader);

		for (const auto& layer : m_Layers)
		{
			layer.Texture->Bind();
			DispatchImageShader(s_Data.CompositeLayerShader);
		}

		DispatchImageShader(s_Data.CompositeGridShader);
	}

	static void SetupDockspace()
	{

	}

	void ImageEditor::OnImGuiRender()
	{
		auto& imageEditorVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::ImageEditor);
		if (!imageEditorVisible)
			return;

		OnUpdate();

		ImGui::Begin(FA_BRUSH " Image Editor", &imageEditorVisible, ImGuiWindowFlags_MenuBar);

		// Menu Bar
		ImGui::BeginMenuBar();
		if (ImGui::BeginMenu("File"))
		{
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Filter"))
		{
		}
		ImGui::EndMenuBar();

		ImGui::DragFloat("##BrushRadiusInput", &s_Data.ImageEditorGPUBuffer.BrushRadius, 1.0f, 1.0f, 5000.0f);
		ImGui::ColorEdit4("##BrushColorPicker", glm::value_ptr(s_Data.ImageEditorGPUBuffer.BrushColor));

		ImGui::BeginChild("##ImageEditorCanvas", ImVec2(), 0, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);

		m_MouseOffset = glm::vec2(ImGui::GetMousePos().x - ImGui::GetWindowPos().x - (ImGui::GetWindowSize().x / 2) + m_CanvasWidth / 2, ImGui::GetMousePos().y - ImGui::GetWindowPos().y - (ImGui::GetWindowSize().y / 2) + m_CanvasHeight / 2);

		const ImVec2 size = m_Zoom * glm::vec2(m_CanvasWidth, m_CanvasHeight);
		ImGui::SetCursorPos(m_Position);
		ImGui::Image((ImTextureID)m_Canvas->GetRendererID(), size, { 0, 1 }, { 1, 0 });
		m_CanvasMin = ImGui::GetItemRectMin();
		m_CanvasSize = ImGui::GetItemRectSize();

		ImGui::SetCursorPos(m_Position);
		ImGui::InvisibleButton("##CanvasInvisibleButton", size);

		if (ImGui::IsItemHovered())
		{
			ImGui::GetForegroundDrawList()->AddCircle(ImGui::GetMousePos(), s_Data.ImageEditorGPUBuffer.BrushRadius * m_Zoom, ImGui::GetColorU32(glm::vec4(glm::vec3(s_Data.ImageEditorGPUBuffer.BrushColor), 1.0)), 0, 4.0f);
			ImGui::SetMouseCursor(ImGuiMouseCursor_None);
		}

		ImGui::EndChild();

		ImGui::End();
	}

	void ImageEditor::DispatchImageShader(Ref<Shader> shader)
	{
		const int numGroupsX = INT_CEILING_DIVISION(m_CanvasWidth, ImageEditorData::ImageDispatchLocalSize);
		const int numGroupsY = INT_CEILING_DIVISION(m_CanvasHeight, ImageEditorData::ImageDispatchLocalSize);
		shader->Dispatch(numGroupsX, numGroupsY, 1);
	}

}