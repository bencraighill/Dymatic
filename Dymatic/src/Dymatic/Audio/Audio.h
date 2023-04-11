#pragma once
#include "Dymatic/Core/Base.h"

#include "Dymatic/Asset/Asset.h"

namespace Dymatic {

	class Audio : public Asset
	{
	public:
		static AssetType GetStaticType() { return AssetType::Audio; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	public:
		static Ref<Audio> Create(const std::string& path) { return CreateRef<Audio>(path); }

		Audio(const std::string& path);
		~Audio();

		void Play(uint32_t startTime = 0);
		void Stop();
		bool GetPaused();
		void SetPaused(bool paused);

		inline bool Is3D() { return m_3D; }
		void SetIs3D(bool is3D);

		inline bool IsLooping() { return m_Looping; }
		void SetLooping(bool looping);

		float GetVolume();
		void SetVolume(float volume);

		float GetPan();
		void SetPan(float pan);

		float GetSpeed();
		void SetSpeed(float speed);

		float GetRadius();
		void SetRadius(float radius);

		bool GetEcho();
		void SetEcho(bool echo);

		uint32_t GetPlayPosition();
		void SetPlayPosition(uint32_t position);
		inline uint32_t GetPlayLength() { return m_PlayLength; }

		void SetPosition(glm::vec3 position);

		bool IsActive();

	private:
		std::string m_Path;

		uint32_t m_PlayLength = 0;

		glm::vec3 m_Position { 0.0f, 0.0f, 0.0f };
		
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