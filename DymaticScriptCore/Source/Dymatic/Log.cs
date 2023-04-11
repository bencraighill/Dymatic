namespace Dymatic
{
    public class Log
    {
        [IsEditorCallable]
        public static void Trace([ParameterName("Message")] string message) { InternalCalls.Log_Trace(message); }
        [IsEditorCallable]
        public static void Info([ParameterName("Message")] string message) { InternalCalls.Log_Info(message); }
        [IsEditorCallable]
        public static void Warn([ParameterName("Message")] string message) { InternalCalls.Log_Warn(message); }
        [IsEditorCallable]
        public static void Error([ParameterName("Message")] string message) { InternalCalls.Log_Error(message); }
        [IsEditorCallable]
        public static void Critical([ParameterName("Message")] string message) { InternalCalls.Log_Critical(message); }
    }
}