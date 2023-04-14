#include "dypch.h"
#include "Dymatic/Audio/Audio.h"

#include <irrKlang/irrKlang.h>

namespace Dymatic {

	using namespace irrklang;
	extern bool g_AudioEngineInitialized;
	extern ISoundEngine* g_AudioEngine;

	Audio::Audio(const std::string& path)
	{
		if (!g_AudioEngineInitialized) return;
		m_Path = path;

		// Calculate Source Length
		auto temp_sound = g_AudioEngine->play2D(path.c_str(), false, true);
		m_PlayLength = temp_sound->getPlayLength();
		temp_sound->stop();
		temp_sound->drop();
	}

	Audio::~Audio()
	{
		if (!g_AudioEngineInitialized) return;

		Stop();
	}

	void Audio::Play(uint32_t startTime)
	{
		if (!g_AudioEngineInitialized) return;

		if (auto sound = (ISound*)m_Sound)
		{
			sound->stop();
			sound->drop();
			m_Sound = nullptr;
		}

		if (m_3D)
			m_Sound = g_AudioEngine->play3D(m_Path.c_str(), { m_Position.x, m_Position.y, m_Position.z }, m_Looping, false, true, {}, true);
		else
			m_Sound = g_AudioEngine->play2D(m_Path.c_str(), m_Looping, false, true, {}, true);

		((ISound*)m_Sound)->setPlayPosition(startTime);
		((ISound*)m_Sound)->setVolume(m_Volume);
		((ISound*)m_Sound)->setPan(m_Pan);
		((ISound*)m_Sound)->setPlaybackSpeed(m_Speed);
		((ISound*)m_Sound)->setMinDistance(m_Radius);
		if (m_Echo)
			((ISound*)m_Sound)->getSoundEffectControl()->enableEchoSoundEffect();
		else
			((ISound*)m_Sound)->getSoundEffectControl()->disableEchoSoundEffect();
	}

	void Audio::Stop()
	{
		if (!g_AudioEngineInitialized) return;
		if (!m_Sound) return;

		ISound* sound = (ISound*)m_Sound;

		sound->stop();
		sound->drop();
		m_Sound = nullptr;
	}

	bool Audio::GetPaused()
	{
		if (!g_AudioEngineInitialized) return false;
		if (!m_Sound) return false;

		auto sound = (ISound*)m_Sound;
		return sound->getIsPaused();
	}

	void Audio::SetPaused(bool paused)
	{
		if (!g_AudioEngineInitialized) return;
		if (!m_Sound) return;

		auto sound = (ISound*)m_Sound;
		sound->setIsPaused(paused);
	}

	void Audio::SetPosition(glm::vec3 position)
	{
		if (!g_AudioEngineInitialized) return;
		if (m_Sound == nullptr) return;

		auto sound = (ISound*)m_Sound;
		
		auto velocity = position - m_Position;
		sound->setVelocity({ velocity.x, velocity.y, velocity.z });

		m_Position = position;
		sound->setPosition({ position.x, position.y, position.z });
	}

	bool Audio::IsActive()
	{
		return m_Sound;
	}

	void Audio::SetIs3D(bool is3D)
	{
		if (!g_AudioEngineInitialized) return;
		m_3D = is3D;

		if (auto sound = (ISound*)m_Sound)
		{
			auto play_pos = sound->getPlayPosition();
			auto paused = sound->getIsPaused();

			Play(play_pos);

			sound = (ISound*)m_Sound;
			sound->setIsPaused(paused);
		}
	}

	void Audio::SetLooping(bool looping)
	{
		if (!g_AudioEngineInitialized) return;
		m_Looping = looping;

		if (m_Sound == nullptr) return;
		auto sound = (ISound*)m_Sound;
		sound->setIsLooped(looping);
	}

	float Audio::GetVolume()
	{
		if (!g_AudioEngineInitialized) return 0;

		if (m_Sound)
			return ((ISound*)m_Sound)->getVolume();
		else
			return m_Volume;
	}

	void Audio::SetVolume(float volume)
	{
		if (!g_AudioEngineInitialized) return;

		if (m_Sound)
			((ISound*)m_Sound)->setVolume(volume);
		else
			m_Volume = volume;
	}

	float Audio::GetPan()
	{
		if (!g_AudioEngineInitialized) return 0;

		if (m_Sound)
			return ((ISound*)m_Sound)->getPan();
		else
			return m_Pan;
	}

	void Audio::SetPan(float pan)
	{
		if (!g_AudioEngineInitialized) return;

		if (m_Sound)
			((ISound*)m_Sound)->setPan(pan);
		else
			m_Pan = pan;
	}

	float Audio::GetSpeed()
	{
		if (!g_AudioEngineInitialized) return 0;

		if (m_Sound)
			return ((ISound*)m_Sound)->getPlaybackSpeed();
		else
			return m_Speed;
	}

	void Audio::SetSpeed(float speed)
	{
		if (!g_AudioEngineInitialized) return;

		if (m_Sound)
			((ISound*)m_Sound)->setPlaybackSpeed(speed);
		else
			m_Speed = speed;
	}

	float Audio::GetRadius()
	{
		if (!g_AudioEngineInitialized) return 0;

		if (m_Sound)
			return ((ISound*)m_Sound)->getMinDistance();
		else
			return m_Radius;
	}

	void Audio::SetRadius(float radius)
	{
		if (!g_AudioEngineInitialized) return;

		if (m_Sound)
			((ISound*)m_Sound)->setMinDistance(radius);
		else
			m_Radius = radius;
	}

	void Audio::SetEcho(bool echo)
	{
		if (!g_AudioEngineInitialized) return;

		if (m_Sound)
			if (echo)
				((ISound*)m_Sound)->getSoundEffectControl()->enableEchoSoundEffect();
			else
				((ISound*)m_Sound)->getSoundEffectControl()->disableEchoSoundEffect();
		else
			m_Echo = echo;
	}

	bool Audio::GetEcho()
	{
		if (!g_AudioEngineInitialized) return false;

		if (m_Sound)
			return ((ISound*)m_Sound)->getSoundEffectControl()->isEchoSoundEffectEnabled();
		else
			return m_Echo;
	}

	uint32_t Audio::GetPlayPosition()
	{
		if (!g_AudioEngineInitialized) return 0;

		if (m_Sound)
			return ((ISound*)m_Sound)->getPlayPosition();
		return 0;
	}

	void Audio::SetPlayPosition(uint32_t position)
	{
		if (!g_AudioEngineInitialized) return;

		if (m_Sound)
			((ISound*)m_Sound)->setPlayPosition(position);
	} 

}