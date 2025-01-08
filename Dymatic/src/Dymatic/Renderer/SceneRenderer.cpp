#include "dypch.h"
#include "Dymatic/Renderer/SceneRenderer.h"

#include "Dymatic/Renderer/RendererConstants.h"
#include "Dymatic/Renderer/VertexArray.h"
#include "Dymatic/Renderer/Shader.h"
#include "Dymatic/Renderer/UniformBuffer.h"
#include "Dymatic/Renderer/RenderCommand.h"
#include "Dymatic/Renderer/ShaderStorageBuffer.h"
#include "Dymatic/Renderer/Framebuffer.h"
#include "Dymatic/Renderer/FSR2.h"

#include "Dymatic/Asset/EngineAsset.h"
#include "Dymatic/Editor/Material/MaterialBuilder.h"

#include "Dymatic/Renderer/Frustum.h"

#include "Dymatic/Math/Math.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/compatibility.hpp>

#include <glad/glad.h>
#include <glm/gtc/epsilon.hpp>

#include <execution>
#include <random>
#include <algorithm>

#define INT_CEILING_DIVISION(dividend, divisor) ((dividend + divisor - 1) / divisor)

namespace Dymatic {

	struct SceneRendererData
	{
		Ref<SceneRendererContext> ActiveContext;

		Ref<Texture2D> WhiteTexture;

		struct ModelData
		{
			ModelData() = default;
			ModelData(const glm::mat4& transform, const glm::mat4& previousTransform, const Ref<Model> model, const std::vector<Ref<MaterialAsset>>& materials, Ref<BoneMatrixList> pose, Ref<BlendShapeWeightList> blendShapeWeights, int entityID, bool selected)
				: Transform(transform), PreviousTransform(previousTransform), Model(model), Materials(materials), Pose(pose), BlendShapeWeights(blendShapeWeights), EntityID(entityID), Selected(selected) {}

			glm::mat4 Transform;
			glm::mat4 PreviousTransform;
			Ref<Model> Model;
			std::vector<Ref<MaterialAsset>> Materials;
			Ref<BoneMatrixList> Pose;
			Ref<BlendShapeWeightList> BlendShapeWeights;
			int EntityID = -1;
			bool Selected = false;
		};

		struct MeshData
		{
			MeshData() = default;
			MeshData(const glm::mat4& transform, const glm::mat4& previousTransform, const Ref<Mesh> mesh, const Ref<MaterialAsset> material, Ref<BoneMatrixList> pose, Ref<BlendShapeWeightList> blendShapeWeights, int entityID, bool selected)
				: Transform(transform), PreviousTransform(previousTransform), Mesh(mesh), Material(material), Pose(pose), BlendShapeWeights(blendShapeWeights), EntityID(entityID), Selected(selected) {}

			glm::mat4 Transform;
			glm::mat4 PreviousTransform;
			Ref<Mesh> Mesh;
			Ref<MaterialAsset> Material;
			Ref<BoneMatrixList> Pose;
			Ref<BlendShapeWeightList> BlendShapeWeights;
			int EntityID = -1;
			bool Selected = false;
		};

		struct ParticleSystemData
		{
			ParticleSystemData() = default;
			ParticleSystemData(const glm::mat4& transform, const Ref<ParticleSystemPlayer> particleSystem, Ref<MaterialAsset> material, int entityID)
				: Transform(transform), ParticleSystem(particleSystem), Material(material), EntityID(entityID) {}

			glm::mat4 Transform;
			Ref<ParticleSystemPlayer> ParticleSystem;
			Ref<MaterialAsset> Material;
			int EntityID = -1;
		};

		struct DecalData
		{
			DecalData() = default;
			DecalData(const glm::mat4& transform, const Ref<Texture2D> albedo, bool constrainAngle, int entityID)
				: Transform(transform), Albedo(albedo), ConstrainAngle(constrainAngle), EntityID(entityID) {}

			glm::mat4 Transform;
			Ref<Texture2D> Albedo;
			bool ConstrainAngle;
			int EntityID = -1;
		};
		
		struct PostProcessVolumeData
		{
			PostProcessVolumeData() = default;
			PostProcessVolumeData(const glm::mat4& transform, const Ref<MaterialAsset> material, bool bounded, int entityID)
				: Transform(transform), Material(material), Bounded(bounded), EntityID(entityID) {}

			glm::mat4 Transform;
			Ref<MaterialAsset> Material;
			bool Bounded;
			int EntityID = -1;
		};

		uint32_t ContextCameraCount;

		Frustum CameraFrustum;

		// Shadow Data
		Ref<Framebuffer> DirectionalShadowFramebuffer;
		uint32_t NextShadowIndex = 0;

		// Deferred Shading
		Ref<Shader> DeferredLightingShader;

		// Default Material
		Ref<MaterialAsset> DefaultMaterial;

		// Editor Only Shaders
		Ref<Shader> BufferVisualizationShader;
		Ref<Shader> WireframeShader;
		Ref<Shader> ColorShader;

		// Cluster Culling
		Ref<Shader> ClusterShader;
		Ref<Shader> ClusterCullLightShader;

		// Outline
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

		Ref<VertexArray> BatchedQuadVertexArray;
		Ref<VertexBuffer> BatchedQuadVertexBuffer;

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

		// Decals
		Ref<Shader> DecalShader;

		// Volumetric Lighting
		Ref<Shader> LightPropagationVolumeShader;
		Ref<Shader> VolumetricLightingShader;
		Ref<Shader> GaussianBlurShader;

		// Post Processing Render Passes
		Ref<Texture2D> SSAONoiseTexture;
		Ref<Shader> SSAOShader;

		Ref<Shader> SSRShader;
		Ref<Shader> FXAAShader;
		Ref<Shader> MotionBlurShader;
		Ref<Shader> LensFlareShader;

		// Bloom
		Ref<Shader> BloomIsolateShader;
		Ref<Shader> BloomBlurHorizontalShader;
		Ref<Shader> BloomBlurVerticalShader;
		Ref<Shader> AddTextureShader;
		Ref<Shader> BloomCompositeShader;
		Ref<Texture2D> BloomDirtTexture;

		Ref<Shader> DOFShader;
		Ref<Shader> BokehDrawShader;
		Ref<Shader> BokehIsolateShader;
		Ref<Texture2D> BokehShapeTexture;

		Ref<Shader> LensDistortionShader;

		Ref<Shader> FinalCompositingFXShader;
		Ref<Shader> FinalCompositingShader;

		// PBR IBL Cubemap
		glm::mat4 CubemapCaptureProjection;
		glm::mat4 CubemapCaptureViews[6];

		Ref<Shader>	EquirectangularToCubemapShader;
		Ref<Shader>	IrradianceShader;
		Ref<Shader> PrefilterShader;
		Ref<Shader>	brdfShader;
		Ref<Shader>	BackgroundShader;

		// Volumetric Clouds
		Ref<Shader> VolumetricCloudsShader;
		Ref<Texture2D> VolumetricNoiseTexture;
		
		// Path Tracing
		Ref<Shader> PathTraceShader;
		Ref<Shader> PathTraceAccumulateShader;
		Ref<Shader> PathTraceDenoiseShader;

		// Culling
		Ref<Shader> CullingComputeShader;

		// TODO: These should be on a per context basis
		std::vector<Ref<Texture>> PathTraceMaterialTextures;
		Ref<Shader> PathTraceMaterialShader = nullptr;

		// VXGI
		// TODO: Move these objects to a 'RendererProperties' interface (which can be configured by a file for persistence and runtime)
		bool EnableVXGI = false;
		bool VXGITraceVisualization = false;
		bool VXGIGridVisualization = false;
		bool VXGIEnableTranslucentLighting = true;
		bool VXGIEnableTranslucentGridContribution = true;
		float VXGIRenderScale = 1.0f;
		Ref<Shader> VXGIConeTracingShader;
		Ref<Shader> VXGIDebugVisualizationShader;
		Ref<Shader> VXGIMipmapShader;
		Ref<Shader> VXGIClearShader;
		Ref<Shader> VXGIMergeShader;

		float StepMultiplier = 0.16f;
		float DebugStepMultiplier = 0.4f;

		struct VXGIGridData
		{
			Ref<Texture3D> VXGIGrid;
			Ref<Texture3D> VXGIGridR;
			Ref<Texture3D> VXGIGridG;
			Ref<Texture3D> VXGIGridB;

			float VXGIGridSize = 20.0f;
			uint32_t VXGIGridResolution = 128;
			glm::vec3 VXGIGridOffset = glm::vec3(0.0f);
		};

		VXGIGridData VXGICascades[RendererConstants::VXGICascadeCount];

		// SSGI
		Ref<Shader> SSGIShader;

		// Volumetric Clouds
		Ref<Texture3D> PerlinWorleyNoise;
		Ref<Texture2D> CurlNoise;

		// TAA
		Ref<Shader> TAAShader;

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
			unsigned int enabled = false;
			float intensity;
			float range;
			int shadowIndex = -1;
		};
		
		struct VolumeTileAABB
		{
			glm::vec4 minPoint;
			glm::vec4 maxPoint;
		};

		RendererSharedData::CameraData CameraBuffer;

		struct LightingData
		{
			DirectionalLight DirectionalLight;
			glm::mat4 LightSpaceMatrices[RendererConstants::MaxCascadeCount];
			float CascadePlaneDistances[RendererConstants::MaxCascadeCount]; // Aligned as vec4 (see shader)
			int UsingDirectionalLight;
			int CascadeCount;
			int UsingSkyLight;
			int UsingFlowMap;
			float SkyLightIntensity;
			float Ambient = 0.015f;
			float BUFF[2];
		};
		LightingData LightingBuffer;
		Ref<UniformBuffer> LightingUniformBuffer;

		struct ObjectData
		{
			glm::mat4 Model;
			glm::mat4 ModelInverse;
			glm::mat4 PreviousModel;
			glm::mat4 Normal;
			int EntityID;
			int Animated;
			float BUFF[2];
		};
		ObjectData ObjectBuffer;
		Ref<UniformBuffer> ObjectUniformBuffer;

		struct AnimationData
		{
			glm::mat4 FinalBonesMatrices[RendererConstants::MaxBones];
		};
		AnimationData AnimationBuffer;
		Ref<UniformBuffer> AnimationUniformBuffer;

		struct GBufferData
		{
			uint64_t Albedo;
			uint64_t EntityID;
			uint64_t Normal;
			uint64_t Emissive;
			uint64_t Roughness_Metallic_Specular_AO;
			uint64_t SubmeshIndex;
			uint64_t Velocity;
			uint64_t Depth;

			uint64_t EnvironmentCubemap;
			uint64_t IrradianceMap;
			uint64_t PrefilterMap;
			uint64_t brdfLUTTexture;
			uint64_t FlowMapCubemap;

			alignas(16) struct
			{
				uint64_t Cascade;
				uint64_t PADD;
			} VXGICascades[RendererConstants::VXGICascadeCount];
		};
		GBufferData GBufferBuffer;
		Ref<UniformBuffer> GBufferUniformBuffer;

		Ref<UniformBuffer> MaterialParameterUniformBuffer;
		Ref<UniformBuffer> MaterialTextureUniformBuffer;

		struct Volume
		{
			glm::vec4 Min;
			glm::vec4 Max;

			glm::vec4 Color;

			int Blend = 0;
			float ScatteringDistribution = 0.5;
			float ScatteringIntensity = 1.0;
			float ExtinctionScale = 0.5;
		};
		
		struct VolumetricData
		{
			Volume Volumes[RendererConstants::MaxVolumes];
			int VolumeCount;
			float VolumetricGridSize;
		};
		VolumetricData VolumetricBuffer;
		Ref<UniformBuffer> VolumetricUniformBuffer;

		struct PostProcessingData
		{
			glm::vec4 SSAOSamples[64];
			
			int VisualizationMode = 0;
			
			uint32_t Frame = 0;
			float Time = 0.0f;
			float DeltaTime = 0.0f;
			float Gamma = 2.2f;
			float LensDistortion = 0.05f;
			float AberrationAmount = 0.01f;
			float GrainAmount = 0.25f;
			float VignetteIntensity = 15.0f;
			float VignettePower = 0.25f;
			uint32_t UsingLUT = 0;
			float BloomThreshold;
			float FocusNearStart = 0.0f;
			float FocusNearEnd = 0.0f;
			float FocusFarStart = 0.0f;
			float FocusFarEnd = 0.0f;
			float FocusScale = 0.0f;
			float BokehThreshold = 0.5f;
			float BokehSize = 1.0f;

			// Anti-Aliasing
			uint32_t AntiAliasingMode = RendererConstants::AntiAliasingMode::FXAA;
			uint32_t TAASampleCount = 6;
			// TODO: Add TAA jitter

			int UsingVXGI = true;
			int UsingVolumetricClouds = false;

