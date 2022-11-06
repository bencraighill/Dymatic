#include "dypch.h"
#include "Dymatic/Renderer/Renderer3D.h"

#include "Dymatic/Renderer/VertexArray.h"
#include "Dymatic/Renderer/Shader.h"
#include "Dymatic/Renderer/UniformBuffer.h"
#include "Dymatic/Renderer/RenderCommand.h"
#include "Dymatic/Renderer/ShaderStorageBuffer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Dymatic/Math/Math.h"

#include <glad/glad.h>

#include "Dymatic/Renderer/Framebuffer.h"

#include <windows.h>

#include "Dymatic/Core/Input.h"

namespace Dymatic {

	struct Renderer3DData
	{
		static const uint32_t MaxBones = 100;
		static const uint32_t GridSizeX = 16, GridSizeY = 9, GridSizeZ = 24;
		static const uint32_t NumClusters = GridSizeX * GridSizeY * GridSizeZ;
		static const uint32_t MaxLightsPerTile = 100;
		static const uint32_t MaxLights = NumClusters * MaxLightsPerTile;

		static const uint32_t MaxCascadeCount = 16;
		static const uint32_t ShadowMapResolution = 4096;

		// Individual Shadow Lights
		static const uint32_t MaxShadowedLights = 10;

		Ref<Framebuffer> ActiveFramebuffer;
		uint32_t ActiveWidth;
		uint32_t ActiveHeight;

		int SelectedEntity = -1;

		struct ModelData
		{
			glm::mat4 transform;
			Ref<Model> model;
			Ref<Animator> animator;
			int entityID = -1;
		};

		// Shadow Data
		Ref<Shader> CSMShadowShader;
		Ref<Framebuffer> DirectionalShadowFramebuffer;
		Ref<Shader> ShadowShader;
		uint32_t NextShadowIndex = 0;
		
		// Buffers
		Ref<Framebuffer> DeferredLightingFBO;

		// Deferred Pass Data
		Ref<Shader> PreDepthShader;
		Ref<Shader> DeferredGBufferShader;
		Ref<Shader> DeferredLightingShader;

		// Cluster Culling
		Ref<Shader> ClusterShader;
		Ref<Shader> ClusterCullLightShader;

		// Outline
		Ref<Framebuffer> OutlineFramebuffer;
		Ref<Shader> OutlineShader;

		// Post Processing Quad
		const glm::vec3 QuadBuffer[4] = {
			{ -1.0f, -1.0f, 0.0f },
			{ 1.0f, -1.0f, 0.0f },
			{ 1.0f,  1.0f, 0.0f },
			{ -1.0f,  1.0f, 0.0f }
		};
		Ref<VertexArray> QuadVertexArray;
		Ref<VertexBuffer> QuadVertexBuffer;

		Ref<Texture2D> SSAONoiseTexture;
		Ref<Shader> SSAOShader;

		Ref<Framebuffer> PreviousFrame;
		Ref<Framebuffer> SSRFramebuffer;
		Ref<Shader> SSRShader;
		Ref<Shader> DrawTextureShader;

		Ref<Framebuffer> FXAAFramebuffer;
		Ref<Shader> FXAAShader;
		Ref<Framebuffer> MotionBlurFramebuffer;
		Ref<Shader> MotionBlurShader;

		Ref<Framebuffer> BloomFramebufferAddA;
		Ref<Framebuffer> BloomFramebufferAddB;
		Ref<Framebuffer> BloomBrightIsolated;
		Ref<Framebuffer> BloomFramebuffer;
		Ref<Shader> BloomIsolateShader;
		Ref<Shader> BloomBlurShader;
		Ref<Shader> BloomAddShader;
		Ref<Shader> BloomCompositeShader;
		Ref<Texture2D> BloomDirtTexture;

		Ref<Framebuffer> DOFFramebuffer;
		Ref<Shader> DOFShader;

		Ref<Framebuffer> LensDistortionFramebuffer;
		Ref<Shader> LensDistortionShader;

		Ref<Shader> FinalCompositingShader;

		// PBR IBL Cubemap
		static const uint32_t MaxSkyboxMipLevels = 5;
		uint32_t SkyboxHDRIID; // Created by client
		Ref<TextureCube> EnvironmentCubemap;
		Ref<TextureCube> IrradianceMap;
		Ref<TextureCube> PrefilterMap;
		Ref<Texture2D> brdfLUTTexture;
		Ref<Shader>	EquirectangularToCubemapShader;
		Ref<Shader>	IrradianceShader;
		Ref<Shader> PrefilterShader;
		Ref<Shader>	brdfShader;
		Ref<Shader>	BackgroundShader;

		// Volumetric Clouds
		Ref<Framebuffer> VolumetricCloudsFramebuffer;
		Ref<Shader> VolumetricCloudsShader;
		Ref<Texture2D> VolumetricNoiseTexture;
		
		// Renderer Stats
		Renderer3D::Statistics Stats;

		float skyboxVertices[24] =
		{
			-1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f, 1.0f, -1.0f,
			-1.0f, 1.0f, -1.0f,
			-1.0f, -1.0f, 1.0f,
			1.0f, -1.0f, 1.0f,
			1.0f, 1.0f, 1.0f,
			-1.0f, 1.0f, 1.0f
		};


		uint32_t SkyboxIndices[36] =
		{
			3, 0, 1, 1, 2, 3,
			4, 0, 3, 3, 7, 4,
			1, 5, 6, 6, 2, 1,
			4, 7, 6, 6, 5, 4,
			3, 2, 6, 6, 7, 3,
			0, 4, 1, 1, 4, 5
		};
		Ref<VertexArray> SkyboxVertexArray;
		Ref<VertexBuffer> SkyboxVertexBuffer;
		Ref<Shader> SkyboxShader;
		Ref<Shader> DynamicSkyShader;

		struct DirectionalLight
		{
			glm::vec4 direction;
			glm::vec3 color;
			float intensity;
		};
		struct PointLight
		{
			glm::vec4 position;
			glm::vec4 color;
			unsigned int enabled;
			float intensity;
			float range;
			int shadowIndex = -1;
		};
		struct VolumeTileAABB {
			glm::vec4 minPoint;
			glm::vec4 maxPoint;
		};
		Ref<ShaderStorageBuffer> ClusterAABB_SSBO;
		Ref<ShaderStorageBuffer> Light_SSBO;
		Ref<ShaderStorageBuffer> LightIndex_SSBO;
		Ref<ShaderStorageBuffer> LightGrid_SSBO;
		Ref<ShaderStorageBuffer> GlobalIndexCountSSBO;

		struct CameraData
		{
			glm::mat4 ViewProjection;
			glm::vec4 ViewPosition;

			glm::mat4 Projection;
			glm::mat4 InverseProjection;
			glm::mat4 View;
			glm::mat4 InverseView;

			glm::uvec4 TileSizes;
			glm::uvec2 ScreenDimensions;
			float Scale;
			float Bias;
			float ZNear;
			float ZFar;
			float BUFF[2];
		};
		CameraData CameraBuffer;
		Ref<UniformBuffer> CameraUniformBuffer;

