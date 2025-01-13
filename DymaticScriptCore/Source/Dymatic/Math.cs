using System;

namespace Dymatic
{
    public static class Math
    {
        private static Vector3 Forward = new Vector3(0.0f, 0.0f, -1.0f);
        private static Vector3 Right = new Vector3(1.0f, 0.0f, 0.0f);
        private static Vector3 Up = new Vector3(0.0f, 1.0f, 0.0f);

        public static T Clamp<T>(this T val, T min, T max) where T : IComparable<T>
        {
            if (val.CompareTo(min) < 0) return min;
            else if (val.CompareTo(max) > 0) return max;
            else return val;
        }

        [IsEditorCallable]
        public static float Lerp(float a, float b, float t)
        {
            t = Math.Clamp(t, 0f, 1f);
            return a + (b - a) * t;
        }

        [IsEditorCallable]
        public static Vector3 Mix(Vector3 start, Vector3 end, float t)
        {
            t = Math.Clamp(t, 0.0f, 1.0f);
            return start + (end - start) * t;
        }

        [IsEditorCallable]
        public static float DegreesToRadians(float degrees) { return degrees * ((float)System.Math.PI / 180.0f); }
        [IsEditorCallable]
        public static float RadiansToDegrees(float radians) { return radians * (180.0f / (float)System.Math.PI); }

        [IsEditorCallable]
        public static float Sin(float a) { return (float)System.Math.Sin(DegreesToRadians(a)); }
        [IsEditorCallable]
        public static float Cos(float a) { return (float)System.Math.Cos(DegreesToRadians(a)); }

        public static Vector3 GetForward(Vector3 rotation)
        {
            InternalCalls.Math_MultiplyRotationVector(ref rotation, ref Forward, out Vector3 forward);
            return forward;
        }

        public static Vector3 GetRight(Vector3 rotation)
        {
            InternalCalls.Math_MultiplyRotationVector(ref rotation, ref Right, out Vector3 right);
            return right;
        }

        public static Vector3 GetUp(Vector3 rotation)
        {

            InternalCalls.Math_MultiplyRotationVector(ref rotation, ref Up, out Vector3 up);
            return up;
        }
    }
}