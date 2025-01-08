namespace Dymatic
{
    public class Input
    {
        [IsEditorCallable]
        public static string GetKeyboardName()
        {
            InternalCalls.Input_GetKeyboardName(out string name);
            return name;
        }

        [IsEditorCallable]
        public static bool IsKeyDown([ParameterName("Key Code")] KeyCode keyCode)
        {
            return InternalCalls.Input_IsKeyDown(keyCode);
        }

        [IsEditorCallable]
        public static string GetMouseName()
        {
            InternalCalls.Input_GetMouseName(out string name);
            return name;
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
        public static Vector2 GetMouseDelta()
        {
            InternalCalls.Input_GetMouseDelta(out Vector2 delta);
            return delta;
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
        public static void SetMouseLocked(bool locked)
        {
            InternalCalls.Input_SetMouseLocked(locked);
        }

        [IsEditorCallable]
        public static uint GetGamepadCount()
        {
            return InternalCalls.Input_GetGamepadCount();
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
        public static int GetGamepadPowerLevel([ParameterName("Gamepad Index")] int gamepadIndex)
        {
            return InternalCalls.Input_GetGamepadPowerLevel(gamepadIndex);
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
        public static Vector2 GetGamepadTouchPad([ParameterName("Gamepad Index")] int gamepadIndex)
        {
            InternalCalls.Input_GetGamepadTouchPad(gamepadIndex, out Vector2 touchPad);
            return touchPad;
        }

        [IsEditorCallable]
        public static bool SetGamepadRumble([ParameterName("Gamepad Index")] int gamepadIndex, [ParameterName("Left")] float left, [ParameterName("Right")] float right, [ParameterName("Duration")] float duration)
        {
            return InternalCalls.Input_SetGamepadRumble(gamepadIndex, left, right, duration);
        }

        [IsEditorCallable]
        public static void SetGamepadLED([ParameterName("Gamepad Index")] int gamepadIndex, [ParameterName("Color")] Vector3 color)
        {
            InternalCalls.Input_SetGamepadLED(gamepadIndex, ref color);
        }

        [IsEditorCallable]
        public static void SetGamepadPlayerLED([ParameterName("Gamepad Index")] int gamepadIndex, [ParameterName("Brightness Level")] int brightnessLevel, [ParameterName("Count")] int count, [ParameterName("Fade In")] bool fadeIn)
        {
            InternalCalls.Input_SetGamepadPlayerLED(gamepadIndex, brightnessLevel, count, fadeIn);
        }

        [IsEditorCallable]
        public static void SetGamepadMicrophoneLED([ParameterName("Gamepad Index")] int gamepadIndex, [ParameterName("Enabled")] bool enabled, [ParameterName("Pulse")] bool pulse)
        {
            InternalCalls.Input_SetGamepadMicrophoneLED(gamepadIndex, enabled, pulse);
        }

        [IsEditorCallable]
        public static void ClearTriggerEffect([ParameterName("Gamepad Index")] int gamepadIndex, [ParameterName("Trigger")] GamepadButtonCode trigger)
        {
            InternalCalls.Input_ClearTriggerEffect(gamepadIndex, trigger);
        }

        [IsEditorCallable]
        public static void SetTriggerEffect([ParameterName("Gamepad Index")] int gamepadIndex, [ParameterName("Trigger")] GamepadButtonCode trigger, [ParameterName("Start Position")] float startPosition, [ParameterName("Keep Effect")] bool keepEffect, [ParameterName("Begin Force")] float beginForce, [ParameterName("Middle Force")] float middleForce, [ParameterName("End Force")] float endForce, [ParameterName("Frequency")] float frequency)
        {
            InternalCalls.Input_SetTriggerEffect(gamepadIndex, trigger, startPosition, keepEffect, beginForce, middleForce, endForce, frequency);
        }

        [IsEditorCallable]
        public static void SetTriggerEffectContinuous([ParameterName("Gamepad Index")] int gamepadIndex, [ParameterName("Trigger")] GamepadButtonCode trigger, [ParameterName("Start Position")] float startPosition, [ParameterName("Force")] float force)
        {
            InternalCalls.Input_SetTriggerEffectContinuous(gamepadIndex, trigger, startPosition, force);
        }

        [IsEditorCallable]
        public static void SetTriggerEffectSection([ParameterName("Gamepad Index")] int gamepadIndex, [ParameterName("Trigger")] GamepadButtonCode trigger, [ParameterName("Start Position")] float startPosition, [ParameterName("End Position")] float endPosition)
        {
            InternalCalls.Input_SetTriggerEffectSection(gamepadIndex, trigger, startPosition, endPosition);
        }
    }
}