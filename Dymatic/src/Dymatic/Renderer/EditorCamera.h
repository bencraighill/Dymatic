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
		struct EditorCameraTransform
		{
			glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
			glm::vec3 FocalPoint = { 0.0f, 0.0f, 0.0f };
			float Distance = 10.0f;
			float Pitch = 0.0f, Yaw = 0.0f;
		};

	public:
		EditorCamera(float fov = 45.0f, float aspectRatio = 1.778f, float nearClip = 0.1f, float farClip = 1000.0f);

		void OnUpdate(Timestep ts);
		void OnEvent(Event& e);
		
		inline void SetViewportSize(float width, float height) { m_ViewportWidth = width, m_ViewportHeight = height; UpdateProjection(); }

		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		glm::mat4 GetViewProjection() const { return m_Projection * m_ViewMatrix; }

		glm::vec3 GetUpDirection() const;
		glm::vec3 GetRightDirection() const;
		glm::vec3 GetForwardDirection() const;

		inline const EditorCameraTransform& GetTransform() const { return m_CameraTransform; }
		void SetTransform(const EditorCameraTransform& transform);
		void SmoothTransform(const EditorCameraTransform& transform);
		inline float GetDistance() const { return m_CameraTransform.Distance; }
		inline void SetDistance(float distance) { m_CameraTransform.Distance = distance; UpdateView(); }
		inline glm::vec3 GetFocalPoint() { return m_CameraTransform.FocalPoint; }
		inline void SetFocalPoint(glm::vec3 focalPoint) { m_CameraTransform.FocalPoint = focalPoint; UpdateView(); }
		const glm::vec3& GetPosition() const { return m_CameraTransform.Position; }
		void SetPosition(const glm::vec3& position) { m_CameraTransform.Position = position; UpdateView(); }

		float GetPitch() const { return m_CameraTransform.Pitch; }
		float GetYaw() const { return m_CameraTransform.Yaw; }

		void SetYaw(float yaw) { m_CameraTransform.Yaw = yaw; UpdateView(); }
		void SetPitch(float pitch) { m_CameraTransform.Pitch = pitch; UpdateView(); }

		glm::quat GetOrientation() const;

		int GetProjectionType() { return m_ProjectionType; }
		void SetProjectionType(int type) { m_ProjectionType = type; UpdateProjection(); }

		virtual float GetFOV() const override { return m_FOV; }
		void SetFOV(float fov) { m_FOV = fov; UpdateProjection(); }
		float GetAspectRatio() { return m_AspectRatio; }

		virtual float GetNearClip() const override { return m_NearClip; }
		void SetNearClip(float nearClip) { m_NearClip = nearClip; UpdateProjection(); }
		virtual float GetFarClip() const override { return m_FarClip; }
		void SetFarClip(float farClip) { m_FarClip = farClip; UpdateProjection(); }

		float GetMoveSpeed() const { return m_MoveSpeed; }
		void SetMoveSpeed(float speed) { m_MoveSpeed = speed; }

		inline bool GetOrbitRequireAlt() const { return m_OrbitRequireAlt; }
		inline void SetOrbitRequireAlt(const bool requireAlt) { m_OrbitRequireAlt = requireAlt; }

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
		EditorCameraTransform m_CameraTransform;

		glm::vec3 m_TargetPosition = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_TargetStart = { 0.0f, 0.0f, 0.0f };
		float m_CurrentTargetTime = 0.0f;
		float m_SmoothingTime = 0.15f;
		float m_MoveSpeed = 20.0f;

		glm::vec2 m_InitialMousePosition;

		bool m_OrbitRequireAlt = true;
		bool m_OrbitalEnabled = true;
		bool m_FirstPersonEnabled = true;

		float m_ViewportWidth = 1600, m_ViewportHeight = 900;

		bool m_FreePan = false;
		bool m_BlockEvents = false;

		int m_ProjectionType = 0;
	};

}
