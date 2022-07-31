#include "dypch.h"
#include "Dymatic/Audio/Sound.h"

#include <irrKlang/irrKlang.h>

namespace Dymatic {

#define CHECK_SOUND() if (m_Sound == nullptr) return;
#define CHECK_ENGINE()if (!g_AudioEngineInitialized) return;

	using namespace irrklang;
	extern bool g_AudioEngineInitialized;
	extern ISoundEngine* g_AudioEngine;

	Sound::Sound(const std::string& path)
	{
		CHECK_ENGINE();
		m_SourcePath = path;

		// Calculate Source Length
		auto temp_sound = g_AudioEngine->play2D(path.c_str(), false, true);
		m_PlayLength = temp_sound->getPlayLength();
		temp_sound->stop();
		temp_sound->drop();
	}

	Sound::~Sound()
	{
		CHECK_ENGINE();
		OnEndSceneInternal();
	}

	void Sound::OnBeginSceneInternal()
	{
		CHECK_ENGINE();

		m_Initialized = true;
		PlaySoundInternal();
	}

	void Sound::PlaySoundInternal()
	{
		CHECK_ENGINE();

		if (auto sound = (ISound*)m_Sound)
		{
			sound->stop();
			sound->drop();
			m_Sound = nullptr;
		}

		if (m_3D)
			m_Sound = g_AudioEngine->play3D(m_SourcePath.c_str(), { m_Position.x, m_Position.y, m_Position.z }, m_Looping, !m_StartOnAwake, true, {}, true);
		else
			m_Sound = g_AudioEngine->play2D(m_SourcePath.c_str(), m_Looping, !m_StartOnAwake, true, {}, true);

		((ISound*)m_Sound)->setPlayPosition(m_StartPosition);
		((ISound*)m_Sound)->setVolume(m_Volume);
		((ISound*)m_Sound)->setPan(m_Pan);
		((ISound*)m_Sound)->setPlaybackSpeed(m_Speed);
		((ISound*)m_Sound)->setMinDistance(m_Radius);
		if (m_Echo)
			((ISound*)m_Sound)->getSoundEffectControl()->enableEchoSoundEffect();
		else
			((ISound*)m_Sound)->getSoundEffectControl()->disableEchoSoundEffect();
	}

	void Sound::OnEndSceneInternal()
	{
		CHECK_ENGINE();
		m_Initialized = false;

		CHECK_SOUND();
		auto sound = ((ISound*)m_Sound);
		sound->stop();
		sound->drop();
		m_Sound = nullptr;
	}

	void Sound::SetPositionInternal(glm::vec3 position)
	{
		CHECK_ENGINE();
		CHECK_SOUND();

		auto sound = (ISound*)m_Sound;
		
		auto velocity = position - m_Position;
		sound->setVelocity({ velocity.x, velocity.y, velocity.z });

		m_Position = position;
		sound->setPosition({ position.x, position.y, position.z });
	}

	void Sound::SetIs3D(bool _3D)
	{
		CHECK_ENGINE();
		m_3D = _3D;

		if (auto sound = (ISound*)m_Sound)
		{
			auto play_pos = sound->getPlayPosition();
			auto paused = sound->getIsPaused();

			PlaySoundInternal();

			sound = (ISound*)m_Sound;
			sound->setPlayPosition(play_pos);
			sound->setIsPaused(paused);
		}
	}

	void Sound::SetLooping(bool looping)
	{
		CHECK_ENGINE();
		m_Looping = looping;

		CHECK_SOUND();
		auto sound = (ISound*)m_Sound;
		sound->setIsLooped(looping);
	}

	void Sound::SetStartOnAwake(bool startOnAwake)
	{
		CHECK_ENGINE();
		m_StartOnAwake = startOnAwake;
	}

	float Sound::GetVolume()
	{
		if (!g_AudioEngineInitialized) return 0;

		if (m_Sound)
			return ((ISound*)m_Sound)->getVolume();
		else
			return m_Volume;
	}

	void Sound::SetVolume(float volume)
	{
		CHECK_ENGINE();

		if (m_Sound)
			((ISound*)m_Sound)->setVolume(volume);
		else
			m_Volume = volume;
	}

	float Sound::GetPan()
	{
		if (!g_AudioEngineInitialized) return 0;

		if (m_Sound)
			return ((ISound*)m_Sound)->getPan();
		else
			return m_Pan;
	}

	void Sound::SetPan(float pan)
	{
		CHECK_ENGINE();

		if (m_Sound)
			((ISound*)m_Sound)->setPan(pan);
		else
			m_Pan = pan;
	}

	float Sound::GetSpeed()
	{
		if (!g_AudioEngineInitialized) return 0;

		if (m_Sound)
			return ((ISound*)m_Sound)->getPlaybackSpeed();
		else
			return m_Speed;
	}

	void Sound::SetSpeed(float speed)
	{
		CHECK_ENGINE();

		if (m_Sound)
			((ISound*)m_Sound)->setPlaybackSpeed(speed);
		else
			m_Speed = speed;
	}

	float Sound::GetRadius()
	{
		if (!g_AudioEngineInitialized) return 0;

		if (m_Sound)
			return ((ISound*)m_Sound)->getMinDistance();
		else
			return m_Radius;
	}

	void Sound::SetRadius(float radius)
	{
		CHECK_ENGINE();

		if (m_Sound)
			((ISound*)m_Sound)->setMinDistance(radius);
		else
			m_Radius = radius;
	}

	void Sound::SetEcho(bool echo)
	{
		CHECK_ENGINE();

		if (m_Sound)
			if (echo)
				((ISound*)m_Sound)->getSoundEffectControl()->enableEchoSoundEffect();
			else
				((ISound*)m_Sound)->getSoundEffectControl()->disableEchoSoundEffect();
		else
			m_Echo = echo;
	}

	bool Sound::GetEcho()
	{
		if (!g_AudioEngineInitialized) return false;

		if (m_Sound)
			return ((ISound*)m_Sound)->getSoundEffectControl()->isEchoSoundEffectEnabled();
		else
			return m_Echo;
	}

	uint32_t Sound::GetPlayPosition()
	{
		if (!g_AudioEngineInitialized) return 0;

		if (m_Sound)
			return ((ISound*)m_Sound)->getPlayPosition();
		else
			return m_StartPosition;
	}

	void Sound::SetPlayPosition(uint32_t position)
	{
		CHECK_ENGINE();

		if (m_Sound)
			((ISound*)m_Sound)->setPlayPosition(position);
		else
			m_StartPosition = position;
	} 

}