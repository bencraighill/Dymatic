#include "Thumbnails/ThumbnailGenerators.h"

#include "Dymatic/Scene/Transform.h"

#include "Dymatic/Video/Video.h"
#include "Dymatic/Video/VideoReader.h"

namespace Dymatic {

	Ref<SceneRendererContext> SceneThumbnailGenerator::s_SceneRendererContext = nullptr;

	SceneThumbnailGenerator::SceneThumbnailGenerator()
	{
		if (!s_SceneRendererContext)
			s_SceneRendererContext = SceneRendererContext::Create(glm::vec2(s_ThumbnailSize, s_ThumbnailSize));

		m_Camera.SetViewportSize(s_ThumbnailSize, s_ThumbnailSize);
	}

	Ref<Texture2D> SceneThumbnailGenerator::RenderSceneThumbnail()
	{
		// Thumbnail generations occurs over one frame so we cannot use any form of temporal anti-aliasing
		const RendererConstants::AntiAliasingMode originalMode = SceneRenderer::GetAntiAliasingMode();
		SceneRenderer::SetAntiAliasingMode(RendererConstants::AntiAliasingMode::FXAA);

		// Bind, Render, copy the data and then Unbind
		s_SceneRendererContext->ActiveFramebuffer->Bind();
		RenderCommand::SetClearColor(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		RenderCommand::Clear();
		SceneRenderer::SetActiveContext(s_SceneRendererContext);
		SceneRenderer::BeginScene();
		SceneRenderer::SubmitCamera(m_Camera, m_CameraTransform);
		OnRender();
		SkyLightComponent slc;
		slc.Intensity = 0.25f;
		slc.EnvironmentMap = EditorResources::DefaultEnvironmentMap;
		slc.FlowMap = EditorResources::DefaultSkyFlowMap;
		SceneRenderer::SubmitSkyLight(slc);

		SceneRenderer::RenderScene();
		SceneRenderer::EndScene();

		// Restore the original anti-aliasing mode
		SceneRenderer::SetAntiAliasingMode(originalMode);

		// Create the thumbnail texture
		TextureSpecification textureSpecification;
		textureSpecification.Width = s_ThumbnailSize;
		textureSpecification.Height = s_ThumbnailSize;
		textureSpecification.Format = TextureFormat::RGBA8;
		Ref<Texture2D> thumbnail = Texture2D::Create(textureSpecification);

		// Copy and convert to RGBA8
		s_SceneRendererContext->ActiveFramebuffer->CopyColor(thumbnail);

		s_SceneRendererContext->ActiveFramebuffer->Unbind();

		// Once we return the reader will be destroyed, hence the thumbnail manager will assume ownership of the frame texture
		return thumbnail;
	}

	TextureThumbnailGenerator::TextureThumbnailGenerator()
	{
		FramebufferSpecification specification;
		specification.Width = s_ThumbnailSize;
		specification.Height = s_ThumbnailSize;
		specification.Attachments = { TextureFormat::RGBA8, TextureFormat::Depth };
		m_Framebuffer = Framebuffer::Create(specification);
	}

	Ref<Texture2D> TextureThumbnailGenerator::GenerateThumbnail(AssetHandle handle)
	{
		// Load the texture and render it
		Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(handle);

		// Bind, Render, copy the data and then Unbind
		m_Framebuffer->Bind();
		RenderCommand::SetClearColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		RenderCommand::Clear();
		Renderer::DrawFullscreenTexture(texture);
		auto& specification = m_Framebuffer->GetSpecification();
		Buffer thumbnailData = m_Framebuffer->CopyColorBuffer(0);
		m_Framebuffer->Unbind();

		// Create a texture with the copied buffer.
		TextureSpecification thumbnailSpecification;
		thumbnailSpecification.Width = specification.Width;
		thumbnailSpecification.Height = specification.Height;
		Ref<Texture2D> thumbnail = Texture2D::Create(thumbnailSpecification, thumbnailData);

		return thumbnail;
	}

	FontThumbnailGenerator::FontThumbnailGenerator()
	{
		FramebufferSpecification specification;
		specification.Width = s_ThumbnailSize;
		specification.Height = s_ThumbnailSize;
		specification.Attachments = { TextureFormat::RGBA8, TextureFormat::Depth };
		m_Framebuffer = Framebuffer::Create(specification);

		m_Camera.SetViewportSize(s_ThumbnailSize, s_ThumbnailSize);
		m_Camera.SetOrthographic(2.0f, 0.0f, 1.0f);
		m_CameraTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f));
	}

	Ref<Texture2D> FontThumbnailGenerator::GenerateThumbnail(AssetHandle handle)
	{
		// Load the font
		Ref<Font> font = AssetManager::GetAsset<Font>(handle);

		// Bind, Render, copy the data and then Unbind
		m_Framebuffer->Bind();
		RenderCommand::SetClearColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		RenderCommand::Clear();
		Renderer2D::BeginScene(m_Camera, m_CameraTransform);
		Renderer2D::DrawText(glm::mat4(1.0f), "Aa", TextAlignment::Center, font, glm::vec4(1.0f));
		Renderer2D::EndScene();
		auto& specification = m_Framebuffer->GetSpecification();
		Buffer thumbnailData = m_Framebuffer->CopyColorBuffer(0);
		m_Framebuffer->Unbind();

		// Create a texture with the copied buffer.
		TextureSpecification thumbnailSpecification;
		thumbnailSpecification.Width = specification.Width;
		thumbnailSpecification.Height = specification.Height;
		Ref<Texture2D> thumbnail = Texture2D::Create(thumbnailSpecification, thumbnailData);
		thumbnailData.Release();

		return thumbnail;
	}

	Ref<Texture2D> VideoThumbnailGenerator::GenerateThumbnail(AssetHandle handle)
	{
		VideoReaderSpecification videoReaderSpecification;
		videoReaderSpecification.VideoStream = AssetManager::GetAsset<Video>(handle);
		videoReaderSpecification.Width = s_ThumbnailSize;
		videoReaderSpecification.Height = s_ThumbnailSize;
		Ref<VideoReader> videoReader = CreateRef<VideoReader>(videoReaderSpecification);

		videoReader->GetNextFrame(0.0f);
		Ref<Texture2D> frame = videoReader->GetNextFrame(1.0f);

		// Convert RGB8 to RGBA8
		Buffer frameBuffer = frame->GetData();
		Buffer thumbnailData(s_ThumbnailSize * s_ThumbnailSize * 4);
		for (uint8_t y = 0; y < s_ThumbnailSize; y++)
		{
			for (uint8_t x = 0; x < s_ThumbnailSize; x++)
			{
				uint32_t index = (y * s_ThumbnailSize + x) * 3;
				uint32_t newIndex = (y * s_ThumbnailSize + x) * 4;
				thumbnailData[newIndex + 0] = frameBuffer[index + 0];
				thumbnailData[newIndex + 1] = frameBuffer[index + 1];
				thumbnailData[newIndex + 2] = frameBuffer[index + 2];
				thumbnailData[newIndex + 3] = 255;
			}
		}
		frameBuffer.Release();

		// Create the thumbnail texture
		TextureSpecification textureSpecification;
		textureSpecification.Width = s_ThumbnailSize;
		textureSpecification.Height = s_ThumbnailSize;
		textureSpecification.Format = TextureFormat::RGBA8;
		Ref<Texture2D> thumbnail = Texture2D::Create(textureSpecification, thumbnailData);
		thumbnailData.Release();

		// Once we return the reader will be destroyed, hence the thumbnail manager will assume ownership of the frame texture
		return thumbnail;
	}

	MaterialThumbnailGenerator::MaterialThumbnailGenerator()
	{
		m_CameraTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 3.5f));
	}

	Ref<Texture2D> MaterialThumbnailGenerator::GenerateThumbnail(AssetHandle handle)
	{
		m_Material = AssetManager::GetAsset<MaterialAsset>(handle);
		Ref<Texture2D> thumbnail = RenderSceneThumbnail();
		m_Material = nullptr;
		return thumbnail;
	}

	void MaterialThumbnailGenerator::OnRender() const
	{
		const bool postProcessing = m_Material->GetProperties().Usage == MaterialAsset::MaterialUsage::PostProcessing;
		SceneRenderer::SubmitModel(glm::mat4(1.0f), EditorResources::SphereMesh, postProcessing ? nullptr : m_Material);

		if (postProcessing)
		{
			PostProcessVolumeComponent ppvc;
			ppvc.Bounded = false;
			ppvc.Material = m_Material;
			SceneRenderer::SubmitPostProcessVolume(glm::mat4(1.0f), ppvc);
		}
	}

	MeshThumbnailGenerator::MeshThumbnailGenerator()
	{
		Transform transform;
		transform.Translation = glm::vec3(1.5f, 1.0f, 1.5f);
		transform.SetRotationDegrees(glm::vec3(-25.0f, 45.0f, 0.0f));
		m_CameraTransform = transform.GetMatrix();
	}

	Ref<Texture2D> MeshThumbnailGenerator::GenerateThumbnail(AssetHandle handle)
	{
		m_Mesh = AssetManager::GetAsset<Model>(handle);
		Ref<Texture2D> thumbnail = RenderSceneThumbnail();
		m_Mesh = nullptr;
		return thumbnail;
	}

	void MeshThumbnailGenerator::OnRender() const
	{
		// Scale the mesh so it's bounding box fits inside the unit cube
		const AABB& aabb = m_Mesh->GetAABB();
		const glm::vec3 size = aabb.GetSize();
		const float maxLength = std::max({ size.x, size.y, size.z });
		const float scale = 1.0f / maxLength;

		// Position the mesh so it's closest AABB face aligns with the origin and other axes are center
		glm::vec3 translation = -aabb.GetCenter();
		//translation.z = -aabb.Max.z;

		const glm::mat4 transform = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(scale)), translation);

		// Render scaled mesh
		SceneRenderer::SubmitModel(transform, m_Mesh);

		// Additional lighting
		const DirectionalLightComponent dlc;
		SceneRenderer::SubmitDirectionalLight(glm::vec3(-90.0f, 45.0f, -90.0f), dlc);
	}

}