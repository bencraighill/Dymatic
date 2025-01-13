namespace Dymatic
{
    public class MaterialInstance : Material
    {
        public MaterialInstance() : base() {}
        public MaterialInstance(ulong handle) : base(handle) {}

        public bool SetParameter(string name, float value) { return InternalCalls.MaterialInstance_SetParameterFloat(Handle, name, value); }
        public bool SetParameter(string name, Vector2 value) { return InternalCalls.MaterialInstance_SetParameterVector2(Handle, name, ref value); }
        public bool SetParameter(string name, Vector3 value) { return InternalCalls.MaterialInstance_SetParameterVector3(Handle, name, ref value); }
        public bool SetParameter(string name, Vector4 value) { return InternalCalls.MaterialInstance_SetParameterVector4(Handle, name, ref value); }
    }
}
