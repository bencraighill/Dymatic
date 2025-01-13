#pragma once

#include "Dymatic/Core/UUID.h"

namespace Dymatic {

	class ScriptGlue
	{
	public:
		static void RegisterComponents();
		static void RegisterFunctions();
		
		static void SetOpenSceneCallback(const std::function<void(UUID)>& callback);

		static void OnRuntimeStart();
		static void OnRuntimeStop();
	};
	
}