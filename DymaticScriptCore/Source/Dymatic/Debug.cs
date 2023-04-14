namespace Dymatic
{
    public class Debug
    {
        [IsEditorCallable]
        public static void DrawDebugLine([ParameterName("Start")] Vector3 start, [ParameterName("End")] Vector3 end, [ParameterName("Color")] Vector3 color, [ParameterName("Duration")] float duration = 0.0f)
        {
            InternalCalls.Debug_DrawDebugLine(ref start, ref end, ref color, duration);
        }

        [IsEditorCallable]
        public static void DrawDebugCube([ParameterName("Position")] Vector3 position, [ParameterName("Size")] Vector3 size, [ParameterName("Color")] Vector3 color, [ParameterName("Duration")] float duration = 0.0f)
        {
            InternalCalls.Debug_DrawDebugCube(ref position, ref size, ref color, duration);
        }

        [IsEditorCallable]
        public static void DrawDebugSphere([ParameterName("Center")] Vector3 center, [ParameterName("Radius")] float radius, [ParameterName("Color")] Vector3 color, [ParameterName("Duration")] float duration = 0.0f)
        {
            InternalCalls.Debug_DrawDebugSphere(ref center, radius, ref color, duration);
        }

        [IsEditorCallable]
        public static void ClearDebugLines()
        {
            InternalCalls.Debug_ClearDebugDrawing();
        }
    }
}
