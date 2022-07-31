#pragma once
#include "Dymatic/Core/Base.h"

namespace Dymatic {

	class Sound
	{
	public:
		static Ref<Sound> Create(const std::string& path) { return CreateRef<Sound>(path); }

		Sound(const std::string& path);
		~Sound();

		// Engine Methods (Internal Use Only!)
		void SetPositionInternal(glm::vec3 position);
		void PlaySoundInternal();
		void OnBeginSceneInternal();
		void OnEndSceneInternal();
		inline bool IsInitalizedInternal() { return m_Initialized; }

		// Application Methods
		inline std::string GetSourcePath() { return m_SourcePath; }

		void Play();
		void Stop();

		inline bool Is3D() { return m_3D; }
		void SetIs3D(bool _3D);

		inline bool IsLooping() { return m_Looping; }
		void SetLooping(bool looping);

		inline bool GetStartOnAwake() { return m_StartOnAwake; }
		void SetStartOnAwake(bool startOnAwake);

		float GetVolume();
		void SetVolume(float volume);

		float GetPan();
		void SetPan(float pan);

		float GetSpeed();
		void SetSpeed(float speed);

		float GetRadius();
		void SetRadius(float radius);

		void SetEcho(bool echo);
		bool GetEcho();

		uint32_t GetPlayPosition();
		void SetPlayPosition(uint32_t position);
		inline uint32_t GetPlayLength() { return m_PlayLength; }

	private:
		std::string m_SourcePath;
		bool m_Initialized = false;

		uint32_t m_PlayLength = 0;

		glm::vec3 m_Position { 0.0f, 0.0f, 0.0f };

		uint32_t m_StartPosition = 0;
		bool m_StartOnAwake = false;
		bool m_Looping = false;
		bool m_3D = false;
		float m_Volume = 1.0f;
		float m_Pan = 0.0f;
		float m_Speed = 1.0f;
		bool m_Echo = false;
		float m_Radius = 1.0f;

		// Internal Sound Reference in irrKlang
		void* m_Sound = nullptr;
	};
}