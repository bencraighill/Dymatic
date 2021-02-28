#include "KeyBinds.h"

namespace Dymatic {

	KeyBinds::KeyBinds()
	{

	}

	void KeyBinds::SetKeyBind(KeyBindEvent bindEvent, KeyCode keyCode, MouseCode mouseCode, BindCatagory bindCatagory, bool Ctrl, bool Shift, bool Alt)
	{
		auto& bind = m_KeyBindValues[bindEvent];
		if (keyCode != NULL)
			bind.keyCode = keyCode;
		if (mouseCode != NULL)
			bind.mouseCode = mouseCode;
		bind.Ctrl = Ctrl;
		bind.Alt = Alt;
		bind.Shift = Shift;
		bind.bindCatagory = bindCatagory;
	}

	bool KeyBinds::IsKey(KeyBindEvent bindEvent, KeyCode keyCode, MouseCode mouseCode, BindCatagory bindCatagory, bool Ctrl, bool Shift, bool Alt, int repeatCount)
	{
		return ((m_KeyBindValues[bindEvent].bindCatagory == Keyboard && bindCatagory == Keyboard ? m_KeyBindValues[bindEvent].keyCode == keyCode : m_KeyBindValues[bindEvent].bindCatagory == MouseButton && bindCatagory == MouseButton ? m_KeyBindValues[bindEvent].mouseCode == mouseCode : false) && m_KeyBindValues[bindEvent].Ctrl == Ctrl && m_KeyBindValues[bindEvent].Shift == Shift && m_KeyBindValues[bindEvent].Alt == Alt
			&& (bindCatagory == Keyboard ? (repeatCount > 0 ? (m_KeyBindValues[bindEvent].repeats) : true) : true) && m_KeyBindValues[bindEvent].enabled);
	}

	Dymatic::KeyBindEvent KeyBinds::GetEventFromString(std::string bindEvent)
	{
		if (bindEvent == "New Scene") return NewSceneBind;
		else if (bindEvent == "Open Scene") return OpenSceneBind;
		else if (bindEvent == "Save Scene") return SaveSceneBind;
		else if (bindEvent == "Save Scene As") return SaveSceneAsBind;
		else if (bindEvent == "Quit") return QuitBind;
		else if (bindEvent == "Mouse Picking") return SelectObjectBind;
		else if (bindEvent == "Gizmo None") return GizmoNoneBind;
		else if (bindEvent == "Gizmo Translate") return GizmoTranslateBind;
		else if (bindEvent == "Gizmo Rotate") return GizmoRotateBind;
		else if (bindEvent == "Gizmo Scale") return GizmoScaleBind;
		else if (bindEvent == "Shading Type Wireframe") return ShadingTypeWireframeBind;
		else if (bindEvent == "Shading Type Unlit") return ShadingTypeUnlitBind;
		else if (bindEvent == "Shading Type Solid") return ShadingTypeSolidBind;
		else if (bindEvent == "Shading Type Rendered") return ShadingTypeRenderedBind;
		else if (bindEvent == "Toggle Shading Type") return ToggleShadingTypeBind;
		else if (bindEvent == "Duplicate") return DuplicateBind;
		else if (bindEvent == "Rename") return RenameBind;
		else if (bindEvent == "Close Popup") return ClosePopupBind;

		else return INVALID_BIND;
	}

	std::string KeyBinds::GetStringFromEvent(KeyBindEvent bindEvent)
	{
		if (bindEvent == NewSceneBind) return "New Scene";
		else if (bindEvent == OpenSceneBind) return "Open Scene";
		else if (bindEvent == SaveSceneBind) return "Save Scene";
		else if (bindEvent == SaveSceneAsBind) return "Save Scene As";
		else if (bindEvent == QuitBind) return "Quit";
		else if (bindEvent == SelectObjectBind) return "Mouse Picking";
		else if (bindEvent == GizmoNoneBind) return "Gizmo None";
		else if (bindEvent == GizmoTranslateBind) return "Gizmo Translate";
		else if (bindEvent == GizmoRotateBind) return "Gizmo Rotate";
		else if (bindEvent == GizmoScaleBind) return "Gizmo Scale";
		else if (bindEvent == ShadingTypeWireframeBind) return "Shading Type Wireframe";
		else if (bindEvent == ShadingTypeUnlitBind) return "Shading Type Unlit";
		else if (bindEvent == ShadingTypeSolidBind) return "Shading Type Solid";
		else if (bindEvent == ShadingTypeRenderedBind) return "Shading Type Rendered";
		else if (bindEvent == ToggleShadingTypeBind) return "Toggle Shading Type";
		else if (bindEvent == DuplicateBind) return "Duplicate";
		else if (bindEvent == RenameBind) return "Rename";
		else if (bindEvent == ClosePopupBind) return "Close Popup";

		else return "";
	}

