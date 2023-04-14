using System.Runtime.InteropServices;

namespace Dymatic
{
    public class Physics
    {
        [IsEditorCallable]
        public static RaycastHit Raycast([ParameterName("Origin")] Vector3 origin, [ParameterName("Direction")] Vector3 direction, [ParameterName("Distance")] float distance)
        {
            InternalCalls.Physics_Raycast(ref origin, ref direction, distance, out ulong entityID, out RaycastHit hit);
            hit.Entity = entityID == 0 ? null : new Entity(entityID);

            return hit;
        }

        [IsEditorCallable]
        public static RaycastHit Raycast([ParameterName("Start")] Vector3 start, [ParameterName("End")] Vector3 end)
        {
            InternalCalls.Physics_RaycastPoints(ref start, ref end, out ulong entityID, out RaycastHit hit);
            hit.Entity = entityID == 0 ? null : new Entity(entityID);

            return hit;
        }
    }
}