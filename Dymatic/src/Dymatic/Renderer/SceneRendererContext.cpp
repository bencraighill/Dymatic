#include "dypch.h"
#include "Dymatic/Renderer/SceneRendererContext.h"

#include "Dymatic/Renderer/SceneRenderer.h"

namespace Dymatic {

	const char* SceneRendererContext::RenderVisualizationModeToString(const RendererVisualizationMode mode)
	{
		switch (mode)
		{
		case SceneRendererContext::RendererVisualizationMode::Invalid:				return "Invalid";
		case SceneRendererContext::RendererVisualizationMode::Rendered:				return "Rendered";
		case SceneRendererContext::RendererVisualizationMode::Wireframe:			return "Wireframe";
		case SceneRendererContext::RendererVisualizationMode::LightingOnly:			return "Lighting Only";
		case SceneRendererContext::RendererVisualizationMode::PrePostProcessing:	return "Pre Post Processing";
		case SceneRendererContext::RendererVisualizationMode::PathTraced:			return "Path Traced";
		case SceneRendererContext::RendererVisualizationMode::Albedo:				return "Albedo";
		case SceneRendererContext::RendererVisualizationMode::Depth:				return "Depth";
		case SceneRendererContext::RendererVisualizationMode::LinearDepth:			return "Linear Depth";
		case SceneRendererContext::RendererVisualizationMode::Position:				return "Position";
		case SceneRendererContext::RendererVisualizationMode::Normal:				return "Normal";
		case SceneRendererContext::RendererVisualizationMode::Emissive:				return "Emissive";
		case SceneRendererContext::RendererVisualizationMode::Roughness:			return "Roughness";
		case SceneRendererContext::RendererVisualizationMode::Metallic:				return "Metallic";
		case SceneRendererContext::RendererVisualizationMode::Specular:				return "Specular";
		case SceneRendererContext::RendererVisualizationMode::AmbientOcclusion:		return "Ambient Occlusion";
		case SceneRendererContext::RendererVisualizationMode::Velocity:				return "Velocity";
		case SceneRendererContext::RendererVisualizationMode::EntityID:				return "Entity ID";
		case SceneRendererContext::RendererVisualizationMode::SubmeshIndex:			return "Submesh Index";
		}

		DY_CORE_ASSERT(false, "Unknown render visualization mode");
		return {};
	}

	SceneRendererContext::RendererVisualizationMode SceneRendererContext::RenderVisualizationModeFromString(const std::string& modeString)
	{
		if (modeString == "Invalid")				return SceneRendererContext::RendererVisualizationMode::Invalid;
		if (modeString == "Rendered")				return SceneRendererContext::RendererVisualizationMode::Rendered;
		if (modeString == "Wireframe")				return SceneRendererContext::RendererVisualizationMode::Wireframe;
		if (modeString == "Lighting Only")			return SceneRendererContext::RendererVisualizationMode::LightingOnly;
		if (modeString == "Pre Post Processing")	return SceneRendererContext::RendererVisualizationMode::PrePostProcessing;
		if (modeString == "Path Traced")			return SceneRendererContext::RendererVisualizationMode::PathTraced;
		if (modeString == "Albedo")					return SceneRendererContext::RendererVisualizationMode::Albedo;
		if (modeString == "Depth")					return SceneRendererContext::RendererVisualizationMode::Depth;
		if (modeString == "Linear Depth")			return SceneRendererContext::RendererVisualizationMode::LinearDepth;
		if (modeString == "Position")				return SceneRendererContext::RendererVisualizationMode::Position;
		if (modeString == "Normal")					return SceneRendererContext::RendererVisualizationMode::Normal;
		if (modeString == "Emissive")				return SceneRendererContext::RendererVisualizationMode::Emissive;
		if (modeString == "Roughness")				return SceneRendererContext::RendererVisualizationMode::Roughness;
		if (modeString == "Metallic")				return SceneRendererContext::RendererVisualizationMode::Metallic;
		if (modeString == "Specular")				return SceneRendererContext::RendererVisualizationMode::Specular;
		if (modeString == "Ambient Occlusion")		return SceneRendererContext::RendererVisualizationMode::AmbientOcclusion;
		if (modeString == "Velocity")				return SceneRendererContext::RendererVisualizationMode::Velocity;
		if (modeString == "Entity ID")				return SceneRendererContext::RendererVisualizationMode::EntityID;
		if (modeString == "Submesh Index")			return SceneRendererContext::RendererVisualizationMode::SubmeshIndex;

		DY_CORE_ASSERT(false, "Unknown render visualization mode");
		return SceneRendererContext::RendererVisualizationMode::Rendered;
	}

	SceneRendererContext::SceneRendererContext(const glm::uvec2& viewportSize)
		: SceneRendererContext(viewportSize, SceneRendererSceneContext::Create())
	{}

