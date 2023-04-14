using System;
using System.Reflection.Emit;
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
        internal extern static string TagComponent_GetTag(ulong entityID, out string tag);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TagComponent_SetTag(ulong entityID, string tag);

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
        internal extern static void Input_GetGamepadName(int gamepadIndex, out string name);
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
        internal extern static void Physics_Raycast(ref Vector3 origin, ref Vector3 direction, float distance, out ulong entityID, out RaycastHit hit);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Physics_RaycastPoints(ref Vector3 start, ref Vector3 end, out ulong entityID, out RaycastHit hit);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Debug_DrawDebugLine(ref Vector3 start, ref Vector3 end, ref Vector3 color, float duration);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Debug_DrawDebugCube(ref Vector3 position, ref Vector3 size, ref Vector3 color, float duration);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Debug_DrawDebugSphere(ref Vector3 center, float radius, ref Vector3 color, float duration);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Debug_ClearDebugDrawing();
        
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