#pragma once

#include "Dymatic/Core/TransactionManager.h"

#include "Dymatic/Scene/Scene.h"

namespace Dymatic {

	class SceneTransaction : public Transaction
	{
	public:
		SceneTransaction(const std::string& name, Ref<Scene> originalScene, Ref<Scene> currentScene)
			: m_Name(name), m_OriginalScene(originalScene), m_CurrentScene(currentScene)
		{
			m_ModifiedScene = Scene::Copy(m_OriginalScene);
		}
		
		virtual void Execute() override
		{
			m_CurrentScene = m_ModifiedScene;
		}

	private:
		std::string m_Name;
		Ref<Scene> m_OriginalScene;
		Ref<Scene> m_ModifiedScene;

		// Reference to the actual scene that is in use by the editor.
		Ref<Scene>& m_CurrentScene;
	};

}