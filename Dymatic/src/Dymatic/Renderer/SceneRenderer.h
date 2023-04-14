#pragma once

#include "Dymatic/Renderer/Texture.h"

#include "Dymatic/Renderer/Camera.h"
#include "Dymatic/Renderer/EditorCamera.h"

#include "Dymatic/Scene/Components.h"

#include "Dymatic/Renderer/Framebuffer.h"

namespace Dymatic {

	class SceneRenderer
	{
	public:
		enum class RendererVisualizationMode
		{
			Rendered = 0, Wireframe, LightingOnly, PrePostProcessing,
			Albedo, Depth, EntityID, Normal, Emissive, Roughness, Metallic, Specular, AmbientOcclusion
		};
		
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(const Camera& camera, const glm::mat4& transform);
		static void BeginScene(const EditorCamera& camera);
		static void EndScene();

		static void Resize();
		static void SetActiveFramebuffer(Ref<Framebuffer> framebuffer);
		static RendererVisualizationMode GetVisualizationMode();
		static void SetVisualizationMode(RendererVisualizationMode visualizationMode); 

		static void UpdateTimestep(Timestep ts);

		static void SubmitModel(const glm::mat4& transform, Ref<Model> model, int entityID = -1, bool selected = false);
		static void SubmitModel(const glm::mat4& transform, Ref<Model> model, const std::vector<Ref<Material>>& materials, Ref<Animator> animator, int entityID = -1, bool selected = false);
		static void SubmitStaticMesh(const glm::mat4& transform, const StaticMeshComponent& mesh, int entityID = -1, bool selected = false);
		static void RenderScene();
		static void DrawMeshOutlineOverlay(const glm::mat4& transform, Ref<Model> model, const glm::vec4& color, int entityID = -1);

		static void SubmitDirectionalLight(const glm::mat4& transform, DirectionalLightComponent& lightComponent);
		static void SubmitPointLight(const glm::mat4& transform, PointLightComponent& lightComponent);
		static void SubmitSpotLight(const glm::mat4& transform, SpotLightComponent& lightComponent);
		static void SubmitLightSetup();
		static void SubmitSkyLight(SkyLightComponent& lightComponent);

		static void SubmitVolume(TransformComponent& transform, VolumeComponent& volumeCompontent);

		// Stats
		struct Statistics
		{
			uint32_t DrawCalls = 0;
		};
		static void ResetStats();
		static Statistics GetStats();

		// Editor Only
		static void OnImGuiRender();

	private:
		static void UpdateSkyLight();
	};

}