			float PADD[1];
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
			} BokehList[RendererConstants::MaxBokehCount];
		};
		Ref<ShaderStorageBuffer> BokehSSBO;

		// Submesh SSBO
		struct SubmeshData
		{
			int SubmeshIndex;
			uint32_t PADD[3];
		};
		SubmeshData SubmeshBuffer;
		Ref<UniformBuffer> SubmeshUniformBuffer;

		// Path Tracing SSBO
		
		struct RayTriangle
		{
			// Note: X/Y Texture Coordinates are packed into the Position/Normal w component 
			glm::vec4 Position[3];
			glm::vec4 Normal[3];
		};

		struct BVHNode
		{
			BVHNode()
				: Min(0.0f), TriangleCount(0), Max(0.0f), TriangleIndex(0), ChildIndex(0)
			{}
			
			glm::vec3 Min;
			uint32_t TriangleCount;
			glm::vec3 Max;

			// We know that the second child will be stored at the next index after the first index (so no need to store one for each)
			// NOTE: Index represents the triangleIndex if a leaf node and otherwise the childIndex (is leaf node if TriangleCount > 0)
			uint32_t TriangleIndex;
			uint32_t ChildIndex;

			uint32_t PADD[3];
		};

		struct BVHSubmesh
		{
			glm::vec3 Min;
			uint32_t NodeIndex;
			glm::vec3 Max;
			uint32_t Material;
		};

		struct BVHModel
		{
			glm::mat4 Model;
			glm::mat4 InverseModel;
			int EntityID;
			uint32_t SubmeshIndex;
			uint32_t SubmeshCount;

			uint32_t PADD[1];
		};

		Ref<ShaderStorageBuffer> PathTraceGeometrySSBO;
		Ref<ShaderStorageBuffer> PathTraceBVHSSBO;
		Ref<ShaderStorageBuffer> PathTraceSubmeshSSBO;
		Ref<ShaderStorageBuffer> PathTraceModelSSBO;
		std::vector<RayTriangle> RayTriangles;
		std::vector<BVHNode> BVHNodes;
		std::vector<BVHSubmesh> BVHSubmeshes;
		std::vector<BVHModel> BVHModels;
		std::unordered_map<AssetHandle, std::vector<uint32_t>> BVHModelNodeMap;
		std::unordered_map<AssetHandle, uint32_t> BVHMaterialMap;
		uint32_t NextBVHMaterialIndex = 1;

		struct MeshInstance
		{
			MeshInstance(const glm::mat4& modelMatrix, const glm::mat4& inverseModelMatrix, const glm::vec3& min, const glm::vec3& max, const uint32_t sdfIndex)
				: ModelMatrix(modelMatrix), InverseModelMatrix(inverseModelMatrix), Min(min), Max(max), SDFIndex(sdfIndex) {}

			glm::mat4 ModelMatrix;
			glm::mat4 InverseModelMatrix;
			glm::vec3 Min;
			float PADD;
			glm::vec3 Max;
			uint32_t SDFIndex;
		};

		Ref<ShaderStorageBuffer> MeshInstanceSSBO;
		Ref<ShaderStorageBuffer> MeshVisibilitySSBO;

		struct VXGIData
		{
			glm::vec4 GridMin[RendererConstants::VXGICascadeCount];
			glm::vec4 GridMax[RendererConstants::VXGICascadeCount];
			//uint32_t GridResolution[RendererConstants::VXGICascadeCount];
			glm::uvec4 GridResolution;

			uint32_t RenderCascade;
			uint32_t RenderAxis;

			float StepMultiplier;			// Range is 0.05 to 1.0
			float DebugConeAngle = 0.0f;	// Range is 0 to 0.5

			uint32_t MaxSamples = 20;
			float GIBoost = 1.3f;
			float GISkyLightBoost = 1.0f / 1.3f;
			float NormalRayOffset = 1.0f;
			float AlphaThreashold = 0.99f;
			float MaxConeAngle = 0.32f;		// 18 degrees
			float MinConeAngle = 0.005f;	// 0.29 degrees
			float ContributionFalloff = 0.0f;
			int UseTemporalAccumulation = true;

			float PADD[3];
		};
		VXGIData VXGIBuffer;
		Ref<UniformBuffer> VXGIUniformBuffer;

		struct BlendShapeWeightData
		{
			uint32_t BlendShapeCount;
			uint32_t BlendShapeVertexCount;
			float PADD[2];
			float BlendShapeWeights[RendererConstants::MaxBlendShapes];
		};
		BlendShapeWeightData BlendShapeWeightBuffer;
		Ref<UniformBuffer> BlendShapeWeightUniformBuffer;

		Ref<ShaderStorageBuffer> MeshSDFSSBO;

		// Performance Flags
		bool TakeFastGeometryShaderPath = false;
		bool TakeAtomicFP16Path = false;

		// Data Lists
		std::vector<ModelData> ModelDrawList;
		std::vector<PointLight> PointLightList;
		std::vector<ParticleSystemData> ParticleSystemList;
		std::vector<DecalData> DecalDrawList;
		std::vector<PostProcessVolumeData> PostProcessVolumeList;
		std::vector<MeshData> TranslucentMeshDrawList;
		uint32_t TranslucentObjectCountPreviousFrame = 0;

		// Render Pass Flags
		bool UsePreDepth = true;
		bool UseSSAO = true;
		bool UseSSR = true;
		bool UseMotionBlur = true;
		bool UseDOF = true;
		bool UseBloom = true;
		bool UseLensDistortion = true;

		// Editor Only
		Ref<Shader> EditorDrawShaderOverride = nullptr;
	};

	static SceneRendererData s_Data;

	static void VXGICreateGrid(const uint32_t cascadeIndex)
	{
		auto& cascade = s_Data.VXGICascades[cascadeIndex];

		// Release original resources
		cascade.VXGIGrid = nullptr;
		cascade.VXGIGridR = nullptr;
		cascade.VXGIGridG = nullptr;
		cascade.VXGIGridB = nullptr;

		if (!s_Data.EnableVXGI)
			return;

		// Create main grid texture
		TextureSpecification gridSpecification;
		gridSpecification.Width = cascade.VXGIGridResolution;
		gridSpecification.Height = cascade.VXGIGridResolution;
		gridSpecification.Depth = cascade.VXGIGridResolution;
		gridSpecification.Format = TextureFormat::RGBA16F;
		gridSpecification.SamplerWrap = TextureWrap::ClampToEdge;
		gridSpecification.SamplerFilter = TextureFilter::LinearMipmapLinear;
		// TODO: When we add separate Min/Mag filters, Mag should be just linear!
		gridSpecification.GenerateMips = true;

		const size_t voxelCount = cascade.VXGIGridResolution * cascade.VXGIGridResolution * cascade.VXGIGridResolution;
		Buffer gridData = Buffer(voxelCount * Utils::GetDymaticTextureFormatBPP(gridSpecification.Format));
		gridData.ZeroInitialize();
		cascade.VXGIGrid = Texture3D::Create(gridSpecification, gridData);
		gridData.Release();

		// Create additional channel staging buffers (if required)
		if (!s_Data.TakeAtomicFP16Path)
		{
			gridSpecification.Format = TextureFormat::RED_UNSIGNED_INTEGER;
			gridSpecification.SamplerFilter = TextureFilter::Nearest;

			Buffer gridData = Buffer(voxelCount * Utils::GetDymaticTextureFormatBPP(gridSpecification.Format));
			gridData.ZeroInitialize();
			cascade.VXGIGridR = Texture3D::Create(gridSpecification, gridData);
			cascade.VXGIGridG = Texture3D::Create(gridSpecification, gridData);
			cascade.VXGIGridB = Texture3D::Create(gridSpecification, gridData);
			gridData.Release();
		}

	}

	static void VXGICreateGrid()
	{
		for (uint32_t cascadeIndex = 0; cascadeIndex < RendererConstants::VXGICascadeCount; cascadeIndex++)
			VXGICreateGrid(cascadeIndex);
	}

	static void SetVXGITakeFastGeometryShaderPath(const bool enable)
	{
		// Ensure that the appropriate API extensions are supported
		if (!RenderCommand::IsExtensionSupported("GL_NV_geometry_shader_passthrough") || !RenderCommand::IsExtensionSupported("GL_NV_viewport_swizzle"))
		{
			DY_CORE_WARN("Required API extensions for VXGI fast geometry shader path not found!");
			return;
		}

		s_Data.TakeFastGeometryShaderPath = enable;
	}

	static void SetVXGITakeAtomicFP16Path(const bool enable)
	{
		// Ensure that the appropriate API extensions are supported
		if (!RenderCommand::IsExtensionSupported("GL_NV_shader_atomic_fp16_vector"))
		{
			DY_CORE_WARN("Required API extensions for VXGI fast atomic path not found!");
			return;
		}

		s_Data.TakeAtomicFP16Path = enable;
		s_Data.VXGIClearShader->SetMacro("TAKE_ATOMIC_FP16_PATH", enable);
	}

	void SceneRenderer::Init()
	{
		DY_PROFILE_FUNCTION();

		if (!RenderCommand::IsExtensionSupported("GL_ARB_bindless_texture"))
		{
			DY_CORE_ERROR("API extension GL_ARB_bindless_texture is required for the SceneRenderer engine! Terminating...");
			exit(1);
		}

		FSRManager::Init();

		// Setup base white texture
		TextureSpecification whiteTextureSpecification;
		whiteTextureSpecification.Width = 1;
		whiteTextureSpecification.Height = 1;
		s_Data.WhiteTexture = Texture2D::Create(whiteTextureSpecification);

		uint32_t whiteTextureData = 0xffffffff;
		s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

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

		// Batched Quad Vertex Buffer
		{
			struct VertexData
			{
				glm::vec3 position;
				int index;
			};

			const uint32_t maxCount = std::max(RendererConstants::MaxBokehCount, RendererConstants::MaxParticleCount);

			s_Data.BatchedQuadVertexArray = VertexArray::Create();
			s_Data.BatchedQuadVertexBuffer = VertexBuffer::Create(maxCount * 4 * sizeof(VertexData));
			s_Data.BatchedQuadVertexBuffer->SetLayout({
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Int, "a_Index" }
				});
			s_Data.BatchedQuadVertexArray->AddVertexBuffer(s_Data.BatchedQuadVertexBuffer);

			{
				uint32_t currentIndex = 0;

				VertexData* data = new VertexData[maxCount * 4];

				for (uint32_t i = 0; i < maxCount * 4; i += 4)
				{
					uint32_t index = currentIndex++;
					data[i + 0].position = s_Data.QuadBuffer[0];
					data[i + 0].index = index;
					data[i + 1].position = s_Data.QuadBuffer[1];
					data[i + 1].index = index;
					data[i + 2].position = s_Data.QuadBuffer[2];
					data[i + 2].index = index;
					data[i + 3].position = s_Data.QuadBuffer[3];
					data[i + 3].index = index;
				}

				s_Data.BatchedQuadVertexBuffer->SetData(data, maxCount * 4 * sizeof(VertexData));
				delete[] data;
			}

			{
				uint32_t* quadIndices = new uint32_t[maxCount * 6];

				uint32_t offset = 0;
				for (uint32_t i = 0; i < maxCount * 6; i += 6)
				{
					quadIndices[i + 0] = offset + 0;
					quadIndices[i + 1] = offset + 1;
					quadIndices[i + 2] = offset + 2;

					quadIndices[i + 3] = offset + 2;
					quadIndices[i + 4] = offset + 3;
					quadIndices[i + 5] = offset + 0;

					offset += 4;
				}

				Ref<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, maxCount * 6);
				s_Data.BatchedQuadVertexArray->SetIndexBuffer(quadIB);
				delete[] quadIndices;
			}
		}

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
		// Slot 0 is reserved for the camera buffer shared across renderers
		s_Data.LightingUniformBuffer = UniformBuffer::Create(sizeof(SceneRendererData::LightingData), RendererConstants::Buffers::Lighting);
		s_Data.ObjectUniformBuffer = UniformBuffer::Create(sizeof(SceneRendererData::ObjectData), RendererConstants::Buffers::Object);
		s_Data.AnimationUniformBuffer = UniformBuffer::Create(sizeof(SceneRendererData::AnimationData), RendererConstants::Buffers::Animation);
		s_Data.GBufferUniformBuffer = UniformBuffer::Create(sizeof(SceneRendererData::GBufferData), RendererConstants::Buffers::GBuffer);
		s_Data.MaterialParameterUniformBuffer = UniformBuffer::Create(RendererConstants::MaxMaterialBufferSize, RendererConstants::Buffers::MaterialParameters);
		s_Data.MaterialTextureUniformBuffer = UniformBuffer::Create(RendererConstants::MaxMaterialBufferSize, RendererConstants::Buffers::MaterialTextures);
		s_Data.VolumetricUniformBuffer = UniformBuffer::Create(sizeof(SceneRendererData::VolumetricData), RendererConstants::Buffers::Volumetric);
		s_Data.PostProcessingUniformBuffer = UniformBuffer::Create(sizeof(SceneRendererData::PostProcessingData), RendererConstants::Buffers::PostProcessing);

		// Lighting SSBOs
		s_Data.ClusterAABBSSBO = ShaderStorageBuffer::Create(sizeof(glm::vec4) * 2 * RendererConstants::NumClusters, RendererConstants::Buffers::ClusterAABB, ShaderStorageBufferUsage::DYNAMIC_DRAW);
		s_Data.PointLightSSBO = ShaderStorageBuffer::Create(RendererConstants::MaxLights * sizeof(SceneRendererData::PointLight), RendererConstants::Buffers::PointLight, ShaderStorageBufferUsage::DYNAMIC_DRAW);
		s_Data.LightIndexSSBO = ShaderStorageBuffer::Create(RendererConstants::MaxLights * sizeof(unsigned int), RendererConstants::Buffers::LightIndex, ShaderStorageBufferUsage::DYNAMIC_DRAW);
		s_Data.LightGridSSBO = ShaderStorageBuffer::Create(RendererConstants::NumClusters * 2 * sizeof(unsigned int), RendererConstants::Buffers::LightGrid, ShaderStorageBufferUsage::DYNAMIC_DRAW);
		s_Data.LightCountsSSBO = ShaderStorageBuffer::Create(2 * sizeof(unsigned int), RendererConstants::Buffers::LightCount, ShaderStorageBufferUsage::DYNAMIC_DRAW);

		// Bokeh SSBO
		s_Data.BokehSSBO = ShaderStorageBuffer::Create(sizeof(SceneRendererData::BokehData), RendererConstants::Buffers::Bokeh, ShaderStorageBufferUsage::DYNAMIC_COPY);

		// [Slot 13] reserved for the bound particle system

		s_Data.SubmeshUniformBuffer = UniformBuffer::Create(sizeof(SceneRendererData::SubmeshData), RendererConstants::Buffers::Submesh);

		// Path Tracing
		s_Data.PathTraceGeometrySSBO = ShaderStorageBuffer::Create(sizeof(SceneRendererData::RayTriangle) * RendererConstants::MaxPathTraceTriangles + sizeof(uint32_t) * 4, RendererConstants::Buffers::PathTraceGeometry, ShaderStorageBufferUsage::DYNAMIC_DRAW);
		s_Data.PathTraceBVHSSBO = ShaderStorageBuffer::Create(sizeof(SceneRendererData::BVHNode) * RendererConstants::MaxPathTraceNodes, RendererConstants::Buffers::PathTraceBVH, ShaderStorageBufferUsage::DYNAMIC_DRAW);
		s_Data.PathTraceSubmeshSSBO = ShaderStorageBuffer::Create(sizeof(SceneRendererData::BVHSubmesh) * RendererConstants::MaxPathTraceSubmesh, RendererConstants::Buffers::PathTraceSubmesh, ShaderStorageBufferUsage::DYNAMIC_DRAW);
		s_Data.PathTraceModelSSBO = ShaderStorageBuffer::Create(sizeof(SceneRendererData::BVHModel) * RendererConstants::MaxPathTraceModels + sizeof(uint32_t) * 4, RendererConstants::Buffers::PathTraceModel, ShaderStorageBufferUsage::DYNAMIC_DRAW);

		// Culling
		s_Data.MeshInstanceSSBO = ShaderStorageBuffer::Create(sizeof(SceneRendererData::MeshInstance) * RendererConstants::MaxModels + sizeof(uint32_t) * 4, RendererConstants::Buffers::MeshInstance, ShaderStorageBufferUsage::DYNAMIC_DRAW);
		s_Data.MeshVisibilitySSBO = ShaderStorageBuffer::Create((RendererConstants::MaxModels + 1) * sizeof(uint32_t), RendererConstants::Buffers::MeshVisibility, ShaderStorageBufferUsage::DYNAMIC_COPY);

		// VXGI
		s_Data.VXGIUniformBuffer = UniformBuffer::Create(sizeof(SceneRendererData::VXGIData), RendererConstants::Buffers::VXGI);

		// Blend Shapes
		s_Data.BlendShapeWeightUniformBuffer = UniformBuffer::Create(sizeof(SceneRendererData::BlendShapeWeightData), RendererConstants::Buffers::BlendShapeWeight);

		s_Data.MeshSDFSSBO = ShaderStorageBuffer::Create(sizeof(uint64_t) * RendererConstants::MaxModels, RendererConstants::Buffers::MeshSDF, ShaderStorageBufferUsage::DYNAMIC_DRAW);

		// Setup Specular IBL
		{
			s_Data.EquirectangularToCubemapShader = Renderer::GetShaderLibrary()->Get("Renderer3D_IBLEquirectangularToCubemap");
			s_Data.IrradianceShader = Renderer::GetShaderLibrary()->Get("Renderer3D_IBLIrradianceConvolution");
			s_Data.PrefilterShader = Renderer::GetShaderLibrary()->Get("Renderer3D_IBLPrefilter");
			s_Data.brdfShader = Renderer::GetShaderLibrary()->Get("Renderer3D_IBLbrdf");
			s_Data.BackgroundShader = Renderer::GetShaderLibrary()->Get("Renderer3D_IBLBackground");
		}

		// Lighting Passes
		{
			// Setup Deferred Rendering
			{
				s_Data.DeferredLightingShader = Renderer::GetShaderLibrary()->Get("Renderer3D_DeferredLighting");

				s_Data.ClusterShader = Renderer::GetShaderLibrary()->Get("Renderer3D_ClusterShader");
				s_Data.ClusterCullLightShader = Renderer::GetShaderLibrary()->Get("Renderer3D_ClusterCullLightShader");

				s_Data.BufferVisualizationShader = Renderer::GetShaderLibrary()->Get("Renderer3D_BufferVisualization");
				s_Data.WireframeShader = Renderer::GetShaderLibrary()->Get("Renderer3D_Wireframe");
				s_Data.ColorShader = Renderer::GetShaderLibrary()->Get("Renderer3D_Color");
			}

			// Decals
			{
				s_Data.DecalShader = Renderer::GetShaderLibrary()->Get("Renderer3D_Decal");
			}

			// Setup the default checkerboard material via the builder interface
			{
				if (AssetManager::UsingAssetPack())
					s_Data.DefaultMaterial = AssetManager::GetAsset<MaterialAsset>((AssetHandle)EngineAsset::DefaultMaterial);
				else
				{
					using namespace Editor;

					MaterialBuilder builder;

					// halfCoords = TexCoords * 0.5f;
					const Ref<MaterialBuilderNode> texCoords = MaterialBuilderNode::Create(MaterialNodeFunction::TextureCoordinates);
					const Ref<MaterialBuilderNode> halfCoords = MaterialBuilderNode::Create(MaterialNodeFunction::Multiply, { texCoords, 0.5f });
					
					// halfMain = texture(MainMap, halfCoords).g;
					const Ref<MaterialBuilderNode> halfMain = MaterialBuilderNode::Create(MaterialNodeFunction::ComponentMask,
					{
						MaterialBuilderNode::Create(MaterialNodeFunction::TextureSample, { halfCoords, (AssetHandle)EngineAsset::DefaultMaterialMainMap }),
						false, true, false, false
					});

					// checkerboard = texture(MainMap, halfCoords / 0.1).g;
					const Ref<MaterialBuilderNode> checkerboard = MaterialBuilderNode::Create(MaterialNodeFunction::ComponentMask,
					{
						MaterialBuilderNode::Create(MaterialNodeFunction::TextureSample,
						{
							MaterialBuilderNode::Create(MaterialNodeFunction::Divide, { halfCoords, 0.1f }),
							(AssetHandle)EngineAsset::DefaultMaterialMainMap
						}),
						false, true, false, false
					});

					// noise = mix(0.4, 1.0, texture(MainMap, TexCoord / 0.1).r);
					Ref<MaterialBuilderNode> noise = MaterialBuilderNode::Create(MaterialNodeFunction::Mix,
					{
						0.4f,
						1.0f,
						MaterialBuilderNode::Create(MaterialNodeFunction::ComponentMask,
						{
							MaterialBuilderNode::Create(MaterialNodeFunction::TextureSample,
							{
								MaterialBuilderNode::Create(MaterialNodeFunction::Divide, { texCoords, 0.1f }),
								(AssetHandle)EngineAsset::DefaultMaterialMainMap
							}),
							true, false, false, false
						})
					});
					
					// noise = mix(noise, 1.0 - noise, checkerboard);
					noise = MaterialBuilderNode::Create(MaterialNodeFunction::Mix,
					{
						noise,
						MaterialBuilderNode::Create(MaterialNodeFunction::Subtract, { 1.0f, noise }),
						checkerboard
					});

					// noise = mix(noise, 1.0, sqrt(min(distance(WorldPosition, CameraPosition) / 5.0, 1.0)));
					noise = MaterialBuilderNode::Create(MaterialNodeFunction::Mix,
					{
						noise,
						1.0f,
						MaterialBuilderNode::Create(MaterialNodeFunction::SquareRoot,
						{
							MaterialBuilderNode::Create(MaterialNodeFunction::Min,
							{
								MaterialBuilderNode::Create(MaterialNodeFunction::Divide,
								{
									MaterialBuilderNode::Create(MaterialNodeFunction::Distance,
									{
										MaterialBuilderNode::Create(MaterialNodeFunction::WorldPosition),
										MaterialBuilderNode::Create(MaterialNodeFunction::CameraPosition),
									}),
									5.0f
								}),
								1.0f
							})
						})
					});

					// color = mix(0.295, 0.66, mix(halfMain + checkerboard, 0.5, 0.5)) * 0.5 * noise;
					builder.Results[MaterialResultPinType::Albedo] = MaterialBuilderNode::Create(MaterialNodeFunction::Multiply,
					{
						MaterialBuilderNode::Create(MaterialNodeFunction::Mix,
						{
							0.295f,
							0.66f,

							MaterialBuilderNode::Create(MaterialNodeFunction::Mix,
							{
								MaterialBuilderNode::Create(MaterialNodeFunction::Add, { halfMain, checkerboard }),
								0.5f,
								0.5f
							})
						}),

						MaterialBuilderNode::Create(MaterialNodeFunction::Multiply, { noise, 0.5f })
					});

					// normal = texture(NormalMap, halfCoords / 0.05).rgb;
					builder.Results[MaterialResultPinType::Normal] = MaterialBuilderNode::Create(MaterialNodeFunction::TextureSample,
					{
						MaterialBuilderNode::Create(MaterialNodeFunction::Divide, { halfCoords, 0.05f }),
						(AssetHandle)EngineAsset::DefaultMaterialNormalMap
					});

					s_Data.DefaultMaterial = builder.BuildMaterial();
					AssetManager::RegisterEngineMemoryOnlyAsset(s_Data.DefaultMaterial, (AssetHandle)EngineAsset::DefaultMaterial, "Default Material");
				}
			}
		}

		// Post Processing Passes
		{
			// Volumetric Lighting
			{
				s_Data.LightPropagationVolumeShader = Renderer::GetShaderLibrary()->Get("Renderer3D_LightPropagationVolume");
				s_Data.VolumetricLightingShader = Renderer::GetShaderLibrary()->Get("Renderer3D_VolumetricLighting");
				s_Data.GaussianBlurShader = Renderer::GetShaderLibrary()->Get("Renderer3D_GaussianBlur");
			}

			// SSAO
			{
				std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
				std::default_random_engine generator;

				// Setup Noise Texture
				{
					const int noiseSize = 4;

					TextureSpecification spec;
					spec.Width = noiseSize;
					spec.Height = noiseSize;
					s_Data.SSAONoiseTexture = Texture2D::Create(spec);

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
				s_Data.SSAOShader = Renderer::GetShaderLibrary()->Get("Renderer3D_SSAO");
			}

			// Setup Volumetric Clouds
			{
				s_Data.VolumetricCloudsShader = Renderer::GetShaderLibrary()->Get("Renderer3D_VolumetricClouds");
				s_Data.VolumetricNoiseTexture = Texture2D::Create("Resources/Textures/NoiseTexture.png");
			}

			// SSR
			s_Data.SSRShader = Renderer::GetShaderLibrary()->Get("Renderer3D_SSR");

			// FXAA
			s_Data.FXAAShader = Renderer::GetShaderLibrary()->Get("Renderer3D_FXAA");

			// Motion Blur
			s_Data.MotionBlurShader = Renderer::GetShaderLibrary()->Get("Renderer3D_MotionBlur");

			// DOF
			{
				s_Data.DOFShader = Renderer::GetShaderLibrary()->Get("Renderer3D_DOF");
				s_Data.BokehDrawShader = Renderer::GetShaderLibrary()->Get("Renderer3D_BokehDraw");
				s_Data.BokehIsolateShader = Renderer::GetShaderLibrary()->Get("Renderer3D_BokehIsolate");

				s_Data.BokehShapeTexture = Texture2D::Create("Resources/Textures/Bokeh/BokehHexegon.png");
			}

			// Lens Flare
			s_Data.LensFlareShader = Renderer::GetShaderLibrary()->Get("Renderer3D_LensFlare");

			// Bloom
			s_Data.BloomIsolateShader = Renderer::GetShaderLibrary()->Get("Renderer3D_BloomIsolate");
			s_Data.BloomBlurHorizontalShader = Renderer::GetShaderLibrary()->Get("Renderer3D_BloomBlurHorizontal");
			s_Data.BloomBlurVerticalShader = Renderer::GetShaderLibrary()->Get("Renderer3D_BloomBlurVertical");
			s_Data.AddTextureShader = Renderer::GetShaderLibrary()->Get("Renderer3D_AddTexture");
			s_Data.BloomCompositeShader = Renderer::GetShaderLibrary()->Get("Renderer3D_BloomComposite");
			s_Data.BloomDirtTexture = Texture2D::Create("Resources/Textures/DirtMaskTexture.png");

			// Lens Distortion
			s_Data.LensDistortionShader = Renderer::GetShaderLibrary()->Get("Renderer3D_LensDistortion");

			// Final Compositing
			s_Data.FinalCompositingFXShader = Renderer::GetShaderLibrary()->Get("Renderer3D_FinalCompositingFX");
			s_Data.FinalCompositingShader = Renderer::GetShaderLibrary()->Get("Renderer3D_FinalCompositing");
		}

		// Shadow Buffer
		{
			FramebufferSpecification fbSpec;
			fbSpec.Attachments = { TextureFormat::Depth };

			// Directional Light Shadow Framebuffer
			fbSpec.Width = RendererConstants::ShadowMapResolution;
			fbSpec.Height = RendererConstants::ShadowMapResolution;

			// Shadow Shader CSM
			s_Data.DirectionalShadowFramebuffer = Framebuffer::Create(fbSpec);
		}

		// Outline
		s_Data.OutlineShader = Renderer::GetShaderLibrary()->Get("Renderer3D_Outline");

		// Setup projections and view matricies for capturing data
		{
			s_Data.CubemapCaptureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
			s_Data.CubemapCaptureViews[0] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
			s_Data.CubemapCaptureViews[1] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
			s_Data.CubemapCaptureViews[2] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			s_Data.CubemapCaptureViews[3] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
			s_Data.CubemapCaptureViews[4] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
			s_Data.CubemapCaptureViews[5] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
		}

		// Path Tracing
		{
			s_Data.PathTraceShader = Renderer::GetShaderLibrary()->Get("Renderer3D_PathTrace");
			s_Data.PathTraceAccumulateShader = Renderer::GetShaderLibrary()->Get("Renderer3D_PathTraceAccumulate");
			s_Data.PathTraceDenoiseShader = Renderer::GetShaderLibrary()->Get("Renderer3D_PathTraceDenoise");
		}

		// Culling
		{
			s_Data.CullingComputeShader = Renderer::GetShaderLibrary()->Get("Renderer3D_Cull");
		}

		// VXGI
		{
			// Create shaders
			s_Data.VXGIConeTracingShader = Renderer::GetShaderLibrary()->Get("Renderer3D_VXGIConeTracing");
			s_Data.VXGIDebugVisualizationShader = Renderer::GetShaderLibrary()->Get("Renderer3D_VXGIDebugVisualization");
			s_Data.VXGIMipmapShader = Renderer::GetShaderLibrary()->Get("Renderer3D_VXGIMipmap");
			s_Data.VXGIClearShader = Renderer::GetShaderLibrary()->Get("Renderer3D_VXGIClear");
			s_Data.VXGIMergeShader = Renderer::GetShaderLibrary()->Get("Renderer3D_VXGIMerge");

			// Default enable optimizations (will auto-fail if extensions are not found)
			SetVXGITakeFastGeometryShaderPath(true);
			SetVXGITakeAtomicFP16Path(true);
			VXGICreateGrid();
		}

		// SSGI
		{
			s_Data.SSGIShader = Renderer::GetShaderLibrary()->Get("Renderer3D_SSGI");
		}

		// Volumetric Clouds
		{
			TextureSpecification perlinWorleySpecification;
			perlinWorleySpecification.Format = TextureFormat::RGBA8;
			perlinWorleySpecification.Width = RendererConstants::PerlinWorleyResolution;
			perlinWorleySpecification.Height = RendererConstants::PerlinWorleyResolution;
			perlinWorleySpecification.Depth = RendererConstants::PerlinWorleyResolution;
			s_Data.PerlinWorleyNoise = Texture3D::Create(perlinWorleySpecification);

			s_Data.PerlinWorleyNoise->BindTexture(0);
			const uint32_t dispatchCount = INT_CEILING_DIVISION(RendererConstants::PerlinWorleyResolution, RendererConstants::PerlinWorleyLocalSize);
			Renderer::GetShaderLibrary()->Get("Renderer3D_PerlinWorleyNoise")->Dispatch(dispatchCount, dispatchCount, dispatchCount);
		}

		// TAA
		{
			s_Data.TAAShader = Renderer::GetShaderLibrary()->Get("Renderer3D_TAA");
		}

		s_Data.DOFShader->GetPackagedShaderBuffers();
	}

	void SceneRenderer::Shutdown()
	{
		DY_PROFILE_FUNCTION();

		FSRManager::Shutdown();
	}

	void SceneRenderer::BeginScene()
	{
		DY_PROFILE_FUNCTION();

		s_Data.PostProcessingBuffer.Frame++;
		s_Data.ContextCameraCount = 0;

		// Reset lighting data
		s_Data.LightingBuffer.UsingDirectionalLight = false;
		s_Data.LightingBuffer.UsingSkyLight = false;
		s_Data.LightingBuffer.UsingFlowMap = false;
		s_Data.VolumetricBuffer.VolumeCount = 0;
	}

	void SceneRenderer::SubmitCamera(const Camera& camera, const glm::mat4& transform)
	{
		DY_PROFILE_FUNCTION();

		s_Data.ContextCameraCount++;

		s_Data.CameraBuffer.ViewProjection = camera.GetProjection() * glm::inverse(transform);
		s_Data.CameraBuffer.ViewPosition = transform[3];

		if (!s_Data.ActiveContext->FreezeFrustumUpdate)
			s_Data.CameraFrustum = Frustum(s_Data.CameraBuffer.ViewProjection);

		// Check if path tracing accumulation should be invalidated
		if (s_Data.ActiveContext->VisualizationMode == SceneRendererContext::RendererVisualizationMode::PathTraced &&
			s_Data.ActiveContext->PreviousViewProjectionMatrix != s_Data.CameraBuffer.ViewProjection
		)
		{
			s_Data.ActiveContext->AccumulationFrames = 0;
		}

		s_Data.CameraBuffer.Projection = camera.GetProjection();
		s_Data.CameraBuffer.InverseProjection = glm::inverse(s_Data.CameraBuffer.Projection);
		s_Data.CameraBuffer.View = glm::inverse(transform);
		s_Data.CameraBuffer.InverseView = transform;
		s_Data.CameraBuffer.PreviousViewProjection = s_Data.ActiveContext->PreviousViewProjectionMatrix;
		s_Data.CameraBuffer.InverseViewProjection = glm::inverse(s_Data.CameraBuffer.ViewProjection);

		s_Data.CameraBuffer.Forward = glm::vec4(s_Data.CameraBuffer.View[0][2], s_Data.CameraBuffer.View[1][2], s_Data.CameraBuffer.View[2][2], 1.0f);
		s_Data.CameraBuffer.Right = glm::vec4(s_Data.CameraBuffer.View[0][0], s_Data.CameraBuffer.View[1][0], s_Data.CameraBuffer.View[2][0], 1.0f);
		s_Data.CameraBuffer.Up = glm::vec4(s_Data.CameraBuffer.View[0][1], s_Data.CameraBuffer.View[1][1], s_Data.CameraBuffer.View[2][1], 1.0f);

		auto sizeX = (unsigned int)std::ceilf(s_Data.ActiveContext->ActiveWidth / (float)RendererConstants::GridSizeX);
		s_Data.CameraBuffer.TileSizes = { RendererConstants::GridSizeX, RendererConstants::GridSizeY, RendererConstants::GridSizeZ, sizeX };

		s_Data.CameraBuffer.ScreenDimensions = { s_Data.ActiveContext->ActiveWidth, s_Data.ActiveContext->ActiveHeight };
		s_Data.CameraBuffer.PixelSize = { 1.0f / float(s_Data.ActiveContext->ActiveWidth), 1.0f / float(s_Data.ActiveContext->ActiveHeight) };
		s_Data.CameraBuffer.ZNear = ((SceneCamera*)&camera)->GetPerspectiveNearClip();
		s_Data.CameraBuffer.ZFar = ((SceneCamera*)&camera)->GetPerspectiveFarClip();

		// Simple pre-calculation to reduce use of log function - TODO: Should really be under lighting
		s_Data.CameraBuffer.Scale = (float)RendererConstants::GridSizeZ / std::log2f(s_Data.CameraBuffer.ZFar / s_Data.CameraBuffer.ZNear);
		s_Data.CameraBuffer.Bias = -((float)RendererConstants::GridSizeZ * std::log2f(s_Data.CameraBuffer.ZNear) / std::log2f(s_Data.CameraBuffer.ZFar / s_Data.CameraBuffer.ZNear));

		Renderer::SetCameraData(s_Data.CameraBuffer);

		FSRManager::UpdateCamera(camera);

		// Camera Settings
		const SceneCamera::CameraSettings& settings = ((SceneCamera*)&camera)->GetCameraSettings();
		s_Data.PostProcessingBuffer.FocusScale = settings.DOFStrength;
		s_Data.PostProcessingBuffer.FocusNearEnd = settings.DOFTarget - settings.DOFFocusRange * 0.5f;
		s_Data.PostProcessingBuffer.FocusNearStart = s_Data.PostProcessingBuffer.FocusNearEnd - settings.DOFFocusFalloff;
		s_Data.PostProcessingBuffer.FocusFarStart = settings.DOFTarget + settings.DOFFocusRange * 0.5f;
		s_Data.PostProcessingBuffer.FocusFarEnd = s_Data.PostProcessingBuffer.FocusFarStart + settings.DOFFocusFalloff;

		s_Data.PostProcessingBuffer.BloomThreshold = settings.BloomThreshold;

		const uint32_t lutID = settings.LUT ? settings.LUT->GetRendererID() : 0;
		if (s_Data.ActiveContext->LUTID != lutID)
		{ 
			// The user specified LUT texture handle does not match, it must have changed since the previous frame.
			s_Data.ActiveContext->LUTID = lutID;
			UpdateLUT(settings.LUT);
		}
	}

	void SceneRenderer::SubmitCamera(const EditorCamera& camera)
	{
		DY_PROFILE_FUNCTION();

		s_Data.ContextCameraCount++;

		s_Data.CameraBuffer.ViewProjection = camera.GetViewProjection();
		s_Data.CameraBuffer.ViewPosition = glm::vec4(camera.GetPosition(), 1.0f);

		if (!s_Data.ActiveContext->FreezeFrustumUpdate)
			s_Data.CameraFrustum = Frustum(s_Data.CameraBuffer.ViewProjection);

		// Check if path tracing accumulation should be invalidated
		if (s_Data.ActiveContext->VisualizationMode == SceneRendererContext::RendererVisualizationMode::PathTraced &&
			s_Data.ActiveContext->PreviousViewProjectionMatrix != s_Data.CameraBuffer.ViewProjection
		)
		{
			s_Data.ActiveContext->AccumulationFrames = 0;
		}

		s_Data.CameraBuffer.Projection = camera.GetProjection();
		s_Data.CameraBuffer.InverseProjection = glm::inverse(s_Data.CameraBuffer.Projection);
		s_Data.CameraBuffer.View = camera.GetViewMatrix();
		s_Data.CameraBuffer.InverseView = glm::inverse(s_Data.CameraBuffer.View);
		s_Data.CameraBuffer.PreviousViewProjection = s_Data.ActiveContext->PreviousViewProjectionMatrix;
		s_Data.CameraBuffer.InverseViewProjection = glm::inverse(s_Data.CameraBuffer.ViewProjection);

		s_Data.CameraBuffer.Forward = glm::vec4(s_Data.CameraBuffer.View[0][2], s_Data.CameraBuffer.View[1][2], s_Data.CameraBuffer.View[2][2], 1.0f);
		s_Data.CameraBuffer.Right = glm::vec4(s_Data.CameraBuffer.View[0][0], s_Data.CameraBuffer.View[1][0], s_Data.CameraBuffer.View[2][0], 1.0f);
		s_Data.CameraBuffer.Up = glm::vec4(s_Data.CameraBuffer.View[0][1], s_Data.CameraBuffer.View[1][1], s_Data.CameraBuffer.View[2][1], 1.0f);

		auto sizeX = (unsigned int)std::ceilf(s_Data.ActiveContext->ActiveWidth / (float)RendererConstants::GridSizeX);
		s_Data.CameraBuffer.TileSizes = { RendererConstants::GridSizeX, RendererConstants::GridSizeY, RendererConstants::GridSizeZ, sizeX };

		s_Data.CameraBuffer.ScreenDimensions = { s_Data.ActiveContext->ActiveWidth, s_Data.ActiveContext->ActiveHeight };
		s_Data.CameraBuffer.PixelSize = { 1.0f / float(s_Data.ActiveContext->ActiveWidth), 1.0f / float(s_Data.ActiveContext->ActiveHeight) };
		s_Data.CameraBuffer.ZNear = camera.GetNearClip();
		s_Data.CameraBuffer.ZFar = camera.GetFarClip();

		// Simple pre-calculation to reduce use of log function - TODO: Should really be under lighting
		s_Data.CameraBuffer.Scale = (float)RendererConstants::GridSizeZ / std::log2f(s_Data.CameraBuffer.ZFar / s_Data.CameraBuffer.ZNear);
		s_Data.CameraBuffer.Bias = - ((float)RendererConstants::GridSizeZ * std::log2f(s_Data.CameraBuffer.ZNear) / std::log2f(s_Data.CameraBuffer.ZFar / s_Data.CameraBuffer.ZNear));

		Renderer::SetCameraData(s_Data.CameraBuffer);

		FSRManager::UpdateCamera(camera);

		// Camera Settings
		s_Data.PostProcessingBuffer.FocusScale = 0.0f;
		s_Data.PostProcessingBuffer.BloomThreshold = 2.0f;

		// The user specified LUT texture handle does not match, it must have changed since the previous frame.
		if (s_Data.ActiveContext->LUTID != 0)
		{
			s_Data.ActiveContext->LUTID = 0;
			UpdateLUT(nullptr);
		}
	}

	static bool IsFirstDraw()
	{
		return s_Data.ContextCameraCount == 1;
	}

	void SceneRenderer::EndScene()
	{
		DY_PROFILE_FUNCTION();

		s_Data.ModelDrawList.clear();
		s_Data.TranslucentMeshDrawList.clear();
		s_Data.PointLightList.clear();
		s_Data.ParticleSystemList.clear();
		s_Data.DecalDrawList.clear();
		s_Data.PostProcessVolumeList.clear();
		s_Data.NextShadowIndex = 0;
		s_Data.EditorDrawShaderOverride = nullptr;

		std::swap(s_Data.ActiveContext->SceneContext->CurrentModelTransforms, s_Data.ActiveContext->SceneContext->PreviousModelTransforms);
		s_Data.ActiveContext->SceneContext->CurrentModelTransforms.clear();
	}

	Ref<SceneRendererContext> SceneRenderer::GetActiveContext()
	{
		return s_Data.ActiveContext;
	}

	void SceneRenderer::SetActiveContext(Ref<SceneRendererContext> context)
	{
		s_Data.ActiveContext = context;
	}

	RendererConstants::AntiAliasingMode SceneRenderer::GetAntiAliasingMode()
	{
		return (RendererConstants::AntiAliasingMode)s_Data.PostProcessingBuffer.AntiAliasingMode;
	}

	void SceneRenderer::SetAntiAliasingMode(const RendererConstants::AntiAliasingMode mode)
	{
		s_Data.PostProcessingBuffer.AntiAliasingMode = mode;
	}

	void SceneRenderer::UpdateTimestep(Timestep ts)
	{
		// Increment the context time
		auto& sceneContext = s_Data.ActiveContext->SceneContext;
		sceneContext->Time += ts;

		// Update the uniform buffer data
		s_Data.PostProcessingBuffer.DeltaTime = ts;
		s_Data.PostProcessingBuffer.Time = sceneContext->Time;
	}

	void SceneRenderer::SubmitModel(const glm::mat4& transform, Ref<Model> model, int entityID, bool selected)
	{
		SubmitModel(transform, model, {}, nullptr, nullptr, entityID, selected);
	}

	void SceneRenderer::SubmitModel(const glm::mat4& transform, Ref<Model> model, Ref<MaterialAsset> material, int entityID, bool selected)
	{
		SubmitModel(transform, model, { material }, nullptr, nullptr, entityID, selected);
	}

	void SceneRenderer::SubmitModel(const glm::mat4& transform, Ref<Model> model, const std::vector<Ref<MaterialAsset>>& materials, Ref<BoneMatrixList> pose, Ref<BlendShapeWeightList> blendShapeWeights, int entityID, bool selected)
	{
		if (!model)
			return;

		auto& sceneContext = s_Data.ActiveContext->SceneContext;

		glm::mat4 previousTransform;
		if (entityID == -1 || sceneContext->PreviousModelTransforms.find(entityID) == sceneContext->PreviousModelTransforms.end())
			previousTransform = transform;
		else
			previousTransform = sceneContext->PreviousModelTransforms.at(entityID);

		if (entityID != -1)
			sceneContext->CurrentModelTransforms[entityID] = transform;

		s_Data.ModelDrawList.emplace_back(transform, previousTransform, model, materials, pose, blendShapeWeights, entityID, selected);
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

	static void UpdateMaterialShaderDefines(Ref<MaterialAsset> material)
	{
		if (!material)
			return;

		const auto& shaders = material->GetShaders();
		for (const auto& [stage, shader] : shaders)
		{
			if (!shader)
				continue;

			if (stage == MaterialAsset::MaterialRenderStage::VXGI)
			{
				if (s_Data.TakeAtomicFP16Path != shader->GetMacroFlag("TAKE_ATOMIC_FP16_PATH"))
					shader->SetMacro("TAKE_ATOMIC_FP16_PATH", s_Data.TakeAtomicFP16Path);

				if (s_Data.TakeFastGeometryShaderPath != shader->GetMacroFlag("TAKE_FAST_GEOMETRY_SHADER_PATH"))
					shader->SetMacro("TAKE_FAST_GEOMETRY_SHADER_PATH", s_Data.TakeFastGeometryShaderPath);
			}
			else if (material->GetProperties().AlphaBlendMode == MaterialAsset::Translucent && stage == MaterialAsset::MaterialRenderStage::Default)
			{
				if (s_Data.VXGIEnableTranslucentLighting != shader->GetMacroFlag("TRANSLUCENT_VXGI"))
					shader->SetMacro("TRANSLUCENT_VXGI", s_Data.VXGIEnableTranslucentLighting);

				const bool useRandomNoise = s_Data.VXGIConeTracingShader->GetMacroFlag("USE_RANDOM_NOISE");
				if (useRandomNoise != shader->GetMacroFlag("USE_RANDOM_NOISE"))
					shader->SetMacro("USE_RANDOM_NOISE", useRandomNoise);
			}
		}
	}

	static bool IncludeEntity(int id)
	{
		const SceneRendererContext::RenderMaskType type = s_Data.ActiveContext->MaskType;
		if (type == SceneRendererContext::RenderMaskType::None)
			return true;

		const auto& renderMask = s_Data.ActiveContext->RenderMask;
		return (type == SceneRendererContext::RenderMaskType::Exclusive) ? (renderMask.find(id) == renderMask.end()) : (renderMask.find(id) != renderMask.end());
	}

	static void UpdateMaterialShaderDefines()
	{
		for (const auto& model : s_Data.ModelDrawList)
			for (const auto& material : model.Materials)
				UpdateMaterialShaderDefines(material);

		for (const auto& mesh : s_Data.TranslucentMeshDrawList)
			UpdateMaterialShaderDefines(mesh.Material);

		UpdateMaterialShaderDefines(s_Data.DefaultMaterial);
	}

	static void BindMaterialResources(Ref<MaterialAsset> material)
	{
		// Upload all textures in use
		s_Data.MaterialTextureUniformBuffer->SetData(material->GetTextureBuffer());

		// Retrieve and upload material parameter data
		const uint32_t parameterBufferSize = material->GetParameterBufferSize();
		if (parameterBufferSize != 0)
		{
			const Buffer& parameterBuffer = material->GetParameterBuffer();
			s_Data.MaterialParameterUniformBuffer->SetData(parameterBuffer);
		}
	}

	static void DrawModel(Ref<Model> model, const std::vector<Ref<MaterialAsset>>& materials, MaterialAsset::MaterialRenderStage renderStage, const bool drawTranslucent = false, const bool useLODs = false)
	{
		Frustum modelFrustum(s_Data.CameraBuffer.ViewProjection * s_Data.ObjectBuffer.Model);

		// Get the set of meshes to draw for the desired LOD at the given screen size
		float screenSize = 1.0f;

		// TODO: Ensure that all draws use LODs and rather just pass the matrix used to compute orientations (e.g. CSM shadow  matrix)
		if (useLODs)
			screenSize = modelFrustum.GetBoxScreenCoverage(model->GetAABB());

		// Reject degenerate models (who contain sub-pixel detail)
		if (screenSize < 0.0001f)
			return;

		auto& meshes = model->GetMeshes(screenSize);

		for (uint32_t i = 0; i < meshes.size(); i++)
		{
			auto& mesh = meshes[i];

			// Hacky check per submesh to cull on CPU.
			// TODO: Allow this to be part of GPU culling (Would be a massive optimization)
			if (renderStage == MaterialAsset::MaterialRenderStage::Default || renderStage == MaterialAsset::MaterialRenderStage::PreDepth)
			{
				if (!modelFrustum.IsBoxVisible(mesh->GetAABB()))
					continue;
			}

			// Get the material and check if we should draw this object
			Ref<MaterialAsset> material = (i < materials.size() && materials[i] != nullptr && materials[i]->IsLoaded()) ? materials[i] : s_Data.DefaultMaterial;
			const auto& properties = material->GetProperties();

			if (!drawTranslucent && properties.AlphaBlendMode == MaterialAsset::Translucent)
				continue;

			if ((renderStage == MaterialAsset::MaterialRenderStage::Shadow || renderStage == MaterialAsset::MaterialRenderStage::ShadowCSM) && !properties.CastShadows)
				continue;

			if (renderStage == MaterialAsset::MaterialRenderStage::Default)
			{
				// Update the submesh index
				s_Data.SubmeshBuffer.SubmeshIndex = i;
				s_Data.SubmeshUniformBuffer->SetData(&s_Data.SubmeshBuffer, sizeof(SceneRendererData::SubmeshData));
			}

			const bool patches = properties.Tessellation;

			// Only bind relevant material data if we have a defined MaterialRenderStage
			if (renderStage != MaterialAsset::MaterialRenderStage::None)
			{
				// Bind the material shader
				auto& shader = material->GetShader(renderStage);
				(shader ? shader : s_Data.DefaultMaterial->GetShader(renderStage))->Bind();

				BindMaterialResources(material);

				const bool doubleSided = properties.AlphaBlendMode == MaterialAsset::Masked || (properties.TwoSided && (renderStage == MaterialAsset::MaterialRenderStage::Default || renderStage == MaterialAsset::MaterialRenderStage::PreDepth));

				bool isCull;
				if (doubleSided)
				{
					isCull = glIsEnabled(GL_CULL_FACE);
					glDisable(GL_CULL_FACE);
				}

				mesh->Draw(patches);

				if (doubleSided && isCull)
					glEnable(GL_CULL_FACE);
			}
			else
				mesh->Draw(patches);
		}
	}

	static void DrawMesh(Ref<Mesh> mesh, Ref<MaterialAsset> material, MaterialAsset::MaterialRenderStage renderStage)
	{
		if (renderStage != MaterialAsset::MaterialRenderStage::None)
		{
			if (!material)
				material = s_Data.DefaultMaterial;

			const auto& properties = material->GetProperties();

			bool doubleSided = properties.AlphaBlendMode == MaterialAsset::Masked;

			bool isCull;
			if (doubleSided)
			{
				isCull = glIsEnabled(GL_CULL_FACE);
				glDisable(GL_CULL_FACE);
			}

			// Bind the material textures and shaders
			auto& shader = material->GetShader(renderStage);
			(shader ? shader : s_Data.DefaultMaterial->GetShader(renderStage))->Bind();

			BindMaterialResources(material);

			const bool patches = properties.Tessellation;
			mesh->Draw(patches);

			if (doubleSided && isCull)
				glEnable(GL_CULL_FACE);
		}
		else
			mesh->Draw();
	}

	static void DrawParticleSystems(const bool deferred)
	{
		for (auto& particleSystem : s_Data.ParticleSystemList)
		{
			if (!particleSystem.ParticleSystem)
				continue;

			const Ref<MaterialAsset> material = particleSystem.Material;

			if (!IncludeEntity(particleSystem.EntityID) || !material)
				continue;

			const auto& properties = material->GetProperties();

			const bool lit = properties.Lit;
			const bool translucent = properties.AlphaBlendMode == MaterialAsset::AlphaBlendMode::Translucent;

			if (deferred && (!lit || translucent))
				continue;
			
			if (!deferred && (lit && !translucent))
				continue;

			if (IsFirstDraw())
			{
				s_Data.ObjectBuffer.Model = particleSystem.Transform;
				s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(glm::mat4));
				particleSystem.ParticleSystem->Update();
			}

			// Always update entity ID
			s_Data.ObjectBuffer.EntityID = particleSystem.EntityID;
			s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer.EntityID, sizeof(int), sizeof(glm::mat4) * 4);

			const uint32_t count = particleSystem.ParticleSystem->GetParticleCount();
			if (count != 0)
			{
				material->GetShader(MaterialAsset::MaterialRenderStage::Default)->Bind();
				BindMaterialResources(material);

				particleSystem.ParticleSystem->Bind();
				RenderCommand::DrawIndexed(s_Data.BatchedQuadVertexArray, count * 6);
			}
		}
	}

	static void SetupAnimationPlayerData(Ref<BoneMatrixList> pose)
	{
		s_Data.ObjectBuffer.Animated = (bool)pose;

		if (!pose)
			return;

		memcpy(s_Data.AnimationBuffer.FinalBonesMatrices, pose->data(), pose->size() * sizeof(glm::mat4));
		s_Data.AnimationUniformBuffer->SetData(&s_Data.AnimationBuffer, sizeof(SceneRendererData::AnimationBuffer));
	}

	static void SetupBlendShapes(Ref<Model> model, Ref<BlendShapeWeightList> blendShapeWeights)
	{
		size_t blendShapeIndex = 0;

		if (blendShapeWeights)
		{
			for (const auto& weight : (*blendShapeWeights))
			{
				s_Data.BlendShapeWeightBuffer.BlendShapeWeights[blendShapeIndex] = weight;
				blendShapeIndex++;
			}
		}

		// Update and upload weight uniform buffer
		s_Data.BlendShapeWeightBuffer.BlendShapeCount = blendShapeIndex;
		s_Data.BlendShapeWeightBuffer.BlendShapeVertexCount = model->GetVertexCount();
		s_Data.BlendShapeWeightUniformBuffer->SetData(&s_Data.BlendShapeWeightBuffer, 4 * sizeof(uint32_t) + sizeof(float) * s_Data.BlendShapeWeightBuffer.BlendShapeCount);

		// Bind specified model's blend shape offset buffer
		if (blendShapeIndex > 0)
			model->GetBlendShapeBuffer()->Bind(RendererConstants::Buffers::BlendShapeComponents);
	}

	static bool CompareMeshDistance(const SceneRendererData::MeshData& a, const SceneRendererData::MeshData& b)
	{
		float distanceA = glm::distance(a.Transform[3], s_Data.CameraBuffer.ViewPosition);
		float distanceB = glm::distance(b.Transform[3], s_Data.CameraBuffer.ViewPosition);
		return distanceA > distanceB; // Sort in descending order of distance
	}

	static void DrawGrass()
	{
		// Draw Grass
		static Ref<VertexArray> s_GrassVertexArray = nullptr;
		if (!s_GrassVertexArray)
		{
			// High LOD (15 verticies)
			const glm::vec3 bladeBuffer[15] = {
				{ -0.036f,  0.0,  0.0f },
				{  0.036f,  0.0,  0.0f },

				{ -0.035f,  0.25,  0.0f },
				{  0.035f,  0.25,  0.0f },

				{ -0.0325f,  0.4,  0.0f },
				{  0.0325f,  0.4,  0.0f },

				{ -0.03f,  0.6,  0.0f },
				{  0.03f,  0.6,  0.0f },

				{ -0.0275f,  0.7,  0.0f },
				{  0.0275f,  0.7,  0.0f },

				{ -0.0225f,  0.8,  0.0f },
				{  0.0225f,  0.8,  0.0f },

				{ -0.01f,  0.9,  0.0f },
				{  0.01f,  0.9,  0.0f },


				{ 0.0f,  1.0f, 0.0f } // Head
			};

			Ref<VertexBuffer> grassVertexBuffer;

			// Grass Blade Vertex Buffer
			s_GrassVertexArray = VertexArray::Create();

			grassVertexBuffer = VertexBuffer::Create(sizeof(bladeBuffer));
			grassVertexBuffer->SetLayout({
				{ ShaderDataType::Float3, "a_Position" }
				});
			s_GrassVertexArray->AddVertexBuffer(grassVertexBuffer);

			uint32_t grassIndices[45] = {
				0,  1,  2,  1,  2,  3,
				2,  3,  4,  3,  4,  5,
				4,  5,  6,  5,  6,  7,
				6,  7,  8,  7,  8,  9,
				8,  9,  10, 9,  10, 11,
				10, 11, 12, 11, 12, 13,
				12, 13, 14, 13, 14, 15,
				14, 15, 16
			};

			Ref<IndexBuffer> grassIB = IndexBuffer::Create(grassIndices, 45);
			s_GrassVertexArray->SetIndexBuffer(grassIB);
			grassVertexBuffer->SetData(&bladeBuffer, sizeof(bladeBuffer));
		}

		glDisable(GL_CULL_FACE);
		static Ref<Texture2D> s_PerlinNoise = Texture2D::Create("Resources/Textures/PerlinNoise.png");
		s_PerlinNoise->Bind(0);
		Renderer::GetShaderLibrary()->Get("Renderer3D_Grass")->Bind();
		RenderCommand::DrawIndexedInstanced(s_GrassVertexArray, 45, 10000);
		glEnable(GL_CULL_FACE);
	}

	static bool s_ReloadRaytracing = false;

	//static float BVHNodeCost(glm::vec3 size, int numTriangles)
	//{
	//	float halfArea = size.x * size.y + size.x * size.z + size.y * size.z;
	//	return halfArea * numTriangles;
	//}

	static constexpr float TriangleIntersectionCost = 1.1f;
	static constexpr float TraversalCost = 1.0f;
	static constexpr uint32_t MaxLeafTriangleCount = 8;

	static float BVHCostInternalNode(float probabilityHitLeftChild, float probabilityHitRightChild, float costLeftChild, float costRightChild, float traversalCost)
	{
		return traversalCost + (probabilityHitLeftChild * costLeftChild + probabilityHitRightChild * costRightChild);
	}

	static float BVHCostLeafNode(int numTriangles, float triangleCost)
	{
		return numTriangles * triangleCost;
	}

	static float EvaluateSplit(const SceneRendererData::BVHNode& node, const uint32_t splitAxis, const float splitPos)
	{
		AABB boundsA, boundsB;
		uint32_t countA = 0, countB = 0;

		for (uint32_t i = node.TriangleIndex; i < node.TriangleIndex + node.TriangleCount; i++)
		{
			const auto& triangle = s_Data.RayTriangles[i];
			const float centerOnSplitAxis = (triangle.Position[0][splitAxis] + triangle.Position[1][splitAxis] + triangle.Position[2][splitAxis]) / 3.0f;

			if (centerOnSplitAxis < splitPos)
			{
				boundsA.Extend(triangle.Position[0]);
				boundsA.Extend(triangle.Position[1]);
				boundsA.Extend(triangle.Position[2]);
				countA++;
			}
			else
			{
				boundsB.Extend(triangle.Position[0]);
				boundsB.Extend(triangle.Position[1]);
				boundsB.Extend(triangle.Position[2]);
				countB++;
			}
		}

		// The cost of a split is equal to the sum of the costs of the two new child nodes
		// return BVHNodeCost(boundsA.GetSize(), countA) + BVHNodeCost(boundsB.GetSize(), countB);

		const float parentArea = AABB(node.Min, node.Max).GetHalfArea();
		return BVHCostInternalNode(
			boundsA.GetHalfArea() / parentArea,
			boundsB.GetHalfArea() / parentArea,
			BVHCostLeafNode(countA, TriangleIntersectionCost),
			BVHCostLeafNode(countB, TriangleIntersectionCost),
			TraversalCost
		);
	}

	static void ExpandBounds(glm::vec3& min, glm::vec3& max, const glm::vec3& point)
	{
		min = glm::min(min, point);
		max = glm::max(max, point);
	}

	static void ChooseSplit(const SceneRendererData::BVHNode& node, uint32_t& splitAxis, float& splitPosition, float& cost)
	{
		// More is better, but slower...
		const uint32_t TestCountPerAxis = 16;
		
		float bestCost = std::numeric_limits<float>::infinity();
		float bestSplitPosition = 0.0f;
		int bestSplitAxis = 0;

		for (uint32_t axis = 0; axis < 3; axis++)
		{
			float boundsStart = node.Min[axis];
			float boundsEnd = node.Max[axis];

			for (uint32_t i = 0; i < TestCountPerAxis; i++)
			{
				// Fraction along the current axis of the bounds we want to split
				float splitT = (i + 1) / (float)(TestCountPerAxis + 1);

				// Linear interpolation along bounds to get actual split position
				float splitPos = boundsStart + (boundsEnd - boundsStart) * splitT;
				float cost = EvaluateSplit(node, axis, splitPos);
				
				if (cost < bestCost)
				{
					bestCost = cost;
					bestSplitPosition = splitPos;
					bestSplitAxis = axis;
				}
			}
		}

		splitAxis = bestSplitAxis;
		splitPosition = bestSplitPosition;
		cost = bestCost;
	}

	static void Split(uint32_t parentIndex, uint32_t depth, uint32_t& nodeCount)
	{
		if (depth == RendererConstants::MaxPathTraceBVHDepth)
			return;

		SceneRendererData::BVHNode& parent = s_Data.BVHNodes[parentIndex];

		if (parent.TriangleCount <= 1)
			return;

		// Choose split axis and position
		uint32_t splitAxis;
		float splitPosition, splitCost;
		ChooseSplit(parent, splitAxis, splitPosition, splitCost);

		// Stop splitting if it doesn't improve the cost
		// const float parentCost = BVHCostLeafNode(parent.Max - parent.Min, parent.TriangleCount);
		const float parentCost = BVHCostLeafNode(parent.TriangleCount, TriangleIntersectionCost);
		if (splitCost >= parentCost)
		{
			if (parent.TriangleCount <= MaxLeafTriangleCount)
				return;

			// Use a median split (along largest axis). Note: We use the middle element as the split NOT the middle of the bounding box
			const glm::vec3 size = parent.Max - parent.Min;
			splitAxis = (size.y > size.x) ? 1 : 0;
			splitAxis = (size.z > size[splitAxis]) ? 2 : splitAxis;

			const uint32_t start = parent.TriangleIndex;
			const uint32_t end = start + parent.TriangleCount;
			std::sort(s_Data.RayTriangles.begin() + start, s_Data.RayTriangles.begin() + end, [splitAxis](const SceneRendererData::RayTriangle& a, const SceneRendererData::RayTriangle& b)
			{
				const float posOnSplitAxisA = (a.Position[0][splitAxis] + a.Position[1][splitAxis] + a.Position[3][splitAxis]) / 3.0f;
				const float posOnSplitAxisB = (b.Position[0][splitAxis] + b.Position[1][splitAxis] + b.Position[3][splitAxis]) / 3.0f;

				return posOnSplitAxisA < posOnSplitAxisB;
			});

			const uint32_t pivot = (start + end) / 2;
			splitPosition = (s_Data.RayTriangles[pivot].Position[0][splitAxis] + s_Data.RayTriangles[pivot].Position[1][splitAxis] + s_Data.RayTriangles[pivot].Position[2][splitAxis]) / 3.0f;
		}

		// Create child nodes
		SceneRendererData::BVHNode childA;
		SceneRendererData::BVHNode childB;
		childA.TriangleIndex = parent.TriangleIndex;
		childB.TriangleIndex = parent.TriangleIndex;

		for (uint32_t i = parent.TriangleIndex; i < parent.TriangleIndex + parent.TriangleCount; i++)
		{
			const glm::vec3 center = (s_Data.RayTriangles[i].Position[0] + s_Data.RayTriangles[i].Position[1] + s_Data.RayTriangles[i].Position[2]) / 3.0f;
			const bool isSideA = center[splitAxis] < splitPosition;
			SceneRendererData::BVHNode& child = isSideA ? childA : childB;
			ExpandBounds(child.Min, child.Max, s_Data.RayTriangles[i].Position[0]);
			ExpandBounds(child.Min, child.Max, s_Data.RayTriangles[i].Position[1]);
			ExpandBounds(child.Min, child.Max, s_Data.RayTriangles[i].Position[2]);
			child.TriangleCount++;

			if (isSideA)
			{
				// Ensure that the triangles of each child node are grouped together so they can be referenced by an index and count
				int swap = child.TriangleIndex + child.TriangleCount - 1;
				std::swap(s_Data.RayTriangles[i], s_Data.RayTriangles[swap]);
				childB.TriangleIndex++;
			}

		}

		if (childA.TriangleCount > 0 && childB.TriangleCount > 0)
		{
			parent.ChildIndex = s_Data.BVHNodes.size();

			const uint32_t childAIndex = s_Data.BVHNodes.size();
			s_Data.BVHNodes.push_back(childA);
			const uint32_t childBIndex = s_Data.BVHNodes.size();
			s_Data.BVHNodes.push_back(childB);

			Split(childAIndex, depth + 1, nodeCount);
			Split(childBIndex, depth + 1, nodeCount);

			nodeCount += 2;
		}
	}

	static void BVHUploadModel(Ref<Model> model)
	{
		if (s_Data.BVHModelNodeMap.find(model->Handle) != s_Data.BVHModelNodeMap.end())
			return;

		const uint32_t uploadNodeIndex = s_Data.BVHNodes.size();
		const uint32_t uploadTriangleIndex = s_Data.RayTriangles.size();

		const auto& submeshes = model->GetMeshes();
		for (const auto& submesh : submeshes)
		{
			s_Data.BVHModelNodeMap[model->Handle].push_back(s_Data.BVHNodes.size());

			const uint32_t rootIndex = s_Data.BVHNodes.size();
			SceneRendererData::BVHNode& rootNode = s_Data.BVHNodes.emplace_back(SceneRendererData::BVHNode());
			rootNode.TriangleIndex = s_Data.RayTriangles.size();

			const auto& verticies = submesh->GetVerticies();
			const auto& indicies = submesh->GetIndicies();

			for (uint32_t i = 0; i < indicies.size(); i += 3)
			{
				SceneRendererData::RayTriangle& triangle = s_Data.RayTriangles.emplace_back(SceneRendererData::RayTriangle());

				for (uint32_t j = 0; j < 3; j++)
				{
					const auto& vertex = verticies[indicies[i + j]];
					triangle.Position[j] = glm::vec4(vertex.Position, vertex.TexCoords.x);
					triangle.Normal[j] = glm::vec4(vertex.Normal, vertex.TexCoords.y);
				}
			}

			const AABB& aabb = submesh->GetAABB();
			rootNode.Min = aabb.Min;
			rootNode.Max = aabb.Max;
			rootNode.TriangleCount = s_Data.RayTriangles.size() - rootNode.TriangleIndex;

			uint32_t nodeCount = 1;
			Split(rootIndex, 0, nodeCount);
		}

		const uint32_t uploadNodeCount = s_Data.BVHNodes.size() - uploadNodeIndex;
		const uint32_t uploadTriangleCount = s_Data.RayTriangles.size() - uploadTriangleIndex;

		const uint32_t triangleCount = s_Data.RayTriangles.size();
		s_Data.PathTraceGeometrySSBO->SetData(&triangleCount, sizeof(uint32_t));

		const void* triangleUploadData = s_Data.RayTriangles.data() + uploadTriangleIndex;
		const auto triangleUploadOffset = sizeof(uint32_t) * 4 + sizeof(SceneRendererData::RayTriangle) * uploadTriangleIndex;
		const auto triangleUploadSize = sizeof(SceneRendererData::RayTriangle) * uploadTriangleCount;
		s_Data.PathTraceGeometrySSBO->SetData(triangleUploadData, triangleUploadSize, triangleUploadOffset);

		const void* bvhUploadData = s_Data.BVHNodes.data() + uploadNodeIndex;
		const auto bvhUploadOffset = sizeof(SceneRendererData::BVHNode) * uploadNodeIndex;
		const auto bvhUploadSize = sizeof(SceneRendererData::BVHNode) * uploadNodeCount;
		s_Data.PathTraceBVHSSBO->SetData(bvhUploadData, bvhUploadSize, bvhUploadOffset);

		DY_CORE_INFO("Regenerated BLAS for model {} with {} nodes and {} triangles", model->Handle, uploadNodeCount, uploadTriangleCount);
	}

	static void StringReplaceFirst(std::string& string, const std::string& from, const std::string& to)
	{
		std::size_t pos = string.find(from);
		if (pos != std::string::npos)
			string.replace(pos, from.length(), to);
	}

	static void InvalidateBVHMaterial()
	{
		DY_CORE_TRACE("Invalidating path tracing material ubershader. Commencing rebuild...");

		s_Data.ActiveContext->AccumulationFrames = 0;

		// Iterate over all materials and add their respective shader code at the assigned index
		std::string materialHeader;
		std::string materialPropertiesFunction;

		s_Data.PathTraceMaterialTextures.clear();

		std::unordered_set<AssetHandle> materialsAdded;
		for (const auto& model : s_Data.ModelDrawList)
		{
			for (const auto& materialAsset : model.Materials)
			{
				if (!materialAsset)
					continue;

				if (materialsAdded.find(materialAsset->Handle) != materialsAdded.end())
					continue;

				materialsAdded.insert(materialAsset->Handle);

				const std::string& pathTraceSource = materialAsset->GetPathTraceSource();

				if (pathTraceSource.empty())
					continue;

				if (s_Data.BVHMaterialMap.find(materialAsset->Handle) == s_Data.BVHMaterialMap.end())
					s_Data.BVHMaterialMap[materialAsset->Handle] = s_Data.NextBVHMaterialIndex++;

				const uint32_t materialIndex = s_Data.BVHMaterialMap[materialAsset->Handle];

				materialPropertiesFunction += fmt::format("if (material == {})", materialIndex);
				materialPropertiesFunction += "\n{\n";
				materialPropertiesFunction += pathTraceSource;
				materialPropertiesFunction += "return;\n}";

				const auto& textures = materialAsset->GetTextures();
				for (const auto& [id, texture] : textures)
				{
					materialHeader += fmt::format("sampler2D u_UserTextureSampler_{};", id) + "\n";
					s_Data.PathTraceMaterialTextures.push_back(texture);
				}
			}
		}

		if (!materialHeader.empty())
			materialHeader = "layout(std140, binding = USER_TEXTURE_SAMPLER_BUFFER_BINDING) uniform UserMaterialTextureBuffer {" + materialHeader + "};";

		// Read in the shader template and replace the material properties/header
		std::ifstream pathTraceTemplate("Resources/Shaders/Path Tracing/Renderer3D_PathTrace.glsl");
		std::string pathTraceShaderSource((std::istreambuf_iterator<char>(pathTraceTemplate)), std::istreambuf_iterator<char>());
		pathTraceTemplate.close();

		StringReplaceFirst(pathTraceShaderSource, "/* MATERIAL_HEADER */", materialHeader);
		StringReplaceFirst(pathTraceShaderSource, "/* MATERIAL_PROPERTIES */", materialPropertiesFunction);

		s_Data.PathTraceMaterialShader = Shader::Create("User_PathTraceMaterial", pathTraceShaderSource, false);

		if (!s_Data.PathTraceMaterialShader->IsLoaded())
		{
			DY_CORE_ERROR("Path Trace Material Shader Build Failed. Dumping Shader Source...");
			DY_CORE_TRACE(pathTraceShaderSource);
		}
	}

	static void PrintBVHNodeTree(uint32_t rootIndex, int depth)
	{
		std::string formatting = std::string(depth, '\t') + "Node {} - Child: {}, TriCount: {}";
		DY_CORE_TRACE(formatting, rootIndex, s_Data.BVHNodes[rootIndex].ChildIndex, s_Data.BVHNodes[rootIndex].TriangleCount);
		
		if (s_Data.BVHNodes[rootIndex].ChildIndex != 0)
		{
			PrintBVHNodeTree(s_Data.BVHNodes[rootIndex].ChildIndex + 0, depth + 1);
			PrintBVHNodeTree(s_Data.BVHNodes[rootIndex].ChildIndex + 1, depth + 1);
		}
	}

	static void PrintBVHBuildInfo()
	{
		DY_CORE_INFO("BVH Build Info");

		size_t modelIndex = 0;
		for (const auto& bvhModel : s_Data.BVHModels)
		{
			DY_CORE_INFO("Model {}", modelIndex);

			for (size_t bvhSubmesh = bvhModel.SubmeshIndex; bvhSubmesh < bvhModel.SubmeshIndex + bvhModel.SubmeshCount; bvhSubmesh++)
			{
				DY_CORE_INFO("\tSubmesh {}", bvhSubmesh);
				PrintBVHNodeTree(s_Data.BVHSubmeshes[bvhSubmesh].NodeIndex, 2);
			}

			modelIndex++;
		}
	}

	static size_t HashModelList(const std::vector<SceneRendererData::ModelData>& modelList)
	{
		size_t hash = 0;
		for (const auto& model : modelList)
		{
			size_t ptrHash = reinterpret_cast<std::size_t>(model.Model.get());
			size_t idHash = std::hash<int>()(model.EntityID);
			
			const float* data = glm::value_ptr(model.Transform);
			std::size_t matHash = 0;
			for (int i = 0; i < 16; ++i)
			{
				size_t floatHash = 0;
				std::memcpy(&floatHash, &data[i], sizeof(float));
				matHash ^= floatHash + 0x9e3779b9 + (matHash << 6) + (matHash >> 2);
			}
			
			size_t elementHash = ptrHash ^ (matHash + 0x9e3779b9 + (ptrHash << 6) + (ptrHash >> 2)) ^ (idHash + 0x9e3779b9 + (matHash << 6) + (matHash >> 2));
			
			hash ^= elementHash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		}
		
		return hash;
	}

	static size_t HashMaterialList(const std::vector<SceneRendererData::ModelData>& modelList)
	{
		size_t hash = 0;
		std::hash<std::string> hasher;

		for (const auto& model : modelList)
		{
			hash ^= model.EntityID + 0x9e3779b9 + (hash << 6) + (hash >> 2);

			size_t materialIndex = 0;
			for (const auto& material : model.Materials)
			{
				const size_t materialHash = material ? hasher(material->GetPathTraceSource()) : 0;
				hash ^= materialHash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
				hash ^= materialIndex +0x9e3779b9 + (hash << 6) + (hash >> 2);

				materialIndex++;
			}
		}

		return hash;
	}

	static void UploadVXGIUniformData()
	{
		for (uint32_t cascadeIndex = 0; cascadeIndex < RendererConstants::VXGICascadeCount; cascadeIndex++)
		{
			// Upload the grid data for each cascade
			const auto& cascade = s_Data.VXGICascades[cascadeIndex];

			// Ensure that the VXGI grid is aligned to a grid with intervals one voxel length apart.
			// This is an effective way to avoid flickering when moving the camera as geometry gets voxelized at the same relative point in different voxels.
			const float voxelSize = cascade.VXGIGridSize / cascade.VXGIGridResolution;
			glm::vec3 gridCenter = glm::vec3(s_Data.CameraBuffer.ViewPosition);
			gridCenter = glm::round(gridCenter / voxelSize) * voxelSize;

			const glm::vec3 halfExtent = glm::vec3(cascade.VXGIGridSize * 0.5f);
			const glm::vec3 gridOffset = s_Data.CameraBuffer.InverseView * glm::vec4(cascade.VXGIGridOffset, 0.0f);
			s_Data.VXGIBuffer.GridResolution[cascadeIndex] = cascade.VXGIGridResolution;
			s_Data.VXGIBuffer.GridMin[cascadeIndex] = glm::vec4(gridCenter + gridOffset - halfExtent, 1.0f);
			s_Data.VXGIBuffer.GridMax[cascadeIndex] = glm::vec4(gridCenter + gridOffset + halfExtent, 1.0f);
		}

		s_Data.VXGIBuffer.StepMultiplier = s_Data.VXGIGridVisualization ? s_Data.DebugStepMultiplier : s_Data.StepMultiplier;

		s_Data.VXGIUniformBuffer->SetData(&s_Data.VXGIBuffer, sizeof(SceneRendererData::VXGIData));
	}

	static Ref<Texture2D> s_OutputTexture = nullptr;

	void SceneRenderer::RenderScene()
	{
		// Upload G-Buffer and Fixed Lighting Texture bindless handles
		auto& gBuffer = s_Data.GBufferBuffer;
		gBuffer.Albedo = s_Data.ActiveContext->ActiveFramebuffer->GetColorHandle(0);
		gBuffer.EntityID = s_Data.ActiveContext->ActiveFramebuffer->GetColorHandle(1);
		gBuffer.Normal = s_Data.ActiveContext->ActiveFramebuffer->GetColorHandle(2);
		gBuffer.Emissive = s_Data.ActiveContext->ActiveFramebuffer->GetColorHandle(3);
		gBuffer.Roughness_Metallic_Specular_AO = s_Data.ActiveContext->ActiveFramebuffer->GetColorHandle(4);
		gBuffer.SubmeshIndex = s_Data.ActiveContext->ActiveFramebuffer->GetColorHandle(5);
		gBuffer.Velocity = s_Data.ActiveContext->ActiveFramebuffer->GetColorHandle(6);
		gBuffer.Depth = s_Data.ActiveContext->ActiveFramebuffer->GetDepthHandle();

		auto& sceneContext = s_Data.ActiveContext->SceneContext;
		gBuffer.EnvironmentCubemap = sceneContext->EnvironmentCubemap->GetHandle();		// IBL Environment Cubemap
		gBuffer.IrradianceMap = sceneContext->IrradianceMap->GetHandle();				// IBL Irradiance Cubemap
		gBuffer.PrefilterMap = sceneContext->PrefilterMap->GetHandle();					// IBL Prefilter Cubemap
		gBuffer.brdfLUTTexture = sceneContext->brdfLUTTexture->GetHandle();				// IBL brdf LUT Texture
		gBuffer.FlowMapCubemap = sceneContext->FlowMapCubemap->GetHandle();				// IBL Cubemap Flowmap

		if (s_Data.EnableVXGI)
		{
			for (uint32_t cascadeIndex = 0; cascadeIndex < RendererConstants::VXGICascadeCount; cascadeIndex++)
				gBuffer.VXGICascades[cascadeIndex].Cascade = s_Data.VXGICascades[cascadeIndex].VXGIGrid->GetHandle();
		}

		s_Data.GBufferUniformBuffer->SetData(&gBuffer, sizeof(SceneRendererData::GBufferData));

		// Pass in the current post processing data
		s_Data.PostProcessingBuffer.VisualizationMode = (int)s_Data.ActiveContext->VisualizationMode;
		s_Data.PostProcessingBuffer.UsingVXGI = s_Data.EnableVXGI;
		s_Data.PostProcessingBuffer.UsingLUT = (bool)s_Data.ActiveContext->LUTTexture;
		s_Data.PostProcessingUniformBuffer->SetData(&s_Data.PostProcessingBuffer, sizeof(SceneRendererData::PostProcessingData));

		// Update all materials for submission
		if (IsFirstDraw())
			UpdateMaterialShaderDefines();

		std::vector<uint32_t> visibleModels;

		{
			// Upload data for analysis (Note: This should only be done once per frame)
			std::vector<SceneRendererData::MeshInstance> meshInstances;
			meshInstances.reserve(s_Data.ModelDrawList.size());

			std::vector<uint64_t> sdfList;
			std::unordered_map<Ref<Texture3D>, uint32_t> sdfIndexMap;

			// Upload mesh instance data
			for (const auto& model : s_Data.ModelDrawList)
			{
				const Ref<Texture3D> sdf = model.Model->GetSDF();

				if (sdf && sdfIndexMap.find(sdf) == sdfIndexMap.end())
				{
					sdfIndexMap[sdf] = sdfList.size();
					sdfList.push_back(sdf->GetHandle());
				}

				const AABB& aabb = model.Model->GetAABB();
				meshInstances.emplace_back(model.Transform, glm::inverse(model.Transform), aabb.Min, aabb.Max, sdf ? sdfIndexMap.at(sdf) : -1);
			}

			// Upload Mesh SDF Texture Handles
			s_Data.MeshSDFSSBO->SetData(sdfList.data(), sizeof(uint64_t) * sdfList.size());

			// GPU Culling
			const uint32_t size = meshInstances.size();
			if (size > 0)
			{
				uint32_t visibleCount = 0;
				s_Data.MeshInstanceSSBO->SetData(&size, sizeof(uint32_t));
				s_Data.MeshInstanceSSBO->SetData(meshInstances.data(), meshInstances.size() * sizeof(SceneRendererData::MeshInstance), sizeof(uint32_t) * 4);
				s_Data.MeshVisibilitySSBO->SetData(&visibleCount, sizeof(uint32_t));

				// Run Culling Algorithm (Done when culling information is required)
				s_Data.CullingComputeShader->Dispatch(INT_CEILING_DIVISION(size, 32), 1, 1);

				// Query Results
				uint32_t* data = (uint32_t*)s_Data.MeshVisibilitySSBO->MapBuffer(READ_ONLY);
				visibleCount = data[0];
				visibleModels = std::vector<uint32_t>(data + 1, data + 1 + visibleCount);
				s_Data.MeshVisibilitySSBO->UnmapBuffer();
			}
		}

		if (s_Data.ActiveContext->VisualizationMode == SceneRendererContext::RendererVisualizationMode::PathTraced)
		{
			if (s_ReloadRaytracing)
			{
				s_ReloadRaytracing = false;

				s_Data.RayTriangles.clear();
				s_Data.BVHNodes.clear();
				s_Data.BVHModels.clear();

				s_Data.BVHModelNodeMap.clear();
				s_Data.BVHMaterialMap.clear();

				s_Data.ActiveContext->PreviousModelListHash = -1;
				s_Data.ActiveContext->PreviousMaterialListHash = -1;
			}

			// Upload model info (only if the model list has changed)
			const size_t modelHash = HashModelList(s_Data.ModelDrawList);
			const size_t materialHash = HashMaterialList(s_Data.ModelDrawList);
			
			const bool invalidateModels = modelHash != s_Data.ActiveContext->PreviousModelListHash;
			const bool invalidateMaterials = materialHash != s_Data.ActiveContext->PreviousMaterialListHash;

			// Check if the BVH compute shader needs to be regenerated due to new materials
			if (invalidateMaterials)
			{
				s_Data.ActiveContext->PreviousMaterialListHash = materialHash;
				InvalidateBVHMaterial();
			}

			// Check if we need to upload model info again (due to an altered material index or object transform/model asset)
			if (invalidateModels || invalidateMaterials)
			{	
				s_Data.ActiveContext->AccumulationFrames = 0;
				s_Data.ActiveContext->PreviousModelListHash = modelHash;
				
				s_Data.BVHModels.clear();
				s_Data.BVHSubmeshes.clear();
				
				for (const auto& model : s_Data.ModelDrawList)
				{
					BVHUploadModel(model.Model);

					SceneRendererData::BVHModel bvhModel;
					bvhModel.Model = model.Transform;
					bvhModel.InverseModel = glm::inverse(model.Transform);
					bvhModel.EntityID = model.EntityID;
					bvhModel.SubmeshIndex = s_Data.BVHSubmeshes.size();

					size_t submeshIndex = 0;
					const auto& submeshNodeIndices = s_Data.BVHModelNodeMap[model.Model->Handle];
					const auto& submeshes = model.Model->GetMeshes();
					for (const auto& submesh : submeshes)
					{
						const AABB& aabb = submesh->GetAABB();

						SceneRendererData::BVHSubmesh bvhSubmesh;
						bvhSubmesh.Min = aabb.Min;
						bvhSubmesh.Max = aabb.Max;
						bvhSubmesh.NodeIndex = submeshNodeIndices[submeshIndex];
						bvhSubmesh.Material = model.Materials[submeshIndex] ? s_Data.BVHMaterialMap[model.Materials[submeshIndex]->Handle] : 0;

						s_Data.BVHSubmeshes.push_back(bvhSubmesh);

						submeshIndex++;
					}

					bvhModel.SubmeshCount = s_Data.BVHSubmeshes.size() - bvhModel.SubmeshIndex;

					s_Data.BVHModels.push_back(bvhModel);
				}

				s_Data.PathTraceSubmeshSSBO->SetData(s_Data.BVHSubmeshes.data(), s_Data.BVHSubmeshes.size() * sizeof(SceneRendererData::BVHSubmesh));

				const uint32_t modelCount = s_Data.BVHModels.size();
				s_Data.PathTraceModelSSBO->SetData(&modelCount, sizeof(uint32_t));
				s_Data.PathTraceModelSSBO->SetData(s_Data.BVHModels.data(), s_Data.BVHModels.size() * sizeof(SceneRendererData::BVHModel), sizeof(uint32_t) * 4);
			}

			// Prepare for path tracing binding and execution...
			
			// Upload the accumulated frames value
			s_Data.PathTraceGeometrySSBO->SetData(&s_Data.ActiveContext->AccumulationFrames, sizeof(uint32_t), sizeof(uint32_t));

			// Upload all texture handles required by materials
			std::vector<uint64_t> pathTraceMaterialTextureBuffer;
			pathTraceMaterialTextureBuffer.reserve(s_Data.PathTraceMaterialTextures.size());

			for (const auto& texture : s_Data.PathTraceMaterialTextures)
				pathTraceMaterialTextureBuffer.push_back(texture->GetHandle());

			s_Data.MaterialTextureUniformBuffer->SetData(pathTraceMaterialTextureBuffer.data(), pathTraceMaterialTextureBuffer.size() * sizeof(uint64_t));

			// Bind Resources and dispatch the path tracing shader
			s_Data.ActiveContext->PathTracingFramebuffer->BindColorTexture(0, 0); // Color
			s_Data.ActiveContext->PathTracingFramebuffer->BindColorTexture(1, 1); // Entity ID
			s_Data.ActiveContext->PathTracingFramebuffer->BindColorTexture(2, 2); // Color
			s_Data.ActiveContext->PathTracingFramebuffer->BindColorTexture(3, 3); // Depth
			s_Data.ActiveContext->ActiveFramebuffer->BindColorTexture(4, 5);
			(s_Data.PathTraceMaterialShader ? s_Data.PathTraceMaterialShader : s_Data.PathTraceShader)->Dispatch(INT_CEILING_DIVISION(s_Data.ActiveContext->ActiveWidth, RendererConstants::RaytraceComputeLocalSize), INT_CEILING_DIVISION(s_Data.ActiveContext->ActiveHeight, RendererConstants::RaytraceComputeLocalSize), 1);
			
			// Draw outline of selected object (Path Tracing Pass) using default rasterisation pass
			s_Data.ActiveContext->OutlineFramebuffer->Bind();
			RenderCommand::Clear();
			bool isSelected = false;
			for (const auto& model : s_Data.ModelDrawList)
			{
				if (model.Selected)
				{
					s_Data.ObjectBuffer.Model = model.Transform;
					s_Data.ObjectBuffer.PreviousModel = model.PreviousTransform;
					s_Data.ObjectBuffer.ModelInverse = glm::transpose(glm::inverse(model.Transform));
					s_Data.ObjectBuffer.Normal = glm::transpose(glm::inverse(glm::mat3(model.Transform)));
					s_Data.ObjectBuffer.EntityID = model.EntityID;
					s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(SceneRendererData::ObjectBuffer));
			
					isSelected = true;
					DrawModel(model.Model, model.Materials, MaterialAsset::MaterialRenderStage::PreDepth);
				}
			}
			s_Data.ActiveContext->OutlineFramebuffer->Unbind();

			// Accumulate the previous two frames and store a copy of the result
			s_Data.ActiveContext->PathTracingAccumulateFramebuffers[1]->Bind();					// Accumulated Frame (Write)
			s_Data.ActiveContext->PathTracingAccumulateFramebuffers[0]->BindColorSampler(0, 0);	// Accumulated Frame (Read)
			s_Data.ActiveContext->PathTracingFramebuffer->BindColorSampler(1, 0);				// Current Frame Color
			RenderQuad(s_Data.PathTraceAccumulateShader);

			s_Data.ActiveContext->AccumulationFrames++;
			
			// Denoise the frame (use buffer 1 as the write frame)
			s_Data.ActiveContext->PathTracingAccumulateFramebuffers[0]->Bind(); // index 0 is no longer in use
			s_Data.ActiveContext->PathTracingAccumulateFramebuffers[1]->BindColorSampler(0, 0);
			s_Data.ActiveContext->PathTracingFramebuffer->BindColorSampler(1, 2); // Normal
			RenderQuad(s_Data.PathTraceDenoiseShader);

			// NOTE: We repurpose the deferred lighting framebuffer here!
			// TODO: We probably shouldn't do this!
			s_Data.ActiveContext->DeferredLightingFramebuffer->Bind();
			s_Data.ActiveContext->PathTracingAccumulateFramebuffers[0]->BindColorSampler(0, 0);	// Accumulated Frame 
			RenderQuad(s_Data.FinalCompositingFXShader);

			// Copy the final path traced result to the framebuffer
			s_Data.ActiveContext->ActiveFramebuffer->Bind();

			// Draw the depth texture
			s_Data.ActiveContext->PathTracingFramebuffer->BindColorSampler(0, 3);
			Renderer::GetShaderLibrary()->Get("Renderer3D_DrawDepth")->Bind();
			RenderCommand::DrawIndexed(s_Data.QuadVertexArray, 6);
			
			// Draw the final color
			s_Data.ActiveContext->DeferredLightingFramebuffer->BindColorSampler(0, 0);
			s_Data.ActiveContext->PathTracingFramebuffer->BindColorSampler(1, 1);	// EntityID
			RenderQuad(s_Data.FinalCompositingShader);

			// Swap the path tracing accumulation frames (so read becomes write next frame)
			std::swap(s_Data.ActiveContext->PathTracingAccumulateFramebuffers[0], s_Data.ActiveContext->PathTracingAccumulateFramebuffers[1]);

			// Render outline over the top
			if (isSelected)
			{
				s_Data.ActiveContext->OutlineFramebuffer->BindDepthSampler(0);
				RenderQuad(s_Data.OutlineShader);
			}

			// Ensure the active framebuffer is bound before exiting
			s_Data.ActiveContext->ActiveFramebuffer->Bind();
		}
		else if (s_Data.EditorDrawShaderOverride || s_Data.ActiveContext->VisualizationMode == SceneRendererContext::RendererVisualizationMode::Wireframe)
		{
			const bool wireframe = s_Data.ActiveContext->VisualizationMode == SceneRendererContext::RendererVisualizationMode::Wireframe;
			if (wireframe)
			{
				glDisable(GL_CULL_FACE);
				RenderCommand::SetWireframe(true);
			}

			s_Data.ActiveContext->ActiveFramebuffer->Bind();

			if (!wireframe)
				s_Data.EditorDrawShaderOverride->Bind();
			
			for (const uint32_t modelIndex : visibleModels)
			{
				auto& model = s_Data.ModelDrawList[modelIndex];

				if (!IncludeEntity(model.EntityID))
					continue;

				if (!model.Model)
					continue;

				s_Data.ObjectBuffer.Model = model.Transform;
				s_Data.ObjectBuffer.PreviousModel = model.PreviousTransform;
				s_Data.ObjectBuffer.ModelInverse = glm::transpose(glm::inverse(model.Transform));
				s_Data.ObjectBuffer.Normal = glm::transpose(glm::inverse(glm::mat3(model.Transform)));
				s_Data.ObjectBuffer.EntityID = model.EntityID;
				SetupAnimationPlayerData(model.Pose);
				SetupBlendShapes(model.Model, model.BlendShapeWeights);
				s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(SceneRendererData::ObjectBuffer));

				if (wireframe && model.Selected)
					s_Data.WireframeShader->Bind();
				
				DrawModel(model.Model, model.Materials, (!wireframe || model.Selected) ? MaterialAsset::MaterialRenderStage::None : MaterialAsset::MaterialRenderStage::Default, true, true);

				glEnable(GL_CULL_FACE);
			}
			
			if (wireframe)
				RenderCommand::SetWireframe(false);
		}
		else
		{
			// Submit all point lights
			{
				// Update count so we don't have to memset unused slots.
				uint32_t count = s_Data.PointLightList.size();
				s_Data.LightCountsSSBO->SetData(&count, sizeof(uint32_t), sizeof(uint32_t));

				// Add 'null' light at end of dataset
				s_Data.PointLightList.emplace_back();

				if (!s_Data.PointLightList.empty())
					s_Data.PointLightSSBO->SetData(s_Data.PointLightList.data(), s_Data.PointLightList.size() * sizeof(SceneRendererData::PointLight));
			}

			// Pass in the current volumetric data
			s_Data.VolumetricBuffer.VolumetricGridSize = RendererConstants::VolumetricLightingVoxelSize;
			s_Data.VolumetricUniformBuffer->SetData(&s_Data.VolumetricBuffer, sizeof(SceneRendererData::VolumetricData));

			// Gather transparent objects
			s_Data.TranslucentMeshDrawList.reserve(s_Data.TranslucentObjectCountPreviousFrame);
			for (auto& model : s_Data.ModelDrawList)
			{
				if (!IncludeEntity(model.EntityID))
					continue;

				const uint32_t materialCount = model.Materials.size();
				uint32_t index = 0;
				for (auto& mesh : model.Model->GetMeshes())
				{
					Ref<MaterialAsset> material = (index < materialCount && model.Materials[index] != nullptr && model.Materials[index]->IsLoaded()) ? model.Materials[index] : s_Data.DefaultMaterial;

					if (material->GetProperties().AlphaBlendMode == MaterialAsset::Translucent)
						s_Data.TranslucentMeshDrawList.emplace_back(model.Transform, model.PreviousTransform, mesh, material, model.Pose, model.BlendShapeWeights, model.EntityID, model.Selected);

					index++;
				}

			};

			s_Data.TranslucentObjectCountPreviousFrame = s_Data.TranslucentMeshDrawList.size();

			// Sort transparent objects
			std::sort(s_Data.TranslucentMeshDrawList.begin(), s_Data.TranslucentMeshDrawList.end(), &CompareMeshDistance);

			// Clear the outline framebuffer
			s_Data.ActiveContext->OutlineFramebuffer->Bind();
			RenderCommand::Clear();

			// Bind the active framebuffer for the pre depth and main deferred rendering pass
			s_Data.ActiveContext->ActiveFramebuffer->Bind();

			// Pre Depth Pass
			if (s_Data.UsePreDepth)
			{
				for (const uint32_t modelIndex : visibleModels)
				{
					auto& model = s_Data.ModelDrawList[modelIndex];

					if (!IncludeEntity(model.EntityID))
						continue;

					if (!model.Model)
						continue;

					s_Data.ObjectBuffer.Model = model.Transform;
					s_Data.ObjectBuffer.PreviousModel = model.PreviousTransform;
					s_Data.ObjectBuffer.ModelInverse = glm::transpose(glm::inverse(model.Transform));
					s_Data.ObjectBuffer.Normal = glm::transpose(glm::inverse(glm::mat3(model.Transform)));
					s_Data.ObjectBuffer.EntityID = model.EntityID;
					SetupAnimationPlayerData(model.Pose);
					SetupBlendShapes(model.Model, model.BlendShapeWeights);
					s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(SceneRendererData::ObjectData));

					DrawModel(model.Model, model.Materials, MaterialAsset::MaterialRenderStage::PreDepth, false, true);

					s_Data.Stats.DrawCalls++;
				}
			}

			// Generate actual G-Buffer
			if (s_Data.UsePreDepth)
				glDepthFunc(GL_EQUAL);	// (Depth already calculated in PreDepth pass)

			glDisable(GL_BLEND);	// We want to store information in the alpha channel so we disable the depth test.

			bool objectSelected = false;
			for (const uint32_t modelIndex : visibleModels)
			{
				auto& model = s_Data.ModelDrawList[modelIndex];

				if (!IncludeEntity(model.EntityID))
					continue;

				if (!model.Model)
					continue;

				s_Data.ObjectBuffer.Model = model.Transform;
				s_Data.ObjectBuffer.PreviousModel = model.PreviousTransform;
				s_Data.ObjectBuffer.ModelInverse = glm::transpose(glm::inverse(model.Transform));
				s_Data.ObjectBuffer.Normal = glm::transpose(glm::inverse(glm::mat3(model.Transform)));
				s_Data.ObjectBuffer.EntityID = model.EntityID;
				SetupAnimationPlayerData(model.Pose);
				SetupBlendShapes(model.Model, model.BlendShapeWeights);
				s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(SceneRendererData::ObjectBuffer));

				DrawModel(model.Model, model.Materials, MaterialAsset::MaterialRenderStage::Default, false, true);

				if (model.Selected)
				{
					objectSelected = true;

					if (s_Data.UsePreDepth)
						glDepthFunc(GL_LESS);

					s_Data.ActiveContext->OutlineFramebuffer->Bind();
					DrawModel(model.Model, model.Materials, MaterialAsset::MaterialRenderStage::PreDepth, false, true);

					if (s_Data.UsePreDepth)
						glDepthFunc(GL_EQUAL);
					
					s_Data.ActiveContext->ActiveFramebuffer->Bind();
				}

				s_Data.Stats.DrawCalls++;
			}

			glEnable(GL_BLEND);
			glDepthFunc(GL_LESS);

