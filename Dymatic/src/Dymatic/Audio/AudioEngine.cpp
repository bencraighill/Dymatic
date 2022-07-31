#include "dypch.h"
#include "Dymatic/Audio/AudioEngine.h"

#include <irrKlang/irrKlang.h>

namespace Dymatic {

	using namespace irrklang;
	bool g_AudioEngineInitialized = false;
	ISoundEngine* g_AudioEngine;

	void AudioEngine::Init()
	{
		// Initialize Audio Engine
		g_AudioEngine = createIrrKlangDevice();

		if (!g_AudioEngine)
			DY_CORE_ERROR("Could not initalize Audio Engine");
		else
			g_AudioEngineInitialized = true;
	}

	void AudioEngine::Update(glm::vec3 listener_pos, glm::vec3 listener_direction)
	{
		if (!g_AudioEngineInitialized)
			return;

		g_AudioEngine->update();
		g_AudioEngine->setListenerPosition({ listener_pos.x, listener_pos.y, listener_pos.z }, { listener_direction.x, listener_direction.y, listener_direction.z });
	}

	void AudioEngine::Shutdown()
	{
		if (!g_AudioEngineInitialized)
			return;

		g_AudioEngine->drop();
	}

}