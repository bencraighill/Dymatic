#include "Panels/Assets/Utils/MaterialPanelViewport.h"

#include "Dymatic/Renderer/SceneRenderer.h"
#include "EditorResources.h"

namespace Dymatic {

	MaterialPanelViewport::MaterialPanelViewport()
	{
		// Setup camera
		EditorCamera& camera = GetEditorCamera();
		camera.SetFirstPersonEnabled(false);
		camera.SetOrbitRequireAlt(false);
		camera.SetDistance(4.5f);
		camera.SetYaw(-0.6672f);
		camera.SetPitch(0.06960005f);
	}

	void MaterialPanelViewport::BeginViewportRender(Timestep ts, const Ref<MaterialAsset> material)
	{
		AssetPanelViewport::BeginViewportRender(ts);

		static const glm::mat4 FloorTransform = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.1f, 0.0f)), glm::vec3(10.0f, 0.1f, 10.0f));

		const bool postProcessing = material->GetProperties().Usage == MaterialAsset::MaterialUsage::PostProcessing;
		SceneRenderer::SubmitModel(FloorTransform, EditorResources::CubeMesh, EditorResources::BasicMaterial);
		SceneRenderer::SubmitModel(glm::mat4(1.0f), EditorResources::SphereMesh, postProcessing ? nullptr : material);

		if (postProcessing)
		{
			PostProcessVolumeComponent ppvc;
			ppvc.Material = material;
			ppvc.Bounded = false;
			SceneRenderer::SubmitPostProcessVolume(glm::mat4(1.0f), ppvc);
		}

		AssetPanelViewport::SubmitViewportRender();
		AssetPanelViewport::EndViewportRender();
	}

}