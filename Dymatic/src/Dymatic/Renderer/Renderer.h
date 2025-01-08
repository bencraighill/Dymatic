#pragma once

#include "Dymatic/Renderer/RenderCommand.h"
#include "Dymatic/Renderer/Shader.h"
#include "Dymatic/Renderer/Texture.h"
#include "Dymatic/Renderer/Font.h"
#include "Dymatic/Renderer/RendererTiering.h"

#include <map>

namespace Dymatic {

	struct RendererTiering
	{
		Tiering::Renderer::ShadowResolution ShadowResolution = Tiering::Renderer::ShadowResolution::High;
		Tiering::Renderer::VolumetricLightingResolution VolumetricLightingResolution = Tiering::Renderer::VolumetricLightingResolution::Medium;
		Tiering::Renderer::FontQuality FontQuality = Tiering::Renderer::FontQuality::Medium;
	};

	struct RendererConfig
	{
		std::filesystem::path ShaderPackPath;

		// Tiering
		RendererTiering RendererTieringData;
	};

	struct RendererSharedData
	{
		struct CameraData
		{
			glm::mat4 ViewProjection;
			glm::vec4 ViewPosition;

			glm::mat4 Projection;
			glm::mat4 InverseProjection;
			glm::mat4 View;
			glm::mat4 InverseView;
			glm::mat4 PreviousViewProjection;
			glm::mat4 InverseViewProjection;

			glm::vec4 Forward;
			glm::vec4 Right;
			glm::vec4 Up;

			glm::uvec4 TileSizes;
			glm::uvec2 ScreenDimensions;
			glm::vec2 PixelSize;
			float Scale;
			float Bias;
			float ZNear;
			float ZFar;
		};
	};

	class Renderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void OnWindowResize(uint32_t width, uint32_t height);
		
		static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

		static const RendererConfig& GetConfig();
		static void SetConfig(const RendererConfig& config);

		static RendererTiering& GetTiering();

		static const Ref<ShaderLibrary> GetShaderLibrary();
		static void SetMacroInShader(Ref<Shader> shader, const std::string& name, const std::string& value);
		static const std::map<std::string, std::string>& GetGlobalShaderMacros();
		static void SetGlobalMacroInShaders(const std::string& name, const std::string& value);

		// TODO: If we ever share any other uniform buffers here, refactor this to be a generic function taking in an enum.
		static void SetCameraData(const RendererSharedData::CameraData& cameraData);
		static void SetCameraData(const RendererSharedData::CameraData& cameraData, size_t size);

		static void SetEditorScratchBufferData(const void* data, size_t size, size_t offset = 0);

		// Render commands
		static void RenderQuad(Ref<Shader> shader);
		static void DrawFullscreenTexture(Ref<Texture2D> texture = nullptr);
	};
}
