#pragma once

#include "Dymatic/Renderer/Texture.h"

#include "Dymatic/Renderer/Camera.h"
#include "Dymatic/Renderer/EditorCamera.h"

#include "Dymatic/Scene/Components.h"

#include "Dymatic/Renderer/Framebuffer.h"

namespace Dymatic {

	class Renderer3D
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(const Camera& camera, const glm::mat4& transform);
		static void BeginScene(const EditorCamera& camera);
		static void EndScene();
		static void Flush();

		static void Resize();
		static void SetActiveFramebuffer(Ref<Framebuffer> framebuffer);

		static void UpdateTimestep(Timestep ts);

		static void SubmitModel(glm::mat4 transform, Ref<Model> model, int entityID = -1);
		static void SubmitModel(glm::mat4 transform, Ref<Model> model, Ref<Animator> animator, int entityID = -1);
		static void SubmitStaticMesh(const glm::mat4& transform, StaticMeshComponent& mesh, int entityID);
		static void RenderScene();

		static void SubmitDirectionalLight(const glm::mat4& transform, DirectionalLightComponent& lightComponent);
		static void SubmitPointLight(const glm::mat4& transform, PointLightComponent& lightComponent);
		static void SubmitSpotLight(const glm::mat4& transform, SpotLightComponent& lightComponent);
		static void SubmitLightSetup();

		static void SubmitSkyLight(SkyLightComponent& lightComponent);

		static void SetSelectedEntity(int id);

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