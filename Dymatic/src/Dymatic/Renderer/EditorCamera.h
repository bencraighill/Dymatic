#pragma once

#include "Camera.h"
#include "Dymatic/Core/Timestep.h"
#include "Dymatic/Events/Event.h"
#include "Dymatic/Events/MouseEvent.h"

#include <glm/glm.hpp>

namespace Dymatic {

	class EditorCamera : public Camera
	{
	public:
		EditorCamera() = default;
		EditorCamera(float fov, float aspectRatio, float nearClip, float farClip);

		void OnUpdate(Timestep ts);
		void OnEvent(Event& e);

		inline float GetDistance() const { return m_Distance; }
		inline void SetDistance(float distance) { m_Distance = distance; UpdateView(); }

		inline glm::vec3 GetFocalPoint() { return m_FocalPoint; }
		inline void SetFocalPoint(glm::vec3 focalPoint) { m_FocalPoint = focalPoint; UpdateView(); }

		inline void SetViewportSize(float width, float height) { m_ViewportWidth = width, m_ViewportHeight = height; UpdateProjection(); }

		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		glm::mat4 GetViewProjection() const { return m_Projection * m_ViewMatrix; }

		glm::vec3 GetUpDirection() const;
		glm::vec3 GetRightDirection() const;
		glm::vec3 GetForwardDirection() const;
		const glm::vec3& GetPosition() const { return m_Position; }
		glm::quat GetOrientation() const;

		float GetPitch() const { return m_Pitch; }
		float GetYaw() const { return m_Yaw; }

		void SetYaw(float yaw) { m_Yaw = yaw; UpdateView(); }
		void SetPitch(float pitch) { m_Pitch = pitch; UpdateView(); }

		int GetProjectionType() { return m_ProjectionType; }
		void SetProjectionType(int type) { m_ProjectionType = type; UpdateProjection(); }

		float GetFOV() { return m_FOV; }
		void SetFOV(float fov) { m_FOV = fov; UpdateProjection(); }
		float GetAspectRatio() { return m_AspectRatio; }

		float GetNearClip() const { return m_NearClip; }
		void SetNearClip(float nearClip) { m_NearClip = nearClip; UpdateProjection(); }
		float GetFarClip() const { return m_FarClip; }
		void SetFarClip(float farClip) { m_FarClip = farClip; UpdateProjection(); }

		float GetMoveSpeed() const { return m_MoveSpeed; }
		void SetMoveSpeed(float speed) { m_MoveSpeed = speed; }

		void SetOrbitalEnabled(bool enabled) { m_OrbitalEnabled = enabled; }
		void SetFirstPersonEnabled(bool enabled) { m_FirstPersonEnabled = enabled; }

		void SetFreePan(bool freePan) { m_FreePan = freePan; }
		void SetBlockEvents(bool blockEvents) { m_BlockEvents = blockEvents; }
		bool GetBlockEvents() { return m_BlockEvents; }

		float GetSmoothingTime() { return m_SmoothingTime; }
		void SetSmoothingTime(float smoothingTime) { m_SmoothingTime = smoothingTime; }

	private:
		void UpdateProjection();
		void UpdateView();

		void MoveInDirection(Timestep ts, glm::vec3 direction);

		bool OnMouseScroll(MouseScrolledEvent& e);

		void MousePan(const glm::vec2& delta);
		void MouseRotate(const glm::vec2& delta);
		void MouseZoom(float delta);

		glm::vec3 CalculatePosition() const;

		std::pair<float, float> PanSpeed() const;
		float RotationSpeed() const;
		float ZoomSpeed() const;

	private:
		float m_FOV = 45.0f, m_AspectRatio = 1.778f, m_NearClip = 0.1f, m_FarClip = 1000.0f;

		glm::mat4 m_ViewMatrix;
		glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };

		glm::vec3 m_TargetPosition = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_TargetStart = { 0.0f, 0.0f, 0.0f };
		float m_CurrentTargetTime = 0.0f;
		float m_SmoothingTime = 0.15f;

		glm::vec2 m_InitialMousePosition;

		float m_Distance = 10.0f;
		float m_Pitch = 0.0f, m_Yaw = 0.0f;
		float m_MoveSpeed = 20.0f;

		bool m_OrbitalEnabled = true;
		bool m_FirstPersonEnabled = true;

		float m_ViewportWidth = 1600, m_ViewportHeight = 900;

		bool m_FreePan = true;
		bool m_BlockEvents = false;

		int m_ProjectionType = 0;
	};

}
