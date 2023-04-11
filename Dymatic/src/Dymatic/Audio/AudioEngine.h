#pragma once
#include "Dymatic/Audio/Audio.h"

namespace Dymatic {

	class AudioEngine
	{
	public:
		static void Init();
		static void Update();
		static void Update(glm::vec3 listenerPosition, glm::vec3 listenerDirection);
		static void Shutdown();

		static void SetGlobalVolume(float volume);
		inline static float GetGlobalVolume();
	};

}