	SceneRendererContext::SceneRendererContext(const glm::uvec2& viewportSize, const Ref<SceneRendererSceneContext> sceneContext)
		: SceneContext(sceneContext)
	{
		ActiveWidth = viewportSize.x;
		ActiveHeight = viewportSize.y;

		// Create the main framebuffer
		FramebufferSpecification framebufferSpecification;
		framebufferSpecification.Width = ActiveWidth;
		framebufferSpecification.Height = ActiveHeight;
		framebufferSpecification.BindlessAttachments = true;
		framebufferSpecification.Attachments = {
			TextureFormat::RGBA16F,			// Color
			TextureFormat::RED_INTEGER,		// EntityID
			TextureFormat::Depth,			// Depth
			TextureFormat::RGBA16F,			// Normal
			TextureFormat::RGBA16F,			// Emissive
			TextureFormat::RGBA8,			// Roughness + Metallic + Specular + AO
			TextureFormat::RED_INTEGER,		// Submesh Index
			TextureFormat::RG16F			// Velocity
		};
		
		// Main framebuffers
		ActiveFramebuffer = Framebuffer::Create(framebufferSpecification);

		// Render Pass Framebuffers
		FramebufferSpecification fbSpec;
		fbSpec.Attachments = { TextureFormat::RGBA16F, TextureFormat::Depth };
		fbSpec.Width = ActiveWidth;
		fbSpec.Height = ActiveHeight;

		// Setup the deferred renderer context
		DeferredLightingFramebuffer = Framebuffer::Create(fbSpec);

		// Forward Shading
		TranslucentLightingFramebuffer = Framebuffer::Create(fbSpec);

		// Volumetric Lighting
		VolumetricLightingFramebuffer = Framebuffer::Create(fbSpec);
		VolumetricBlurFramebuffer = Framebuffer::Create(fbSpec);
		VolumetricLightingCompositeFramebuffer = Framebuffer::Create(fbSpec);

		// Volumetric Clouds
		VolumetricCloudsFramebuffer = Framebuffer::Create(fbSpec);
		
		// General Post Processing Effects
		SSAOFramebuffer = Framebuffer::Create(fbSpec);
		
		PreviousSSRFrame = Framebuffer::Create(fbSpec);
		SSRFramebuffer = Framebuffer::Create(fbSpec);

		FXAAFramebuffer = Framebuffer::Create(fbSpec);

		MotionBlurFramebuffer = Framebuffer::Create(fbSpec);

		DOFFramebuffer = Framebuffer::Create(fbSpec);
		LensFlareFramebuffer = Framebuffer::Create(fbSpec);
		LensDistortionFramebuffer = Framebuffer::Create(fbSpec);

		PostProcessVolumeFramebuffer = Framebuffer::Create(fbSpec);

		// Bloom
		BloomBrightIsolated = Framebuffer::Create(fbSpec);
		BloomFramebufferAddA = Framebuffer::Create(fbSpec);
		BloomFramebufferAddB = Framebuffer::Create(fbSpec);
		BloomFramebuffer = Framebuffer::Create(fbSpec);

		// Bloom downsample Framebuffers
		{
			FramebufferSpecification fbspec;
			fbspec.Attachments = { TextureFormat::RGBA16F };
			fbspec.Width = ActiveWidth / 2;
			fbspec.Height = ActiveHeight / 2;

			for (uint32_t i = 0; i < RendererConstants::NumBloomDownsamples; i += 2)
			{
				BloomDownsampleFramebuffers[i] = Framebuffer::Create(fbspec);
				BloomDownsampleFramebuffers[i + 1] = Framebuffer::Create(fbspec);
				fbspec.Width /= 2;
				fbspec.Height /= 2;
			}
		}

		// Outline
		{
			FramebufferSpecification fbSpec;
			fbSpec.Attachments = { TextureFormat::Depth };
			fbSpec.Width = ActiveWidth * 2; // TODO: Do we really need a double size framebuffer
			fbSpec.Height = ActiveHeight * 2;
			OutlineFramebuffer = Framebuffer::Create(fbSpec);
		}

		// Path Tracing
		{
			FramebufferSpecification fbSpec;
			fbSpec.Width = ActiveWidth;
			fbSpec.Height = ActiveHeight;

			fbSpec.Attachments = {
				TextureFormat::RGBA16F,		// Color
				TextureFormat::RED_INTEGER,	// EntityID
				TextureFormat::RGBA16F,		// Normal
				TextureFormat::R32F			// Depth
			}; 
			PathTracingFramebuffer = Framebuffer::Create(fbSpec);

			fbSpec.Attachments = { TextureFormat::RGBA16F };
			PathTracingAccumulateFramebuffers[0] = Framebuffer::Create(fbSpec);
			PathTracingAccumulateFramebuffers[1] = Framebuffer::Create(fbSpec);
		}

		// VXGI
		{
			FramebufferSpecification fbSpec;
			fbSpec.Width = ActiveWidth;
			fbSpec.Height = ActiveHeight;
			fbSpec.Attachments = { TextureFormat::RGBA16F };

			VXGIFramebuffer = Framebuffer::Create(fbSpec);
			SSGIFramebuffer = Framebuffer::Create(fbSpec);
			TAAFramebuffer = Framebuffer::Create(fbSpec);
			TAAHistoryColorFramebuffer = Framebuffer::Create(fbSpec);
		}
	}

