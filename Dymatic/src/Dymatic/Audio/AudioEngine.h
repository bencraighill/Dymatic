#pragma once
#include "Dymatic/Audio/Sound.h"

namespace Dymatic {

	class AudioEngine
	{
	public:
		static void Init();
		static void Update(glm::vec3 listener_pos, glm::vec3 listener_direction);
		static void Shutdown();
	};

}