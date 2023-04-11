using System;
using System.Runtime.CompilerServices;

namespace Dymatic
{
    public static class InternalCalls
    {
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool Entity_HasComponent(ulong entityID, Type componentType);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static ulong Entity_FindEntityByName(string name);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static object GetScriptInstance(ulong entityID);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_GetTranslation(ulong entityID, out Vector3 translation);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_SetTranslation(ulong entityID, ref Vector3 translation);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Rigidbody2DComponent_ApplyLinearImpulse(ulong entityID, ref Vector2 impulse, ref Vector2 point, bool wake);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Rigidbody2DComponent_ApplyLinearImpulseToCenter(ulong entityID, ref Vector2 impulse, bool wake);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool Input_IsKeyDown(KeyCode keycode);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool Input_IsMouseButtonPressed(MouseCode mouseCode);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Input_GetMousePosition(out Vector2 position);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float Input_GetMouseX();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float Input_GetMouseY();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool Input_IsGamepadConnected(int gamepadIndex);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool Input_IsGamepadButtonPressed(int gamepadIndex, GamepadButtonCode gamepadButton);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float Input_GetGamepadAxis(int gamepadIndex, GamepadAxisCode gamepadAxis);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Input_GetGamepadSensor(int gamepadIndex, GamepadSensorCode gamepadSensor, out Vector3 value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool Input_SetGamepadRumble(int gamepadIndex, float left, float right, float duration);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool Input_SetGamepadLED(int gamepadIndex, ref Vector3 color);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Log_Trace(string message);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Log_Info(string message);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Log_Warn(string message);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Log_Error(string message);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Log_Critical(string message);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Core_Assert(bool condition, string message);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static ulong Animation_GetAnimationPose(string animation, float time);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static ulong Animation_BlendPoses(ulong basePose, ulong blendPose, float weight);
    }
}