#if 0
			DrawGrass();
#endif

			// Draw Deferred particle systems after other geometry (Note: This does not use the pre-depth pass!)
			DrawParticleSystems(true);

			// Draw deferred decals after other geometry
			// TODO: Batch all this geometry and use bindless textures
			if (!s_Data.DecalDrawList.empty())
				s_Data.DecalShader->Bind();

			for (const auto& decal : s_Data.DecalDrawList)
			{
				if (!IncludeEntity(decal.EntityID))
					continue;

				s_Data.ObjectBuffer.Model = decal.Transform;
				s_Data.ObjectBuffer.ModelInverse = glm::inverse(decal.Transform);
				s_Data.ObjectBuffer.EntityID = decal.EntityID;
				s_Data.ObjectBuffer.Animated = decal.ConstrainAngle;
				s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(SceneRendererData::ObjectData));

				decal.Albedo->Bind();
				RenderCube();
			}

			Ref<Framebuffer> PriorStageFramebuffer = s_Data.ActiveContext->DeferredLightingFramebuffer;

			if (s_Data.ActiveContext->VisualizationMode == SceneRendererContext::RendererVisualizationMode::Rendered ||
				s_Data.ActiveContext->VisualizationMode == SceneRendererContext::RendererVisualizationMode::LightingOnly ||
				s_Data.ActiveContext->VisualizationMode == SceneRendererContext::RendererVisualizationMode::PrePostProcessing)
			{
				// Shadow Calculations
				// Point Light Shadow Pass
				if (IsFirstDraw())
				{
					glCullFace(GL_FRONT);
					static bool init = false;

					static uint32_t lightDepthMaps, lightDepthFBO;

					if (!init)
					{
						init = true;

						// Create Cubemap Array
						{
							// create depth cubemap texture
							glGenTextures(1, &lightDepthMaps);
							glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, lightDepthMaps);

							glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_DEPTH_COMPONENT32F, RendererConstants::PointShadowResolution, RendererConstants::PointShadowResolution, (RendererConstants::MaxShadowedLights * 6), 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

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

					glViewport(0, 0, RendererConstants::PointShadowResolution, RendererConstants::PointShadowResolution);
					glBindFramebuffer(GL_FRAMEBUFFER, lightDepthFBO);

					RenderCommand::Clear();

					for (auto& light : s_Data.PointLightList)
					{
						if (light.shadowIndex == -1)
							continue;

						const uint32_t index = light.shadowIndex;

						// Render Light from cameras perspective to shadow atlas, repeat for all six tiles

						float near_plane = 0.1f;
						float far_plane = light.range;
						glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)RendererConstants::PointShadowResolution / (float)RendererConstants::PointShadowResolution, near_plane, far_plane);
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
						s_Data.LightingUniformBuffer->SetData(&s_Data.LightingBuffer, sizeof(SceneRendererData::LightingData));

						for (auto& model : s_Data.ModelDrawList)
						{
							if (!IncludeEntity(model.EntityID))
								continue;

							if (!model.Model)
								continue;

							if (!model.Model->GetAABB().Transform(model.Transform).Intersects(light.position, light.range))
								continue;

							s_Data.ObjectBuffer.Model = model.Transform;
							s_Data.ObjectBuffer.PreviousModel = model.PreviousTransform;
							s_Data.ObjectBuffer.ModelInverse = glm::transpose(glm::inverse(model.Transform));
							SetupAnimationPlayerData(model.Pose);
							SetupBlendShapes(model.Model, model.BlendShapeWeights);
							s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(SceneRendererData::ObjectBuffer));

							DrawModel(model.Model, model.Materials, MaterialAsset::MaterialRenderStage::Shadow);
						}
					}
					glBindTextureUnit(14, lightDepthMaps);
					glCullFace(GL_BACK);
				}

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
					s_Data.LightingUniformBuffer->SetData(&s_Data.LightingBuffer, sizeof(SceneRendererData::LightingData));
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
							GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, RendererConstants::ShadowMapResolution, RendererConstants::ShadowMapResolution, int(4) + 1,
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
							DY_CORE_ERROR("Framebuffer is not complete!");
							throw 0;
						}

						glBindFramebuffer(GL_FRAMEBUFFER, 0);
					}
					glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);
					glViewport(0, 0, RendererConstants::ShadowMapResolution, RendererConstants::ShadowMapResolution);

					RenderCommand::Clear();

					for (auto& model : s_Data.ModelDrawList)
					{
						if (!IncludeEntity(model.EntityID))
							continue;

						if (!model.Model)
							continue;

						s_Data.ObjectBuffer.Model = model.Transform;
						s_Data.ObjectBuffer.PreviousModel = model.PreviousTransform;
						s_Data.ObjectBuffer.ModelInverse = glm::transpose(glm::inverse(model.Transform));
						SetupAnimationPlayerData(model.Pose);
						SetupBlendShapes(model.Model, model.BlendShapeWeights);
						s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(SceneRendererData::ObjectBuffer));

						DrawModel(model.Model, model.Materials, MaterialAsset::MaterialRenderStage::ShadowCSM);
					}

					glActiveTexture(GL_TEXTURE0 + 13);
					glBindTexture(GL_TEXTURE_2D_ARRAY, lightDepthMaps);
				}

				glEnable(GL_CULL_FACE);
				glDisable(GL_DEPTH_CLAMP);
				glCullFace(GL_BACK);

				// VXGI Voxelization and Compute Pass
				if (s_Data.EnableVXGI)
				{
					// Upload VXGI uniform buffer data to the GPU
					UploadVXGIUniformData();

					for (uint32_t cascadeIndex = 0; cascadeIndex < RendererConstants::VXGICascadeCount; cascadeIndex++)
					{
						const auto& cascade = s_Data.VXGICascades[cascadeIndex];
						const float voxelSize = cascade.VXGIGridSize / cascade.VXGIGridResolution;

						// Voxelization Pass
						cascade.VXGIGrid->BindTexture(0);

						// Bind channel staging buffers if needed 
						if (!s_Data.TakeAtomicFP16Path)
						{
							cascade.VXGIGridR->BindTexture(1);
							cascade.VXGIGridG->BindTexture(2);
							cascade.VXGIGridB->BindTexture(3);
						}

						// Clear all data
						uint32_t clearComputeSize = INT_CEILING_DIVISION(cascade.VXGIGridResolution, RendererConstants::VXGIClearLocalSize);
						s_Data.VXGIClearShader->Dispatch(clearComputeSize, clearComputeSize, clearComputeSize);

						RenderCommand::UnbindFramebuffers();

						const AABB voxelGridBounds = AABB(s_Data.VXGIBuffer.GridMin[cascadeIndex], s_Data.VXGIBuffer.GridMax[cascadeIndex]);

						// Setup viewports (if GL_NV_geometry_shader_passthrough and GL_NV_viewport_swizzle are supported)
						if (s_Data.TakeFastGeometryShaderPath)
						{
							constexpr uint32_t swizzleViewportCount = 3;
							glm::vec4 viewports[swizzleViewportCount];

							const glm::ivec4 swizzleAxes[swizzleViewportCount] = {
								glm::ivec4(GL_VIEWPORT_SWIZZLE_POSITIVE_X_NV, GL_VIEWPORT_SWIZZLE_POSITIVE_Y_NV, GL_VIEWPORT_SWIZZLE_POSITIVE_Z_NV, GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV),
								glm::ivec4(GL_VIEWPORT_SWIZZLE_POSITIVE_X_NV, GL_VIEWPORT_SWIZZLE_POSITIVE_Z_NV, GL_VIEWPORT_SWIZZLE_POSITIVE_Y_NV, GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV),
								glm::ivec4(GL_VIEWPORT_SWIZZLE_POSITIVE_Z_NV, GL_VIEWPORT_SWIZZLE_POSITIVE_Y_NV, GL_VIEWPORT_SWIZZLE_POSITIVE_X_NV, GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV)
							};

							for (uint32_t i = 0; i < swizzleViewportCount; i++)
							{
								// Viewport is packed into vec4 (x = left, y = bottom, z = width, w = height)
								viewports[i] = glm::vec4(0, 0, cascade.VXGIGridResolution * 2, cascade.VXGIGridResolution * 2);
								const auto& swizzleAxis = swizzleAxes[i];
								glViewportSwizzleNV(i, swizzleAxis.x, swizzleAxis.y, swizzleAxis.z, swizzleAxis.w);
							}

							glViewportArrayv(0, swizzleViewportCount, (float*)viewports);
						}
						else
						{
							RenderCommand::SetViewport(0, 0, cascade.VXGIGridResolution * 2, cascade.VXGIGridResolution * 2);
						}

						// Render all meshes to VXGI grid
						glDisable(GL_CULL_FACE);

						// Draw models
						for (const auto& model : s_Data.ModelDrawList)
						{
							if (!IncludeEntity(model.EntityID))
								continue;

							if (!model.Model)
								continue;

							const auto& aabb = model.Model->GetAABB();
							if (!voxelGridBounds.Overlaps(aabb.Transform(model.Transform)))
								continue;

							const glm::vec3 size = aabb.GetSize();
							if (glm::max(glm::max(size.x, size.y), size.z) < voxelSize)
								continue;

							s_Data.ObjectBuffer.Model = model.Transform;
							s_Data.ObjectBuffer.PreviousModel = model.PreviousTransform;
							s_Data.ObjectBuffer.ModelInverse = glm::transpose(glm::inverse(model.Transform));
							s_Data.ObjectBuffer.Normal = glm::transpose(glm::inverse(glm::mat3(model.Transform)));
							SetupAnimationPlayerData(model.Pose);
							SetupBlendShapes(model.Model, model.BlendShapeWeights);
							s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(SceneRendererData::ObjectBuffer));

							if (s_Data.TakeFastGeometryShaderPath)
							{
								s_Data.VXGIBuffer.RenderCascade = cascadeIndex;
								s_Data.VXGIUniformBuffer->SetData(&s_Data.VXGIBuffer.RenderCascade, sizeof(uint32_t), 2 * RendererConstants::VXGICascadeCount * sizeof(glm::vec4) + sizeof(glm::uvec4));

								DrawModel(model.Model, model.Materials, MaterialAsset::MaterialRenderStage::VXGI);
							}
							else
							{
								for (uint32_t i = 0; i < 3; i++)
								{
									s_Data.VXGIBuffer.RenderCascade = cascadeIndex;
									s_Data.VXGIBuffer.RenderAxis = i;
									s_Data.VXGIUniformBuffer->SetData(&s_Data.VXGIBuffer.RenderCascade, 2 * sizeof(uint32_t), 2 * RendererConstants::VXGICascadeCount * sizeof(glm::vec4) + sizeof(glm::uvec4));

									DrawModel(model.Model, model.Materials, MaterialAsset::MaterialRenderStage::VXGI);
								}
							}
						}

						// Draw translucent meshes (as they can also contribute)
						if (s_Data.VXGIEnableTranslucentGridContribution)
						{
							for (const auto& mesh : s_Data.TranslucentMeshDrawList)
							{
								if (!IncludeEntity(mesh.EntityID))
									continue;

								if (!mesh.Mesh)
									continue;

								const auto& aabb = mesh.Mesh->GetAABB();
								if (!voxelGridBounds.Overlaps(aabb.Transform(mesh.Transform)))
									continue;

								const glm::vec3 size = aabb.GetSize();
								if (glm::max(glm::max(size.x, size.y), size.z) < voxelSize)
									continue;

								s_Data.ObjectBuffer.Model = mesh.Transform;
								s_Data.ObjectBuffer.PreviousModel = mesh.PreviousTransform;
								s_Data.ObjectBuffer.ModelInverse = glm::transpose(glm::inverse(mesh.Transform));
								s_Data.ObjectBuffer.Normal = glm::transpose(glm::inverse(glm::mat3(mesh.Transform)));
								SetupAnimationPlayerData(mesh.Pose);
								s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(SceneRendererData::ObjectBuffer));

								if (s_Data.TakeFastGeometryShaderPath)
								{
									s_Data.VXGIBuffer.RenderCascade = cascadeIndex;
									s_Data.VXGIUniformBuffer->SetData(&s_Data.VXGIBuffer.RenderCascade, sizeof(uint32_t), 2 * RendererConstants::VXGICascadeCount * sizeof(glm::vec4) + sizeof(glm::uvec4));

									DrawMesh(mesh.Mesh, mesh.Material, MaterialAsset::MaterialRenderStage::VXGI);
								}
								else
								{
									for (uint32_t i = 0; i < 3; i++)
									{
										s_Data.VXGIBuffer.RenderCascade = cascadeIndex;
										s_Data.VXGIBuffer.RenderAxis = i;
										s_Data.VXGIUniformBuffer->SetData(&s_Data.VXGIBuffer.RenderCascade, 2 * sizeof(uint32_t), 2 * RendererConstants::VXGICascadeCount * sizeof(glm::vec4) + sizeof(glm::uvec4));

										DrawMesh(mesh.Mesh, mesh.Material, MaterialAsset::MaterialRenderStage::VXGI);
									}
								}
							}
						}

						glEnable(GL_CULL_FACE);

						// Merge staging buffers if needed
						if (!s_Data.TakeAtomicFP16Path)
						{
							cascade.VXGIGridR->Bind(0);
							cascade.VXGIGridG->Bind(1);
							cascade.VXGIGridB->Bind(2);

							const uint32_t clearComputeSize = INT_CEILING_DIVISION(cascade.VXGIGridResolution, RendererConstants::VXGIMergeLocalSize);
							s_Data.VXGIMergeShader->Dispatch(clearComputeSize, clearComputeSize, clearComputeSize);
						}

						// Generate Mipmaps
						{
							cascade.VXGIGrid->Bind(0);
							const uint32_t levels = cascade.VXGIGrid->GetMipLevels();
							for (uint32_t i = 1; i < levels; i++)
							{
								// Note: We reuse the submesh uniform buffer here to represent active LOD/mipmap (probably shouldn't do this)
								s_Data.SubmeshBuffer.SubmeshIndex = i - 1;
								s_Data.SubmeshUniformBuffer->SetData(&s_Data.SubmeshBuffer, sizeof(SceneRendererData::SubmeshData));

								const glm::uvec3 size = cascade.VXGIGrid->GetMipmapLevelSize(i);
								cascade.VXGIGrid->BindTexture(0, i, true);
								s_Data.VXGIMipmapShader->Dispatch(INT_CEILING_DIVISION(size.x, RendererConstants::VXGIMipmapLocalSize), INT_CEILING_DIVISION(size.y, RendererConstants::VXGIMipmapLocalSize), INT_CEILING_DIVISION(size.z, RendererConstants::VXGIMipmapLocalSize));
							}
						}
					}

					if (!s_Data.VXGIGridVisualization)
					{
						const glm::uvec2 vxgiFramebufferSize = (glm::vec2)s_Data.ActiveContext->GetActiveSize() * s_Data.VXGIRenderScale;

						if (vxgiFramebufferSize != s_Data.ActiveContext->VXGIFramebuffer->GetSize())
							s_Data.ActiveContext->VXGIFramebuffer->Resize(vxgiFramebufferSize);

						s_Data.ActiveContext->VXGIFramebuffer->BindColorTexture(0);
						s_Data.VXGIConeTracingShader->Dispatch(INT_CEILING_DIVISION(vxgiFramebufferSize.x, RendererConstants::VXGIConeTracingLocalSize), INT_CEILING_DIVISION(vxgiFramebufferSize.y, RendererConstants::VXGIConeTracingLocalSize), 1);
						PriorStageFramebuffer = s_Data.ActiveContext->VXGIFramebuffer;
					}
				}

				// Deferred Lighting Rendering
				s_Data.ClusterShader->Dispatch(16, 9, 24); // Should only be run when camera changes
				s_Data.ClusterCullLightShader->Dispatch(1, 1, 6); // Should be run every frames

				if (s_Data.EnableVXGI)
					s_Data.ActiveContext->VXGIFramebuffer->BindColorSampler(6, 0);	// VXGI Cone Traced Indirect Lighting

				if (s_Data.PostProcessingBuffer.UsingVolumetricClouds)
					s_Data.PerlinWorleyNoise->Bind(7);

				s_Data.ActiveContext->DeferredLightingFramebuffer->Bind();
				RenderQuad(s_Data.DeferredLightingShader);

				// Copy the G-Buffer depth to the deferred lighting framebuffer
				s_Data.ActiveContext->ActiveFramebuffer->CopyDepth(s_Data.ActiveContext->DeferredLightingFramebuffer);

				// Render Translucent
				{
					// Copy both color and depth buffers to translucent framebuffer
					s_Data.ActiveContext->DeferredLightingFramebuffer->Copy(s_Data.ActiveContext->TranslucentLightingFramebuffer);

					s_Data.ActiveContext->TranslucentLightingFramebuffer->Bind();
					s_Data.ActiveContext->DeferredLightingFramebuffer->BindColorSampler(2, 0);

					glDepthFunc(GL_LESS);	// Use the depth buffer as normal
					glDepthMask(GL_FALSE);	// Disable depth buffer writing

					// Draw particle systems
					DrawParticleSystems(false);

					// Draw translucent meshes
					for (auto& mesh : s_Data.TranslucentMeshDrawList)
					{
						if (!IncludeEntity(mesh.EntityID))
							continue;

						s_Data.ObjectBuffer.Model = mesh.Transform;
						s_Data.ObjectBuffer.PreviousModel = mesh.PreviousTransform;
						s_Data.ObjectBuffer.ModelInverse = glm::transpose(glm::inverse(mesh.Transform));
						s_Data.ObjectBuffer.Normal = glm::transpose(glm::inverse(glm::mat3(mesh.Transform)));
						s_Data.ObjectBuffer.EntityID = mesh.EntityID;
						SetupAnimationPlayerData(mesh.Pose);
						s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(SceneRendererData::ObjectBuffer));

						DrawMesh(mesh.Mesh, mesh.Material, MaterialAsset::MaterialRenderStage::Default);

						if (mesh.Selected)
						{
							objectSelected = true;
							
							glDepthMask(GL_TRUE);
							s_Data.ActiveContext->OutlineFramebuffer->Bind();
							DrawMesh(mesh.Mesh, mesh.Material, MaterialAsset::MaterialRenderStage::PreDepth);
							glDepthMask(GL_FALSE);

							s_Data.ActiveContext->TranslucentLightingFramebuffer->Bind();
						}
					}

					glDepthMask(GL_TRUE);
				}

				// Volumetric Lighting
				{
					// Update lighting voxel grid
					{
						static uint32_t voxelTexture = 0;
						if (!voxelTexture)
						{
							glCreateTextures(GL_TEXTURE_3D, 1, &voxelTexture);

							// Set the texture wrapping and filtering parameters
							glTextureParameteri(voxelTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
							glTextureParameteri(voxelTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
							glTextureParameteri(voxelTexture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
							glTextureParameteri(voxelTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
							glTextureParameteri(voxelTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

							// Allocate storage for the texture
							glTextureStorage3D(voxelTexture, 1, GL_RGBA32F, RendererConstants::VolumetricLightingVoxelSize, RendererConstants::VolumetricLightingVoxelSize, RendererConstants::VolumetricLightingVoxelSize);
						}

						// Dispatch voxel grid compute shader
						glBindImageTexture(0, voxelTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

						const uint32_t invocations = INT_CEILING_DIVISION(RendererConstants::VolumetricLightingVoxelSize, RendererConstants::VolumetricLightPropagationShaderLocalSize);
						s_Data.LightPropagationVolumeShader->Dispatch(invocations, invocations, invocations);

						glBindTextureUnit(1, voxelTexture);
					}
					
					// Render volumetric lighting
					s_Data.ActiveContext->VolumetricLightingFramebuffer->Bind();
					RenderCommand::Clear();
					RenderQuad(s_Data.VolumetricLightingShader);

					// Blur volumetric lighting
					s_Data.ActiveContext->VolumetricBlurFramebuffer->Bind();
					RenderCommand::Clear();
					s_Data.ActiveContext->VolumetricLightingFramebuffer->BindColorSampler(0, 0);
					RenderQuad(s_Data.GaussianBlurShader);

					// Composite blurred lighting with base deferred texture
					s_Data.ActiveContext->VolumetricLightingCompositeFramebuffer->Bind();
					RenderCommand::Clear();
					s_Data.ActiveContext->TranslucentLightingFramebuffer->BindColorSampler(0, 0);
					s_Data.ActiveContext->VolumetricBlurFramebuffer->BindColorSampler(1, 0);
					RenderQuad(s_Data.AddTextureShader);
					PriorStageFramebuffer = s_Data.ActiveContext->VolumetricLightingCompositeFramebuffer;
				}
			}
			else
			{
				// G Buffer Visualization Rendering
				s_Data.ActiveContext->DeferredLightingFramebuffer->Bind();
				RenderQuad(s_Data.BufferVisualizationShader);
			}

			if (s_Data.ActiveContext->VisualizationMode == SceneRendererContext::RendererVisualizationMode::Rendered ||
				s_Data.ActiveContext->VisualizationMode == SceneRendererContext::RendererVisualizationMode::LightingOnly)
			{
				// SSGI
				if (false)
				{
					// Main screen space pass
					s_Data.ActiveContext->SSGIFramebuffer->Bind();
					RenderCommand::Clear();
					PriorStageFramebuffer->BindColorSampler(0, 0);
					RenderQuad(s_Data.SSGIShader);
					PriorStageFramebuffer = s_Data.ActiveContext->SSGIFramebuffer;

					// TODO: De-noise Pass
					// TODO: Only apply SSGI where VXGI coverage is not available
				}

				// Isolate bright, exposed areas
				{
					s_Data.ActiveContext->BloomFramebuffer->Bind();
					RenderCommand::Clear();
					PriorStageFramebuffer->BindColorSampler(0, 0);
					RenderQuad(s_Data.BloomIsolateShader);
				}

				// SSAO
				if (s_Data.UseSSAO)
				{
					PriorStageFramebuffer->Bind();
					s_Data.SSAONoiseTexture->Bind(0);
					RenderQuad(s_Data.SSAOShader);
				}

				// Auto Exposure

				// Make a frame copy for SSR
				{
					s_Data.ActiveContext->PreviousSSRFrame->Bind();
					PriorStageFramebuffer->BindColorSampler(0, 0);
					Renderer::DrawFullscreenTexture();
				}

				// Screen Space Reflections
				if (s_Data.UseSSR)
				{
					s_Data.ActiveContext->SSRFramebuffer->Bind();
					RenderCommand::Clear();

					s_Data.ActiveContext->PreviousSSRFrame->BindColorSampler(0, 0);
					RenderQuad(s_Data.SSRShader);

					PriorStageFramebuffer = s_Data.ActiveContext->SSRFramebuffer;
				}

				// TAA
				// TODO: Jitter TAA
				if (s_Data.PostProcessingBuffer.AntiAliasingMode == RendererConstants::AntiAliasingMode::TAA)
				{
					const glm::uvec2 taaFramebufferSize = s_Data.ActiveContext->TAAFramebuffer->GetSize();
					s_Data.ActiveContext->TAAFramebuffer->BindColorTexture(0, 0);
					PriorStageFramebuffer->BindColorSampler(0, 0);
					s_Data.ActiveContext->TAAHistoryColorFramebuffer->BindColorSampler(1, 0);
					s_Data.TAAShader->Dispatch(INT_CEILING_DIVISION(taaFramebufferSize.x, RendererConstants::TAALocalSize), INT_CEILING_DIVISION(taaFramebufferSize.y, RendererConstants::TAALocalSize), 1);

					// Copy current color to previous color
					PriorStageFramebuffer->Copy(s_Data.ActiveContext->TAAHistoryColorFramebuffer);

					// Update the previous framebuffer stage
					PriorStageFramebuffer = s_Data.ActiveContext->TAAFramebuffer;
				}

				// FSR
				if (false)
				{
					FSRManager::FSRContext context;
					context.RenderSize = glm::uvec2(s_Data.ActiveContext->ActiveWidth, s_Data.ActiveContext->ActiveHeight);
					context.DisplaySize;
					
					context.ColorRendererID = PriorStageFramebuffer->GetColorAttachmentRendererID();
					context.ColorFormat = TextureFormat::RGBA16F;
					context.DepthRendererID = s_Data.ActiveContext->ActiveFramebuffer->GetDepthAttachmentRendererID();
					context.DepthFormat = TextureFormat::Depth;
					context.MotionVectorsRendererID = s_Data.ActiveContext->ActiveFramebuffer->GetColorAttachmentRendererID(6);
					context.MotionVectorsFormat = TextureFormat::RG16F;
					
					FSRManager::SetContext(context);
					FSRManager::Dispatch(s_Data.PostProcessingBuffer.DeltaTime);
					s_OutputTexture = FSRManager::GetOutput();
				}

				// VXGI Debug Visualization
				if (s_Data.EnableVXGI && s_Data.VXGIGridVisualization)
				{
					PriorStageFramebuffer->Bind();
					RenderQuad(s_Data.VXGIDebugVisualizationShader);
				}

				// Execute User Render Passes
				for (const auto& postProcessVolume : s_Data.PostProcessVolumeList)
				{
					if (!IncludeEntity(postProcessVolume.EntityID))
						continue;

					if (!postProcessVolume.Material || postProcessVolume.Material->GetProperties().Usage != MaterialAsset::PostProcessing || !postProcessVolume.Material->HasShader(MaterialAsset::MaterialRenderStage::Default))
						continue;

					if (postProcessVolume.Bounded)
					{
						// Transform the point to unit cube's local space
						glm::vec4 localPoint = glm::inverse(postProcessVolume.Transform) * glm::vec4(glm::vec3(s_Data.CameraBuffer.ViewPosition), 1.0f);

						// Check if the point is inside the unit cube
						if (localPoint.x <= -0.5f || localPoint.x >= 0.5f || localPoint.y <= -0.5f || localPoint.y >= 0.5f || localPoint.z <= -0.5f || localPoint.z >= 0.5f)
							continue;
					}

					BindMaterialResources(postProcessVolume.Material);

					// Bind Required Textures
					PriorStageFramebuffer->BindColorSampler(20, 0);

					// Draw the post process volume
					s_Data.ActiveContext->PostProcessVolumeFramebuffer->Bind();
					RenderCommand::Clear();
					RenderQuad(postProcessVolume.Material->GetShader(MaterialAsset::MaterialRenderStage::Default));
					PriorStageFramebuffer = s_Data.ActiveContext->PostProcessVolumeFramebuffer;
				}

				// Motion Blur
				if (s_Data.UseMotionBlur)
				{
					s_Data.ActiveContext->MotionBlurFramebuffer->Bind();
					RenderCommand::Clear();

					s_Data.MotionBlurShader->Bind();

					s_Data.ObjectBuffer.Model = s_Data.ActiveContext->PreviousViewProjectionMatrix;
					s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(SceneRendererData::ObjectBuffer));

					PriorStageFramebuffer->BindColorSampler(0, 0);

					RenderQuad(s_Data.MotionBlurShader);

					PriorStageFramebuffer = s_Data.ActiveContext->MotionBlurFramebuffer;
				}

				// DOF
				if (s_Data.UseDOF && s_Data.PostProcessingBuffer.FocusScale != 0.0f)
				{
					// Main Blur Pass
					s_Data.ActiveContext->DOFFramebuffer->Bind();
					RenderCommand::Clear();
					PriorStageFramebuffer->BindColorSampler(0, 0);
					RenderQuad(s_Data.DOFShader);

					// Bokeh isolation pass
					const uint32_t dispatchX = (s_Data.ActiveContext->ActiveWidth / 16) + (s_Data.ActiveContext->ActiveWidth % 16 == 0 ? 0 : 1);
					const uint32_t dispatchY = (s_Data.ActiveContext->ActiveHeight / 16) + (s_Data.ActiveContext->ActiveHeight % 16 == 0 ? 0 : 1);
					s_Data.BokehIsolateShader->Dispatch(dispatchX, dispatchY, 1);

					// Retrieve the bokeh count from GPU SSBO memory
					unsigned int count;
					s_Data.BokehSSBO->GetData(&count, sizeof(unsigned int));

					// Use this count to instance draw quads
					if (count != 0)
					{
						s_Data.BokehDrawShader->Bind();
						s_Data.BokehShapeTexture->Bind();
						glDisable(GL_DEPTH_TEST);
						RenderCommand::DrawIndexed(s_Data.BatchedQuadVertexArray, count * 6);
						glEnable(GL_DEPTH_TEST);
					}

					PriorStageFramebuffer = s_Data.ActiveContext->DOFFramebuffer;
				}

				// Lens Flares
				{
					s_Data.ActiveContext->LensFlareFramebuffer->Bind();
					PriorStageFramebuffer->BindColorSampler(0, 0);
					s_Data.VolumetricNoiseTexture->Bind(1);
					RenderQuad(s_Data.LensFlareShader);
					PriorStageFramebuffer = s_Data.ActiveContext->LensFlareFramebuffer;
				}

				// Bloom
				if (s_Data.UseBloom)
				{
					Ref<Framebuffer> CurrentLevelFramebuffer = s_Data.ActiveContext->BloomFramebuffer;

					for (size_t i = 0; i < RendererConstants::NumBloomDownsamples; i++)
					{
						s_Data.ActiveContext->BloomDownsampleFramebuffers[i]->Bind();
						RenderCommand::Clear();

						CurrentLevelFramebuffer->BindColorSampler(0, 0);
						RenderQuad(i % 2 == 0 ? s_Data.BloomBlurHorizontalShader : s_Data.BloomBlurVerticalShader);
						CurrentLevelFramebuffer = s_Data.ActiveContext->BloomDownsampleFramebuffers[i];
					}

					s_Data.ActiveContext->BloomFramebufferAddA->Bind();
					RenderCommand::Clear();
					s_Data.ActiveContext->BloomFramebufferAddB->Bind();
					RenderCommand::Clear();

					for (size_t i = 0; i < RendererConstants::NumBloomDownsamples; i++)
					{
						bool even = i % 2 == 0;
						(even ? s_Data.ActiveContext->BloomFramebufferAddB : s_Data.ActiveContext->BloomFramebufferAddA)->Bind();
						(even ? s_Data.ActiveContext->BloomFramebufferAddA : s_Data.ActiveContext->BloomFramebufferAddB)->BindColorSampler(0, 0);
						s_Data.ActiveContext->BloomDownsampleFramebuffers[i]->BindColorSampler(1, 0);

						RenderQuad(s_Data.AddTextureShader);
					}

					s_Data.ActiveContext->BloomFramebuffer->Bind();
					RenderCommand::Clear();
					PriorStageFramebuffer->BindColorSampler(0, 0);
					s_Data.ActiveContext->BloomFramebufferAddB->BindColorSampler(1, 0);
					s_Data.BloomDirtTexture->Bind(2);
					RenderQuad(s_Data.BloomCompositeShader);
					PriorStageFramebuffer = s_Data.ActiveContext->BloomFramebuffer;
				}

				// Lens Distortion
				if (s_Data.UseLensDistortion)
				{
					s_Data.ActiveContext->LensDistortionFramebuffer->Bind();
					RenderCommand::Clear();
					PriorStageFramebuffer->BindColorSampler(0, 0);
					RenderQuad(s_Data.LensDistortionShader);
					PriorStageFramebuffer = s_Data.ActiveContext->LensDistortionFramebuffer;
				}

				// FXAA
				if (s_Data.PostProcessingBuffer.AntiAliasingMode == RendererConstants::AntiAliasingMode::FXAA)
				{
					s_Data.ActiveContext->FXAAFramebuffer->Bind();

					PriorStageFramebuffer->BindColorSampler(0, 0);
					RenderQuad(s_Data.FXAAShader);
					PriorStageFramebuffer = s_Data.ActiveContext->FXAAFramebuffer;
				}

				// Draw Final Quad
				{
					s_Data.ActiveContext->ActiveFramebuffer->Bind();

					// Bind Color Buffer from prior stage
					PriorStageFramebuffer->BindColorSampler(0, 0);
					s_Data.ActiveContext->ActiveFramebuffer->BindColorSampler(1, 1);

					if (s_Data.PostProcessingBuffer.UsingLUT)
						s_Data.ActiveContext->LUTTexture->Bind(2);

					glDepthMask(GL_FALSE);
					RenderQuad(s_Data.FinalCompositingFXShader);
					glDepthMask(GL_TRUE);
				}
			}
			else
			{
				s_Data.ActiveContext->ActiveFramebuffer->Bind();

				s_Data.ActiveContext->DeferredLightingFramebuffer->BindColorSampler(0, 0);
				s_Data.ActiveContext->ActiveFramebuffer->BindColorSampler(1, 1);

				glDepthMask(GL_FALSE);
				RenderQuad(s_Data.FinalCompositingShader);
				glDepthMask(GL_TRUE);
			}

			// Outline Post Processing
			if (objectSelected)
			{
				// Run FXAA algorithm on outline framebuffer
				{
					s_Data.ActiveContext->FXAAFramebuffer->Bind();

					s_Data.ActiveContext->OutlineFramebuffer->BindDepthSampler(0);
					RenderQuad(s_Data.FXAAShader);
				}

				s_Data.ActiveContext->ActiveFramebuffer->Bind();
				s_Data.ActiveContext->FXAAFramebuffer->BindColorSampler(0, 0);
				RenderQuad(s_Data.OutlineShader);
			}
		}

		s_Data.ActiveContext->PreviousViewProjectionMatrix = s_Data.CameraBuffer.ViewProjection;
		s_Data.ActiveContext->SwapBuffers();
	}

	void SceneRenderer::DrawMeshOutlineOverlay(const glm::mat4& transform, Ref<Model> model, const glm::vec4& color, int entityID)
	{
		glDepthFunc(GL_LEQUAL);

		s_Data.ActiveContext->ActiveFramebuffer->Bind();
		
		s_Data.ColorShader->Bind();
		s_Data.MaterialParameterUniformBuffer->SetData(&color, sizeof(glm::vec4));

		s_Data.ObjectBuffer.Model = transform;
		s_Data.ObjectBuffer.ModelInverse = glm::inverse(transform);
		s_Data.ObjectBuffer.EntityID = entityID;
		s_Data.ObjectBuffer.Animated = false;
		s_Data.ObjectUniformBuffer->SetData(&s_Data.ObjectBuffer, sizeof(SceneRendererData::ObjectData));
		
		RenderCommand::SetWireframe(true);
		auto& meshes = model->GetMeshes();
		for (auto& mesh : meshes)
			mesh->Draw();
		RenderCommand::SetWireframe(false);

		glDepthFunc(GL_LESS);
	}

	void SceneRenderer::SubmitDirectionalLight(const glm::vec3& rotation, const DirectionalLightComponent& lightComponent)
	{
		auto& light = s_Data.LightingBuffer.DirectionalLight;

		glm::vec3 direction;
		direction.x = cos(rotation.z) * cos(rotation.y);
		direction.y = sin(rotation.z) * cos(rotation.y);
		direction.z = -sin(rotation.y);

		s_Data.LightingBuffer.DirectionalLight.direction = glm::vec4(direction, 1.0f);
		s_Data.LightingBuffer.DirectionalLight.color = lightComponent.Color;
		s_Data.LightingBuffer.DirectionalLight.intensity = lightComponent.Intensity;
		s_Data.LightingBuffer.UsingDirectionalLight = 1;
	}

	void SceneRenderer::SubmitPointLight(const glm::vec3& translation, PointLightComponent& lightComponent)
	{
		s_Data.PointLightList.push_back({});
		auto& light = s_Data.PointLightList.back();

		light.position = glm::vec4(translation, 1.0f);
		light.color = glm::vec4(lightComponent.Color, 1.0f);
		light.intensity = lightComponent.Intensity;
		light.range = lightComponent.Radius;
		light.enabled = true;
		light.shadowIndex = ((s_Data.NextShadowIndex < RendererConstants::MaxShadowedLights && lightComponent.CastsShadows) ? (s_Data.NextShadowIndex++) : -1);
	}

	void SceneRenderer::SubmitSpotLight(const Transform& transform, SpotLightComponent& lightComponent)
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

		//  let OpenGL generate mipmaps from first mip face (combating visible dots artifact)
		glGenerateTextureMipmap(cubemap);
	}

	void SceneRenderer::SubmitSkyLight(const SkyLightComponent& lightComponent)
	{
		if (!s_Data.LightingBuffer.UsingSkyLight) // Prevent multiple skylights being submitted per frame.
		{
			s_Data.LightingBuffer.SkyLightIntensity = lightComponent.Intensity;
			auto& skyboxHDRIID = s_Data.ActiveContext->SceneContext->SkyboxHDRIID;

			if (lightComponent.Type == 0)
			{
				if (!lightComponent.EnvironmentMap)
					return;

				s_Data.LightingBuffer.UsingSkyLight = true;

				// Submit the skylight
				if (skyboxHDRIID != lightComponent.EnvironmentMap->GetMap()->GetRendererID())
				{
					skyboxHDRIID = lightComponent.EnvironmentMap->GetMap()->GetRendererID();
					UpdateSkyLight();
				}

				if (lightComponent.FlowMap)
				{
					s_Data.LightingBuffer.UsingFlowMap = true;
					auto& skyboxFlowMapID = s_Data.ActiveContext->SceneContext->SkyboxFlowMapID;

					if (skyboxFlowMapID != lightComponent.FlowMap->GetRendererID())
					{
						skyboxFlowMapID = lightComponent.FlowMap->GetRendererID();

						glDisable(GL_CULL_FACE);
						GLuint fbo;
						glCreateFramebuffers(1, &fbo);
						glBindFramebuffer(GL_FRAMEBUFFER, fbo);
						glDrawBuffer(GL_COLOR_ATTACHMENT0);
						EquirectangularToCubemap(skyboxFlowMapID, s_Data.ActiveContext->SceneContext->FlowMapCubemap->GetRendererID());
						glDeleteFramebuffers(1, &fbo);
					}
				}
			}
			else
			{
				s_Data.LightingBuffer.UsingSkyLight = true;
				s_Data.LightingBuffer.UsingFlowMap = false;

				s_Data.ActiveContext->VolumetricCloudsFramebuffer->Bind();
				RenderCommand::Clear();
				s_Data.VolumetricNoiseTexture->Bind();
				RenderQuad(s_Data.VolumetricCloudsShader);
				skyboxHDRIID = s_Data.ActiveContext->VolumetricCloudsFramebuffer->GetColorAttachmentRendererID();
				UpdateSkyLight();
			}
		}
	}

	void SceneRenderer::SubmitParticleSystem(const glm::mat4& transform, Ref<ParticleSystemPlayer> particleSystem, Ref<MaterialAsset> material, int entityID)
	{
		s_Data.ParticleSystemList.emplace_back(transform, particleSystem, material, entityID);
	}

	void SceneRenderer::SubmitDecal(const glm::mat4& transform, Ref<Texture2D> texture, bool constrainAngle, int entityID)
	{
		if (!texture)
			return;

		s_Data.DecalDrawList.emplace_back(transform, texture, constrainAngle, entityID);
	}

	void SceneRenderer::SubmitVolume(const Transform& transform, const VolumeComponent& volumeCompontent, int entityID)
	{
		if (s_Data.VolumetricBuffer.VolumeCount < RendererConstants::MaxVolumes)
		{
			auto& volume = s_Data.VolumetricBuffer.Volumes[s_Data.VolumetricBuffer.VolumeCount++];
			volume.Min = glm::vec4(glm::vec3(-0.5f, -0.5f, -0.5f) * transform.Scale + transform.Translation, 1.0f);
			volume.Max = glm::vec4(glm::vec3(0.5f, 0.5f, 0.5f) * transform.Scale + transform.Translation, 1.0f);

			volume.Color = glm::vec4(volumeCompontent.Color, 1.0f);

			volume.Blend = (int)volumeCompontent.Blend;
			volume.ScatteringDistribution = volumeCompontent.ScatteringDistribution;
			volume.ScatteringIntensity = volumeCompontent.ScatteringIntensity;
			volume.ExtinctionScale = volumeCompontent.ExtinctionScale;
		}
	}

	void SceneRenderer::SubmitPostProcessVolume(const glm::mat4& transform, PostProcessVolumeComponent& volumeComponent, int entityID)
	{
		if (!volumeComponent.Material || volumeComponent.Material->GetProperties().Usage != MaterialAsset::PostProcessing)
			return;

		s_Data.PostProcessVolumeList.emplace_back(transform, volumeComponent.Material, volumeComponent.Bounded, entityID);
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

		auto& sceneContext = s_Data.ActiveContext->SceneContext;

		EquirectangularToCubemap(sceneContext->SkyboxHDRIID, sceneContext->EnvironmentCubemap->GetRendererID());
		sceneContext->EnvironmentCubemap->Bind(0);

		// Create irradiance cubemap
		{
			s_Data.IrradianceShader->Bind();

			glViewport(0, 0, 32, 32);

			for (size_t i = 0; i < 6; i++)
			{
				uniformBuffer.viewProjection = s_Data.CubemapCaptureProjection * s_Data.CubemapCaptureViews[i];
				// TODO: Fix. This is bad, reusing a spare uniform buffer!
				s_Data.ObjectUniformBuffer->SetData(&uniformBuffer, sizeof(BufferData));

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, sceneContext->IrradianceMap->GetRendererID(), 0);
				RenderCommand::Clear();

				RenderCube();
			}
		}

		// Create prefilter cubemap with a quasi monte-carlo simulation
		{
			s_Data.PrefilterShader->Bind();

			for (unsigned int mip = 0; mip < RendererConstants::MaxSkyboxMipLevels; mip++)
			{
				unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
				unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));

				glViewport(0, 0, mipWidth, mipHeight);

				uniformBuffer.roughness = (float)mip / (float)(RendererConstants::MaxSkyboxMipLevels - 1);
				for (size_t i = 0; i < 6; i++)
				{
					uniformBuffer.viewProjection = s_Data.CubemapCaptureProjection * s_Data.CubemapCaptureViews[i];
					// TODO: Fix. This is bad, reusing a spare uniform buffer!
					s_Data.ObjectUniformBuffer->SetData(&uniformBuffer, sizeof(BufferData));

					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, sceneContext->PrefilterMap->GetRendererID(), mip);
					RenderCommand::Clear();

					RenderCube();
				}
			}
		}

		// Generate a 2D LUT from BRDF equations
		{
			sceneContext->brdfLUTTexture->Bind();

			glViewport(0, 0, 512, 512);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneContext->brdfLUTTexture->GetRendererID(), 0);
			RenderCommand::Clear();
			
			RenderQuad(s_Data.brdfShader);
		}

		glDeleteFramebuffers(1, &fbo);

		glEnable(GL_CULL_FACE);
	}

	void SceneRenderer::UpdateLUT(const Ref<Texture2D> lutMap)
	{
		// Destroy the 3D LUT texture if there is no longer a LUT in use.
		if (!lutMap)
		{
			s_Data.ActiveContext->LUTTexture = nullptr;
			return;
		}

		// Generate the 3D Post Processing LUT from the 2D texture
		Buffer data = lutMap->GetData();

		// Verify the LUT texture is formatted with height for single component (e.g. 16) and height squared for width (e.g. 256) to represent a 3D Texture
		// Note: All shaders assume this texture is formatted with G channel along height (0 at top left) and R channel along width (0) top left, with
		// copies of this texture for each B channel entry horizontally.
		const uint32_t width = lutMap->GetWidth();
		const uint32_t height = lutMap->GetHeight();
		DY_CORE_VERIFY(height * height == width, "Invalid Dymatic LUT texture size");

		TextureSpecification lutSpecification;
		lutSpecification.Width = height;
		lutSpecification.Height = height;
		lutSpecification.Depth = height;
		lutSpecification.Format = lutMap->GetSpecification().Format;
		lutSpecification.SamplerWrap = TextureWrap::ClampToEdge;
		lutSpecification.SamplerFilter = TextureFilter::Linear;
		s_Data.ActiveContext->LUTTexture = Texture3D::Create(lutSpecification, data);

		data.Release();
	}

	void SceneRenderer::ResetStats()
	{
		memset(&s_Data.Stats, 0, sizeof(Statistics));
	}

	SceneRenderer::Statistics SceneRenderer::GetStats()
	{
		return s_Data.Stats;
	}

	float SceneRenderer::GetVXGIRenderScale()
	{
		return s_Data.VXGIRenderScale;
	}

	void SceneRenderer::SubmitDrawShaderOverride(Ref<Shader> shader)
	{
		s_Data.EditorDrawShaderOverride = shader;
	}

}

