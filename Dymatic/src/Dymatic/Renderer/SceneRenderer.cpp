#include "dypch.h"
#include "Dymatic/Renderer/SceneRenderer.h"

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

#include "Dymatic/Core/Input.h"

namespace Dymatic {

	struct Renderer3DData
	{
		static const uint32_t MaxBones = 100;
		static const uint32_t MaxBoneInfluence = 4;
		
		static const uint32_t GridSizeX = 16, GridSizeY = 9, GridSizeZ = 24;
		static const uint32_t NumClusters = GridSizeX * GridSizeY * GridSizeZ;
		static const uint32_t MaxLightsPerTile = 100;
		static const uint32_t MaxLights = NumClusters * MaxLightsPerTile;

		static const uint32_t MaxCascadeCount = 16;
		static const uint32_t ShadowMapResolution = 4096;

		// Individual Shadow Lights
		static const uint32_t MaxShadowedLights = 10;

		static const uint32_t NumBloomDownsamples = 14; // Must be even

		static const uint32_t MaxVolumes = 32;

		static const uint32_t MaxBokehCount = 2048;

		Ref<Framebuffer> ActiveFramebuffer;
		uint32_t ActiveWidth;
		uint32_t ActiveHeight;

		SceneRenderer::RendererVisualizationMode VisualizationMode = SceneRenderer::RendererVisualizationMode::Rendered;

		struct ModelData
		{
			glm::mat4 Transform;
			Ref<Model> Model;
			std::vector<Ref<Material>> Materials;
			Ref<Animator> Animator;
			int EntityID = -1;
			bool Selected = false;
		};

		// Shadow Data
		Ref<Shader> CSMShadowShader;
		Ref<Framebuffer> DirectionalShadowFramebuffer;
		Ref<Shader> ShadowShader;
		uint32_t NextShadowIndex = 0;
		
		// Buffers
		Ref<Framebuffer> DeferredLightingFramebuffer;

		// Deferred Pass Data
		Ref<Shader> PreDepthShader;
		Ref<Shader> DeferredGBufferShader;
		Ref<Shader> DeferredLightingShader;

		Ref<Shader> BufferVisualizationShader;
		Ref<Shader> WireframeShader;

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

		// Cube
		float CubeVertices[24] =
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

		uint32_t CubeIndices[36] =
		{
			3, 0, 1, 1, 2, 3,
			4, 0, 3, 3, 7, 4,
			1, 5, 6, 6, 2, 1,
			4, 7, 6, 6, 5, 4,
			3, 2, 6, 6, 7, 3,
			0, 4, 1, 1, 4, 5
		};

		Ref<VertexArray> CubeVertexArray;
		Ref<VertexBuffer> CubeVertexBuffer;

		Ref<Shader> LightPropagationVolumeShader;

		Ref<Framebuffer> SSAOFramebuffer;
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

		// Bloom
		Ref<Framebuffer> BloomFramebufferAddA;
		Ref<Framebuffer> BloomFramebufferAddB;
		Ref<Framebuffer> BloomBrightIsolated;
		Ref<Framebuffer> BloomFramebuffer;
		Ref<Framebuffer> BloomDownsampleFramebuffers[NumBloomDownsamples];
		Ref<Shader> BloomIsolateShader;
		Ref<Shader> BloomBlurHorizontalShader;
		Ref<Shader> BloomBlurVerticalShader;
		Ref<Shader> BloomAddShader;
		Ref<Shader> BloomCompositeShader;
		Ref<Texture2D> BloomDirtTexture;

		Ref<Framebuffer> DOFFramebuffer;
		Ref<Shader> DOFShader;
		Ref<Shader> BokehDrawShader;
		Ref<Shader> BokehIsolateShader;
		Ref<Texture2D> BokehShapeTexture;
		Ref<VertexArray> BokehVertexArray;
		Ref<VertexBuffer> BokehVertexBuffer;

		Ref<Framebuffer> LensDistortionFramebuffer;
		Ref<Shader> LensDistortionShader;

		Ref<Shader> FinalCompositingFXShader;
		Ref<Shader> FinalCompositingShader;

		// PBR IBL Cubemap
		static const uint32_t MaxSkyboxMipLevels = 5;
		uint32_t SkyboxHDRIID; // Created by client
		Ref<TextureCube> EnvironmentCubemap;
		Ref<TextureCube> IrradianceMap;
		Ref<TextureCube> PrefilterMap;
		Ref<Texture2D> brdfLUTTexture;
		uint32_t SkyboxFlowMapID; // Created by client
		Ref<TextureCube> FlowMapCubemap;

		glm::mat4 CubemapCaptureProjection;
		glm::mat4 CubemapCaptureViews[6];

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
		SceneRenderer::Statistics Stats;

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
			glm::vec2 PixelSize;
			float Scale;
			float Bias;
			float ZNear;
			float ZFar;
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
			float BUFF[1];
		};
		LightingData LightingBuffer;
		Ref<UniformBuffer> LightingUniformBuffer;

		struct ObjectData
		{
			glm::mat4 Model;
			glm::mat4 ModelInverse;
			glm::mat4 FinalBonesMatrices[MaxBones];
			int EntityID;
			int Animated;
			float BUFF[2];
		};
		ObjectData ObjectBuffer;
		Ref<UniformBuffer> ObjectUniformBuffer;

		Ref<UniformBuffer> MaterialUniformBuffer;

		struct Volume
		{
			glm::vec4 Min;
			glm::vec4 Max;

			int Blend = 0;
			float ScatteringDistribution = 0.5;
			float ScatteringIntensity = 1.0;
			float ExtinctionScale = 0.5;
		};
		
		struct VolumetricData
		{
			Volume Volumes[MaxVolumes];
			int VolumeCount;
		};
		VolumetricData VolumetricBuffer;
		Ref<UniformBuffer> VolumetricUniformBuffer;

		struct PostProcessingData
		{
			glm::vec4 SSAOSamples[64];
			
			int VisualizationMode = 0;
			
