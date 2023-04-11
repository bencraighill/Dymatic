namespace Dymatic
{
    public class Core
    {
        [IsEditorCallable]
        public static void Assert([ParameterName("Condition")] bool condition, [ParameterName("Message")] string message = "")
        {
            if (!condition)
            {
                InternalCalls.Core_Assert(condition, message);
            }
        }
    }
}
