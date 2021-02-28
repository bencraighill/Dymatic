#pragma once
#include "Dymatic/Core/Input.h"
#include <array>
#include <string>
#include <vector>

namespace Dymatic {

	enum BindCatagory
	{
		Keyboard,
		MouseButton
	};

	struct KeyBindData
	{
		KeyCode keyCode = Key::Space;
		MouseCode mouseCode = Mouse::ButtonLeft;
		BindCatagory bindCatagory = Keyboard;
		bool Ctrl;
		bool Shift;
		bool Alt;
		bool enabled = true;
		bool repeats = false;
	};

	enum KeyBindEvent
	{
		INVALID_BIND,
		NewSceneBind,
		OpenSceneBind,
		SaveSceneBind,
		SaveSceneAsBind,
		QuitBind,
		SelectObjectBind,
		GizmoNoneBind,
		GizmoTranslateBind,
		GizmoRotateBind,
		GizmoScaleBind,
		ShadingTypeWireframeBind,
		ShadingTypeUnlitBind,
		ShadingTypeSolidBind,
		ShadingTypeRenderedBind,
		ToggleShadingTypeBind,
		DuplicateBind,
		RenameBind,
		ClosePopupBind,
		KEY_BINDS_SIZE
	};

	class KeyBinds
	{
	public:
		KeyBinds();
		KeyBindData& GetKeyBind(KeyBindEvent bindEvent) { return m_KeyBindValues[bindEvent]; }
		std::array<KeyBindData, KEY_BINDS_SIZE>& GetAllKeyBinds() { return m_KeyBindValues; }
		void SetKeyBind(KeyBindEvent bindEvent, KeyCode keyCode, MouseCode mouseCode, BindCatagory bindCatagory, bool Ctrl = false, bool Shift = false, bool Alt = false);
		bool IsKey(KeyBindEvent bindEvent, KeyCode keyCode, MouseCode mouseCode, BindCatagory bindCatagory, bool Ctrl, bool Shift, bool Alt, int repeatCount);

		KeyBindEvent GetEventFromString(std::string bindEvent);
		std::string GetStringFromEvent(KeyBindEvent bindEvent);

		KeyCode GetKeyFromString(std::string keyCode);
		std::string GetStringFromKey(KeyCode keyCode);

		MouseCode GetMouseButtonFromString(std::string mouseButton);
		std::string GetStringFromMouseButton(MouseCode mouseButton);

		BindCatagory GetBindCatagoryFromString(std::string bindCatagory);
		std::string GetStringFromBindCatagory(BindCatagory bindCatagory);

		void SetKeyStrings(std::string name, std::string valueName, std::string value);
		std::vector<KeyCode> GetAllKeys() { return m_AllKeys; }
		std::vector<KeyCode> GetAllMouseButtons() { return m_AllMouseButtons; }

	private:
		std::array<KeyBindData, KEY_BINDS_SIZE>&& m_KeyBindValues = {};
		std::vector<KeyCode> m_AllKeys = {
			Key::Space, Key::Apostrophe, Key::Comma, Key::Minus, Key::Period, Key::Slash, Key::D0, Key::D1, Key::D2, Key::D3, Key::D4, Key::D5, Key::D6, Key::D7, Key::D8, Key::D9, Key::Semicolon,
			Key::Equal, Key::A, Key::B, Key::C, Key::D, Key::E, Key::F, Key::G, Key::H, Key::I, Key::J, Key::K, Key::L, Key::M, Key::N, Key::O, Key::P, Key::Q, Key::R, Key::S, Key::T, Key::U, Key::V,
			Key::W, Key::X, Key::Y, Key::Z, Key::LeftBracket, Key::Backslash, Key::RightBracket, Key::GraveAccent, Key::World1, Key::World2, Key::Escape, Key::Enter, Key::Tab, Key::Backspace, Key::Insert,
			Key::Delete, Key::Right, Key::Left, Key::Down, Key::Up, Key::PageUp, Key::PageDown, Key::Home, Key::End, Key::CapsLock, Key::ScrollLock, Key::NumLock, Key::PrintScreen, Key::Pause, Key::F1,
			Key::F2, Key::F3 , Key::F4, Key::F5, Key::F6, Key::F7, Key::F8, Key::F9, Key::F10, Key::F11, Key::F12, Key::F13, Key::F14, Key::F15, Key::F16, Key::F17, Key::F18, Key::F19, Key::F20, Key::F21,
			Key::F22, Key::F23, Key::F24, Key::F25, Key::KP0, Key::KP1, Key::KP2, Key::KP3, Key::KP4, Key::KP5, Key::KP6, Key::KP7, Key::KP8, Key::KP9, Key::KPDecimal, Key::KPDivide, Key::KPMultiply,
			Key::KPSubtract, Key::KPAdd, Key::KPEnter, Key::KPEqual, Key::LeftShift, Key::LeftControl, Key::LeftAlt, Key::LeftSuper, Key::RightShift, Key::RightControl, Key::RightAlt, Key::RightSuper,
			Key::Menu};

		std::vector<MouseCode> m_AllMouseButtons = { Mouse::ButtonLeft, Mouse::ButtonRight, Mouse::ButtonMiddle, Mouse::ButtonLast, Mouse::Button0, Mouse::Button1, Mouse::Button2, Mouse::Button3, Mouse::Button4, Mouse::Button5, Mouse::Button6, Mouse::Button7 };
	};

}
