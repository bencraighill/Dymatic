namespace Dymatic
{
    public struct RaycastHit
    {
        public bool Hit;
        public Entity Entity;
        public float Distance;
        public Vector3 Position;
        public Vector3 Normal;
    }
}