			float Gamma = 2.2f;
			float Time = 0.0f;
			float LensDistortion = 0.05f;
			float AberrationAmount = 0.01f;
			float GrainAmount = 0.25f;
			float VignetteIntensity = 15.0f;
			float VignettePower = 0.25f;
			float FocusNearStart = 0.0f;
			float FocusNearEnd = 0.0f;
			float FocusFarStart = 0.0f;
			float FocusFarEnd = 0.0f;
			float FocusScale = 0.0f;
			float BokehThreshold = 0.5f;
			float BokehSize = 1.0f;
			float BUFF[1];
		};
		PostProcessingData PostProcessingBuffer;
		Ref<UniformBuffer> PostProcessingUniformBuffer;

		// Lighting SSBOs
		Ref<ShaderStorageBuffer> ClusterAABBSSBO;
		Ref<ShaderStorageBuffer> PointLightSSBO;
		Ref<ShaderStorageBuffer> LightIndexSSBO;
		Ref<ShaderStorageBuffer> LightGridSSBO;
		Ref<ShaderStorageBuffer> LightCountsSSBO;

		// Bokeh SSBO
		struct BokehData
		{
			unsigned int GlobalBokehIndex;
			struct Bokeh
			{
				glm::vec2 position;
				float size;
				glm::vec3 color;
			} BokehList[MaxBokehCount];
		};
		Ref<ShaderStorageBuffer> BokehSSBO;

		// Data Lists
		std::vector<ModelData> ModelDrawList;
		std::vector<PointLight> LightList;