	void SceneRendererContext::Resize(const glm::uvec2& size)
	{
		Resize(size.x, size.y);
	}

	void SceneRendererContext::Resize(const uint32_t width, const uint32_t height)
	{
		ActiveWidth = width;
		ActiveHeight = height;

		// Main Framebuffers
		ActiveFramebuffer->Resize(width, height);

		// Render Pass Framebuffers
		DeferredLightingFramebuffer->Resize(width, height);
		TranslucentLightingFramebuffer->Resize(width, height);

		VolumetricLightingFramebuffer->Resize(width, height);
		VolumetricBlurFramebuffer->Resize(width, height);
		VolumetricLightingCompositeFramebuffer->Resize(width, height);

		SSAOFramebuffer->Resize(width, height);
		PreviousSSRFrame->Resize(width, height);
		SSRFramebuffer->Resize(width, height);
		FXAAFramebuffer->Resize(width, height);
		MotionBlurFramebuffer->Resize(width, height);
		LensFlareFramebuffer->Resize(width, height);

		PostProcessVolumeFramebuffer->Resize(width, height);

		DOFFramebuffer->Resize(width, height);
		LensDistortionFramebuffer->Resize(width, height);
		VolumetricCloudsFramebuffer->Resize(width, height);

		// Outline
		OutlineFramebuffer->Resize(width * 2, height * 2); // TODO: Do we really need a double size framebuffer?

		// Path Tracing
		PathTracingFramebuffer->Resize(width, height);
		PathTracingAccumulateFramebuffers[0]->Resize(width, height);
		PathTracingAccumulateFramebuffers[1]->Resize(width, height);

		const float scale = SceneRenderer::GetVXGIRenderScale();
		VXGIFramebuffer->Resize(width * scale, height * scale);
		
		SSGIFramebuffer->Resize(width, height);
		TAAFramebuffer->Resize(width, height);
		TAAHistoryColorFramebuffer->Resize(width, height);

		// Bloom
		BloomFramebufferAddA->Resize(width, height);
		BloomFramebufferAddB->Resize(width, height);
		BloomBrightIsolated->Resize(width, height);
		BloomFramebuffer->Resize(width, height);
		{
			uint32_t downsampleWidth = width, downsampleHeight = height;
			for (uint32_t i = 0; i < RendererConstants::NumBloomDownsamples; i += 2)
			{
				BloomDownsampleFramebuffers[i]->Resize(downsampleWidth, downsampleHeight);
				BloomDownsampleFramebuffers[i + 1]->Resize(downsampleWidth, downsampleHeight);
				downsampleWidth /= 2;
				downsampleHeight /= 2;
			}
		}
	}

	void SceneRendererContext::SwapBuffers()
	{
		// This introduces flickering artifacts when resizing the viewport window
		//std::swap(ActiveFramebuffer, PreviousFramebuffer);
	}

	SceneRendererSceneContext::SceneRendererSceneContext()
	{
		// Setup PBR IBL Cubemap
		TextureSpecification cubemapSpecification;
		cubemapSpecification.Format = TextureFormat::RGB16F;

		cubemapSpecification.Width = 512;
		cubemapSpecification.Height = 512;
		cubemapSpecification.Depth = 1;
		EnvironmentCubemap = TextureCube::Create(cubemapSpecification);
		FlowMapCubemap = TextureCube::Create(cubemapSpecification);

		cubemapSpecification.Width = 32;
		cubemapSpecification.Height = 32;
		IrradianceMap = TextureCube::Create(cubemapSpecification);

		cubemapSpecification.Width = 128;
		cubemapSpecification.Height = 128;
		cubemapSpecification.Depth = RendererConstants::MaxSkyboxMipLevels;
		PrefilterMap = TextureCube::Create(cubemapSpecification);

		TextureSpecification brdfLUTSpecification;
		brdfLUTSpecification.Format = TextureFormat::RG16F;
		brdfLUTSpecification.Width = 512;
		brdfLUTSpecification.Height = 512;
		brdfLUTTexture = Texture2D::Create(brdfLUTSpecification);
	}

}