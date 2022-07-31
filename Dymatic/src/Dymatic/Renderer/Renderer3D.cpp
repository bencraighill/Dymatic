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

		Ref<Framebuffer> ActiveFramebuffer;
		uint32_t ActiveWidth;
		uint32_t ActiveHeight;

		// Mesh Data
		Ref<Shader> MeshShader;
		Ref<Shader> PreDepthShader;

		int SelectedEntity = -1;

		struct ModelData
		{
			glm::mat4 transform;
			Ref<Model> model;
			Ref<Animator> animator;
			int entityID = -1;
		};

		// Shadow Data
		const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
		Ref<Shader> ShadowShader;
		Ref<Framebuffer> ShadowFramebuffer;

		// Deferred Pass Data
		Ref<Shader> PreDeferredPassShader;
		Ref<Shader> DeferredLightingShader;

		// Cluster Culling
		Ref<Shader> ClusterShader;
		Ref<Shader> ClusterCullLightShader;

		// Outline
		Ref<Framebuffer> OutlineFramebuffer;
		Ref<Shader> OutlineShader;

		// Wireframe
		Ref<Shader> WireframeShader;

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
		Ref<Shader> SSRShader;
		Ref<Shader> DrawTextureShader;

		Ref<Framebuffer> FXAAFramebuffer;
		Ref<Shader> FXAAShader;
		Ref<Framebuffer> MotionBlurFramebuffer;
		Ref<Shader> MotionBlurShader;
		Ref<Framebuffer> DOFFramebuffer;
		Ref<Shader> DOFShader;

		Ref<Framebuffer> BloomFramebufferAddA;
		Ref<Framebuffer> BloomFramebufferAddB;
		Ref<Framebuffer> BloomBrightIsolated;
		Ref<Framebuffer> BloomFramebuffer;
		Ref<Shader> BloomIsolateShader;
		Ref<Shader> BloomShader;
		Ref<Shader> BloomShaderAdd;

		Ref<Shader> FinalCompositingShader;

		// Renderer Stats
		Renderer3D::Statistics Stats;

		// Grid
		const int GridBuffer[4] = { 0, 1, 2, 3 };
		Ref<VertexArray> GridVertexArray;
		Ref<VertexBuffer> GridVertexBuffer;
		Ref<Shader> GridShader;

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
			glm::vec4 color;
		};
		struct PointLight
		{
			glm::vec4 position;
			glm::vec4 color;
			unsigned int enabled;
			float intensity;
			float range;
			float BUFF;
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
			bool UsingDirectionalLight;
		};
		LightingData LightingBuffer;
		Ref<UniformBuffer> LightingUniformBuffer;

		struct ObjectData
		{
			glm::mat4 Model;
			glm::mat4 ModelInverse; 
			glm::mat4 LightSpaceMatrix;
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
		s_Data.SkyboxVertexArray->AddVertexBuffer(s_Data.SkyboxVertexBuffer);
		Ref<IndexBuffer> skyboxIB = IndexBuffer::Create(s_Data.SkyboxIndices, 36);
		s_Data.SkyboxVertexArray->SetIndexBuffer(skyboxIB);

		s_Data.SkyboxShader = Shader::Create("assets/shaders/Renderer3D_Skybox.glsl");
		s_Data.DynamicSkyShader = Shader::Create("assets/shaders/Renderer3D_PreethamSky.glsl");

		// Setup Model Rendering Data
		s_Data.MeshShader = Shader::Create("assets/shaders/Renderer3D_LightingCulled.glsl");
		ShaderManager::Add(s_Data.MeshShader);
		s_Data.PreDepthShader = Shader::Create("assets/shaders/Renderer3D_PreDepth.glsl");

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

		// Setup Grid Rendering Data
		{
			s_Data.GridVertexArray = VertexArray::Create();

			s_Data.GridVertexBuffer = VertexBuffer::Create(4 * sizeof(int));
			s_Data.GridVertexBuffer->SetLayout({
				{ ShaderDataType::Int, "a_VertexIndex" }
				});
			s_Data.GridVertexArray->AddVertexBuffer(s_Data.GridVertexBuffer);

			uint32_t quadIndices[6];

			quadIndices[0] = 0;
			quadIndices[1] = 1;
			quadIndices[2] = 2;
			quadIndices[3] = 2;
			quadIndices[4] = 3;
			quadIndices[5] = 0;

			Ref<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, 6);
			s_Data.GridVertexArray->SetIndexBuffer(quadIB);

			s_Data.GridShader = Shader::Create("assets/shaders/Renderer3D_Grid.glsl");
		}

		// Setup Pre Deferred pass
		{
			s_Data.PreDeferredPassShader = Shader::Create("assets/shaders/Renderer3D_DeferredPrePass.glsl");
			s_Data.DeferredLightingShader = Shader::Create("assets/shaders/Renderer3D_DeferredLight.glsl");
		}

		{
			s_Data.ClusterShader = Shader::Create("assets/shaders/Renderer3D_ClusterShader.glsl");
			s_Data.ClusterCullLightShader = Shader::Create("assets/shaders/Renderer3D_ClusterCullLightShader.glsl");
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
			fbSpec.Attachments = { FramebufferTextureFormat::RGBA16F };
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

			// SSR
			s_Data.PreviousFrame = Framebuffer::Create(fbSpec);
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
			s_Data.BloomShader = Shader::Create("assets/shaders/Renderer3D_Bloom.glsl");
			s_Data.BloomShaderAdd = Shader::Create("assets/shaders/Renderer3D_BloomAdd.glsl");

			// Final Compositing
			s_Data.FinalCompositingShader = Shader::Create("assets/shaders/Renderer3D_FinalCompositing.glsl");
		}

		// Shadow Buffer
		{
			FramebufferSpecification fbSpec;
			fbSpec.Attachments = { FramebufferTextureFormat::Depth };
			fbSpec.Width = s_Data.SHADOW_WIDTH;
			fbSpec.Height = s_Data.SHADOW_HEIGHT;
			s_Data.ShadowFramebuffer = Framebuffer::Create(fbSpec);

			s_Data.ShadowShader = Shader::Create("assets/shaders/Renderer3D_ShadowDepth.glsl");
		}

		// Outline
		{
			FramebufferSpecification fbSpec;
			fbSpec.Attachments = { FramebufferTextureFormat::Depth };
			fbSpec.Width = 1920 * 2;
			fbSpec.Height = 1080 * 2;
			s_Data.OutlineFramebuffer = Framebuffer::Create(fbSpec);

			s_Data.OutlineShader = Shader::Create("assets/shaders/Renderer3D_Outline.glsl");
		}
		
		// Wireframe
		s_Data.WireframeShader = Shader::Create("assets/shaders/Renderer3D_Wireframe.glsl");
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
		s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer3DData::CameraData));
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
		s_Data.LightingBuffer.UsingDirectionalLight = true;
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
		s_Data.OutlineFramebuffer->Resize(s_Data.ActiveWidth, s_Data.ActiveHeight);
	}

	void Renderer3D::SetActiveFramebuffer(Ref<Framebuffer> framebuffer)
	{
		s_Data.ActiveFramebuffer = framebuffer;
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

	static void DrawFullscreenQuad(Ref<Shader> shader)
	{
		glDisable(GL_DEPTH_TEST);
		shader->Bind();
		RenderCommand::DrawIndexed(s_Data.QuadVertexArray, 6);
		glEnable(GL_DEPTH_TEST);
	}

	void Renderer3D::DrawMeshData()
	{
		// Render Shadow Pass
		glCullFace(GL_FRONT);
		{
			glm::mat4 lightProjection, lightView;
			glm::mat4 lightSpaceMatrix;
			float near_plane = 0.001f, far_plane = 10.0f;
			//lightProjection = glm::perspective(glm::radians(45.0f), (GLfloat)SHADOW_WIDTH / (GLfloat)SHADOW_HEIGHT, near_plane, far_plane); // note that if you use a perspective projection matrix you'll have to change the light position as the current light position isn't enough to reflect the whole scene
			lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
			lightView = glm::lookAt(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
			lightSpaceMatrix = lightProjection * lightView;


			// Render scene from light's point of view
			s_Data.ShadowFramebuffer->Bind();
			RenderCommand::Clear();

			for (auto& model : s_Data.ModelDrawData)
			{
				if (!model.model)
					continue;

				s_Data.ObjectBuffer.Model = model.transform;
				s_Data.ObjectBuffer.ModelInverse = glm::transpose(glm::inverse(model.transform));
				s_Data.ObjectBuffer.LightSpaceMatrix = lightSpaceMatrix;

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
		glCullFace(GL_BACK);

		s_Data.ActiveFramebuffer->Bind();

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

		s_Data.ClusterShader->Dispatch(16, 9, 24); // Should only be run when camera changes
		s_Data.ClusterCullLightShader->Dispatch(1, 1, 6); // Should be run every frame

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

			model.model->Draw(s_Data.PreDepthShader, false);

			s_Data.Stats.DrawCalls++;
		}

		// Submit Lighting Data
		s_Data.LightingUniformBuffer->SetData(&s_Data.LightingBuffer, sizeof(Renderer3DData::LightingData));

		// Submit Shadow Map
		s_Data.ShadowFramebuffer->BindDepthSampler(6);

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

				model.model->Draw(s_Data.MeshShader);

				glDepthFunc(GL_LESS);
				s_Data.OutlineFramebuffer->Bind();
				RenderCommand::Clear();
				model.model->Draw(s_Data.MeshShader, false);

				glDepthFunc(GL_EQUAL);
				s_Data.ActiveFramebuffer->Bind();
			}
			else
			{
				model.model->Draw(s_Data.MeshShader);
			}

			s_Data.Stats.DrawCalls++;
		}
		glDepthFunc(GL_LESS);

		Ref<Framebuffer> PriorStageFramebuffer = s_Data.ActiveFramebuffer;

		if (Input::IsKeyPressed(Key::Equal))
		{
			s_Data.PostProcessingBuffer.Exposure = (s_Data.PostProcessingBuffer.Exposure + 0.025f);
		}
		if (Input::IsKeyPressed(Key::Minus))
		{
			s_Data.PostProcessingBuffer.Exposure = (s_Data.PostProcessingBuffer.Exposure - 0.025f);
		}
		DY_CORE_INFO("Exposure: {0}", s_Data.PostProcessingBuffer.Exposure);

		// Pass in the current post processing data
		s_Data.PostProcessingUniformBuffer->SetData(&s_Data.PostProcessingBuffer, sizeof(Renderer3DData::PostProcessingData));

		// SSAO
		if (Input::IsKeyPressed(Key::D1))
		{
			PriorStageFramebuffer->BindDepthSampler(0);
			PriorStageFramebuffer->BindColorSampler(1, 2); // Normal
			s_Data.SSAONoiseTexture->Bind(2);
			DrawFullscreenQuad(s_Data.SSAOShader);
		}

		// Lens Distortion

		// DOF

		// Auto Exposure (need to implement HDR first)

		//// Make a frame copy for SSR
		//{
		//	s_Data.PreviousFrame->Bind();
		//	PriorStageFramebuffer->BindColorSampler(0, 0);
		//	DrawFullscreenQuad(s_Data.DrawTextureShader);
		//}
		//
		//// Screen Space Reflections
		//{
		//	s_Data.ActiveFramebuffer->Bind();
		//	s_Data.PreviousFrame->BindColorSampler(0, 0);
		//	PriorStageFramebuffer->BindDepthSampler(1);
		//	PriorStageFramebuffer->BindColorSampler(2, 2); // Normal
		//	PriorStageFramebuffer->BindColorSampler(3, 3); // Metallic + Specular
		//	DrawFullscreenQuad(s_Data.SSRShader);
		//}

		// Motion Blur
		//{
		//	s_Data.MotionBlurFramebuffer->Bind();
		//	RenderCommand::Clear();
		//
		//	static glm::mat4 previousViewProjectionMatrix = glm::mat4(1.0f);
		//	s_Data.MotionBlurShader->Bind();
		//
		//	s_Data.ObjectBuffer.Model = previousViewProjectionMatrix;
		//	s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(Renderer3DData::ObjectBuffer));
		//
		//	PriorStageFramebuffer->BindColorSampler(0, 0);
		//	PriorStageFramebuffer->BindDepthSampler(1);
		//
		//	DrawFullscreenQuad(s_Data.MotionBlurShader);
		//	previousViewProjectionMatrix = s_Data.CameraBuffer.ViewProjection;
		//
		//	PriorStageFramebuffer = s_Data.MotionBlurFramebuffer;
		//}

		//Bloom
		//{
		//	static Ref<Framebuffer> fbolist[13];
		//	static bool init = false;
		//	if (!init)
		//	{
		//		init = true;
		//
		//		FramebufferSpecification fbspec;
		//		fbspec.Attachments = { FramebufferTextureFormat::RGBA8 };
		//		fbspec.Width = 1920 * 0.5f;
		//		fbspec.Height = 1080 * 0.5f;
		//		for (size_t i = 0; i < 13; i += 2)
		//		{
		//			fbolist[i] = Framebuffer::Create(fbspec);
		//			fbolist[i + 1] = Framebuffer::Create(fbspec);
		//			fbspec.Width *= 0.5f;
		//			fbspec.Height *= 0.5f;
		//		}
		//	}
		//
		//	// Isolate Exposed
		//	s_Data.BloomFramebuffer->Bind();
		//	RenderCommand::Clear();
		//	PriorStageFramebuffer->BindColorSampler(0, 0);
		//	DrawFullscreenQuad(s_Data.BloomIsolateShader);
		//
		//	Ref<Framebuffer> CurrentLevelFramebuffer = s_Data.BloomFramebuffer;
		//
		//	for (size_t i = 0; i < 13; i++)
		//	{
		//		fbolist[i]->Bind();
		//		RenderCommand::Clear();
		//
		//		CurrentLevelFramebuffer->BindColorSampler(0, 0);
		//		DrawFullscreenQuad(s_Data.BloomShader);
		//		CurrentLevelFramebuffer = fbolist[i];
		//	}
		//
		//	s_Data.BloomFramebufferAddA->Bind();
		//	RenderCommand::Clear();
		//	s_Data.BloomFramebufferAddB->Bind();
		//	RenderCommand::Clear();
		//
		//	for (size_t i = 0; i < 13; i++)
		//	{
		//		bool even = i % 2 == 0;
		//		(even ? s_Data.BloomFramebufferAddB : s_Data.BloomFramebufferAddA)->Bind();
		//
		//		(even ? s_Data.BloomFramebufferAddA : s_Data.BloomFramebufferAddB)->BindColorSampler(0, 0);
		//		fbolist[i]->BindColorSampler(1, 0);
		//
		//		DrawFullscreenQuad(s_Data.BloomShaderAdd);
		//	}
		//
		//	s_Data.BloomFramebuffer->Bind();
		//	RenderCommand::Clear();
		//	PriorStageFramebuffer->BindColorSampler(0, 0);
		//	s_Data.BloomFramebufferAddB->BindColorSampler(1, 0);
		//	DrawFullscreenQuad(s_Data.BloomShaderAdd);
		//	PriorStageFramebuffer = s_Data.BloomFramebuffer;
		//}

		// FXAA
		{
			s_Data.FXAAFramebuffer->Bind();

			PriorStageFramebuffer->BindColorSampler(0, 0);
			DrawFullscreenQuad(s_Data.FXAAShader);
			PriorStageFramebuffer = s_Data.FXAAFramebuffer;
		}

		// Get Average Exposure
		{
			//PriorStageFramebuffer->ReadPixel();
		}

		// Draw Final Quad
		{
			s_Data.ActiveFramebuffer->Bind();

			// Bind Color Buffer from prior stage
			PriorStageFramebuffer->BindColorSampler(0, 0);


			// Bind Color Buffer from prior stage
			s_Data.ActiveFramebuffer->BindColorSampler(1, 1);

			glDepthMask(GL_FALSE);
			DrawFullscreenQuad(s_Data.FinalCompositingShader);
			glDepthMask(GL_TRUE);
		}

		// Outline Post Processing
		if (selectedModel)
		{
			s_Data.ActiveFramebuffer->Bind();
			s_Data.OutlineFramebuffer->BindDepthSampler(0);
			DrawFullscreenQuad(s_Data.OutlineShader);
		}

		//static Ref<Texture2D> tex = Texture2D::Create(s_Data.ActiveFramebuffer->GetSpecification().Width, s_Data.ActiveFramebuffer->GetSpecification().Height);
		// Run a compute shader
		//s_Data.ActiveFramebuffer->BindColorTexture(0, 0);
		//s_Data.ActiveFramebuffer->BindColorTexture(1, 0);
		//s_Data.TestComputeShader->Bind();
		//tex->Bind(0);
		//s_Data.ActiveFramebuffer->BindColorSampler(1, 1);
		//DrawFullscreenQuad(s_Data.FinalCompositingShader);

		s_Data.ModelDrawData.clear();
		s_Data.LightList.clear();
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
		s_Data.LightingBuffer.DirectionalLight.color = glm::vec4(lightComponent.Color, 1.0f);
		s_Data.LightingBuffer.UsingDirectionalLight = true;
	}

	void Renderer3D::SubmitPointLight(const glm::mat4& transform, PointLightComponent& lightComponent)
	{
		s_Data.LightList.push_back({});
		auto& light = s_Data.LightList.back();

		light.position = transform[3];
		light.color = glm::vec4(lightComponent.Color, 1.0f);
		light.intensity = lightComponent.Intensity;
		light.range = lightComponent.Linear;
		light.enabled = true;
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

	void Renderer3D::SubmitSkyLight(SkylightComponent& lightComponent)
	{
		if (lightComponent.EnvironmentMap)
		{
			// TODO: Remove gl Function calls and <glad/glad.h> header file.
			glDepthFunc(GL_LEQUAL);

			s_Data.SkyboxVertexBuffer->SetData(s_Data.skyboxVertices, 8 * 3 * sizeof(float));

			lightComponent.EnvironmentMap->Bind(7);
			
			s_Data.SkyboxShader->Bind();
			RenderCommand::DrawIndexed(s_Data.SkyboxVertexArray, 36);

			glDepthFunc(GL_LESS);
		}
	}

	void Renderer3D::DrawGrid()
	{
		s_Data.GridVertexBuffer->SetData(s_Data.GridBuffer, sizeof(int) * 4);

		s_Data.GridShader->Bind();
		RenderCommand::DrawIndexed(s_Data.GridVertexArray, 6);
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