#include "dypch.h"
#include "EditorCamera.h"

#include "Dymatic/Core/Input.h"
#include "Dymatic/Core/KeyCodes.h"
#include "Dymatic/Core/MouseCodes.h"

#include <glfw/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Dymatic {

	EditorCamera::EditorCamera(float fov, float aspectRatio, float nearClip, float farClip)
		: m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip), Camera(glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip))
	{
		UpdateView();
	}

	void EditorCamera::UpdateProjection()
	{
		if (m_ProjectionType == 0)
		{
			m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
			m_Projection = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
		}
		else if (m_ProjectionType == 1)
		{
			float m_OrthographicSize = m_Distance * 0.5f;

			float orthoLeft = -m_OrthographicSize * m_AspectRatio * 0.5f;
			float orthoRight = m_OrthographicSize * m_AspectRatio * 0.5f;
			float orthoBottom = -m_OrthographicSize * 0.5f;
			float orthoTop = m_OrthographicSize * 0.5f;

			m_Projection = glm::ortho(orthoLeft, orthoRight,
				orthoBottom, orthoTop, -m_FarClip, m_FarClip);
		}
	}
	
	void EditorCamera::UpdateView()
	{
		//m_Yaw = m_Pitch = 0.0f; //Lock the camera's rotation
		{
			glm::quat orientation = GetOrientation();
			m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
			m_ViewMatrix = glm::inverse(m_ViewMatrix);
		}
		if (m_ProjectionType == 1)
		{

			float m_OrthographicSize = m_Distance * 0.5f;

			float orthoLeft = -m_OrthographicSize * m_AspectRatio * 0.5f;
			float orthoRight = m_OrthographicSize * m_AspectRatio * 0.5f;
			float orthoBottom = -m_OrthographicSize * 0.5f;
			float orthoTop = m_OrthographicSize * 0.5f;

			m_Projection = glm::ortho(orthoLeft, orthoRight,
				orthoBottom, orthoTop, -m_FarClip, m_FarClip);
		}
	}

	std::pair<float, float> EditorCamera::PanSpeed() const
	{
		float x = std::min(m_ViewportWidth / 1000.0f, 2.4f); //max = 2.4f
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = std::min(m_ViewportWidth / 1000.0f, 2.4f); //max = 2.4f
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	float EditorCamera::RotationSpeed() const
	{
		return 0.8f;
	}

	float EditorCamera::ZoomSpeed() const
	{
		float distance = m_Distance * 0.2f;
		distance = std::max(distance, 0.0f);
		float speed = distance * distance;
		speed = std::min(speed, 100.0f); // max speed = 100
		return speed;
	}

	void EditorCamera::OnUpdate(Timestep ts)
	{
		const glm::vec2& mouse{ Input::GetMouseX(), Input::GetMouseY() };
		glm::vec2 delta = (mouse - m_InitialMousePosition) * 0.003f;
		m_InitialMousePosition = mouse;

		if ((!m_BlockEvents && m_OrbitalEnabled && Input::IsKeyPressed(Key::LeftAlt)) || m_FreePan)
		{

			if (Input::IsMouseButtonPressed(Mouse::ButtonMiddle))
				MousePan(delta);
			else if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
				MouseRotate(delta);
			else if (Input::IsMouseButtonPressed(Mouse::ButtonRight))
				MouseZoom(delta.y);

			m_Position = CalculatePosition();
			m_TargetPosition = m_Position;
			UpdateView();
		}
		else if (m_FirstPersonEnabled)
		{
			// Keyboard Input
			if (Input::IsMouseButtonPressed(Mouse::ButtonRight) && !m_BlockEvents || m_FreePan)
			{
				MouseRotate(delta);

				const float mult = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift) ? 2.0f : 1.0f;

				if (Input::IsKeyPressed(Key::W))
					MoveInDirection(ts, GetForwardDirection());
				if (Input::IsKeyPressed(Key::S))
					MoveInDirection(ts, -GetForwardDirection());
				if (Input::IsKeyPressed(Key::D))
					MoveInDirection(ts, GetRightDirection());
				if (Input::IsKeyPressed(Key::A))
					MoveInDirection(ts, -GetRightDirection());
				if (Input::IsKeyPressed(Key::E))
					MoveInDirection(ts, glm::vec3(0.0f, 1.0f, 0.0f));
				if (Input::IsKeyPressed(Key::Q))
					MoveInDirection(ts, glm::vec3(0.0f, -1.0f, 0.0f));
			}

			// Gamepad Input
			if (Input::IsGamepadConnected(0))
			{
				static const float deadZone = 0.25f;
				const float mult = Input::IsGamepadButtonPressed(0, Gamepad::LeftThumb) ? 2.0f : 1.0f;

				glm::vec2 rotation = { Input::GetGamepadAxis(0, Gamepad::RightX), Input::GetGamepadAxis(0, Gamepad::RightY) };
				if (std::abs(rotation.x) < deadZone) rotation.x = 0.0f;
				if (std::abs(rotation.y) < deadZone) rotation.y = 0.0f;

				rotation = rotation * 0.1f * mult;

				MouseRotate(rotation);

				if (std::abs(Input::GetGamepadAxis(0, Gamepad::LeftX)) >= deadZone)
					MoveInDirection(ts, GetRightDirection() * Input::GetGamepadAxis(0, Gamepad::LeftX));
				if (std::abs(Input::GetGamepadAxis(0, Gamepad::LeftY)) >= deadZone)
					MoveInDirection(ts, GetForwardDirection() * -Input::GetGamepadAxis(0, Gamepad::LeftY));
				if (Input::GetGamepadAxis(0, Gamepad::LeftTrigger != -1.0f))
					MoveInDirection(ts, glm::vec3(0.0f, -1.0f * (Input::GetGamepadAxis(0, Gamepad::LeftTrigger) * 0.5f + 0.5f), 0.0f));
				if (Input::GetGamepadAxis(0, Gamepad::RightTrigger) != -1.0f)
					MoveInDirection(ts, glm::vec3(0.0f, 1.0f * (Input::GetGamepadAxis(0, Gamepad::RightTrigger) * 0.5f + 0.5f), 0.0f));

			}

			UpdateView();
		}

		// Update our smooth camera interpolation if needed
		if (m_CurrentTargetTime != 0.0f)
		{
			m_CurrentTargetTime -= ts;
			if (m_CurrentTargetTime < 0.0f)
				m_CurrentTargetTime = 0.0f;
			
			float interpolation = (m_CurrentTargetTime / m_SmoothingTime);
			m_Position = m_TargetPosition + (m_TargetStart - m_TargetPosition) * (interpolation * interpolation);
			m_FocalPoint = m_Position + GetForwardDirection() * m_Distance;
			
			UpdateView();
		}
	}

	void EditorCamera::MoveInDirection(Timestep ts, glm::vec3 direction)
	{
		const float mult = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift) ? 2.0f : 1.0f;
		
		if (m_SmoothingTime == 0.0f)
			m_Position += direction * mult * GetMoveSpeed() * ts.GetSeconds();
		else
		{
			m_TargetPosition += direction * mult * GetMoveSpeed() * ts.GetSeconds();
			m_TargetStart = m_Position;
			m_CurrentTargetTime = m_SmoothingTime;
		}
	}

	void EditorCamera::OnEvent(Event& e)
	{
		if (m_BlockEvents)
			return;

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseScrolledEvent>(DY_BIND_EVENT_FN(EditorCamera::OnMouseScroll));
	}

	bool EditorCamera::OnMouseScroll(MouseScrolledEvent& e)
	{
		float delta = e.GetYOffset() * 0.1f;
		MouseZoom(delta);
		UpdateView();
		return false;
	}

	void EditorCamera::MousePan(const glm::vec2& delta)
	{
		auto [xSpeed, ySpeed] = PanSpeed();
		m_FocalPoint += -GetRightDirection() * delta.x * xSpeed * m_Distance;
		m_FocalPoint += GetUpDirection() * delta.y * ySpeed * m_Distance;
	}

	void EditorCamera::MouseRotate(const glm::vec2& delta)
	{
		float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
		m_Yaw += yawSign * delta.x * RotationSpeed();
		m_Pitch += delta.y * RotationSpeed();
	}

	void EditorCamera::MouseZoom(float delta)
	{
		m_Distance -= delta * ZoomSpeed();
		if (m_Distance < 1.0f)
		{
			m_FocalPoint += GetForwardDirection();
			m_Distance = 1.0f;
		}
	}

	glm::vec3 EditorCamera::GetUpDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::vec3 EditorCamera::GetRightDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::vec3 EditorCamera::GetForwardDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
	}

	glm::vec3 EditorCamera::CalculatePosition() const
	{
		return m_FocalPoint - GetForwardDirection() * m_Distance;
	}

	glm::quat EditorCamera::GetOrientation() const
	{
		return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
	}

}