		struct LightingData
		{
			DirectionalLight DirectionalLight;
			glm::mat4 LightSpaceMatrices[MaxCascadeCount];
			float CascadePlaneDistances[MaxCascadeCount];
			int UsingDirectionalLight;
			int CascadeCount;
			int UsingSkyLight;
		};
		LightingData LightingBuffer;
		Ref<UniformBuffer> LightingUniformBuffer;

		struct ObjectData
		{
			glm::mat4 Model;
			glm::mat4 ModelInverse; 
			glm::mat4 FinalBonesMatrices[MaxBones];
			int EntityID;
			bool Animated;
			float BUFF[2];
		};
		ObjectData ObjectBuffer;
		Ref<UniformBuffer> ObjectUniformBuffer;

		struct PostProcessingData
		{
			glm::vec4 SSAOSamples[64];
			float Exposure = 1.0f;
			int BlurHorizontal = 0;
			float Time = 0.0f;
		};
		PostProcessingData PostProcessingBuffer;
		Ref<UniformBuffer> PostProcessingUniformBuffer;

		// Data Lists
		std::vector<ModelData> ModelDrawData;
		std::vector<PointLight> LightList;
	};

	static Renderer3DData s_Data;

	void Renderer3D::Init()
	{
		DY_PROFILE_FUNCTION();

		// Setup Skybox Rendering Data
		s_Data.SkyboxVertexArray = VertexArray::Create();

		s_Data.SkyboxVertexBuffer = VertexBuffer::Create(8 * 3 * sizeof(float));
		s_Data.SkyboxVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position" }
			});
		s_Data.SkyboxVertexBuffer->SetData(s_Data.skyboxVertices, 8 * 3 * sizeof(float));

		s_Data.SkyboxVertexArray->AddVertexBuffer(s_Data.SkyboxVertexBuffer);
		Ref<IndexBuffer> skyboxIB = IndexBuffer::Create(s_Data.SkyboxIndices, 36);
		s_Data.SkyboxVertexArray->SetIndexBuffer(skyboxIB);

		s_Data.SkyboxShader = Shader::Create("assets/shaders/Renderer3D_Skybox.glsl");
		s_Data.DynamicSkyShader = Shader::Create("assets/shaders/Renderer3D_PreethamSky.glsl");

		s_Data.CameraUniformBuffer = UniformBuffer::Create(sizeof(Renderer3DData::CameraData), 0);
		s_Data.LightingUniformBuffer = UniformBuffer::Create(sizeof(Renderer3DData::LightingData), 1);
		s_Data.ObjectUniformBuffer = UniformBuffer::Create(sizeof(Renderer3DData::ObjectData), 2);
		// Material UBO held statically in material class (binding = 3)
		s_Data.PostProcessingUniformBuffer = UniformBuffer::Create(sizeof(Renderer3DData::PostProcessingData), 4);

		// Lighting SSBO
		s_Data.ClusterAABB_SSBO = ShaderStorageBuffer::Create(sizeof(glm::vec4) * 2 * s_Data.NumClusters, 5, ShaderStorageBufferUsage::STATIC_COPY);
		s_Data.Light_SSBO = ShaderStorageBuffer::Create(s_Data.MaxLights * sizeof(Renderer3DData::PointLight), 6, ShaderStorageBufferUsage::DYNAMIC_DRAW);
		s_Data.LightIndex_SSBO = ShaderStorageBuffer::Create(s_Data.MaxLights * sizeof(unsigned int), 7, ShaderStorageBufferUsage::STATIC_COPY);
		s_Data.LightGrid_SSBO = ShaderStorageBuffer::Create(s_Data.NumClusters * 2 * sizeof(unsigned int), 8, ShaderStorageBufferUsage::STATIC_COPY);
		s_Data.GlobalIndexCountSSBO = ShaderStorageBuffer::Create(sizeof(unsigned int), 9, ShaderStorageBufferUsage::STATIC_COPY);

		// Setup Specular IBL
		{
			s_Data.EquirectangularToCubemapShader = Shader::Create("assets/shaders/Renderer3D_IBLEquirectangularToCubemap.glsl");
			s_Data.IrradianceShader = Shader::Create("assets/shaders/Renderer3D_IBLIrradianceConvolution.glsl");
			s_Data.PrefilterShader = Shader::Create("assets/shaders/Renderer3D_IBLPrefilter.glsl");
			s_Data.brdfShader = Shader::Create("assets/shaders/Renderer3D_IBLbrdf.glsl");
			s_Data.BackgroundShader = Shader::Create("assets/shaders/Renderer3D_IBLBackground.glsl");
		}

		// Setup Deferred Rendering
		{
			{
				s_Data.PreDepthShader = Shader::Create("assets/shaders/Renderer3D_PreDepth.glsl");
				s_Data.DeferredGBufferShader = Shader::Create("assets/shaders/Renderer3D_DeferredPrePass.glsl");
			}

			{
				s_Data.DeferredLightingShader = Shader::Create("assets/shaders/Renderer3D_DeferredLighting.glsl");

				FramebufferSpecification fbSpec;
				fbSpec.Width = 1920;
				fbSpec.Height = 1080;
				fbSpec.Attachments = { TextureFormat::RGBA16F, TextureFormat::Depth };
				s_Data.DeferredLightingFBO = Framebuffer::Create(fbSpec);

				s_Data.ClusterShader = Shader::Create("assets/shaders/Renderer3D_ClusterShader.glsl");
				s_Data.ClusterCullLightShader = Shader::Create("assets/shaders/Renderer3D_ClusterCullLightShader.glsl");
			}
		}

		// Setup Post Processing Quad Render Data
		{
			s_Data.QuadVertexArray = VertexArray::Create();

			s_Data.QuadVertexBuffer = VertexBuffer::Create(sizeof(s_Data.QuadBuffer));
			s_Data.QuadVertexBuffer->SetLayout({
				{ ShaderDataType::Float3, "a_Position" }
				});
			s_Data.QuadVertexArray->AddVertexBuffer(s_Data.QuadVertexBuffer);

			uint32_t quadIndices[6];

			quadIndices[0] = 0;
			quadIndices[1] = 1;
			quadIndices[2] = 2;
			quadIndices[3] = 2;
			quadIndices[4] = 3;
			quadIndices[5] = 0;

			Ref<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, 6);
			s_Data.QuadVertexArray->SetIndexBuffer(quadIB);

			s_Data.QuadVertexBuffer->SetData(&s_Data.QuadBuffer, sizeof(s_Data.QuadBuffer));

			FramebufferSpecification fbSpec;
			fbSpec.Attachments = { TextureFormat::RGBA16F, TextureFormat::Depth };
			fbSpec.Width = 1920;
			fbSpec.Height = 1080;

			// SSAO
			{
				std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
				std::default_random_engine generator;

				// Setup Noise Texture
				{
					const int noiseSize = 4;
					s_Data.SSAONoiseTexture = Texture2D::Create(noiseSize, noiseSize);

					unsigned char ssaoNoise[noiseSize * noiseSize * 4];
					for (size_t i = 0; i < noiseSize * noiseSize; i++)
					{
						ssaoNoise[i * 4 + 0] = randomFloats(generator);
						ssaoNoise[i * 4 + 1] = randomFloats(generator);
						ssaoNoise[i * 4 + 2] = 0.0f;
						ssaoNoise[i * 4 + 3] = 1.0f;
					}

					s_Data.SSAONoiseTexture->SetData(&ssaoNoise, noiseSize * noiseSize * 4);
				}

				// Setup Kernel Values
				{
					const int kernelSize = 64;

					auto& ssaoKernel = s_Data.PostProcessingBuffer.SSAOSamples;
					for (size_t i = 0; i < kernelSize; ++i)
					{
						glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
						sample = glm::normalize(sample);
						sample *= randomFloats(generator);
						float scale = float(i) / float(kernelSize);

						// scale samples s.t. they're more aligned to center of kernel
						scale = glm::lerp(0.1f, 1.0f, scale * scale);
						sample *= scale;
						ssaoKernel[i] = glm::vec4(sample, 1.0f);
					}
				}

				// Setup Shader
				s_Data.SSAOShader = Shader::Create("assets/shaders/Renderer3D_SSAO.glsl");
			}

			// Setup Volumetric Clouds
			{
				s_Data.VolumetricCloudsFramebuffer = Framebuffer::Create(fbSpec);
				s_Data.VolumetricCloudsShader = Shader::Create("assets/shaders/Renderer3D_VolumetricClouds.glsl");
				s_Data.VolumetricNoiseTexture = Texture2D::Create("assets/textures/NoiseTexture.png");
			}

			// SSR
			s_Data.PreviousFrame = Framebuffer::Create(fbSpec);
			s_Data.SSRFramebuffer = Framebuffer::Create(fbSpec);
			s_Data.SSRShader = Shader::Create("assets/shaders/Renderer3D_SSR.glsl");
			s_Data.DrawTextureShader = Shader::Create("assets/shaders/Renderer3D_DrawTexture.glsl");

			// FXAA
			s_Data.FXAAFramebuffer = Framebuffer::Create(fbSpec);
			s_Data.FXAAShader = Shader::Create("assets/shaders/Renderer3D_FXAA.glsl");

			// Motion Blur
			s_Data.MotionBlurFramebuffer = Framebuffer::Create(fbSpec);
			s_Data.MotionBlurShader = Shader::Create("assets/shaders/Renderer3D_MotionBlur.glsl");

			// DOF
			s_Data.DOFFramebuffer = Framebuffer::Create(fbSpec);
			s_Data.DOFShader = Shader::Create("assets/shaders/Renderer3D_DOF.glsl");

			// Bloom
			s_Data.BloomBrightIsolated = Framebuffer::Create(fbSpec);
			s_Data.BloomFramebufferAddA = Framebuffer::Create(fbSpec);
			s_Data.BloomFramebufferAddB = Framebuffer::Create(fbSpec);
			s_Data.BloomFramebuffer = Framebuffer::Create(fbSpec);
			s_Data.BloomIsolateShader = Shader::Create("assets/shaders/Renderer3D_BloomIsolate.glsl");
			s_Data.BloomBlurShader = Shader::Create("assets/shaders/Renderer3D_Bloom.glsl");
			s_Data.BloomAddShader = Shader::Create("assets/shaders/Renderer3D_BloomAdd.glsl");
			s_Data.BloomCompositeShader = Shader::Create("assets/shaders/Renderer3D_BloomComposite.glsl");
			s_Data.BloomDirtTexture = Texture2D::Create("assets/textures/DirtMaskTexture.png");

			// Lens Distortion
			s_Data.LensDistortionFramebuffer = Framebuffer::Create(fbSpec);
			s_Data.LensDistortionShader = Shader::Create("assets/shaders/Renderer3D_LensDistortion.glsl");

			// Final Compositing
			s_Data.FinalCompositingShader = Shader::Create("assets/shaders/Renderer3D_FinalCompositing.glsl");
		}

		// Shadow Buffer
		{
			FramebufferSpecification fbSpec;
			fbSpec.Attachments = { TextureFormat::Depth };

			// Directional Light Shadow Framebuffer
			fbSpec.Width = s_Data.ShadowMapResolution;
			fbSpec.Height = s_Data.ShadowMapResolution;
			s_Data.DirectionalShadowFramebuffer = Framebuffer::Create(fbSpec);

			// Shadow Framebuffer
			//fbSpec.Width = s_Data.ShadowsResolution;
			//fbSpec.Height = s_Data.ShadowsResolution;
			//s_Data.ShadowFramebuffer = Framebuffer::Create(fbSpec);
			//
			//// Shadow Atlas
			//s_Data.ShadowAtlas = Texture2D::Create(s_Data.ShadowAtlasResolution, s_Data.ShadowAtlasResolution);

			// Shadow Shader CSM
			s_Data.CSMShadowShader = Shader::Create("assets/shaders/Renderer3D_ShadowDepthCSM.glsl");
			s_Data.ShadowShader = Shader::Create("assets/shaders/Renderer3D_ShadowDepth.glsl");
		}

		// Outline
		{
			FramebufferSpecification fbSpec;
			fbSpec.Attachments = { TextureFormat::Depth };
			fbSpec.Width = 1920 * 2;
			fbSpec.Height = 1080 * 2;
			s_Data.OutlineFramebuffer = Framebuffer::Create(fbSpec);

			s_Data.OutlineShader = Shader::Create("assets/shaders/Renderer3D_Outline.glsl");
		}
		
		// Setup PBR IBL Cubemap
		{
			s_Data.EnvironmentCubemap = TextureCube::Create(512, 512, 1, TextureFormat::RGB16F);
			s_Data.IrradianceMap = TextureCube::Create(32, 32, 1, TextureFormat::RGB16F);
			s_Data.PrefilterMap = TextureCube::Create(128, 128, s_Data.MaxSkyboxMipLevels, TextureFormat::RGB16F);
			s_Data.brdfLUTTexture = Texture2D::Create(512, 512, TextureFormat::RG16F);
		}
	}

	void Renderer3D::Shutdown()
	{
		DY_PROFILE_FUNCTION();
	}

	void Renderer3D::BeginScene(const Camera& camera, const glm::mat4& transform)
	{
		DY_PROFILE_FUNCTION();

		s_Data.CameraBuffer.ViewProjection = camera.GetProjection() * glm::inverse(transform);
		s_Data.CameraBuffer.ViewPosition = transform[3];

		s_Data.CameraBuffer.Projection = camera.GetProjection();
		s_Data.CameraBuffer.InverseProjection = glm::inverse(s_Data.CameraBuffer.Projection);
		s_Data.CameraBuffer.View = glm::inverse(transform);
		s_Data.CameraBuffer.InverseView = transform;

		auto sizeX = (unsigned int)std::ceilf(s_Data.ActiveWidth / (float)s_Data.GridSizeX);
		s_Data.CameraBuffer.TileSizes = { s_Data.GridSizeX, s_Data.GridSizeY, s_Data.GridSizeZ, sizeX };

		s_Data.CameraBuffer.ScreenDimensions = { s_Data.ActiveWidth, s_Data.ActiveHeight };
		s_Data.CameraBuffer.ZNear = ((SceneCamera*)&camera)->GetPerspectiveNearClip();
		s_Data.CameraBuffer.ZFar = ((SceneCamera*)&camera)->GetPerspectiveFarClip();

		// Simple pre-calculation to reduce use of log function - TODO: Should really be under lighting
		s_Data.CameraBuffer.Scale = (float)s_Data.GridSizeZ / std::log2f(s_Data.CameraBuffer.ZFar / s_Data.CameraBuffer.ZNear);
		s_Data.CameraBuffer.Bias = -((float)s_Data.GridSizeZ * std::log2f(s_Data.CameraBuffer.ZNear) / std::log2f(s_Data.CameraBuffer.ZFar / s_Data.CameraBuffer.ZNear));

		s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer3DData::CameraData));

		// Reset Lighting Data
		s_Data.LightingBuffer.UsingDirectionalLight = false;
		s_Data.LightingBuffer.UsingSkyLight = false;
	}

	void Renderer3D::BeginScene(const EditorCamera& camera)
	{
		DY_PROFILE_FUNCTION();

		s_Data.CameraBuffer.ViewProjection = camera.GetViewProjection();
		s_Data.CameraBuffer.ViewPosition = glm::vec4(camera.GetPosition(), 1.0f);

		s_Data.CameraBuffer.Projection = camera.GetProjection();
		s_Data.CameraBuffer.InverseProjection = glm::inverse(s_Data.CameraBuffer.Projection);
		s_Data.CameraBuffer.View = camera.GetViewMatrix();
		s_Data.CameraBuffer.InverseView = glm::inverse(s_Data.CameraBuffer.View);

		auto sizeX = (unsigned int)std::ceilf(s_Data.ActiveWidth / (float)s_Data.GridSizeX);
		s_Data.CameraBuffer.TileSizes = { s_Data.GridSizeX, s_Data.GridSizeY, s_Data.GridSizeZ, sizeX };

		s_Data.CameraBuffer.ScreenDimensions = { s_Data.ActiveWidth, s_Data.ActiveHeight };
		s_Data.CameraBuffer.ZNear = camera.GetNearClip();
		s_Data.CameraBuffer.ZFar = camera.GetFarClip();

		// Simple pre-calculation to reduce use of log function - TODO: Should really be under lighting
		s_Data.CameraBuffer.Scale = (float)s_Data.GridSizeZ / std::log2f(s_Data.CameraBuffer.ZFar / s_Data.CameraBuffer.ZNear);
		s_Data.CameraBuffer.Bias = - ((float)s_Data.GridSizeZ * std::log2f(s_Data.CameraBuffer.ZNear) / std::log2f(s_Data.CameraBuffer.ZFar / s_Data.CameraBuffer.ZNear));

		s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer3DData::CameraData));

		// Reset Lighting Data
		s_Data.LightingBuffer.UsingDirectionalLight = false;
		s_Data.LightingBuffer.UsingSkyLight = false;
	}

	void Renderer3D::EndScene()
	{
		DY_PROFILE_FUNCTION();

		Flush();
	}

	void Renderer3D::Flush()
	{
		DY_PROFILE_FUNCTION();
	}

	void Renderer3D::Resize()
	{
		s_Data.ActiveWidth = s_Data.ActiveFramebuffer->GetSpecification().Width;
		s_Data.ActiveHeight = s_Data.ActiveFramebuffer->GetSpecification().Height;

		// Update All Framebuffer Sizes
		s_Data.MotionBlurFramebuffer->Resize(s_Data.ActiveWidth, s_Data.ActiveHeight);
		s_Data.SSRFramebuffer->Resize(s_Data.ActiveWidth, s_Data.ActiveHeight);

		s_Data.OutlineFramebuffer->Resize(s_Data.ActiveWidth, s_Data.ActiveHeight);
		s_Data.DeferredLightingFBO->Resize(s_Data.ActiveWidth, s_Data.ActiveHeight);
	}

	void Renderer3D::SetActiveFramebuffer(Ref<Framebuffer> framebuffer)
	{
		s_Data.ActiveFramebuffer = framebuffer;
	}

	void Renderer3D::UpdateTimestep(Timestep ts)
	{
		s_Data.PostProcessingBuffer.Time += ts;
	}

	void Renderer3D::SubmitModel(glm::mat4 transform, Ref<Model> model, int entityID)
	{
		SubmitModel(transform, model, nullptr, entityID);
	}

	void Renderer3D::SubmitModel(glm::mat4 transform, Ref<Model> model, Ref<Animator> animator, int entityID)
	{
		s_Data.ModelDrawData.push_back({ transform, model, animator, entityID });
	}

	void Renderer3D::SubmitStaticMesh(const glm::mat4& transform, StaticMeshComponent& mesh, int entityID)
	{
		if (mesh.m_Model)
			SubmitModel(transform, mesh.m_Model, mesh.m_Animator, entityID);
	}

	static void RenderQuad(Ref<Shader> shader)
	{
		glDisable(GL_DEPTH_TEST);
		shader->Bind();
		RenderCommand::DrawIndexed(s_Data.QuadVertexArray, 6);
		glEnable(GL_DEPTH_TEST);
	}

	static void RenderCube()
	{
		glDisable(GL_DEPTH_TEST);
		RenderCommand::DrawIndexed(s_Data.SkyboxVertexArray, 36);
		glEnable(GL_DEPTH_TEST);
	}

	static std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4& viewProjection)
	{
		auto inverseViewProjection = glm::inverse(viewProjection);

		std::vector<glm::vec4> frustumCorners;
		for (unsigned int x = 0; x < 2; ++x)
		{
			for (unsigned int y = 0; y < 2; ++y)
			{
				for (unsigned int z = 0; z < 2; ++z)
				{
					const glm::vec4 pt = inverseViewProjection * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
					frustumCorners.push_back(pt / pt.w);
				}
			}
		}

		return frustumCorners;
	}

	glm::mat4 GetLightSpaceMatrix(const float nearPlane, const float farPlane)
	{
		if (s_Data.CameraBuffer.ScreenDimensions.x == 0 || s_Data.CameraBuffer.ScreenDimensions.y == 0)
			return {};

		auto& viewProjection = glm::perspective(glm::radians(/*s_Data.CameraFOV*/45.0f), (float)s_Data.CameraBuffer.ScreenDimensions.x / (float)s_Data.CameraBuffer.ScreenDimensions.y, nearPlane, farPlane) * s_Data.CameraBuffer.View;

		const auto corners = GetFrustumCornersWorldSpace(viewProjection);

		glm::vec3 center = glm::vec3(0, 0, 0);
		for (const auto& v : corners)
		{
			center += glm::vec3(v);
		}
		center /= corners.size();

		const auto lightView = glm::lookAt(center - glm::vec3(s_Data.LightingBuffer.DirectionalLight.direction), center, glm::vec3(0.0f, 1.0f, 0.0f));

		float minX = std::numeric_limits<float>::max();
		float maxX = std::numeric_limits<float>::min();
		float minY = std::numeric_limits<float>::max();
		float maxY = std::numeric_limits<float>::min();
		float minZ = std::numeric_limits<float>::max();
		float maxZ = std::numeric_limits<float>::min();
		for (const auto& v : corners)
		{
			const auto trf = lightView * v;
			minX = std::min(minX, trf.x);
			maxX = std::max(maxX, trf.x);
			minY = std::min(minY, trf.y);
			maxY = std::max(maxY, trf.y);
			minZ = std::min(minZ, trf.z);
			maxZ = std::max(maxZ, trf.z);
		}

		// Controls Z Fighting Offset
		constexpr float zMult = 10.0f;
		if (minZ < 0)
			minZ *= zMult;
		else
			minZ /= zMult;
		if (maxZ < 0)
			maxZ /= zMult;
		else
			maxZ *= zMult;

		const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

		return lightProjection * lightView;
	}

	std::vector<glm::mat4> GetLightSpaceMatrices()
	{
		std::vector<glm::mat4> ret;
		for (size_t i = 0; i < s_Data.LightingBuffer.CascadeCount + 1; i++)
		{
			if (i == 0)
				ret.push_back(GetLightSpaceMatrix(s_Data.CameraBuffer.ZNear, s_Data.LightingBuffer.CascadePlaneDistances[i]));
			else if (i < s_Data.LightingBuffer.CascadeCount)
				ret.push_back(GetLightSpaceMatrix(s_Data.LightingBuffer.CascadePlaneDistances[i - 1], s_Data.LightingBuffer.CascadePlaneDistances[i]));
			else
				ret.push_back(GetLightSpaceMatrix(s_Data.LightingBuffer.CascadePlaneDistances[i - 1], s_Data.CameraBuffer.ZFar));
		}
		return ret;
	}

	void Renderer3D::RenderScene()
	{
		// Submit lights when changes occur
		if (Input::IsKeyPressed(Key::Q))
		{
			Renderer3DData::PointLight* lights = (Renderer3DData::PointLight*)s_Data.Light_SSBO->MapBuffer();
			memset(lights, 0, sizeof(Renderer3DData::PointLight) * s_Data.MaxLights);
			for (size_t i = 0; i < s_Data.LightList.size(); i++)
			{
				lights[i] = s_Data.LightList[i];
			}
			s_Data.Light_SSBO->UnmapBuffer();
		}

		// Point Light Shadow Pass
		glCullFace(GL_FRONT);
		//glDisable(GL_CULL_FACE);
		{
			static bool init = false;
			
			static uint32_t lightDepthMaps, lightDepthFBO;
			const unsigned int ShadowResolution = 1024;

			if (!init)
			{
				init = true;
				
				// Create Cubemap Array
				{
					// create depth cubemap texture
					glGenTextures(1, &lightDepthMaps);
					glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, lightDepthMaps);

					glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_DEPTH_COMPONENT32F, ShadowResolution, ShadowResolution, (s_Data.MaxShadowedLights * 6), 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

					glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

					//glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);

					// attach depth texture as FBO's depth buffer
					glGenFramebuffers(1, &lightDepthFBO);
					glBindFramebuffer(GL_FRAMEBUFFER, lightDepthFBO);
					glDrawBuffer(GL_NONE);
					glReadBuffer(GL_NONE);

					glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, lightDepthMaps, 0);

					glBindFramebuffer(GL_FRAMEBUFFER, 0);
				}
			}

			glViewport(0, 0, ShadowResolution, ShadowResolution);
			glBindFramebuffer(GL_FRAMEBUFFER, lightDepthFBO);

			RenderCommand::Clear();

			for (auto& light : s_Data.LightList)
			{
				if (light.shadowIndex == -1)
					continue;

				uint32_t index = light.shadowIndex;

				// Render Light from cameras perspective to shadow atlas, repeat for all six tiles

				float near_plane = 0.1f;
				float far_plane = 25.0f;
				glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)ShadowResolution / (float)ShadowResolution, near_plane, far_plane);
				glm::vec3 lightPos = light.position;

				// TODO: This is bad --> should not reuse existing UBOs, have seperate one
				s_Data.LightingBuffer.LightSpaceMatrices[0] = (shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
				s_Data.LightingBuffer.LightSpaceMatrices[1] = (shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
				s_Data.LightingBuffer.LightSpaceMatrices[2] = (shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
				s_Data.LightingBuffer.LightSpaceMatrices[3] = (shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
				s_Data.LightingBuffer.LightSpaceMatrices[4] = (shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
				s_Data.LightingBuffer.LightSpaceMatrices[5] = (shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
				s_Data.LightingBuffer.CascadeCount = index;
				s_Data.LightingBuffer.CascadePlaneDistances[0] = far_plane;
				s_Data.LightingUniformBuffer ->SetData(&s_Data.LightingBuffer, sizeof(Renderer3DData::LightingData));

				for (auto& model : s_Data.ModelDrawData)
				{
					if (!model.model)
						continue;
				
					s_Data.ObjectBuffer.Model = model.transform;
					s_Data.ObjectBuffer.ModelInverse = glm::transpose(glm::inverse(model.transform));
				
					if (model.animator)
						if (s_Data.ObjectBuffer.Animated = model.animator->HasAnimation())
						{
							auto& matricies = model.animator->GetFinalBoneMatrices();
							for (size_t i = 0; i < matricies.size(); i++)
								s_Data.ObjectBuffer.FinalBonesMatrices[i] = matricies[i];
						}
				
					s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(Renderer3DData::ObjectBuffer));
				
					model.model->Draw(s_Data.ShadowShader);
				}
			}
			glBindTextureUnit(7, lightDepthMaps);
		}
		//glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		// Setup CSM lighting data
		{
			s_Data.LightingBuffer.CascadeCount = 4;

			auto& zfar = s_Data.CameraBuffer.ZFar;
			s_Data.LightingBuffer.CascadePlaneDistances[0] = zfar / 50.0f;
			s_Data.LightingBuffer.CascadePlaneDistances[1] = zfar / 25.0f;
			s_Data.LightingBuffer.CascadePlaneDistances[2] = zfar / 10.0f;
			s_Data.LightingBuffer.CascadePlaneDistances[3] = zfar / 2.0f;

			auto& matricies = GetLightSpaceMatrices();
			for (size_t i = 0; i < matricies.size(); i++)
				s_Data.LightingBuffer.LightSpaceMatrices[i] = matricies[i];

			// Submit Lighting Data
			s_Data.LightingUniformBuffer->SetData(&s_Data.LightingBuffer, sizeof(Renderer3DData::LightingData));
		}

		// CSM Shadow Pass
		glCullFace(GL_FRONT);
		glEnable(GL_DEPTH_CLAMP);
		glDisable(GL_CULL_FACE);
		{
			// Render scene from light's point of view
			//s_Data.ShadowFramebuffer->Bind();

			static bool init = false;
			static uint32_t lightFBO, lightDepthMaps;
			if (!init)
			{
				init = true;
				glGenFramebuffers(1, &lightFBO);

				glGenTextures(1, &lightDepthMaps);
				glBindTexture(GL_TEXTURE_2D_ARRAY, lightDepthMaps);
				glTexImage3D(
					GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, s_Data.ShadowMapResolution, s_Data.ShadowMapResolution, int(4) + 1,
					0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

				constexpr float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
				glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, bordercolor);

				glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);
				glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, lightDepthMaps, 0);
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);

				int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				if (status != GL_FRAMEBUFFER_COMPLETE)
				{
					std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!";
					throw 0;
				}

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
			glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);
			glViewport(0, 0, s_Data.ShadowMapResolution, s_Data.ShadowMapResolution);

			RenderCommand::Clear();

			for (auto& model : s_Data.ModelDrawData)
			{
				if (!model.model)
					continue;

				s_Data.ObjectBuffer.Model = model.transform;
				s_Data.ObjectBuffer.ModelInverse = glm::transpose(glm::inverse(model.transform));

				if (model.animator)
					if (s_Data.ObjectBuffer.Animated = model.animator->HasAnimation())
					{
						auto& matricies = model.animator->GetFinalBoneMatrices();
						for (size_t i = 0; i < matricies.size(); i++)
							s_Data.ObjectBuffer.FinalBonesMatrices[i] = matricies[i];
					}

				s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(Renderer3DData::ObjectBuffer));

				model.model->Draw(s_Data.CSMShadowShader);
			}

			// Submit Shadow Map
			//s_Data.ShadowFramebuffer->BindDepthSampler(6);

			glActiveTexture(GL_TEXTURE0 + 6);
			glBindTexture(GL_TEXTURE_2D_ARRAY, lightDepthMaps);
		}
		glEnable(GL_CULL_FACE);
		glDisable(GL_DEPTH_CLAMP);
		glCullFace(GL_BACK);

		s_Data.ActiveFramebuffer->Bind();

		// Pre Depth Pass
		for (auto& model : s_Data.ModelDrawData)
		{
			if (!model.model)
				continue;

			s_Data.ObjectBuffer.Model = model.transform;
			s_Data.ObjectBuffer.ModelInverse = glm::transpose(glm::inverse(model.transform));
			s_Data.ObjectBuffer.EntityID = model.entityID;

			if (model.animator)
				if (s_Data.ObjectBuffer.Animated = model.animator->HasAnimation())
				{
					auto& matricies = model.animator->GetFinalBoneMatrices();
					for (size_t i = 0; i < matricies.size(); i++)
						s_Data.ObjectBuffer.FinalBonesMatrices[i] = matricies[i];
				}

			s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(Renderer3DData::ObjectData));

			model.model->Draw(s_Data.PreDepthShader);

			s_Data.Stats.DrawCalls++;
		}

		glDepthFunc(GL_EQUAL); // (Depth already calculated in PreDepth pass)
		// Render Actual Models with
		Renderer3DData::ModelData* selectedModel = nullptr;
		for (auto& model : s_Data.ModelDrawData)
		{
			if (!model.model)
				continue;

			s_Data.ObjectBuffer.Model = model.transform;
			s_Data.ObjectBuffer.ModelInverse = glm::transpose(glm::inverse(model.transform));
			s_Data.ObjectBuffer.EntityID = model.entityID;

			if (model.animator)
				if (s_Data.ObjectBuffer.Animated = model.animator->HasAnimation())
				{
					auto& matricies = model.animator->GetFinalBoneMatrices();
					for (size_t i = 0; i < matricies.size(); i++)
						s_Data.ObjectBuffer.FinalBonesMatrices[i] = matricies[i];
				}

			s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(Renderer3DData::ObjectBuffer));

			if (model.entityID == s_Data.SelectedEntity)
			{
				selectedModel = &model;

				model.model->Draw(s_Data.DeferredGBufferShader);

				glDepthFunc(GL_LESS);
				s_Data.OutlineFramebuffer->Bind();
				RenderCommand::Clear();
				model.model->Draw(s_Data.DeferredGBufferShader, false);

				glDepthFunc(GL_EQUAL);
				s_Data.ActiveFramebuffer->Bind();
			}
			else
				model.model->Draw(s_Data.DeferredGBufferShader);

			s_Data.Stats.DrawCalls++;
		}
		glDepthFunc(GL_LESS);

		// Deferred Rendering
		{
			s_Data.ClusterShader->Dispatch(16, 9, 24); // Should only be run when camera changes
			s_Data.ClusterCullLightShader->Dispatch(1, 1, 6); // Should be run every frame

			s_Data.ActiveFramebuffer->BindColorSampler(0, 0);	// Albedo
			s_Data.ActiveFramebuffer->BindDepthSampler(1);		// Depth
			s_Data.ActiveFramebuffer->BindColorSampler(2, 2);	// Normal
			s_Data.ActiveFramebuffer->BindColorSampler(3, 3);	// Roughness + Metallic + Specular

			s_Data.EnvironmentCubemap->Bind(8);						// IBL Environment Cubemap
			s_Data.IrradianceMap->Bind(9);						// IBL Irradiance Cubemap
			s_Data.PrefilterMap->Bind(10);						// IBL Prefilter Cubemap
			s_Data.brdfLUTTexture->Bind(11);						// IBL brdf LUT Texture

			s_Data.DeferredLightingFBO->Bind();
			RenderQuad(s_Data.DeferredLightingShader);
		}

		// Pass in the current post processing data
		s_Data.PostProcessingUniformBuffer->SetData(&s_Data.PostProcessingBuffer, sizeof(Renderer3DData::PostProcessingData));
		Ref<Framebuffer> PriorStageFramebuffer = s_Data.DeferredLightingFBO;

		// SSAO
		{
			s_Data.ActiveFramebuffer->BindDepthSampler(0);
			s_Data.ActiveFramebuffer->BindColorSampler(1, 2); // Normal
			s_Data.SSAONoiseTexture->Bind(2);
			RenderQuad(s_Data.SSAOShader);
		}

		// Auto Exposure

		// Make a frame copy for SSR
		{
			s_Data.PreviousFrame->Bind();
			PriorStageFramebuffer->BindColorSampler(0, 0);
			RenderQuad(s_Data.DrawTextureShader);
		}
		
		// Screen Space Reflections
		{
			s_Data.SSRFramebuffer->Bind();
			RenderCommand::Clear();

			s_Data.PreviousFrame->BindColorSampler(0, 0);		// Color Buffer
			s_Data.ActiveFramebuffer->BindDepthSampler(1);		// Depth Buffer

			s_Data.ActiveFramebuffer->BindColorSampler(2, 2);	// Normal
			s_Data.ActiveFramebuffer->BindColorSampler(3, 3);	// Roughness + Metallic + Specular
			RenderQuad(s_Data.SSRShader);

			PriorStageFramebuffer = s_Data.SSRFramebuffer;
		}

		// Motion Blur
		{
			s_Data.MotionBlurFramebuffer->Bind();
			RenderCommand::Clear();

			static glm::mat4 previousViewProjectionMatrix = glm::mat4(1.0f);
			s_Data.MotionBlurShader->Bind();

			s_Data.ObjectBuffer.Model = previousViewProjectionMatrix;
			s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(Renderer3DData::ObjectBuffer));

			PriorStageFramebuffer->BindColorSampler(0, 0);
			PriorStageFramebuffer->BindDepthSampler(1);

			RenderQuad(s_Data.MotionBlurShader);
			previousViewProjectionMatrix = s_Data.CameraBuffer.ViewProjection;

			PriorStageFramebuffer = s_Data.MotionBlurFramebuffer;
		}

		// Bloom
		{
			static Ref<Framebuffer> fbolist[14];
			static bool init = false;
			if (!init)
			{
				init = true;
		
				FramebufferSpecification fbspec;
				fbspec.Attachments = { TextureFormat::RGBA16F };
				fbspec.Width = 1920 * 0.5f;
				fbspec.Height = 1080 * 0.5f;
				for (size_t i = 0; i < 13; i += 2)
				{
					fbolist[i] = Framebuffer::Create(fbspec);
					fbolist[i + 1] = Framebuffer::Create(fbspec);
					fbspec.Width *= 0.5f;
					fbspec.Height *= 0.5f;
				}
			}
		
			// Isolate Exposed
			s_Data.BloomFramebuffer->Bind();
			RenderCommand::Clear();
			PriorStageFramebuffer->BindColorSampler(0, 0);
			RenderQuad(s_Data.BloomIsolateShader);
		
			Ref<Framebuffer> CurrentLevelFramebuffer = s_Data.BloomFramebuffer;
		
			for (size_t i = 0; i < 13; i++)
			{
				fbolist[i]->Bind();
				RenderCommand::Clear();

				// Swap between horizontal and vertical blurring
				s_Data.PostProcessingBuffer.BlurHorizontal = i % 2 == 0;
				s_Data.PostProcessingUniformBuffer->SetData(&s_Data.PostProcessingBuffer, sizeof(Renderer3DData::PostProcessingData));
		
				CurrentLevelFramebuffer->BindColorSampler(0, 0);
				RenderQuad(s_Data.BloomBlurShader);
				CurrentLevelFramebuffer = fbolist[i];
			}
		
			s_Data.BloomFramebufferAddA->Bind();
			RenderCommand::Clear();
			s_Data.BloomFramebufferAddB->Bind();
			RenderCommand::Clear();
		
			for (size_t i = 0; i < 13; i++)
			{
				bool even = i % 2 == 0;
				(even ? s_Data.BloomFramebufferAddB : s_Data.BloomFramebufferAddA)->Bind();
				(even ? s_Data.BloomFramebufferAddA : s_Data.BloomFramebufferAddB)->BindColorSampler(0, 0);
				fbolist[i]->BindColorSampler(1, 0);
		
				RenderQuad(s_Data.BloomAddShader);
			}
		
			s_Data.BloomFramebuffer->Bind();
			RenderCommand::Clear();
			PriorStageFramebuffer->BindColorSampler(0, 0);
			s_Data.BloomFramebufferAddB->BindColorSampler(1, 0);
			s_Data.BloomDirtTexture->Bind(2);
			RenderQuad(s_Data.BloomCompositeShader);
			PriorStageFramebuffer = s_Data.BloomFramebuffer;
		}

		// FXAA
		{
			s_Data.FXAAFramebuffer->Bind();

			PriorStageFramebuffer->BindColorSampler(0, 0);
			RenderQuad(s_Data.FXAAShader);
			PriorStageFramebuffer = s_Data.FXAAFramebuffer;
		}

		// Get Average Exposure
		{
			//PriorStageFramebuffer->ReadPixel();
		}

		// DOF
		if (false)
		{
			s_Data.DOFFramebuffer->Bind();
			RenderCommand::Clear();
			PriorStageFramebuffer->BindColorSampler(0, 0);
			s_Data.ActiveFramebuffer->BindDepthSampler(1);
			RenderQuad(s_Data.DOFShader);
			PriorStageFramebuffer = s_Data.DOFFramebuffer;
		}

		// Lens Distortion
		{
			s_Data.LensDistortionFramebuffer->Bind();
			RenderCommand::Clear();
			PriorStageFramebuffer->BindColorSampler(0, 0);
			RenderQuad(s_Data.LensDistortionShader);
			PriorStageFramebuffer = s_Data.LensDistortionFramebuffer;
		}

		// Draw Final Quad
		{
			s_Data.ActiveFramebuffer->Bind();

			// Bind Color Buffer from prior stage
			PriorStageFramebuffer->BindColorSampler(0, 0);

			// Bind entity ID buffer from initial render pass
			s_Data.ActiveFramebuffer->BindColorSampler(1, 1);

			glDepthMask(GL_FALSE);
			RenderQuad(s_Data.FinalCompositingShader);
			glDepthMask(GL_TRUE);
		}

		// Outline Post Processing
		if (selectedModel)
		{
			s_Data.ActiveFramebuffer->Bind();
			s_Data.OutlineFramebuffer->BindDepthSampler(0);
			RenderQuad(s_Data.OutlineShader);
		}

		s_Data.ModelDrawData.clear();
		s_Data.LightList.clear();
		s_Data.NextShadowIndex = 0;
	}

	void Renderer3D::SubmitDirectionalLight(const glm::mat4& transform, DirectionalLightComponent& lightComponent)
	{
		auto& light = s_Data.LightingBuffer.DirectionalLight;

		glm::vec3 translation, rotation, scale;
		Math::DecomposeTransform(transform, translation, rotation, scale);

		glm::vec3 direction;
		direction.x = cos(rotation.z) * cos(rotation.y);
		direction.y = sin(rotation.z) * cos(rotation.y);
		direction.z = -sin(rotation.y);

		s_Data.LightingBuffer.DirectionalLight.direction = glm::vec4(direction, 1.0f);
		s_Data.LightingBuffer.DirectionalLight.color = lightComponent.Color;
		s_Data.LightingBuffer.DirectionalLight.intensity = lightComponent.Intensity;
		s_Data.LightingBuffer.UsingDirectionalLight = 1;
	}

	void Renderer3D::SubmitPointLight(const glm::mat4& transform, PointLightComponent& lightComponent)
	{
		s_Data.LightList.push_back({});
		auto& light = s_Data.LightList.back();

		light.position = transform[3];
		light.color = glm::vec4(lightComponent.Color, 1.0f);
		light.intensity = lightComponent.Intensity;
		light.range = lightComponent.Radius;
		light.enabled = true;
		light.shadowIndex = ((s_Data.NextShadowIndex < s_Data.MaxShadowedLights && lightComponent.CastsShadows) ? (s_Data.NextShadowIndex++) : -1);
	}

	void Renderer3D::SubmitSpotLight(const glm::mat4& transform, SpotLightComponent& lightComponent)
	{
		//auto& light = s_Data.LightingBuffer.SpotLight;
		//
		//glm::vec3 rotation, scale;
		//Math::DecomposeTransform(transform, light.position, rotation, scale);
		//
		//glm::vec3 direction;
		//direction.x = cos(rotation.z) * cos(rotation.y);
		//direction.y = sin(rotation.z) * cos(rotation.y);
		//direction.z = -sin(rotation.y);
		//
		//light.direction = glm::normalize(direction);
		//light.ambient = lightComponent.Color * 0.f;
		//light.diffuse = lightComponent.Color * 0.8f;
		//light.specular = lightComponent.Color * 1.0f;
		//light.constant = lightComponent.Constant; //1.0f;
		//light.linear = lightComponent.Linear; //0.09f;
		//light.quadratic = lightComponent.Quadratic; //0.032f;
		//light.cutOff = glm::cos(glm::radians(lightComponent.CutOff)); // 12.5f
		//light.outerCutOff = glm::cos(glm::radians(lightComponent.OuterCutOff)); // 15.0f
	}

	void Renderer3D::SubmitLightSetup()
	{
		//s_Data.LightingUniformBuffer->SetData(&s_Data.LightingBuffer, sizeof(Renderer3DData::LightingData));
	}

	void Renderer3D::SubmitSkyLight(SkyLightComponent& lightComponent)
	{
		if (!s_Data.LightingBuffer.UsingSkyLight) // Prevent multiple skylights being submitted per frame.
		{
			if (lightComponent.SkyboxHDRI && lightComponent.Type == 0)
			{
				s_Data.LightingBuffer.UsingSkyLight = true;

				if (s_Data.SkyboxHDRIID != lightComponent.SkyboxHDRI->GetRendererID())
				{
					s_Data.SkyboxHDRIID = lightComponent.SkyboxHDRI->GetRendererID();
					UpdateSkyLight();
				}
			}
			else
			{
				s_Data.LightingBuffer.UsingSkyLight = true;

				s_Data.VolumetricCloudsFramebuffer->Bind();
				RenderCommand::Clear();
				s_Data.VolumetricNoiseTexture->Bind();
				RenderQuad(s_Data.VolumetricCloudsShader);
				s_Data.SkyboxHDRIID = s_Data.VolumetricCloudsFramebuffer->GetColorAttachmentRendererID();
				UpdateSkyLight();
			}
		}
	}

	void Renderer3D::UpdateSkyLight()
	{
		glDisable(GL_CULL_FACE);

		GLuint fbo;
		glCreateFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		// Setup projections and view matricies for capturing data
		glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		glm::mat4 captureViews[] =
		{
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
		};

		struct BufferData
		{
			glm::mat4 viewProjection;
			float roughness;
		};
		BufferData uniformBuffer;

		// Equirectangular to cubemap
		{
			s_Data.EquirectangularToCubemapShader->Bind();
			glBindTextureUnit(0, s_Data.SkyboxHDRIID);

			glViewport(0, 0, 512, 512);

			for (size_t i = 0; i < 6; i++)
			{
				uniformBuffer.viewProjection = captureProjection * captureViews[i];
				// TODO: Fix. This is bad, reusing a spare uniform buffer!
				s_Data.ObjectUniformBuffer->SetData(&uniformBuffer, sizeof(BufferData));

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, s_Data.EnvironmentCubemap->GetRendererID(), 0);
				//captureFBO->SetAttachmentTarget(0, (FramebufferTextureTarget)((size_t)(FramebufferTextureTarget::CUBE_MAP_POSITIVE_X) + i));
				RenderCommand::Clear();

				RenderCube();
			}

			//  let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
			glGenerateTextureMipmap(s_Data.EnvironmentCubemap->GetRendererID());
		}

		// Create irradiance cubemap
		{
			s_Data.IrradianceShader->Bind();
			s_Data.EnvironmentCubemap->Bind(0);

			glViewport(0, 0, 32, 32);

			for (size_t i = 0; i < 6; i++)
			{
				uniformBuffer.viewProjection = captureProjection * captureViews[i];
				// TODO: Fix. This is bad, reusing a spare uniform buffer!
				s_Data.ObjectUniformBuffer->SetData(&uniformBuffer, sizeof(BufferData));

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, s_Data.IrradianceMap->GetRendererID(), 0);
				RenderCommand::Clear();

				RenderCube();
			}
		}

		// Create prefilter cubemap with a quasi monte-carlo simulation
		{
			s_Data.PrefilterShader->Bind();
			s_Data.EnvironmentCubemap->Bind(0);


			for (unsigned int mip = 0; mip < s_Data.MaxSkyboxMipLevels; mip++)
			{
				unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
				unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));

				glViewport(0, 0, mipWidth, mipHeight);

				uniformBuffer.roughness = (float)mip / (float)(s_Data.MaxSkyboxMipLevels - 1);
				for (size_t i = 0; i < 6; i++)
				{
					uniformBuffer.viewProjection = captureProjection * captureViews[i];
					// TODO: Fix. This is bad, reusing a spare uniform buffer!
					s_Data.ObjectUniformBuffer->SetData(&uniformBuffer, sizeof(BufferData));

					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, s_Data.PrefilterMap->GetRendererID(), mip);
					RenderCommand::Clear();

					RenderCube();
				}
			}
		}

		// Generate a 2D LUT from BRDF equations
		{
			s_Data.brdfLUTTexture->Bind();

			glViewport(0, 0, 512, 512);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_Data.brdfLUTTexture->GetRendererID(), 0);
			RenderCommand::Clear();
			
			RenderQuad(s_Data.brdfShader);
		}

		glDeleteFramebuffers(1, &fbo);

		glEnable(GL_CULL_FACE);
	}

	void Renderer3D::SetSelectedEntity(int id)
	{
		s_Data.SelectedEntity = id;
	}

	void Renderer3D::ResetStats()
	{
		memset(&s_Data.Stats, 0, sizeof(Statistics));
	}

	Renderer3D::Statistics Renderer3D::GetStats()
	{
		return s_Data.Stats;
	}
}

// Editor Only
#include <imgui.h>
namespace Dymatic {

	void Renderer3D::OnImGuiRender()
	{
		ImGui::Begin("Renderer Settings");

		if (ImGui::CollapsingHeader("G-BUFFER"))
		{
			auto& spec = s_Data.ActiveFramebuffer->GetSpecification();
			const float width = ImGui::GetContentRegionAvailWidth();
			const float ar = (float)spec.Height / (float)spec.Width;
			for (uint32_t i = 0; i < spec.Attachments.Attachments.size() - 1; i++)
			{
				ImGui::Image((ImTextureID)s_Data.ActiveFramebuffer->GetColorAttachmentRendererID(i),
					{ width, width * ar },
					{ 0, 1 }, { 1, 0 });
			}	
		}

		if (ImGui::CollapsingHeader("EXPOSURE"))
		{
			ImGui::DragFloat("Exposure", &s_Data.PostProcessingBuffer.Exposure, 0.1f, 0.0f);
		}

		ImGui::End();
	}
}
