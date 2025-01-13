using System.Linq.Expressions;
using System.Runtime.InteropServices;

namespace Dymatic
{
    public class Physics
    {
        [IsEditorCallable]
        public static RaycastHit Raycast([ParameterName("Origin")] Vector3 origin, [ParameterName("Direction")] Vector3 direction, [ParameterName("Distance")] float distance)
        {
            InternalCalls.Physics_Raycast(ref origin, ref direction, distance, out RaycastHit hit);
            return hit;
        }

        [IsEditorCallable]
        public static RaycastHit Raycast([ParameterName("Start")] Vector3 start, [ParameterName("End")] Vector3 end)
        {
            InternalCalls.Physics_RaycastPoints(ref start, ref end, out RaycastHit hit);
            return hit;
        }

        [IsEditorCallable]
        public static RaycastHit[] RaycastMultihit([ParameterName("Origin")] Vector3 origin, [ParameterName("Direction")] Vector3 direction, [ParameterName("Distance")] float distance)
        {
            return InternalCalls.Physics_RaycastMultihit(ref origin, ref direction, distance);
        }

        [IsEditorCallable]
        public static RaycastHit BoxShapeCast([ParameterName("Origin")] Vector3 origin, [ParameterName("Direction")] Vector3 direction, [ParameterName("Distance")] float distance, [ParameterName("Half Size")] Vector3 halfSize, [ParameterName("Orientation")] Vector3 orientation)
        {
            InternalCalls.Physics_BoxShapeCast(ref origin, ref direction, distance, ref halfSize, ref orientation, out RaycastHit hit);
            return hit;
        }

        [IsEditorCallable]
        public static RaycastHit[] BoxShapeCastMultihit([ParameterName("Origin")] Vector3 origin, [ParameterName("Direction")] Vector3 direction, [ParameterName("Distance")] float distance, [ParameterName("Half Size")] Vector3 halfSize, [ParameterName("Orientation")] Vector3 orientation)
        {
            return InternalCalls.Physics_BoxShapeCastMultihit(ref origin, ref direction, distance, ref halfSize, ref orientation);
        }

        [IsEditorCallable]
        public static RaycastHit CapsuleShapeCast([ParameterName("Origin")] Vector3 origin, [ParameterName("Direction")] Vector3 direction, [ParameterName("Distance")] float distance, [ParameterName("Radius")] float radius, [ParameterName("Half Height")] float halfHeight)
        {
            InternalCalls.Physics_CapsuleShapeCast(ref origin, ref direction, distance, radius, halfHeight, out RaycastHit hit);
            return hit;
        }

        [IsEditorCallable]
        public static RaycastHit[] CapsuleShapeCastMultihit([ParameterName("Origin")] Vector3 origin, [ParameterName("Direction")] Vector3 direction, [ParameterName("Distance")] float distance, [ParameterName("Radius")] float radius, [ParameterName("Half Height")] float halfHeight)
        {
            return InternalCalls.Physics_CapsuleShapeCastMultihit(ref origin, ref direction, distance, radius, halfHeight);
        }

        [IsEditorCallable]
        public static RaycastHit SphereShapeCast([ParameterName("Origin")] Vector3 origin, [ParameterName("Direction")] Vector3 direction, [ParameterName("Distance")] float distance, [ParameterName("Radius")] float radius)
        {
            InternalCalls.Physics_SphereShapeCast(ref origin, ref direction, distance, radius, out RaycastHit hit);
            return hit;
        }

        [IsEditorCallable]
        public static RaycastHit[] SphereShapeCastMultihit([ParameterName("Origin")] Vector3 origin, [ParameterName("Direction")] Vector3 direction, [ParameterName("Distance")] float distance, [ParameterName("Radius")] float radius)
        {
            return InternalCalls.Physics_SphereShapeCastMultihit(ref origin, ref direction, distance, radius);
        }

        [IsEditorCallable]
        public static Entity[] GetEntitiesInBounds([ParameterName("Min")] Vector3 min, [ParameterName("Max")] Vector3 max)
        {
            return InternalCalls.Physics_GetEntitiesInBounds(ref min, ref max);
        }

        [IsEditorCallable]
        public static Entity[] GetEntitiesInBounds([ParameterName("Min")] Vector3 min, [ParameterName("Max")] Vector3 max, [ParameterName("Layer")] string layer)
        {
            return InternalCalls.Physics_GetEntitiesInBoundsOnLayer(ref min, ref max, layer);
        }
    }
}