	Dymatic::KeyCode KeyBinds::GetKeyFromString(std::string keyCode)
	{
		if (keyCode == "Space") return Key::Space;
		else if (keyCode == "Apostrophe") return Key::Apostrophe;
		else if (keyCode == "Comma") return Key::Comma;
		else if (keyCode == "Minus") return Key::Minus;
		else if (keyCode == "Period") return Key::Period;
		else if (keyCode == "Slash") return Key::Slash;

		else if (keyCode == "Number 0") return Key::D0;
		else if (keyCode == "Number 1") return Key::D1;
		else if (keyCode == "Number 2") return Key::D2;
		else if (keyCode == "Number 3") return Key::D3;
		else if (keyCode == "Number 4") return Key::D4;
		else if (keyCode == "Number 5") return Key::D5;
		else if (keyCode == "Number 6") return Key::D6;
		else if (keyCode == "Number 7") return Key::D7;
		else if (keyCode == "Number 8") return Key::D8;
		else if (keyCode == "Number 9") return Key::D9;

		else if (keyCode == "Semicolon") return Key::Semicolon;
		else if (keyCode == "Equal") return Key::Equal;

		else if (keyCode == "A") return Key::A;
		else if (keyCode == "B") return Key::B;
		else if (keyCode == "C") return Key::C;
		else if (keyCode == "D") return Key::D;
		else if (keyCode == "E") return Key::E;
		else if (keyCode == "F") return Key::F;
		else if (keyCode == "G") return Key::G;
		else if (keyCode == "H") return Key::H;
		else if (keyCode == "I") return Key::I;
		else if (keyCode == "J") return Key::J;
		else if (keyCode == "K") return Key::K;
		else if (keyCode == "L") return Key::L;
		else if (keyCode == "M") return Key::M;
		else if (keyCode == "N") return Key::N;
		else if (keyCode == "O") return Key::O;
		else if (keyCode == "P") return Key::P;
		else if (keyCode == "Q") return Key::Q;
		else if (keyCode == "R") return Key::R;
		else if (keyCode == "S") return Key::S;
		else if (keyCode == "T") return Key::T;
		else if (keyCode == "U") return Key::U;
		else if (keyCode == "V") return Key::V;
		else if (keyCode == "W") return Key::W;
		else if (keyCode == "X") return Key::X;
		else if (keyCode == "Y") return Key::Y;
		else if (keyCode == "Z") return Key::Z;

		else if (keyCode == "Left Bracket") return Key::LeftBracket;
		else if (keyCode == "Backslash") return Key::Backslash;
		else if (keyCode == "Right Bracket") return Key::RightBracket;
		else if (keyCode == "Grave Accent") return Key::GraveAccent;

		else if (keyCode == "World 1") return Key::World1;
		else if (keyCode == "World 2") return Key::World2;

		/* Function keys */
		else if (keyCode == "Escape") return Key::Escape;
		else if (keyCode == "Enter") return Key::Enter;
		else if (keyCode == "Tab") return Key::Tab;
		else if (keyCode == "Backspace") return Key::Backspace;
		else if (keyCode == "Insert") return Key::Insert;
		else if (keyCode == "Delete") return Key::Delete;
		else if (keyCode == "Right") return Key::Right;
		else if (keyCode == "Left") return Key::Left;
		else if (keyCode == "Down") return Key::Down;
		else if (keyCode == "Up") return Key::Up;
		else if (keyCode == "Page Up") return Key::PageUp;
		else if (keyCode == "Page Down") return Key::PageDown;
		else if (keyCode == "Home") return Key::Home;
		else if (keyCode == "End") return Key::End;
		else if (keyCode == "Caps Lock") return Key::CapsLock;
		else if (keyCode == "Scroll Lock") return Key::ScrollLock;
		else if (keyCode == "Num Lock") return Key::NumLock;
		else if (keyCode == "Print Screen") return Key::PrintScreen;
		else if (keyCode == "Pause") return Key::Pause;
		else if (keyCode == "F1") return Key::F1;
		else if (keyCode == "F2") return Key::F2;
		else if (keyCode == "F3") return Key::F3;
		else if (keyCode == "F4") return Key::F4;
		else if (keyCode == "F5") return Key::F5;
		else if (keyCode == "F6") return Key::F6;
		else if (keyCode == "F7") return Key::F7;
		else if (keyCode == "F8") return Key::F8;
		else if (keyCode == "F9") return Key::F9;
		else if (keyCode == "F10") return Key::F10;
		else if (keyCode == "F11") return Key::F11;
		else if (keyCode == "F12") return Key::F12;
		else if (keyCode == "F13") return Key::F13;
		else if (keyCode == "F14") return Key::F14;
		else if (keyCode == "F15") return Key::F15;
		else if (keyCode == "F16") return Key::F16;
		else if (keyCode == "F17") return Key::F17;
		else if (keyCode == "F18") return Key::F18;
		else if (keyCode == "F19") return Key::F19;
		else if (keyCode == "F20") return Key::F20;
		else if (keyCode == "F21") return Key::F21;
		else if (keyCode == "F22") return Key::F22;
		else if (keyCode == "F23") return Key::F23;
		else if (keyCode == "F24") return Key::F24;
		else if (keyCode == "F25") return Key::F25;
		
		/* Keypad */
		else if (keyCode == "Key Pad 0") return Key::KP0;
		else if (keyCode == "Key Pad 1") return Key::KP1;
		else if (keyCode == "Key Pad 2") return Key::KP2;
		else if (keyCode == "Key Pad 3") return Key::KP3;
		else if (keyCode == "Key Pad 4") return Key::KP4;
		else if (keyCode == "Key Pad 5") return Key::KP5;
		else if (keyCode == "Key Pad 6") return Key::KP6;
		else if (keyCode == "Key Pad 7") return Key::KP7;
		else if (keyCode == "Key Pad 8") return Key::KP8;
		else if (keyCode == "Key Pad 9") return Key::KP9;
		else if (keyCode == "Key Pad Decimal") return Key::KPDecimal;
		else if (keyCode == "Key Pad Divide") return Key::KPDivide;
		else if (keyCode == "Key Pad Multiply") return Key::KPMultiply;
		else if (keyCode == "Key Pad Subtract") return Key::KPSubtract;
		else if (keyCode == "Key Pad Add") return Key::KPAdd;
		else if (keyCode == "Key Pad Enter") return Key::KPEnter;
		else if (keyCode == "Key Pad Equal") return Key::KPEqual;

		else if (keyCode == "Left Shift") return Key::LeftShift;
		else if (keyCode == "Left Control") return Key::LeftControl;
		else if (keyCode == "Left Alt") return Key::LeftAlt;
		else if (keyCode == "Left Super") return Key::LeftSuper;
		else if (keyCode == "Right Shift") return Key::RightShift;
		else if (keyCode == "Right Control") return Key::RightControl;
		else if (keyCode == "Right Alt") return Key::RightAlt;
		else if (keyCode == "Right Super") return Key::RightSuper;
		else if (keyCode == "Menu") return Key::Menu;

		else return -1;
	}

