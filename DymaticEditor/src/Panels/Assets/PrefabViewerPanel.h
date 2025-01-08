#pragma once
#include "Panels/EditorPanel.h"

#include "Dymatic/Scene/Prefab.h"
#include "Dymatic/Scene/Scene.h"

namespace Dymatic {

	class PrefabViewerPanel : public EditorPanel
	{
	public:
		PrefabViewerPanel(Ref<Prefab> prefab);

		virtual void OnUpdate(Timestep ts);
		virtual void OnImGuiRender(bool& open) override;

	private:
		Ref<Prefab> m_Prefab;
		Ref<Scene> m_Scene;
	};

}