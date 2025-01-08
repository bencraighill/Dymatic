#pragma once
#include "Dymatic/Renderer/Camera.h"

#include "Dymatic/Renderer/Texture.h"

namespace Dymatic {

	class SceneCamera : public Camera
	{
	public:
		enum class ProjectionType
		{
			Perspective = 0,
			Orthographic = 1
		};

		struct CameraSettings
		{
			// DOF
			float DOFStrength = 0.0f;
			float DOFTarget = 0.0f;
			float DOFFocusRange = 1.0f;
			float DOFFocusFalloff = 0.0f;

			// Bloom
			Ref<Texture2D> BloomDirtTexture = nullptr;
			float BloomThreshold = 2.0f;

			// LUT
			Ref<Texture2D> LUT = nullptr;
		};

	public:
		SceneCamera();
		virtual ~SceneCamera() = default;

		void SetPerspective(float verticalFOV, float nearClip, float farClip);
		void SetOrthographic(float size, float nearClip, float farClip);

		void SetViewportSize(uint32_t width, uint32_t height);

		virtual float GetNearClip() const override { return GetPerspectiveNearClip(); }
		virtual float GetFarClip() const override { return GetPerspectiveFarClip(); }
		virtual float GetFOV() const override { return m_PerspectiveFOV; }

		float GetPerspectiveVerticalFOV() const { return m_PerspectiveFOV; }
		void SetPerspectiveVerticalFOV(float verticalFov) { m_PerspectiveFOV = verticalFov; RecalculateProjection(); }
		float GetPerspectiveNearClip() const { return m_PerspectiveNear; }
		void SetPerspectiveNearClip(float nearClip) { m_PerspectiveNear = nearClip; RecalculateProjection(); }
		float GetPerspectiveFarClip() const { return m_PerspectiveFar; }
		void SetPerspectiveFarClip(float farClip) { m_PerspectiveFar = farClip; RecalculateProjection(); }

		float GetOrthographicSize() const { return m_OrthographicSize; }
		void SetOrthographicSize(float size) { m_OrthographicSize = size; RecalculateProjection(); }
		float GetOrthographicNearClip() const { return m_OrthographicNear; }
		void SetOrthographicNearClip(float nearClip) { m_OrthographicNear = nearClip; RecalculateProjection(); }
		float GetOrthographicFarClip() const { return m_OrthographicFar; }
		void SetOrthographicFarClip(float farClip) { m_OrthographicFar = farClip; RecalculateProjection(); }

		ProjectionType GetProjectionType() const { return m_ProjectionType; }
		void SetProjectionType(ProjectionType type) { m_ProjectionType = type; RecalculateProjection(); }

		float GetAspectRatio() { return m_AspectRatio; }

		CameraSettings& GetCameraSettings() { return m_Settings; }
		const CameraSettings& GetCameraSettings() const { return m_Settings; }
	private: 
		void RecalculateProjection();
	private:
		ProjectionType m_ProjectionType = ProjectionType::Perspective;

		float m_PerspectiveFOV = glm::radians(45.0f);
		float m_PerspectiveNear = 0.01f, m_PerspectiveFar = 1000.0f;

		float m_OrthographicSize = 10.0f;
		float m_OrthographicNear = -1.0f, m_OrthographicFar = 1.0f;

		float m_AspectRatio = 0.0f;

		CameraSettings m_Settings;
	};

}