	std::string KeyBinds::GetStringFromKey(KeyCode keyCode)
	{
		if (keyCode == Key::Space) return "Space";
		else if (keyCode == Key::Apostrophe) return "Apostrophe";
		else if (keyCode == Key::Comma) return "Comma";
		else if (keyCode == Key::Minus) return "Minus";
		else if (keyCode == Key::Period) return "Period";
		else if (keyCode == Key::Slash) return "Slash";

		else if (keyCode == Key::D0) return "Number 0";
		else if (keyCode == Key::D1) return "Number 1";
		else if (keyCode == Key::D2) return "Number 2";
		else if (keyCode == Key::D3) return "Number 3";
		else if (keyCode == Key::D4) return "Number 4";
		else if (keyCode == Key::D5) return "Number 5";
		else if (keyCode == Key::D6) return "Number 6";
		else if (keyCode == Key::D7) return "Number 7";
		else if (keyCode == Key::D8) return "Number 8";
		else if (keyCode == Key::D9) return "Number 9";

		else if (keyCode == Key::Semicolon) return "Semicolon";
		else if (keyCode == Key::Equal) return "Equal";

		else if (keyCode == Key::A) return "A";
		else if (keyCode == Key::B) return "B";
		else if (keyCode == Key::C) return "C";
		else if (keyCode == Key::D) return "D";
		else if (keyCode == Key::E) return "E";
		else if (keyCode == Key::F) return "F";
		else if (keyCode == Key::G) return "G";
		else if (keyCode == Key::H) return "H";
		else if (keyCode == Key::I) return "I";
		else if (keyCode == Key::J) return "J";
		else if (keyCode == Key::K) return "K";
		else if (keyCode == Key::L) return "L";
		else if (keyCode == Key::M) return "M";
		else if (keyCode == Key::N) return "N";
		else if (keyCode == Key::O) return "O";
		else if (keyCode == Key::P) return "P";
		else if (keyCode == Key::Q) return "Q";
		else if (keyCode == Key::R) return "R";
		else if (keyCode == Key::S) return "S";
		else if (keyCode == Key::T) return "T";
		else if (keyCode == Key::U) return "U";
		else if (keyCode == Key::V) return "V";
		else if (keyCode == Key::W) return "W";
		else if (keyCode == Key::X) return "X";
		else if (keyCode == Key::Y) return "Y";
		else if (keyCode == Key::Z) return "Z";

		else if (keyCode == Key::LeftBracket) return "Left Bracket";
		else if (keyCode == Key::Backslash) return "Backslash";
		else if (keyCode == Key::RightBracket) return "Right Bracket";
		else if (keyCode == Key::GraveAccent) return "Grave Accent";

		else if (keyCode == Key::World1) return "World 1";
		else if (keyCode == Key::World2) return "World 2";

		/* Function keys */
		else if (keyCode == Key::Escape) return "Escape";
		else if (keyCode == Key::Enter) return "Enter";
		else if (keyCode == Key::Tab) return "Tab";
		else if (keyCode == Key::Backspace) return "Backspace";
		else if (keyCode == Key::Insert) return "Insert";
		else if (keyCode == Key::Delete) return "Delete";
		else if (keyCode == Key::Right) return "Right";
		else if (keyCode == Key::Left) return "Left";
		else if (keyCode == Key::Down) return "Down";
		else if (keyCode == Key::Up) return "Up";
		else if (keyCode == Key::PageUp) return "Page Up";
		else if (keyCode == Key::PageDown) return "Page Down";
		else if (keyCode == Key::Home) return "Home";
		else if (keyCode == Key::End) return "End";
		else if (keyCode == Key::CapsLock) return "Caps Lock";
		else if (keyCode == Key::ScrollLock) return "Scroll Lock";
		else if (keyCode == Key::NumLock) return "Num Lock";
		else if (keyCode == Key::PrintScreen) return "Print Screen";
		else if (keyCode == Key::Pause) return "Pause";
		else if (keyCode == Key::F1) return "F1";
		else if (keyCode == Key::F2) return "F2";
		else if (keyCode == Key::F3) return "F3";
		else if (keyCode == Key::F4) return "F4";
		else if (keyCode == Key::F5) return "F5";
		else if (keyCode == Key::F6) return "F6";
		else if (keyCode == Key::F7) return "F7";
		else if (keyCode == Key::F8) return "F8";
		else if (keyCode == Key::F9) return "F9";
		else if (keyCode == Key::F10) return "F10";
		else if (keyCode == Key::F11) return "F11";
		else if (keyCode == Key::F12) return "F12";
		else if (keyCode == Key::F13) return "F13";
		else if (keyCode == Key::F14) return "F14";
		else if (keyCode == Key::F15) return "F15";
		else if (keyCode == Key::F16) return "F16";
		else if (keyCode == Key::F17) return "F17";
		else if (keyCode == Key::F18) return "F18";
		else if (keyCode == Key::F19) return "F19";
		else if (keyCode == Key::F20) return "F20";
		else if (keyCode == Key::F21) return "F21";
		else if (keyCode == Key::F22) return "F22";
		else if (keyCode == Key::F23) return "F23";
		else if (keyCode == Key::F24) return "F24";
		else if (keyCode == Key::F25) return "F25";

		/* Keypad */
		else if (keyCode == Key::KP0) return "Key Pad 0";
		else if (keyCode == Key::KP1) return "Key Pad 1";
		else if (keyCode == Key::KP2) return "Key Pad 2";
		else if (keyCode == Key::KP3) return "Key Pad 3";
		else if (keyCode == Key::KP4) return "Key Pad 4";
		else if (keyCode == Key::KP5) return "Key Pad 5";
		else if (keyCode == Key::KP6) return "Key Pad 6";
		else if (keyCode == Key::KP7) return "Key Pad 7";
		else if (keyCode == Key::KP8) return "Key Pad 8";
		else if (keyCode == Key::KP9) return "Key Pad 9";
		else if (keyCode == Key::KPDecimal) return "Key Pad Decimal";
		else if (keyCode == Key::KPDivide) return "Key Pad Divide";
		else if (keyCode == Key::KPMultiply) return "Key Pad Multiply";
		else if (keyCode == Key::KPSubtract) return "Key Pad Subtract";
		else if (keyCode == Key::KPAdd) return "Key Pad Add";
		else if (keyCode == Key::KPEnter) return "Key Pad Enter";
		else if (keyCode == Key::KPEqual) return "Key Pad Equal";

		else if (keyCode == Key::LeftShift) return "Left Shift";
		else if (keyCode == Key::LeftControl) return "Left Control";
		else if (keyCode == Key::LeftAlt) return "Left Alt";
		else if (keyCode == Key::LeftSuper) return "Left Super";
		else if (keyCode == Key::RightShift) return "Right Shift";
		else if (keyCode == Key::RightControl) return "Right Control";
		else if (keyCode == Key::RightAlt) return "Right Alt";
		else if (keyCode == Key::RightSuper) return "Right Super";
		else if (keyCode == Key::Menu) return "Menu";

		else return "";
	}

