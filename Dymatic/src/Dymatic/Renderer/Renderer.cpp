#include "dypch.h"
#include "Dymatic/Renderer/Renderer.h"
#include "Dymatic/Renderer/Renderer2D.h"
#include "Dymatic/Renderer/SceneRenderer.h"

#include "Dymatic/Renderer/UniformBuffer.h"

#include "Dymatic/Core/Application.h"

#include "Dymatic/Asset/AssetManager.h"
#include "Dymatic/Asset/EngineAsset.h"

namespace Dymatic {

	struct RendererData
	{
		Ref<ShaderLibrary> ShaderLibrary;
		std::map<std::string, std::string> GlobalShaderMacros;

		// TODO: Put default textures here e.g. white texture, black texture, black cube/skybox etc

		// Shared renderer resources
		Ref<UniformBuffer> CameraUniformBuffer;
		Ref<UniformBuffer> EditorUniformBuffer = nullptr;

		Ref<Shader> DrawTextureShader;

		const glm::vec3 QuadBuffer[4] = {
			{ -1.0f, -1.0f, 0.0f },
			{ 1.0f, -1.0f, 0.0f },
			{ 1.0f,  1.0f, 0.0f },
			{ -1.0f,  1.0f, 0.0f }
		};

		Ref<VertexArray> QuadVertexArray;
		Ref<VertexBuffer> QuadVertexBuffer;
	};

	static RendererConfig s_Config;
	static RendererData* s_Data;
	
