#pragma once
#include <inttypes.h>

namespace Dymatic {

	// TODO: Could this not just be a namespace or part of the Renderer class?
	class RendererConstants
	{
	public:

		// NOTE: These values must match the bindings specified in the 'Buffers.glslh' shader header file
		enum Buffers
		{
			Camera = 0,
			Lighting = 1,
			Object = 2,
			Animation = 3,
			GBuffer = 4,

			MaterialParameters = 5,
			MaterialTextures = 6,
			
			Volumetric = 7,
			PostProcessing = 8,

			ClusterAABB = 9,
			PointLight = 10,
			LightIndex = 11,
			LightGrid = 12,
			LightCount = 13,

			Bokeh = 14,
			ParticleSystem = 15,
			
			Submesh = 16,

			PathTraceGeometry = 17,
			PathTraceBVH = 18,
			PathTraceSubmesh = 19,
			PathTraceModel = 20,

			MeshInstance = 21,
			MeshVisibility = 22,

			VXGI = 23,

			BlendShapeWeight = 24,
			BlendShapeComponents = 25,

			MeshSDF = 26,

			Editor = 27,
		};

		enum AntiAliasingMode
		{
			None = 0,
			FXAA = 1,
			TAA = 2,
			FSR = 3,
		};

		// Renderer2D
		static constexpr uint32_t MaxQuads = 20000;
		static constexpr uint32_t MaxVertices = MaxQuads * 4;
		static constexpr uint32_t MaxIndices = MaxQuads * 6;
		static constexpr uint32_t MaxTextureSlots = 32;
		static constexpr uint32_t MaxFontSlots = 32;
		static constexpr uint32_t MaxLineVertices = 4096;
		static constexpr uint32_t MaxPoints = 256;
		
		// Meshes/Bones
		static constexpr uint32_t MaxBones = 256;
		static constexpr uint32_t MaxBoneInfluence = 4;
		static constexpr uint32_t MaxBlendShapes = 256;

		// Lighting limits
		static constexpr uint32_t GridSizeX = 16, GridSizeY = 9, GridSizeZ = 24;
		static constexpr uint32_t NumClusters = GridSizeX * GridSizeY * GridSizeZ;
		static constexpr uint32_t MaxLightsPerTile = 100;
		static constexpr uint32_t MaxLights = NumClusters * MaxLightsPerTile;

		static constexpr uint32_t MaxCascadeCount = 16;
		static constexpr uint32_t ShadowMapResolution = 4096;
		static constexpr uint32_t PointShadowResolution = 1024;

		static constexpr uint32_t MaxShadowedLights = 10;

		static constexpr uint32_t VolumetricLightingVoxelSize = 128;
		static constexpr uint32_t VolumetricLightPropagationShaderLocalSize = 8;

		// Particle Limits
		static constexpr uint32_t MaxParticleCount = 10000;

		// Post Processing Limits
		static constexpr uint32_t NumBloomDownsamples = 14; // Must be even
		static constexpr uint32_t MaxVolumes = 32;
		static constexpr uint32_t MaxBokehCount = 2048;

		static constexpr uint32_t MaxSkyboxMipLevels = 5;

		// Materials
		static constexpr uint32_t MaxMaterialBufferSize = 1024;

		// Path Tracing
		static constexpr uint32_t RaytraceComputeLocalSize = 16;
		static constexpr uint32_t MaxPathTraceTriangles = 300000;
		// TODO: Submesh and Model buffers should be dynamically reallocated to avoid hard limits
		static constexpr uint32_t MaxPathTraceModels = 8;
		static constexpr uint32_t MaxPathTraceSubmesh = 64 * MaxPathTraceModels;
		static constexpr uint32_t MaxPathTraceBVHDepth = 32; // Requires replication in 'Buffers.glslh'
		static constexpr uint32_t MaxPathTraceNodes = MaxPathTraceModels * ((1 << 18) - 1);

		static constexpr uint32_t MaxModels = 10000;

		static constexpr uint32_t VXGICascadeCount = 4;

		static constexpr uint32_t VXGIConeTracingLocalSize = 8;
		static constexpr uint32_t VXGIMipmapLocalSize = 4;
		static constexpr uint32_t VXGIClearLocalSize = 4;
		static constexpr uint32_t VXGIMergeLocalSize = 4;

		static constexpr uint32_t PerlinWorleyResolution = 256;
		static constexpr uint32_t PerlinWorleyLocalSize = 8;
		static constexpr uint32_t CurlResolution = 128;

		static constexpr uint32_t TAALocalSize = 8;

		static constexpr uint32_t MeshSDFResolution = 128;
		static constexpr uint32_t SDFGenerationLocalSize = 8;

		// Editor Only
		static constexpr size_t EditorUniformBufferSize = 256;
	};

}