using System;
using System.Runtime.Remoting;

namespace Dymatic
{
    public abstract class Asset
    {
        protected Asset() { Handle = 0; }

        internal Asset(ulong handle)
        {
            Handle = handle;
        }

        ~Asset()
        {
            Release();
        }

        public ulong Handle { get; protected set; }
        private bool Holding = false;

        public void Hold()
        {
            Holding = true;
            InternalCalls.Asset_RegisterScriptReference(Handle);
        }

        public void Release()
        {
            if (!Holding)
                return;

            InternalCalls.Asset_UnregisterScriptReference(Handle);
            Holding = false;
        }

        static public T GetAsset<T>(ulong handle) where T : Asset, new()
        {
            if (!InternalCalls.Asset_DoesAssetExist(handle))
                return null;

            T asset = new T();
            asset.Handle = handle;
            return asset;
        }

        static public T GetAsset<T>(string filepath) where T : Asset, new()
        {
            ulong handle = InternalCalls.Asset_GetAssetHandle(filepath);

            if (handle == 0)
                return null;

            T asset = new T();
            asset.Handle = handle;
            return asset;
        }

        public static bool operator true(Asset asset)
        {
            return asset != null && asset.Handle != 0;
        }

        public static bool operator false(Asset asset)
        {
            return asset == null || asset.Handle != 0;
        }

        public static bool operator !(Asset asset)
        {
            return asset == null || asset.Handle != 0;
        }
    }
}