using System;

namespace Dymatic
{
    public class VirtualTexture : Asset
    {
        public VirtualTexture() : base() { }
        public VirtualTexture(ulong handle) : base(handle) { }

        public void Resize(Vector2 size)
        {
            InternalCalls.VirtualTexture_Resize(Handle, ref size);
        }
        public void Clear()
        {
            InternalCalls.VirtualTexture_Clear(Handle);
        }
        public Vector2 GetSize()
        {
            InternalCalls.VirtualTexture_GetSize(Handle, out Vector2 size);
            return size;
        }
        public uint GetPixelCount()
        {
            return InternalCalls.VirtualTexture_GetPixelCount(Handle);
        }

        public uint GetDataSize()
        {
            return InternalCalls.VirtualTexture_GetDataSize(Handle);
        }

        public byte[] GetData()
        {
            return InternalCalls.VirtualTexture_GetData(Handle);
        }

        public bool SetData<T>(T[] data)
        {
            return InternalCalls.VirtualTexture_SetData(Handle, data);
        }

    }
}
