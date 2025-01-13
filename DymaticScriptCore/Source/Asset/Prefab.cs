namespace Dymatic
{
    public class Prefab : Asset
    {
        public Prefab() : base() { }
        public Prefab(ulong handle) : base(handle) { }

        public Entity Instantiate()
        {
            return InternalCalls.Prefab_Instantiate(Handle);
        }

        public Entity Instantiate(Transform transform)
        {
            return InternalCalls.Prefab_InstantiateAtTransform(Handle, ref transform);
        }
    }
}