// Editor Only
#include <imgui.h>
namespace Dymatic {

	static const char* GetAntiAliasingModeString(RendererConstants::AntiAliasingMode antiAliasingMode)
	{
		switch (antiAliasingMode)
		{
		case RendererConstants::AntiAliasingMode::FXAA: return "FXAA";
		case RendererConstants::AntiAliasingMode::TAA: return "TAA";
		case RendererConstants::AntiAliasingMode::FSR: return "FSR";
		}

		return "None";
	}

	void SceneRenderer::OnImGuiRender()
	{
		ImGui::Begin(u8"\uf013" " Renderer Settings");

		if (ImGui::CollapsingHeader("LIGHTING"))
		{
			ImGui::DragFloat("Ambient", &s_Data.LightingBuffer.Ambient);
		}

		if (ImGui::CollapsingHeader("RENDER PASSES"))
		{			
			ImGui::Checkbox("Pre-Depth", &s_Data.UsePreDepth);
			ImGui::Checkbox("SSAO", &s_Data.UseSSAO);
			ImGui::Checkbox("SSR", &s_Data.UseSSR);
			ImGui::Checkbox("Motion Blur", &s_Data.UseMotionBlur);
			ImGui::Checkbox("DOF", &s_Data.UseDOF);
			ImGui::Checkbox("Bloom", &s_Data.UseBloom);
			ImGui::Checkbox("Lens Distortion", &s_Data.UseLensDistortion);
		}

		if (ImGui::CollapsingHeader("GAMMA"))
		{
			ImGui::DragFloat("Gamma", &s_Data.PostProcessingBuffer.Gamma, 0.1f, 0.0f);
		}

		if (ImGui::CollapsingHeader("ANTI ALIASING"))
		{
			const char* modeString = "None";
			
			ImGui::Text("Anti-Aliasing Mode");
			ImGui::SameLine();
			if (ImGui::BeginCombo("##AntiAliasingModeInput", GetAntiAliasingModeString((RendererConstants::AntiAliasingMode)s_Data.PostProcessingBuffer.AntiAliasingMode)))
			{
				for (uint32_t mode = RendererConstants::AntiAliasingMode::None; mode <= RendererConstants::AntiAliasingMode::FSR; mode++)
					if (ImGui::MenuItem(GetAntiAliasingModeString((RendererConstants::AntiAliasingMode)mode)))
						s_Data.PostProcessingBuffer.AntiAliasingMode = mode;

				ImGui::EndCombo();
			}
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

		if (ImGui::CollapsingHeader("Shader Library"))
		{
			auto& shaders = Renderer::GetShaderLibrary()->GetShaders();

			if (ImGui::Button("Reload All"))
				for (auto& [name, shader] : shaders)
					shader->Reload();

			ImGui::Separator();
			
			for (auto& [name, shader] : shaders)
			{
				ImGui::Text(name.c_str());
				ImGui::SameLine();
				ImGui::PushID(shader->GetRendererID());
				if (ImGui::Button("Reload"))
				{
					shader->Reload();
				}
				ImGui::PopID();
			}
		}

		if (ImGui::CollapsingHeader("VOLUMETRIC CLOUDS"))
		{
			bool usingVolumetricClouds = s_Data.PostProcessingBuffer.UsingVolumetricClouds;
			if (ImGui::Checkbox("Enable Volumetric Clouds", &usingVolumetricClouds))
				s_Data.PostProcessingBuffer.UsingVolumetricClouds = usingVolumetricClouds;
		}

		if (ImGui::CollapsingHeader("VXGI"))
		{
			if (ImGui::Checkbox("Enable VXGI", &s_Data.EnableVXGI))
				VXGICreateGrid();

			if (s_Data.EnableVXGI)
			{
				if (s_Data.VXGIGridVisualization)
				{
					ImGui::Separator();
					ImGui::TextDisabled("Debug Settings");
				}

				ImGui::Checkbox("VXGI Trace Visualization", &s_Data.VXGITraceVisualization);
				ImGui::Checkbox("VXGI Grid Visualization", &s_Data.VXGIGridVisualization);

				if (s_Data.VXGIGridVisualization)
				{
					if (ImGui::DragFloat("Debug Step Multiplier", &s_Data.DebugStepMultiplier, 0.01f, 0.05f, 1.0f))
						s_Data.DebugStepMultiplier = std::clamp(s_Data.DebugStepMultiplier, 0.05f, 1.0f);

					ImGui::DragFloat("Debug Cone Angle", &s_Data.VXGIBuffer.DebugConeAngle, 0.01f, 0.0f, 0.5f);
				}

				ImGui::Separator();
				ImGui::TextDisabled("Grid Generation Settings");

				bool takeFastGeometryShaderPath = s_Data.TakeFastGeometryShaderPath;
				if (ImGui::Checkbox("Take Fast Geometry Shader Path", &takeFastGeometryShaderPath))
					SetVXGITakeFastGeometryShaderPath(takeFastGeometryShaderPath);

				bool takeAtomicFP16Path = s_Data.TakeAtomicFP16Path;
				if (ImGui::Checkbox("Take Atomic FP16 Path", &takeAtomicFP16Path))
				{
					SetVXGITakeAtomicFP16Path(takeAtomicFP16Path);
					VXGICreateGrid();
				}

				if (ImGui::CollapsingHeader("Cascades"))
				{
					ImGui::Indent();

					for (uint32_t cascadeIndex = 0; cascadeIndex < RendererConstants::VXGICascadeCount; cascadeIndex++)
					{
						if (ImGui::CollapsingHeader(fmt::format("Cascade {}", cascadeIndex + 1).c_str()))
						{
							ImGui::Indent();
							ImGui::PushID(cascadeIndex);
							auto& cascade = s_Data.VXGICascades[cascadeIndex];

							ImGui::Text("Grid Size");
							ImGui::SameLine();
							ImGui::DragFloat("##GridSizeInput", &cascade.VXGIGridSize);

							ImGui::Text("Grid Offset");
							ImGui::SameLine();
							ImGui::DragFloat3("##GridOffsetInput", glm::value_ptr(cascade.VXGIGridOffset));

							ImGui::Text("Grid Resolution");
							ImGui::SameLine();
							int gridResolution = cascade.VXGIGridResolution;
							if (ImGui::DragInt("##GridResolutionInput", &gridResolution, 1.0, 0, 512))
							{
								cascade.VXGIGridResolution = glm::clamp(gridResolution, 0, 512);
								VXGICreateGrid(cascadeIndex);
							}

							ImGui::PopID();
							ImGui::Unindent();
						}
					}

					ImGui::Unindent();
				}

				ImGui::TextDisabled("Grid Sampling Settings");

				ImGui::Text("Scalability");
				ImGui::DragFloat("Resolution Scale", &s_Data.VXGIRenderScale, 0.05f, 0.0f, 2.0f);

				ImGui::Text("Translucent Geometry");
				ImGui::Checkbox("Translucent Lighting", &s_Data.VXGIEnableTranslucentLighting);
				ImGui::Checkbox("Translucent Grid Contribution", &s_Data.VXGIEnableTranslucentGridContribution);

				ImGui::Text("Noise Type");
				ImGui::SameLine();
				if (ImGui::BeginCombo("##NoiseType", s_Data.VXGIConeTracingShader->GetMacroFlag("USE_RANDOM_NOISE") ? "Random" : "Interleaved Gradient"))
				{
					if (ImGui::MenuItem("Random"))
						s_Data.VXGIConeTracingShader->SetMacro("USE_RANDOM_NOISE", true);

					if (ImGui::MenuItem("Interleaved Gradient"))
						s_Data.VXGIConeTracingShader->SetMacro("USE_RANDOM_NOISE", false);

					ImGui::EndCombo();
				}

				if (ImGui::DragFloat("Step Multiplier", &s_Data.StepMultiplier, 0.01f, 0.05f, 1.0f))
					s_Data.StepMultiplier = std::clamp(s_Data.StepMultiplier, 0.05f, 1.0f);

				ImGui::DragScalar("Max Samples", ImGuiDataType_U32, &s_Data.VXGIBuffer.MaxSamples, 1.0f);

				ImGui::Text("GI Boost");
				ImGui::SameLine();
				ImGui::DragFloat("##GIBoostInput", &s_Data.VXGIBuffer.GIBoost);

				ImGui::Text("GI Sky Light Boost");
				ImGui::SameLine();
				ImGui::DragFloat("##GISkyLightBoostInput", &s_Data.VXGIBuffer.GISkyLightBoost);

				ImGui::Text("Normal Ray Offset");
				ImGui::SameLine();
				ImGui::DragFloat("##NormalRayOffsetInput", &s_Data.VXGIBuffer.NormalRayOffset);

				ImGui::Text("Alpha Threashold");
				ImGui::SameLine();
				ImGui::DragFloat("##AlphaThreasholdInput", &s_Data.VXGIBuffer.AlphaThreashold);

				ImGui::Text("Max Cone Angle");
				ImGui::SameLine();
				ImGui::DragFloat("##MaxConeAngleInput", &s_Data.VXGIBuffer.MaxConeAngle);

				ImGui::Text("Min Cone Angle");
				ImGui::SameLine();
				ImGui::DragFloat("##MinConeAngleInput", &s_Data.VXGIBuffer.MinConeAngle);

				bool useTemporalAccumulation = s_Data.VXGIBuffer.UseTemporalAccumulation;
				if (ImGui::Checkbox("Use Temporal Accumulation", &useTemporalAccumulation))
					s_Data.VXGIBuffer.UseTemporalAccumulation = useTemporalAccumulation;

				ImGui::Text("Contribution Falloff");
				ImGui::SameLine();
				ImGui::DragFloat("##ContributionFalloffInput", &s_Data.VXGIBuffer.ContributionFalloff);
			}
		}

		if (ImGui::CollapsingHeader("Raytracing"))
		{
			if (ImGui::Button("Clean Raytrace Acceleration Structure"))
				s_ReloadRaytracing = true;

			if (ImGui::Button("Log BVH Acceleration Build Info"))
				PrintBVHBuildInfo();
		}

		ImGui::End();
	}
}
