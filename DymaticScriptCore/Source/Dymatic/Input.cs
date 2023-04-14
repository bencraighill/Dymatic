namespace Dymatic
{
    public class Input
    {
        [IsEditorCallable]
        public static bool IsKeyDown([ParameterName("Key Code")] KeyCode keyCode)
        {
            return InternalCalls.Input_IsKeyDown(keyCode);
        }
        
        [IsEditorCallable]
        public static bool IsMouseButtonPressed([ParameterName("Mouse Code")] MouseCode mouseCode)
        {
            return InternalCalls.Input_IsMouseButtonPressed(mouseCode);
        }

        [IsEditorCallable]
        public static Vector2 GetMousePosition()
        {
            InternalCalls.Input_GetMousePosition(out Vector2 position);
            return position;
        }

        [IsEditorCallable]
        public static float GetMouseX()
        {
            return InternalCalls.Input_GetMouseX();
        }
        
        [IsEditorCallable]
        public static float GetMouseY()
        {
            return InternalCalls.Input_GetMouseY();
        }

        [IsEditorCallable]
        public static bool IsGamepadConnected([ParameterName("Gamepad Index")] int gamepadIndex)
        {
            return InternalCalls.Input_IsGamepadConnected(gamepadIndex);
        }

        [IsEditorCallable]
        public static string GetGamepadName([ParameterName("Gamepad Index")] int gamepadIndex)
        {
            InternalCalls.Input_GetGamepadName(gamepadIndex, out string name);
            return name;
        }

        [IsEditorCallable]
        public static bool IsGamepadButtonPressed([ParameterName("Gamepad Index")] int gamepadIndex, [ParameterName("Gamepad Button Code")] GamepadButtonCode gamepadButton)
        {
            return InternalCalls.Input_IsGamepadButtonPressed(gamepadIndex, gamepadButton);
        }

        [IsEditorCallable]
        public static float GetGamepadAxis([ParameterName("Gamepad Index")] int gamepadIndex, [ParameterName("Gamepad Axis Code")] GamepadAxisCode gamepadAxis)
        {
            return InternalCalls.Input_GetGamepadAxis(gamepadIndex, gamepadAxis);
        }

        [IsEditorCallable]
        public static Vector3 GetGamepadSensor([ParameterName("Gamepad Index")] int gamepadIndex, [ParameterName("Gamepad Sensor Code")] GamepadSensorCode gamepadSensor)
        {
            InternalCalls.Input_GetGamepadSensor(gamepadIndex, gamepadSensor, out Vector3 sensor);
            return sensor;
        }

        [IsEditorCallable]
        public static bool SetGamepadRumble([ParameterName("Gamepad Index")] int gamepadIndex, [ParameterName("Left")] float left, [ParameterName("Right")] float right, [ParameterName("Duration")] float duration)
        {
            return InternalCalls.Input_SetGamepadRumble(gamepadIndex, left, right, duration);
        }

        [IsEditorCallable]
        public static bool SetGamepadLED([ParameterName("Gamepad Index")] int gamepadIndex, [ParameterName("Color")] Vector3 color)
        {
            return InternalCalls.Input_SetGamepadLED(gamepadIndex, ref color);
        }
    }
}