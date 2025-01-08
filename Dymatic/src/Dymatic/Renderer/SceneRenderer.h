#pragma once

#include "Dymatic/Renderer/Renderer.h"
#include "Dymatic/Renderer/RendererSpecification.h"
#include "Dymatic/Renderer/RendererTiering.h"
#include "Dymatic/Renderer/SceneRendererContext.h"

#include "Dymatic/Renderer/Texture.h"
#include "Dymatic/Renderer/Framebuffer.h"

#include "Dymatic/Renderer/Camera.h"
#include "Dymatic/Renderer/EditorCamera.h"

#include "Dymatic/Scene/Components.h"
#include "Dymatic/Renderer/ParticleSystemPlayer.h"

namespace Dymatic {
	
	struct SceneRendererSpecification
	{
		Tiering::Renderer::ShadowResolution ShadowResolution = Tiering::Renderer::ShadowResolution::Medium;
	};

	class SceneRenderer
	{		
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene();
		static void SubmitCamera(const Camera& camera, const glm::mat4& transform);
		static void SubmitCamera(const EditorCamera& camera);
		static void EndScene();
		
		static Ref<SceneRendererContext> GetActiveContext();
		static void SetActiveContext(Ref<SceneRendererContext> context);
		static RendererConstants::AntiAliasingMode GetAntiAliasingMode();
		static void SetAntiAliasingMode(const RendererConstants::AntiAliasingMode mode);

		static void UpdateTimestep(Timestep ts);

		static void RenderScene();

		static void SubmitModel(const glm::mat4& transform, Ref<Model> model, int entityID = -1, bool selected = false);
		static void SubmitModel(const glm::mat4& transform, Ref<Model> model, Ref<MaterialAsset> material, int entityID = -1, bool selected = false);
		static void SubmitModel(const glm::mat4& transform, Ref<Model> model, const std::vector<Ref<MaterialAsset>>& materials, Ref<BoneMatrixList> pose, Ref<BlendShapeWeightList> blendShapeWeights, int entityID = -1, bool selected = false);
		static void DrawMeshOutlineOverlay(const glm::mat4& transform, Ref<Model> model, const glm::vec4& color, int entityID = -1);

		static void SubmitDirectionalLight(const glm::vec3& rotation, const DirectionalLightComponent& lightComponent);
		static void SubmitPointLight(const glm::vec3& translation, PointLightComponent& lightComponent);
		static void SubmitSpotLight(const Transform& transform, SpotLightComponent& lightComponent);
		static void SubmitLightSetup();
		static void SubmitSkyLight(const SkyLightComponent& lightComponent);

		static void SubmitParticleSystem(const glm::mat4& transform, Ref<ParticleSystemPlayer> particleSystem, Ref<MaterialAsset> material, int entityID = -1);

		static void SubmitDecal(const glm::mat4& transform, Ref<Texture2D> texture, bool constrainAngle, int entityID = -1);

		static void SubmitVolume(const Transform& transform, const VolumeComponent& volumeCompontent, int entityID = -1);

		static void SubmitPostProcessVolume(const glm::mat4& transform, PostProcessVolumeComponent& volumeComponent, int entityID = -1);

		// Stats
		struct Statistics
		{
			uint32_t DrawCalls = 0;
		};
		static void ResetStats();
		static Statistics GetStats();

		// Queries (Note: If we have more properties being accessed by Context objects here in the future, move into a single queryable object)
		static float GetVXGIRenderScale();

		// Editor Only
		static void SubmitDrawShaderOverride(Ref<Shader> shader);
		static void OnImGuiRender();

	private:
		static void UpdateSkyLight();
		static void UpdateLUT(const Ref<Texture2D> lutMap);
	};

}