	Dymatic::MouseCode KeyBinds::GetMouseButtonFromString(std::string mouseButton)
	{
		if (mouseButton == "Button Left") return Mouse::ButtonLeft;
		else if (mouseButton == "Button Right") return Mouse::ButtonRight;
		else if (mouseButton == "Button Middle") return Mouse::ButtonMiddle;
		else if (mouseButton == "Button Last") return Mouse::ButtonLast;
		else if (mouseButton == "Button 0") return Mouse::Button0;
		else if (mouseButton == "Button 1") return Mouse::Button1;
		else if (mouseButton == "Button 2") return Mouse::Button2;
		else if (mouseButton == "Button 3") return Mouse::Button3;
		else if (mouseButton == "Button 4") return Mouse::Button4;
		else if (mouseButton == "Button 5") return Mouse::Button5;
		else if (mouseButton == "Button 6") return Mouse::Button6;
		else if (mouseButton == "Button 7") return Mouse::Button7;

		else return -1;
	}

	std::string KeyBinds::GetStringFromMouseButton(MouseCode mouseButton)
	{
		if (mouseButton == Mouse::ButtonLeft) return "Button Left";
		else if (mouseButton == Mouse::ButtonRight) return "Button Right";
		else if (mouseButton == Mouse::ButtonMiddle) return "Button Middle";
		else if (mouseButton == Mouse::ButtonLast) return "Button Last";
		else if (mouseButton == Mouse::Button0) return "Button 0";
		else if (mouseButton == Mouse::Button1) return "Button 1";
		else if (mouseButton == Mouse::Button2) return "Button 2";
		else if (mouseButton == Mouse::Button3) return "Button 3";
		else if (mouseButton == Mouse::Button4) return "Button 4";
		else if (mouseButton == Mouse::Button5) return "Button 5";
		else if (mouseButton == Mouse::Button6) return "Button 6";
		else if (mouseButton == Mouse::Button7) return "Button 7";

		else return "";
	}



