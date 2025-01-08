namespace Dymatic
{
    public class Material : Asset
    {
        public Material() : base() {}
        public Material(ulong handle) : base(handle) {}

        public MaterialInstance CreateInstance()
        {
            ulong handle = InternalCalls.Material_CreateInstance(Handle);
            return handle == 0 ? null : new MaterialInstance(handle);
        }
    }
}
