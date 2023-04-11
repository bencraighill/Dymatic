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

	void AudioEngine::Update()
	{
		if (!g_AudioEngineInitialized)
			return;

		g_AudioEngine->update();
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

		g_AudioEngineInitialized = false;
		g_AudioEngine = nullptr;
	}

	static float s_GlobalVolume;

	void AudioEngine::SetGlobalVolume(float volume)
	{
		if (!g_AudioEngineInitialized)
			return;

		s_GlobalVolume = volume;

		g_AudioEngine->setSoundVolume(volume);
	}

	float AudioEngine::GetGlobalVolume()
	{
		if (!g_AudioEngineInitialized)
			return 0.0f;

		return s_GlobalVolume;
	}

}