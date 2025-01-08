#pragma once

#include "Dymatic/Renderer/Texture.h"
#include "Dymatic/Renderer/Framebuffer.h"

#include "Dymatic/Renderer/RendererConstants.h"

#include <unordered_set>

namespace Dymatic {

	class SceneRendererSceneContext
	{
	public:
		static Ref<SceneRendererSceneContext> Create() { return CreateRef<SceneRendererSceneContext>(); }
		SceneRendererSceneContext();
		
	private:
		float Time = 0.0f;

		// PBR IBL Cubemap Data
		uint32_t SkyboxHDRIID; // Created by client
		Ref<TextureCube> EnvironmentCubemap;
		Ref<TextureCube> IrradianceMap;
		Ref<TextureCube> PrefilterMap;
		Ref<Texture2D> brdfLUTTexture; // TODO: This does not need to be per render context
		uint32_t SkyboxFlowMapID; // Created by client
		Ref<TextureCube> FlowMapCubemap;

		// Velocity and Motion Blur
		std::unordered_map<int, glm::mat4> CurrentModelTransforms;
		std::unordered_map<int, glm::mat4> PreviousModelTransforms;

		friend class SceneRenderer;
	};

	class SceneRendererContext
	{
	public:
		// Note: Changes to the RendererVisualizationMode enum need to be replicated in Buffers.glslh and Components.cs
		enum class RendererVisualizationMode
		{
			Invalid = -1,
			Rendered = 0, Wireframe, LightingOnly, PrePostProcessing, PathTraced,
			Albedo, Depth, LinearDepth, Position, Normal, Emissive, Roughness, Metallic, Specular, AmbientOcclusion, Velocity, EntityID, SubmeshIndex
		};

		enum class RenderMaskType
		{
			None,
			Exclusive,
			Inclusive
		};

		static const char* RenderVisualizationModeToString(const RendererVisualizationMode mode);
		static RendererVisualizationMode RenderVisualizationModeFromString(const std::string& modeString);

	public:
		// TODO: We probably don't need to specify a viewport size? We can just have a default size and wait for a Resize call.
		static Ref<SceneRendererContext> Create(const glm::uvec2& viewportSize) { return CreateRef<SceneRendererContext>(viewportSize); }
		static Ref<SceneRendererContext> Create(const glm::uvec2& viewportSize, const Ref<SceneRendererSceneContext> sceneContext) { return CreateRef<SceneRendererContext>(viewportSize, sceneContext); }

		SceneRendererContext(const glm::uvec2& viewportSize);
		SceneRendererContext(const glm::uvec2& viewportSize, const Ref<SceneRendererSceneContext> sceneContext);

		void Resize(const glm::uvec2& size);
		void Resize(const uint32_t width, const uint32_t height);

		glm::uvec2 GetActiveSize() const { return glm::uvec2(ActiveWidth, ActiveHeight); }

	private:
		void SwapBuffers();

	public:
		Ref<SceneRendererSceneContext> SceneContext;

		// Main Framebuffers
		Ref<Framebuffer> ActiveFramebuffer;
		Ref<Framebuffer> PreviousFramebuffer; // Store a full framebuffer with all attachments. Note: Originally just stored a copy of color buffer rather than swapping

		uint32_t ActiveWidth;
		uint32_t ActiveHeight;

		// General Render Data
		RendererVisualizationMode VisualizationMode = RendererVisualizationMode::Rendered;

		// Deferred Shading
		Ref<Framebuffer> DeferredLightingFramebuffer;

		// Forward Shading
		Ref<Framebuffer> TranslucentLightingFramebuffer;

		// Outline
		Ref<Framebuffer> OutlineFramebuffer;

		// Volumetric Lighting
		Ref<Framebuffer> VolumetricLightingFramebuffer;
		Ref<Framebuffer> VolumetricBlurFramebuffer;
		Ref<Framebuffer> VolumetricLightingCompositeFramebuffer;

		// Post Processing Render Pass Framebuffers
		Ref<Framebuffer> SSAOFramebuffer;
		Ref<Framebuffer> PreviousSSRFrame; // Frame prior to SSR pass in the previous frame.
		Ref<Framebuffer> SSRFramebuffer;
		Ref<Framebuffer> FXAAFramebuffer;
		Ref<Framebuffer> MotionBlurFramebuffer;
		Ref<Framebuffer> LensFlareFramebuffer;
		Ref<Framebuffer> PostProcessVolumeFramebuffer;

		Ref<Framebuffer> DOFFramebuffer;
		Ref<Framebuffer> LensDistortionFramebuffer;
		Ref<Framebuffer> VolumetricCloudsFramebuffer;

		Ref<Framebuffer> BloomFramebufferAddA;
		Ref<Framebuffer> BloomFramebufferAddB;
		Ref<Framebuffer> BloomBrightIsolated;
		Ref<Framebuffer> BloomFramebuffer;
		Ref<Framebuffer> BloomDownsampleFramebuffers[RendererConstants::NumBloomDownsamples];

		// Path Tracing Framebuffers (0 = accumulated frame (read), 1 = accumulated frame (write))
		Ref<Framebuffer> PathTracingFramebuffer;
		Ref<Framebuffer> PathTracingAccumulateFramebuffers[2];
		uint32_t AccumulationFrames = 0;
		size_t PreviousModelListHash = -1;
		size_t PreviousMaterialListHash = -1;

		// VXGI
		Ref<Framebuffer> VXGIFramebuffer;
		Ref<Framebuffer> SSGIFramebuffer;

		// TAA
		Ref<Framebuffer> TAAFramebuffer;
		Ref<Framebuffer> TAAHistoryColorFramebuffer;

		// Velocity and Motion Blur
		glm::mat4 PreviousViewProjectionMatrix = glm::mat4(1.0f);

		// LUT
		Ref<Texture3D> LUTTexture = nullptr;
		uint32_t LUTID = 0; // Created by Client

		// Render Masks
		std::unordered_set<int> RenderMask;
		RenderMaskType MaskType = RenderMaskType::None;

		// Editor Settings
		bool FreezeFrustumUpdate = false;

		friend class SceneRenderer;
	};

}