#pragma once

#include "Thumbnails/ThumbnailManager.h"
#include "Dymatic/Asset/AssetManager.h"

#include "Dymatic/Scene/Entity.h"
#include "Dymatic/Renderer/Framebuffer.h"

#include "EditorResources.h"
#include "Dymatic/Renderer/Renderer.h"
#include "Dymatic/Renderer/SceneRenderer.h"
#include "Dymatic/Renderer/Renderer2D.h"

namespace Dymatic {

	static const uint32_t s_ThumbnailSize = 128;

	class ThumbnailGenerator
	{
	public:
		virtual Ref<Texture2D> GenerateThumbnail(AssetHandle handle) = 0;
	};

	class SceneThumbnailGenerator : public ThumbnailGenerator
	{
	public:
		SceneThumbnailGenerator();
		virtual void OnRender() const = 0;
	protected:
		Ref<Texture2D> RenderSceneThumbnail();
	protected:
		// One scene renderer context shared by all scene based thumbnail generators
		static Ref<SceneRendererContext> s_SceneRendererContext;
		
		SceneCamera m_Camera;
		glm::mat4 m_CameraTransform;
	};

	class TextureThumbnailGenerator : public ThumbnailGenerator
	{
	public:
		TextureThumbnailGenerator();
		virtual Ref<Texture2D> GenerateThumbnail(AssetHandle handle) override;
	private:
		Ref<Framebuffer> m_Framebuffer;
	};

	class FontThumbnailGenerator : public ThumbnailGenerator
	{
	public:
		FontThumbnailGenerator();
		virtual Ref<Texture2D> GenerateThumbnail(AssetHandle handle) override;
	private:
		Ref<Framebuffer> m_Framebuffer;
		SceneCamera m_Camera;
		glm::mat4 m_CameraTransform;
	};

	class VideoThumbnailGenerator : public ThumbnailGenerator
	{
	public:
		VideoThumbnailGenerator() = default;
		virtual Ref<Texture2D> GenerateThumbnail(AssetHandle handle) override;
	};

	class MaterialThumbnailGenerator : public SceneThumbnailGenerator
	{
	public:
		MaterialThumbnailGenerator();
		virtual Ref<Texture2D> GenerateThumbnail(AssetHandle handle) override;
	private:
		virtual void OnRender() const override;
	private:
		Ref<MaterialAsset> m_Material;
	};

	class MeshThumbnailGenerator : public SceneThumbnailGenerator
	{
	public:
		MeshThumbnailGenerator();
		virtual Ref<Texture2D> GenerateThumbnail(AssetHandle handle) override;
	private:
		virtual void OnRender() const override;
	private:
		Ref<Model> m_Mesh;
		glm::mat4 m_DirectionalLightTransform;
	};

}