		// Render Passes
		bool UseSSAO = false;
		bool UseSSR = false;
		bool UseMotionBlur = false;
		bool UseDOF = false;
		bool UseBloom = false;
		bool UseLensDistortion = false;
		bool UseFXAA = false;
	};

	static Renderer3DData s_Data;
	
	static GLuint vao, vbo;

	void SceneRenderer::Init()
	{
		DY_PROFILE_FUNCTION();
		
		// Quad Vertex Buffer
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

		// Cube Vertex Buffer
		s_Data.CubeVertexArray = VertexArray::Create();

		s_Data.CubeVertexBuffer = VertexBuffer::Create(8 * 3 * sizeof(float));
		s_Data.CubeVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position" }
			});
		s_Data.CubeVertexBuffer->SetData(s_Data.CubeVertices, 8 * 3 * sizeof(float));

		s_Data.CubeVertexArray->AddVertexBuffer(s_Data.CubeVertexBuffer);
		Ref<IndexBuffer> skyboxIB = IndexBuffer::Create(s_Data.CubeIndices, 36);
		s_Data.CubeVertexArray->SetIndexBuffer(skyboxIB);

		// Setup Buffers
		s_Data.CameraUniformBuffer = UniformBuffer::Create(sizeof(Renderer3DData::CameraData), 0);
		s_Data.LightingUniformBuffer = UniformBuffer::Create(sizeof(Renderer3DData::LightingData), 1);
		s_Data.ObjectUniformBuffer = UniformBuffer::Create(sizeof(Renderer3DData::ObjectData), 2);
		s_Data.MaterialUniformBuffer = UniformBuffer::Create(sizeof(Material::MaterialData), 3);
		s_Data.VolumetricUniformBuffer = UniformBuffer::Create(sizeof(Renderer3DData::VolumetricData), 4);
		s_Data.PostProcessingUniformBuffer = UniformBuffer::Create(sizeof(Renderer3DData::PostProcessingData), 5);

		// Lighting SSBOs
		s_Data.ClusterAABBSSBO = ShaderStorageBuffer::Create(sizeof(glm::vec4) * 2 * s_Data.NumClusters, 6, ShaderStorageBufferUsage::STATIC_COPY);
		s_Data.PointLightSSBO = ShaderStorageBuffer::Create(sizeof(unsigned int) + s_Data.MaxLights * sizeof(Renderer3DData::PointLight), 7, ShaderStorageBufferUsage::DYNAMIC_DRAW);
		s_Data.LightIndexSSBO = ShaderStorageBuffer::Create(s_Data.MaxLights * sizeof(unsigned int), 8, ShaderStorageBufferUsage::STATIC_COPY);
		s_Data.LightGridSSBO = ShaderStorageBuffer::Create(s_Data.NumClusters * 2 * sizeof(unsigned int), 9, ShaderStorageBufferUsage::STATIC_COPY);
		s_Data.LightCountsSSBO = ShaderStorageBuffer::Create(2 * sizeof(unsigned int), 10, ShaderStorageBufferUsage::STATIC_COPY);

		// Bokeh SSBO
		s_Data.BokehSSBO = ShaderStorageBuffer::Create(sizeof(Renderer3DData::BokehData), 11, ShaderStorageBufferUsage::DYNAMIC_COPY);

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
				s_Data.DeferredLightingFramebuffer = Framebuffer::Create(fbSpec);

				s_Data.ClusterShader = Shader::Create("assets/shaders/Renderer3D_ClusterShader.glsl");
				s_Data.ClusterCullLightShader = Shader::Create("assets/shaders/Renderer3D_ClusterCullLightShader.glsl");

				s_Data.BufferVisualizationShader = Shader::Create("assets/shaders/Renderer3D_BufferVisualization.glsl");
				s_Data.WireframeShader = Shader::Create("assets/shaders/Renderer3D_Wireframe.glsl");
			}
		}

		// Setup Post Processing
		{

			FramebufferSpecification fbSpec;
			fbSpec.Attachments = { TextureFormat::RGBA16F, TextureFormat::Depth };
			fbSpec.Width = 1920;
			fbSpec.Height = 1080;

			s_Data.LightPropagationVolumeShader = Shader::Create("assets/shaders/Renderer3D_LightPropagationVolume.glsl");

			// SSAO
			{
				s_Data.SSAOFramebuffer = Framebuffer::Create(fbSpec);

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
				s_Data.VolumetricNoiseTexture = Texture2D::Create("Resources/Textures/NoiseTexture.png");
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
			{
				s_Data.DOFFramebuffer = Framebuffer::Create(fbSpec);

				s_Data.DOFShader = Shader::Create("assets/shaders/Renderer3D_DOF.glsl");
				s_Data.BokehDrawShader = Shader::Create("assets/shaders/Renderer3D_BokehDraw.glsl");
				s_Data.BokehIsolateShader = Shader::Create("assets/shaders/Renderer3D_BokehIsolate.glsl");

				s_Data.BokehShapeTexture = Texture2D::Create("Resources/Textures/Bokeh/BokehHexegon.png");
				
				{
					struct VertexData
					{
						glm::vec3 position;
						int index;
					};

					s_Data.BokehVertexArray = VertexArray::Create();
					s_Data.BokehVertexBuffer = VertexBuffer::Create(s_Data.MaxBokehCount * 4 * sizeof(VertexData));
					s_Data.BokehVertexBuffer->SetLayout({ 
						{ ShaderDataType::Float3, "a_Position" },
						{ ShaderDataType::Int, "a_BokehIndex" }
						});
					s_Data.BokehVertexArray->AddVertexBuffer(s_Data.BokehVertexBuffer);

					{
						uint32_t currentIndex = 0;

						VertexData* data = new VertexData[s_Data.MaxBokehCount * 4];

						for (uint32_t i = 0; i < s_Data.MaxBokehCount * 4; i += 4)
						{
							uint32_t index = currentIndex++;
							data[i + 0].position = { -1.0f, -1.0f, 0.0f };
							data[i + 0].index = index;
							data[i + 1].position = { 1.0f, -1.0f, 0.0f };
							data[i + 1].index = index;
							data[i + 2].position = { 1.0f,  1.0f, 0.0f };
							data[i + 2].index = index;
							data[i + 3].position = { -1.0f,  1.0f, 0.0f };
							data[i + 3].index = index;
						}

						s_Data.BokehVertexBuffer->SetData(data, s_Data.MaxBokehCount * 4 * sizeof(VertexData));
						delete[] data;
					}

					{
						uint32_t* quadIndices = new uint32_t[s_Data.MaxBokehCount * 6];

						uint32_t offset = 0;
						for (uint32_t i = 0; i < s_Data.MaxBokehCount * 6; i += 6)
						{
							quadIndices[i + 0] = offset + 0;
							quadIndices[i + 1] = offset + 1;
							quadIndices[i + 2] = offset + 2;

							quadIndices[i + 3] = offset + 2;
							quadIndices[i + 4] = offset + 3;
							quadIndices[i + 5] = offset + 0;

							offset += 4;
						}

						Ref<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, s_Data.MaxBokehCount * 6);
						s_Data.BokehVertexArray->SetIndexBuffer(quadIB);
						delete[] quadIndices;
					}
				}
			}

			// Bloom
			s_Data.BloomBrightIsolated = Framebuffer::Create(fbSpec);
			s_Data.BloomFramebufferAddA = Framebuffer::Create(fbSpec);
			s_Data.BloomFramebufferAddB = Framebuffer::Create(fbSpec);
			s_Data.BloomFramebuffer = Framebuffer::Create(fbSpec);
			s_Data.BloomIsolateShader = Shader::Create("assets/shaders/Renderer3D_BloomIsolate.glsl");
			s_Data.BloomBlurHorizontalShader = Shader::Create("assets/shaders/Renderer3D_BloomBlurHorizontal.glsl");
			s_Data.BloomBlurVerticalShader = Shader::Create("assets/shaders/Renderer3D_BloomBlurVertical.glsl");
			s_Data.BloomAddShader = Shader::Create("assets/shaders/Renderer3D_BloomAdd.glsl");
			s_Data.BloomCompositeShader = Shader::Create("assets/shaders/Renderer3D_BloomComposite.glsl");
			s_Data.BloomDirtTexture = Texture2D::Create("assets/textures/DirtMaskTexture.png");

			// Bloom downsample FBOs
			{
				FramebufferSpecification fbspec;
				fbspec.Attachments = { TextureFormat::RGBA16F };
				fbspec.Width = 1920 * 0.5f;
				fbspec.Height = 1080 * 0.5f;

				for (uint32_t i = 0; i < s_Data.NumBloomDownsamples; i+=2)
				{
					s_Data.BloomDownsampleFramebuffers[i]     = Framebuffer::Create(fbspec);
					s_Data.BloomDownsampleFramebuffers[i + 1] = Framebuffer::Create(fbspec);
					fbspec.Width  *= 0.5f;
					fbspec.Height *= 0.5f;
				}
			}

			// Lens Distortion
			s_Data.LensDistortionFramebuffer = Framebuffer::Create(fbSpec);
			s_Data.LensDistortionShader = Shader::Create("assets/shaders/Renderer3D_LensDistortion.glsl");

			// Final Compositing
			s_Data.FinalCompositingFXShader = Shader::Create("assets/shaders/Renderer3D_FinalCompositingFX.glsl");
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

			s_Data.FlowMapCubemap = TextureCube::Create(512, 512, 1, TextureFormat::RGB16F);

			// Setup projections and view matricies for capturing data
			s_Data.CubemapCaptureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
			s_Data.CubemapCaptureViews[0] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
			s_Data.CubemapCaptureViews[1] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
			s_Data.CubemapCaptureViews[2] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			s_Data.CubemapCaptureViews[3] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
			s_Data.CubemapCaptureViews[4] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
			s_Data.CubemapCaptureViews[5] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
		}
	}

	void SceneRenderer::Shutdown()
	{
		DY_PROFILE_FUNCTION();
	}

	void SceneRenderer::BeginScene(const Camera& camera, const glm::mat4& transform)
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
		s_Data.CameraBuffer.PixelSize = { 1.0f / float(s_Data.ActiveWidth), 1.0f / float(s_Data.ActiveHeight) };
		s_Data.CameraBuffer.ZNear = ((SceneCamera*)&camera)->GetPerspectiveNearClip();
		s_Data.CameraBuffer.ZFar = ((SceneCamera*)&camera)->GetPerspectiveFarClip();

		// Simple pre-calculation to reduce use of log function - TODO: Should really be under lighting
		s_Data.CameraBuffer.Scale = (float)s_Data.GridSizeZ / std::log2f(s_Data.CameraBuffer.ZFar / s_Data.CameraBuffer.ZNear);
		s_Data.CameraBuffer.Bias = -((float)s_Data.GridSizeZ * std::log2f(s_Data.CameraBuffer.ZNear) / std::log2f(s_Data.CameraBuffer.ZFar / s_Data.CameraBuffer.ZNear));

		s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer3DData::CameraData));

		// Reset Lighting Data
		s_Data.LightingBuffer.UsingDirectionalLight = false;
		s_Data.LightingBuffer.UsingSkyLight = false;

		s_Data.VolumetricBuffer.VolumeCount = 0;
	}

	void SceneRenderer::BeginScene(const EditorCamera& camera)
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
		s_Data.CameraBuffer.PixelSize = { 1.0f / float(s_Data.ActiveWidth), 1.0f / float(s_Data.ActiveHeight) };
		s_Data.CameraBuffer.ZNear = camera.GetNearClip();
		s_Data.CameraBuffer.ZFar = camera.GetFarClip();

		// Simple pre-calculation to reduce use of log function - TODO: Should really be under lighting
		s_Data.CameraBuffer.Scale = (float)s_Data.GridSizeZ / std::log2f(s_Data.CameraBuffer.ZFar / s_Data.CameraBuffer.ZNear);
		s_Data.CameraBuffer.Bias = - ((float)s_Data.GridSizeZ * std::log2f(s_Data.CameraBuffer.ZNear) / std::log2f(s_Data.CameraBuffer.ZFar / s_Data.CameraBuffer.ZNear));

		s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer3DData::CameraData));

		// Reset Lighting Data
		s_Data.LightingBuffer.UsingDirectionalLight = false;
		s_Data.LightingBuffer.UsingSkyLight = false;

		s_Data.VolumetricBuffer.VolumeCount = 0;
	}

	void SceneRenderer::EndScene()
	{
		DY_PROFILE_FUNCTION();
	}

	void SceneRenderer::Resize()
	{
		s_Data.ActiveWidth = s_Data.ActiveFramebuffer->GetSpecification().Width;
		s_Data.ActiveHeight = s_Data.ActiveFramebuffer->GetSpecification().Height;

		// Update All Framebuffer Sizes
		s_Data.DeferredLightingFramebuffer->Resize(s_Data.ActiveWidth, s_Data.ActiveHeight);
		s_Data.OutlineFramebuffer->Resize(s_Data.ActiveWidth, s_Data.ActiveHeight);

		s_Data.SSAOFramebuffer->Resize(s_Data.ActiveWidth, s_Data.ActiveHeight);

		s_Data.PreviousFrame->Resize(s_Data.ActiveWidth, s_Data.ActiveHeight);
		s_Data.SSRFramebuffer->Resize(s_Data.ActiveWidth, s_Data.ActiveHeight);

		s_Data.FXAAFramebuffer->Resize(s_Data.ActiveWidth, s_Data.ActiveHeight);

		s_Data.MotionBlurFramebuffer->Resize(s_Data.ActiveWidth, s_Data.ActiveHeight);

		// Bloom
		s_Data.BloomFramebufferAddA->Resize(s_Data.ActiveWidth, s_Data.ActiveHeight);
		s_Data.BloomFramebufferAddB->Resize(s_Data.ActiveWidth, s_Data.ActiveHeight);
		s_Data.BloomBrightIsolated->Resize(s_Data.ActiveWidth, s_Data.ActiveHeight);
		s_Data.BloomFramebuffer->Resize(s_Data.ActiveWidth, s_Data.ActiveHeight);
		{
			uint32_t width = s_Data.ActiveWidth, height = s_Data.ActiveHeight;
			for (uint32_t i = 0; i < s_Data.NumBloomDownsamples; i += 2)
			{
				s_Data.BloomDownsampleFramebuffers[i]->Resize(width, height);
				s_Data.BloomDownsampleFramebuffers[i + 1]->Resize(width, height);
				width *= 0.5f;
				height *= 0.5f;
			}
		}

		s_Data.DOFFramebuffer->Resize(s_Data.ActiveWidth, s_Data.ActiveHeight);

		s_Data.LensDistortionFramebuffer->Resize(s_Data.ActiveWidth, s_Data.ActiveHeight);

		// Volumetric Clouds
		s_Data.VolumetricCloudsFramebuffer->Resize(s_Data.ActiveWidth, s_Data.ActiveHeight);
	}

	void SceneRenderer::SetActiveFramebuffer(Ref<Framebuffer> framebuffer)
	{
		s_Data.ActiveFramebuffer = framebuffer;
	}

	SceneRenderer::RendererVisualizationMode SceneRenderer::GetVisualizationMode()
	{
		return s_Data.VisualizationMode;
	}

	void SceneRenderer::SetVisualizationMode(RendererVisualizationMode visualizationMode)
	{
		s_Data.VisualizationMode = visualizationMode;
		s_Data.PostProcessingBuffer.VisualizationMode = (int)visualizationMode;
	}

	void SceneRenderer::UpdateTimestep(Timestep ts)
	{
		s_Data.PostProcessingBuffer.Time += ts;
	}

	void SceneRenderer::SubmitModel(const glm::mat4& transform, Ref<Model> model, int entityID, bool selected)
	{
		SubmitModel(transform, model, {}, nullptr, entityID, selected);
	}

	void SceneRenderer::SubmitModel(const glm::mat4& transform, Ref<Model> model, const std::vector<Ref<Material>>& materials, Ref<Animator> animator, int entityID, bool selected)
	{
		s_Data.ModelDrawList.push_back({ transform, model, materials, animator, entityID, selected });
	}

	void SceneRenderer::SubmitStaticMesh(const glm::mat4& transform, const StaticMeshComponent& mesh, int entityID, bool selected)
	{
		if (mesh.m_Model)
			SubmitModel(transform, mesh.m_Model, mesh.m_Materials, mesh.m_Animator, entityID, selected);
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
		RenderCommand::DrawIndexed(s_Data.CubeVertexArray, 36);
		glEnable(GL_DEPTH_TEST);
	}

	static std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4& viewProjection)
	{
		auto inverseViewProjection = glm::inverse(viewProjection);

		std::vector<glm::vec4> frustumCorners;
		for (uint32_t x = 0; x < 2; ++x)
		{
			for (uint32_t y = 0; y < 2; ++y)
			{
				for (uint32_t z = 0; z < 2; ++z)
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

		// TODO: Fix this FOV
		auto& viewProjection = glm::perspective(glm::radians(/*s_Data.CameraFOV*/45.0f), (float)s_Data.CameraBuffer.ScreenDimensions.x / (float)s_Data.CameraBuffer.ScreenDimensions.y, nearPlane, farPlane) * s_Data.CameraBuffer.View;

		const auto corners = GetFrustumCornersWorldSpace(viewProjection);

		glm::vec3 center = glm::vec3(0.0f);
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

	static void DrawModel(Ref<Model> model, const std::vector<Ref<Material>>& materials, bool bindMaterial = true)
	{
		for (uint32_t i = 0; i < model->GetMeshes().size(); i++)
		{
			auto& mesh = model->GetMeshes()[i];

			if (bindMaterial)
			{
				Ref<Material> material;
				if (i < materials.size() && materials[i] != nullptr)
					material = materials[i];
				else
					material = mesh->GetMaterial();

				bool doubleSided = material->GetAlphaBlendMode() == Material::Masked;

				if (doubleSided)
					glDisable(GL_CULL_FACE);

				s_Data.MaterialUniformBuffer->SetData(&material->GetMaterialData(), sizeof(Material::MaterialData));
				material->Bind();

				if (doubleSided)
					glEnable(GL_CULL_FACE);
			}
			
			mesh->Draw();
		}
	}

	void SceneRenderer::RenderScene()
	{
		if (s_Data.VisualizationMode == RendererVisualizationMode::Wireframe)
		{
			RenderCommand::SetWireframe(true);

			s_Data.ActiveFramebuffer->Bind();
			
			for (auto& model : s_Data.ModelDrawList)
			{
				if (!model.Model)
					continue;

				s_Data.ObjectBuffer.Model = model.Transform;
				s_Data.ObjectBuffer.ModelInverse = glm::transpose(glm::inverse(model.Transform));
				s_Data.ObjectBuffer.EntityID = model.EntityID;

				if (model.Animator)
					if (s_Data.ObjectBuffer.Animated = model.Animator->HasAnimation())
					{
						auto& matricies = model.Animator->GetFinalBoneMatrices();
						for (uint32_t i = 0; i < matricies.size(); i++)
							s_Data.ObjectBuffer.FinalBonesMatrices[i] = matricies[i];
					}

				s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(Renderer3DData::ObjectBuffer));

				(model.Selected ? s_Data.WireframeShader : s_Data.DeferredGBufferShader)->Bind();
				DrawModel(model.Model, model.Materials);
			}
			
			RenderCommand::SetWireframe(false);
		}
		else
		{

			// Submit all point lights
			{
				// Update count so we don't have to memset unused slots.
				unsigned int count = s_Data.LightList.size();
				s_Data.LightCountsSSBO->SetData(&count, sizeof(unsigned int), sizeof(unsigned int));

				if (!s_Data.LightList.empty())
					s_Data.PointLightSSBO->SetData(s_Data.LightList.data(), s_Data.LightList.size() * sizeof(Renderer3DData::PointLight));
			}

			// Pass in the current volumetric data
			s_Data.VolumetricUniformBuffer->SetData(&s_Data.VolumetricBuffer, sizeof(Renderer3DData::VolumetricData));

			// Pass in the current post processing data
			s_Data.PostProcessingUniformBuffer->SetData(&s_Data.PostProcessingBuffer, sizeof(Renderer3DData::PostProcessingData));

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
					float far_plane = light.range;
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
					s_Data.LightingUniformBuffer->SetData(&s_Data.LightingBuffer, sizeof(Renderer3DData::LightingData));

					for (auto& model : s_Data.ModelDrawList)
					{
						if (!model.Model)
							continue;

						s_Data.ObjectBuffer.Model = model.Transform;
						s_Data.ObjectBuffer.ModelInverse = glm::transpose(glm::inverse(model.Transform));

						if (model.Animator)
							if (s_Data.ObjectBuffer.Animated = model.Animator->HasAnimation())
							{
								auto& matricies = model.Animator->GetFinalBoneMatrices();
								for (size_t i = 0; i < matricies.size(); i++)
									s_Data.ObjectBuffer.FinalBonesMatrices[i] = matricies[i];
							}

						s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(Renderer3DData::ObjectBuffer));

						s_Data.ShadowShader->Bind();
						DrawModel(model.Model, model.Materials);
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

				for (auto& model : s_Data.ModelDrawList)
				{
					if (!model.Model)
						continue;

					s_Data.ObjectBuffer.Model = model.Transform;
					s_Data.ObjectBuffer.ModelInverse = glm::transpose(glm::inverse(model.Transform));

					if (model.Animator)
						if (s_Data.ObjectBuffer.Animated = model.Animator->HasAnimation())
						{
							auto& matricies = model.Animator->GetFinalBoneMatrices();
							for (size_t i = 0; i < matricies.size(); i++)
								s_Data.ObjectBuffer.FinalBonesMatrices[i] = matricies[i];
						}

					s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(Renderer3DData::ObjectBuffer));

					s_Data.CSMShadowShader->Bind();
					DrawModel(model.Model, model.Materials);
				}

				glActiveTexture(GL_TEXTURE0 + 6);
				glBindTexture(GL_TEXTURE_2D_ARRAY, lightDepthMaps);
			}
			glEnable(GL_CULL_FACE);
			glDisable(GL_DEPTH_CLAMP);
			glCullFace(GL_BACK);

			// Clear the outline framebuffer
			s_Data.OutlineFramebuffer->Bind();
			RenderCommand::Clear();

			// Bind the active framebuffer for the pre depth and main deferred rendering pass
			s_Data.ActiveFramebuffer->Bind();

			// Pre Depth Pass
			for (auto& model : s_Data.ModelDrawList)
			{
				if (!model.Model)
					continue;

				s_Data.ObjectBuffer.Model = model.Transform;
				s_Data.ObjectBuffer.ModelInverse = glm::transpose(glm::inverse(model.Transform));
				s_Data.ObjectBuffer.EntityID = model.EntityID;

				if (model.Animator)
					if (s_Data.ObjectBuffer.Animated = model.Animator->HasAnimation())
					{
						auto& matricies = model.Animator->GetFinalBoneMatrices();
						for (size_t i = 0; i < matricies.size(); i++)
							s_Data.ObjectBuffer.FinalBonesMatrices[i] = matricies[i];
					}

				s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(Renderer3DData::ObjectData));

				s_Data.PreDepthShader->Bind();
				DrawModel(model.Model, model.Materials);

				s_Data.Stats.DrawCalls++;
			}

			// Generate actual G-Buffer
			glDepthFunc(GL_EQUAL);	// (Depth already calculated in PreDepth pass)
			glDisable(GL_BLEND);	// We want to store information in the alpha channel so we disable the depth test.

			bool objectSelected = false;
			for (auto& model : s_Data.ModelDrawList)
			{
				if (!model.Model)
					continue;

				s_Data.ObjectBuffer.Model = model.Transform;
				s_Data.ObjectBuffer.ModelInverse = glm::transpose(glm::inverse(model.Transform));
				s_Data.ObjectBuffer.EntityID = model.EntityID;

				if (model.Animator)
					if (s_Data.ObjectBuffer.Animated = model.Animator->HasAnimation())
					{
						auto& matricies = model.Animator->GetFinalBoneMatrices();
						for (size_t i = 0; i < matricies.size(); i++)
							s_Data.ObjectBuffer.FinalBonesMatrices[i] = matricies[i];
					}

				s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(Renderer3DData::ObjectBuffer));

				if (model.Selected)
				{
					objectSelected = true;

					s_Data.DeferredGBufferShader->Bind();
					DrawModel(model.Model, model.Materials);

					glDepthFunc(GL_LESS);
					s_Data.OutlineFramebuffer->Bind();
					s_Data.PreDepthShader->Bind();
					DrawModel(model.Model, model.Materials, false);

					glDepthFunc(GL_EQUAL);
					s_Data.ActiveFramebuffer->Bind();
				}
				else
				{
					s_Data.DeferredGBufferShader->Bind();
					DrawModel(model.Model, model.Materials);
				}

				s_Data.Stats.DrawCalls++;
			}
			glEnable(GL_BLEND);
			glDepthFunc(GL_LESS);

			//// Calculate the light propagation volume
			//{
			//	static bool init = false;
			//	static uint32_t voxelTexture;
			//	if (!init)
			//	{
			//		init = true;
			//		glCreateTextures(GL_TEXTURE_3D, 1, &voxelTexture);
			//
			//		// Set the texture wrapping and filtering parameters
			//		glTextureParameteri(voxelTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			//		glTextureParameteri(voxelTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			//		glTextureParameteri(voxelTexture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			//		glTextureParameteri(voxelTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			//		glTextureParameteri(voxelTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			//
			//		// Allocate storage for the texture
			//		glTextureStorage3D(voxelTexture, 1, GL_RGBA32F, 64, 64, 64);
			//	}
			//
			//	// Bind the voxel texture
			//	glBindImageTexture(0, voxelTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
			//	s_Data.LightPropagationVolumeShader->Dispatch(8, 8, 8);
			//
			//	// Bind for use later - TODO: add 3d texture support to the texture class
			//	glBindTextureUnit(13, voxelTexture);
			//
			//}


			if (s_Data.VisualizationMode == RendererVisualizationMode::Rendered || s_Data.VisualizationMode == RendererVisualizationMode::LightingOnly || s_Data.VisualizationMode == RendererVisualizationMode::PrePostProcessing)
			{
				// Deferred Lighting Rendering

				s_Data.ClusterShader->Dispatch(16, 9, 24); // Should only be run when camera changes
				s_Data.ClusterCullLightShader->Dispatch(1, 1, 6); // Should be run every frame

				s_Data.ActiveFramebuffer->BindColorSampler(0, 0);	// Albedo
				s_Data.ActiveFramebuffer->BindDepthSampler(1);		// Depth
				s_Data.ActiveFramebuffer->BindColorSampler(2, 2);	// Normal
				s_Data.ActiveFramebuffer->BindColorSampler(3, 3);	// Emissive
				s_Data.ActiveFramebuffer->BindColorSampler(4, 4);	// Roughness + Metallic + Specular

				s_Data.EnvironmentCubemap->Bind(8);					// IBL Environment Cubemap
				s_Data.IrradianceMap->Bind(9);						// IBL Irradiance Cubemap
				s_Data.PrefilterMap->Bind(10);						// IBL Prefilter Cubemap
				s_Data.brdfLUTTexture->Bind(11);					// IBL brdf LUT Texture

				s_Data.FlowMapCubemap->Bind(12);

				s_Data.DeferredLightingFramebuffer->Bind();
				RenderQuad(s_Data.DeferredLightingShader);
			}
			else
			{
				// G Buffer Visualization Rendering
				s_Data.ActiveFramebuffer->BindColorSampler(0, 0);	// Albedo
				s_Data.ActiveFramebuffer->BindColorSampler(1, 1);	// EntityID
				s_Data.ActiveFramebuffer->BindColorSampler(2, 2);	// Normal
				s_Data.ActiveFramebuffer->BindColorSampler(3, 3);	// Emissive
				s_Data.ActiveFramebuffer->BindColorSampler(4, 4);	// Roughness + Metallic + Specular
				s_Data.ActiveFramebuffer->BindDepthSampler(5);		// Depth

				s_Data.DeferredLightingFramebuffer->Bind();
				RenderQuad(s_Data.BufferVisualizationShader);
			}

			if (s_Data.VisualizationMode == RendererVisualizationMode::Rendered || s_Data.VisualizationMode == RendererVisualizationMode::LightingOnly)
			{
				Ref<Framebuffer> PriorStageFramebuffer = s_Data.DeferredLightingFramebuffer;

				// Isolate bright, exposed areas
				{
					s_Data.BloomFramebuffer->Bind();
					RenderCommand::Clear();
					PriorStageFramebuffer->BindColorSampler(0, 0);
					RenderQuad(s_Data.BloomIsolateShader);
				}

				// SSAO
				if (s_Data.UseSSAO)
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
				if (s_Data.UseSSR)
				{
					s_Data.SSRFramebuffer->Bind();
					RenderCommand::Clear();

					s_Data.PreviousFrame->BindColorSampler(0, 0);		// Color Buffer
					s_Data.ActiveFramebuffer->BindDepthSampler(1);		// Depth Buffer

					s_Data.ActiveFramebuffer->BindColorSampler(2, 2);	// Normal
					s_Data.ActiveFramebuffer->BindColorSampler(3, 4);	// Roughness + Metallic + Specular
					RenderQuad(s_Data.SSRShader);

					PriorStageFramebuffer = s_Data.SSRFramebuffer;
				}

				// Motion Blur
				if (s_Data.UseMotionBlur)
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

				// DOF
				if (s_Data.UseDOF)
				{
					// Main Blur Pass
					s_Data.DOFFramebuffer->Bind();
					RenderCommand::Clear();
					PriorStageFramebuffer->BindColorSampler(0, 0);
					s_Data.ActiveFramebuffer->BindDepthSampler(1);
					RenderQuad(s_Data.DOFShader);

					// Bokeh isolation pass
					s_Data.ActiveFramebuffer->BindColorSampler(0, 0);
					s_Data.ActiveFramebuffer->BindDepthSampler(1);
					s_Data.BokehIsolateShader->Dispatch((s_Data.ActiveWidth / 16) + (s_Data.ActiveWidth % 16 == 0 ? 0 : 1), (s_Data.ActiveHeight / 16) + +(s_Data.ActiveHeight % 16 == 0 ? 0 : 1), 1);

					// Retrieve the bokeh count from GPU SSBO memory
					unsigned int count;
					s_Data.BokehSSBO->GetData(&count, sizeof(unsigned int));

					// Use this count to instance draw quads
					if (count != 0)
					{
						s_Data.BokehDrawShader->Bind();
						s_Data.BokehShapeTexture->Bind();
						glDisable(GL_DEPTH_TEST);
						RenderCommand::DrawIndexed(s_Data.BokehVertexArray, count * 6);
						glEnable(GL_DEPTH_TEST);
					}

					PriorStageFramebuffer = s_Data.DOFFramebuffer;
				}

				// Bloom
				if (s_Data.UseBloom)
				{
					Ref<Framebuffer> CurrentLevelFramebuffer = s_Data.BloomFramebuffer;

					for (size_t i = 0; i < s_Data.NumBloomDownsamples; i++)
					{
						s_Data.BloomDownsampleFramebuffers[i]->Bind();
						RenderCommand::Clear();

						CurrentLevelFramebuffer->BindColorSampler(0, 0);
						RenderQuad(i % 2 == 0 ? s_Data.BloomBlurHorizontalShader : s_Data.BloomBlurVerticalShader);
						CurrentLevelFramebuffer = s_Data.BloomDownsampleFramebuffers[i];
					}

					s_Data.BloomFramebufferAddA->Bind();
					RenderCommand::Clear();
					s_Data.BloomFramebufferAddB->Bind();
					RenderCommand::Clear();

					for (size_t i = 0; i < s_Data.NumBloomDownsamples; i++)
					{
						bool even = i % 2 == 0;
						(even ? s_Data.BloomFramebufferAddB : s_Data.BloomFramebufferAddA)->Bind();
						(even ? s_Data.BloomFramebufferAddA : s_Data.BloomFramebufferAddB)->BindColorSampler(0, 0);
						s_Data.BloomDownsampleFramebuffers[i]->BindColorSampler(1, 0);

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

				// Lens Distortion
				if (s_Data.UseLensDistortion)
				{
					s_Data.LensDistortionFramebuffer->Bind();
					RenderCommand::Clear();
					PriorStageFramebuffer->BindColorSampler(0, 0);
					RenderQuad(s_Data.LensDistortionShader);
					PriorStageFramebuffer = s_Data.LensDistortionFramebuffer;
				}

				// FXAA
				if (s_Data.UseFXAA)
				{
					s_Data.FXAAFramebuffer->Bind();

					PriorStageFramebuffer->BindColorSampler(0, 0);
					RenderQuad(s_Data.FXAAShader);
					PriorStageFramebuffer = s_Data.FXAAFramebuffer;
				}

				// Draw Final Quad
				{
					s_Data.ActiveFramebuffer->Bind();

					// Bind Color Buffer from prior stage
					PriorStageFramebuffer->BindColorSampler(0, 0);

					// Bind entity ID buffer from initial render pass
					s_Data.ActiveFramebuffer->BindColorSampler(1, 1);

					glDepthMask(GL_FALSE);
					RenderQuad(s_Data.FinalCompositingFXShader);
					glDepthMask(GL_TRUE);
				}
			}
			else
			{
				s_Data.ActiveFramebuffer->Bind();

				s_Data.DeferredLightingFramebuffer->BindColorSampler(0, 0);
				s_Data.ActiveFramebuffer->BindColorSampler(1, 1);

				glDepthMask(GL_FALSE);
				RenderQuad(s_Data.FinalCompositingShader);
				glDepthMask(GL_TRUE);
			}

			// Outline Post Processing
			if (objectSelected && s_Data.VisualizationMode != RendererVisualizationMode::Wireframe)
			{
				// Run FXAA algorithm on outline framebuffer
				{
					s_Data.FXAAFramebuffer->Bind();

					s_Data.OutlineFramebuffer->BindDepthSampler(0);
					RenderQuad(s_Data.FXAAShader);
				}

				s_Data.ActiveFramebuffer->Bind();
				s_Data.FXAAFramebuffer->BindColorSampler(0, 0);
				RenderQuad(s_Data.OutlineShader);
			}
		}

		
		s_Data.ModelDrawList.clear();
		s_Data.LightList.clear();
		s_Data.NextShadowIndex = 0;
	}

	void SceneRenderer::SubmitDirectionalLight(const glm::mat4& transform, DirectionalLightComponent& lightComponent)
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

	void SceneRenderer::SubmitPointLight(const glm::mat4& transform, PointLightComponent& lightComponent)
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

	void SceneRenderer::SubmitSpotLight(const glm::mat4& transform, SpotLightComponent& lightComponent)
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

	void SceneRenderer::SubmitLightSetup()
	{
	}

	static void EquirectangularToCubemap(uint32_t texture, uint32_t cubemap)
	{
		struct BufferData
		{
			glm::mat4 viewProjection;
			float roughness;
		};
		BufferData uniformBuffer;

		s_Data.EquirectangularToCubemapShader->Bind();
		glBindTextureUnit(0, texture);

		glViewport(0, 0, 512, 512);

		for (size_t i = 0; i < 6; i++)
		{
			uniformBuffer.viewProjection = s_Data.CubemapCaptureProjection * s_Data.CubemapCaptureViews[i];
			// TODO: Fix. This is bad, reusing a spare uniform buffer!
			s_Data.ObjectUniformBuffer->SetData(&uniformBuffer, sizeof(BufferData));

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap, 0);
			//captureFBO->SetAttachmentTarget(0, (FramebufferTextureTarget)((size_t)(FramebufferTextureTarget::CUBE_MAP_POSITIVE_X) + i));
			RenderCommand::Clear();

			RenderCube();
		}

		//  let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
		glGenerateTextureMipmap(cubemap);
	}

	void SceneRenderer::SubmitSkyLight(SkyLightComponent& lightComponent)
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
				if (lightComponent.SkyboxFlowMap && s_Data.SkyboxFlowMapID != lightComponent.SkyboxFlowMap->GetRendererID())
				{
					s_Data.SkyboxFlowMapID = lightComponent.SkyboxFlowMap->GetRendererID();

					glDisable(GL_CULL_FACE);
					GLuint fbo;
					glCreateFramebuffers(1, &fbo);
					glBindFramebuffer(GL_FRAMEBUFFER, fbo);
					glDrawBuffer(GL_COLOR_ATTACHMENT0);
					EquirectangularToCubemap(s_Data.SkyboxFlowMapID, s_Data.FlowMapCubemap->GetRendererID());
					glDeleteFramebuffers(1, &fbo);
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

	void SceneRenderer::SubmitVolume(TransformComponent& transform, VolumeComponent& volumeCompontent)
	{
		if (s_Data.VolumetricBuffer.VolumeCount < s_Data.MaxVolumes)
		{
			auto& volume = s_Data.VolumetricBuffer.Volumes[s_Data.VolumetricBuffer.VolumeCount++];
			volume.Min = glm::vec4(glm::vec3(-0.5f, -0.5f, -0.5f) * transform.Scale + transform.Translation, 1.0f);
			volume.Max = glm::vec4(glm::vec3(0.5f, 0.5f, 0.5f) * transform.Scale + transform.Translation, 1.0f);

			volume.Blend = (int)volumeCompontent.Blend;
			volume.ScatteringDistribution = volumeCompontent.ScatteringDistribution;
			volume.ScatteringIntensity = volumeCompontent.ScatteringIntensity;
			volume.ExtinctionScale = volumeCompontent.ExtinctionScale;
		}
	}

	void SceneRenderer::UpdateSkyLight()
	{
		glDisable(GL_CULL_FACE);

		GLuint fbo;
		glCreateFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		struct BufferData
		{
			glm::mat4 viewProjection;
			float roughness;
		};
		BufferData uniformBuffer;

		EquirectangularToCubemap(s_Data.SkyboxHDRIID, s_Data.EnvironmentCubemap->GetRendererID());

		// Create irradiance cubemap
		{
			s_Data.IrradianceShader->Bind();
			s_Data.EnvironmentCubemap->Bind(0);

			glViewport(0, 0, 32, 32);

			for (size_t i = 0; i < 6; i++)
			{
				uniformBuffer.viewProjection = s_Data.CubemapCaptureProjection * s_Data.CubemapCaptureViews[i];
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
					uniformBuffer.viewProjection = s_Data.CubemapCaptureProjection * s_Data.CubemapCaptureViews[i];
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

	void SceneRenderer::ResetStats()
	{
		memset(&s_Data.Stats, 0, sizeof(Statistics));
	}

	SceneRenderer::Statistics SceneRenderer::GetStats()
	{
		return s_Data.Stats;
	}
}

// Editor Only
#include <imgui.h>
namespace Dymatic {

	void SceneRenderer::OnImGuiRender()
	{
		ImGui::Begin("Renderer Settings");

		if (ImGui::CollapsingHeader("POST PROCESS EFFECTS"))
		{			
			ImGui::Checkbox("SSAO", &s_Data.UseSSAO);
			ImGui::Checkbox("SSR", &s_Data.UseSSR);
			ImGui::Checkbox("Motion Blur", &s_Data.UseMotionBlur);
			ImGui::Checkbox("DOF", &s_Data.UseDOF);
			ImGui::Checkbox("Bloom", &s_Data.UseBloom);
			ImGui::Checkbox("Lens Distortion", &s_Data.UseLensDistortion);
			ImGui::Checkbox("FXAA", &s_Data.UseFXAA);			
		}

		if (ImGui::CollapsingHeader("GAMMA"))
		{
			ImGui::DragFloat("Gamma", &s_Data.PostProcessingBuffer.Gamma, 0.1f, 0.0f);
		}

		if (ImGui::CollapsingHeader("LENSE DISTORTION"))
		{
			ImGui::DragFloat("Distortion Amount", &s_Data.PostProcessingBuffer.LensDistortion, 0.1f, 0.0f);
		}

		if (ImGui::CollapsingHeader("ABERRATION"))
		{
			ImGui::DragFloat("Aberration Amount", &s_Data.PostProcessingBuffer.AberrationAmount, 0.1f, 0.0f);
		}
		
		if (ImGui::CollapsingHeader("FILM GRAIN"))
		{
			ImGui::DragFloat("Grain Amount", &s_Data.PostProcessingBuffer.GrainAmount, 0.1f, 0.0f);
		}

		if (ImGui::CollapsingHeader("VIGNETTE"))
		{
			ImGui::DragFloat("Vignette Intensity", &s_Data.PostProcessingBuffer.VignetteIntensity, 0.1f, 0.0f);
			ImGui::DragFloat("Vignette Power", &s_Data.PostProcessingBuffer.VignettePower, 0.1f, 0.0f);
		}

		if (ImGui::CollapsingHeader("DOF"))
		{
			ImGui::DragFloat("Focus Near Start", &s_Data.PostProcessingBuffer.FocusNearStart, 0.1f, 0.1f);
			ImGui::DragFloat("Focus Near End", &s_Data.PostProcessingBuffer.FocusNearEnd, 0.1f, 0.1f);
			ImGui::DragFloat("Focus Far Start", &s_Data.PostProcessingBuffer.FocusFarStart, 0.1f, 0.1f);
			ImGui::DragFloat("Focus Far End", &s_Data.PostProcessingBuffer.FocusFarEnd, 0.1f, 0.1f);
			ImGui::Separator();
			ImGui::DragFloat("Focus Scale", &s_Data.PostProcessingBuffer.FocusScale, 0.1f, 0.0f);
			ImGui::DragFloat("Bokeh Threshold", &s_Data.PostProcessingBuffer.BokehThreshold, 0.1f, 0.0f);
			ImGui::DragFloat("Bokeh Size", &s_Data.PostProcessingBuffer.BokehSize, 0.1f, 0.0f);
		}

		ImGui::End();
	}
}