	void Renderer::Init()
	{
		DY_PROFILE_FUNCTION();

		DY_CORE_INFO("Initializing Renderer...");

		s_Data = new RendererData();
		s_Data->ShaderLibrary = CreateScope<ShaderLibrary>();

		// Setup the render command
		DY_CORE_INFO("Initializing Render Command...");
		RenderCommand::Init();

		// Initialize data shared across renderers.
		// Note if we share more UBOs in the future, introduce an enum and unordered map to store them so we can conveniently set them.
		s_Data->CameraUniformBuffer = UniformBuffer::Create(sizeof(RendererSharedData::CameraData), RendererConstants::Buffers::Camera);

		// Initialize data objects shared across renderers
		// 
		// Quad Vertex Buffer
		s_Data->QuadVertexArray = VertexArray::Create();
		s_Data->QuadVertexBuffer = VertexBuffer::Create(sizeof(s_Data->QuadBuffer));
		s_Data->QuadVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position" }
			});
		s_Data->QuadVertexArray->AddVertexBuffer(s_Data->QuadVertexBuffer);

		uint32_t quadIndices[6] = { 0, 1, 2, 2, 3, 0 };

		Ref<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, 6);
		s_Data->QuadVertexArray->SetIndexBuffer(quadIB);
		s_Data->QuadVertexBuffer->SetData(&s_Data->QuadBuffer, sizeof(s_Data->QuadBuffer));

		// Load required shaders
		if (s_Config.ShaderPackPath.empty())
		{
			DY_CORE_INFO("Loading Shader Resources...");
			
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer3D_DrawTexture.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer3D_DrawDepth.glsl");

			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer2D/Renderer2D_Quad.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer2D/Renderer2D_Circle.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer2D/Renderer2D_Line.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer2D/Renderer2D_Text.glsl");

			s_Data->ShaderLibrary->Load("Resources/Shaders/Cubemap/Renderer3D_IBLEquirectangularToCubemap.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Cubemap/Renderer3D_IBLIrradianceConvolution.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Cubemap/Renderer3D_IBLPrefilter.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Cubemap/Renderer3D_IBLbrdf.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Cubemap/Renderer3D_IBLBackground.glsl");
			
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer/Renderer3D_DeferredLighting.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer/Renderer3D_ClusterShader.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer/Renderer3D_ClusterCullLightShader.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer/Renderer3D_BufferVisualization.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer/Renderer3D_Wireframe.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer/Renderer3D_Color.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer/Renderer3D_Outline.glsl");
			
			s_Data->ShaderLibrary->Load("Resources/Shaders/Particles/Renderer3D_ParticleUpdate.glsl");
			
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer3D_Decal.glsl");

			s_Data->ShaderLibrary->Load("Resources/Shaders/Volumetric Lighting/Renderer3D_LightPropagationVolume.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Volumetric Lighting/Renderer3D_VolumetricLighting.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Volumetric Lighting/Renderer3D_GaussianBlur.glsl");
			
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer3D_SSAO.glsl");
			
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer3D_VolumetricClouds.glsl");
			
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer3D_SSR.glsl");
			
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer3D_FXAA.glsl");
			
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer3D_MotionBlur.glsl");
			
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer3D_DOF.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer3D_BokehDraw.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer3D_BokehIsolate.glsl");
			
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer3D_LensFlare.glsl");

			s_Data->ShaderLibrary->Load("Resources/Shaders/Bloom/Renderer3D_BloomIsolate.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Bloom/Renderer3D_BloomBlurHorizontal.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Bloom/Renderer3D_BloomBlurVertical.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Bloom/Renderer3D_AddTexture.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Bloom/Renderer3D_BloomComposite.glsl");
			
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer3D_LensDistortion.glsl");
			
			s_Data->ShaderLibrary->Load("Resources/Shaders/Post Process/Renderer3D_FinalCompositingFX.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Post Process/Renderer3D_FinalCompositing.glsl");

			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer3D_Grass.glsl");
			
			s_Data->ShaderLibrary->Load("Resources/Shaders/Path Tracing/Renderer3D_PathTrace.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Path Tracing/Renderer3D_PathTraceAccumulate.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Path Tracing/Renderer3D_PathTraceDenoise.glsl");

			s_Data->ShaderLibrary->Load("Resources/Shaders/Culling/Renderer3D_Cull.glsl");

			s_Data->ShaderLibrary->Load("Resources/Shaders/SDF/Renderer3D_GenerateSDF.glsl");

			s_Data->ShaderLibrary->Load("Resources/Shaders/VXGI/Renderer3D_VXGIConeTracing.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/VXGI/Renderer3D_VXGIDebugVisualization.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/VXGI/Renderer3D_VXGIMipmap.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/VXGI/Renderer3D_VXGIClear.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/VXGI/Renderer3D_VXGIMerge.glsl");
			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer3D_SSGI.glsl");

			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer3D_PerlinWorleyNoise.glsl");

			s_Data->ShaderLibrary->Load("Resources/Shaders/Renderer3D_TAA.glsl");
		}
		else
		{
			DY_CORE_INFO("Loading Shader Pack...");
			s_Data->ShaderLibrary->LoadShaderPack(s_Config.ShaderPackPath);
		}
		
		// Store shaders required by the renderer
		s_Data->DrawTextureShader = s_Data->ShaderLibrary->Get("Renderer3D_DrawTexture");

		// Initialize individual renderers
		DY_CORE_INFO("Initializing Renderer 2D...");
		Renderer2D::Init();
		
		DY_CORE_INFO("Initializing Scene Renderer...");
		SceneRenderer::Init();
	}

	void Renderer::Shutdown()
	{
		Renderer2D::Shutdown();
		SceneRenderer::Shutdown();

		delete s_Data;
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

	const RendererConfig& Renderer::GetConfig()
	{
		return s_Config;
	}

	void Renderer::SetConfig(const RendererConfig& config)
	{
		s_Config = config;
	}

	RendererTiering& Renderer::GetTiering()
	{
		return s_Config.RendererTieringData;
	}

	const Ref<ShaderLibrary> Renderer::GetShaderLibrary()
	{
		return s_Data->ShaderLibrary;
	}

	void Renderer::SetMacroInShader(Ref<Shader> shader, const std::string& name, const std::string& value)
	{
		//shader->SetMacro(name, value);
		//s_GlobalShaderInfo.DirtyShaders.emplace(shader.Raw());
	}

	const std::map<std::string, std::string>& Renderer::GetGlobalShaderMacros()
	{
		return s_Data->GlobalShaderMacros;
	}

	void Renderer::SetGlobalMacroInShaders(const std::string& name, const std::string& value)
	{
		if (s_Data->GlobalShaderMacros.find(name) != s_Data->GlobalShaderMacros.end())
		{
			if (s_Data->GlobalShaderMacros.at(name) == value)
				return;
		}

		s_Data->GlobalShaderMacros[name] = value;
	}

	void Renderer::SetCameraData(const RendererSharedData::CameraData& cameraData)
	{
		s_Data->CameraUniformBuffer->SetData(&cameraData, sizeof(RendererSharedData::CameraData));
	}

	void Renderer::SetCameraData(const RendererSharedData::CameraData& cameraData, size_t size)
	{
		s_Data->CameraUniformBuffer->SetData(&cameraData, size, 0);
	}

	void Renderer::SetEditorScratchBufferData(const void* data, size_t size, size_t offset)
	{
		// Create the uniform buffer if it doesn't already exist (in case the client program doesn't use it e.g. runtime)
		// Used by editor programs (e.g. Image Editor and Vertex Paint) as well as editor-only engine routines (e.g. Mesh SDF Generation)
		if (!s_Data->EditorUniformBuffer)
			s_Data->EditorUniformBuffer = UniformBuffer::Create(RendererConstants::EditorUniformBufferSize, RendererConstants::Buffers::Editor);

		s_Data->EditorUniformBuffer->SetData(data, size, offset);
	}

	void Renderer::RenderQuad(Ref<Shader> shader)
	{
		RenderCommand::SetDepthTest(false);
		shader->Bind();
		RenderCommand::DrawIndexed(s_Data->QuadVertexArray, 6);
		RenderCommand::SetDepthTest(true);
	}

	void Renderer::DrawFullscreenTexture(Ref<Texture2D> texture)
	{
		if (texture)
			texture->Bind(0);
		RenderQuad(s_Data->DrawTextureShader);
	}

}