	Dymatic::BindCatagory KeyBinds::GetBindCatagoryFromString(std::string bindCatagory)
	{
		if (bindCatagory == "Keyboard") return Keyboard;
		else if (bindCatagory == "Mouse Button") return MouseButton;

		else return Keyboard;
	}

	std::string KeyBinds::GetStringFromBindCatagory(BindCatagory bindCatagory)
	{
		if (bindCatagory == Keyboard) return "Keyboard";
		else if (bindCatagory == MouseButton) return "Mouse Button";

		else return "";
	}

	void KeyBinds::SetKeyStrings(std::string name, std::string valueName, std::string value)
	{
		if (valueName == "Key") { m_KeyBindValues[GetEventFromString(name)].keyCode = GetKeyFromString(value); }
		else if (valueName == "MouseButton") { m_KeyBindValues[GetEventFromString(name)].mouseCode = GetMouseButtonFromString(value); }
		else if (valueName == "BindCatagory") { m_KeyBindValues[GetEventFromString(name)].bindCatagory = GetBindCatagoryFromString(value); }
		else if (valueName == "Ctrl") { m_KeyBindValues[GetEventFromString(name)].Ctrl = value == "true" ? true : false; }
		else if (valueName == "Shift") { m_KeyBindValues[GetEventFromString(name)].Shift = value == "true" ? true : false; }
		else if (valueName == "Alt") { m_KeyBindValues[GetEventFromString(name)].Alt = value == "true" ? true : false; }
		else if (valueName == "Enabled") { m_KeyBindValues[GetEventFromString(name)].enabled = value == "true" ? true : false; }
		else if (valueName == "Repeats") { m_KeyBindValues[GetEventFromString(name)].repeats = value == "true" ? true : false; }
	}

}