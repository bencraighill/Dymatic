#pragma once

#include "Dymatic/Events/Event.h"
#include "Dymatic/Core/GamepadCodes.h"

namespace Dymatic {

	class GamepadConnectedEvent : public Event
	{
	public:
		GamepadConnectedEvent(uint32_t gamepad)
			: m_Gamepad(gamepad) {}

		uint32_t GetGamepad() const { return m_Gamepad; }

		EVENT_CLASS_TYPE(GamepadConnected)
		EVENT_CLASS_CATEGORY(EventCategoryGamepad | EventCategoryInput)
	protected:
		uint32_t m_Gamepad;
	};

	class GamepadDisconnectedEvent : public Event
	{
	public:
		GamepadDisconnectedEvent(uint32_t gamepad)
			: m_Gamepad(gamepad) {}

		uint32_t GetGamepad() const { return m_Gamepad; }

		EVENT_CLASS_TYPE(GamepadDisconnected)
		EVENT_CLASS_CATEGORY(EventCategoryGamepad | EventCategoryInput)
	protected:
		uint32_t m_Gamepad;
	};
	
	class GamepadButtonEvent : public Event
	{
	public:
		uint32_t GetGamepad() const { return m_Gamepad; }
		GamepadButtonCode GetButton() const { return m_Button; }
		
		EVENT_CLASS_CATEGORY(EventCategoryGamepad | EventCategoryInput)
	protected:
		GamepadButtonEvent(uint32_t gamepad, GamepadButtonCode button)
			: m_Gamepad(gamepad), m_Button(button) {}

		uint32_t m_Gamepad;
		GamepadButtonCode m_Button;
	};

	class GamepadButtonPressedEvent : public GamepadButtonEvent
	{
	public:
		GamepadButtonPressedEvent(uint32_t gamepad, const GamepadButtonCode button)
			: GamepadButtonEvent(gamepad, button) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "GamepadButtonPressedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(GamepadButtonPressed)
	};

	class GamepadButtonReleasedEvent : public GamepadButtonEvent
	{
	public:
		GamepadButtonReleasedEvent(uint32_t gamepad, const GamepadButtonCode button)
			: GamepadButtonEvent(gamepad, button) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "GamepadButtonReleasedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(GamepadButtonReleased)
	};

	class GamepadAxisMovedEvent : public Event
	{
	public:
		GamepadAxisMovedEvent(uint32_t gamepad, GamepadAxisCode axis, float axisValue)
			: m_Gamepad(gamepad), m_Axis(axis), m_AxisValue(axisValue) {}

		uint32_t GetGamepad() const { return m_Gamepad; }
		GamepadAxisCode GetAxis() const { return m_Axis; }
		float GetAxisValue() const { return m_AxisValue; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "GamepadAxisEvent: " << m_Axis << " " << m_AxisValue;
			return ss.str();
		}

		EVENT_CLASS_TYPE(GamepadAxisMoved)
		EVENT_CLASS_CATEGORY(EventCategoryGamepad | EventCategoryInput)
	protected:
		uint32_t m_Gamepad;
		GamepadAxisCode m_Axis;
		float m_AxisValue;
	};
	
	
}