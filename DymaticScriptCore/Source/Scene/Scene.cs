namespace Dymatic
{
    public class Scene : Asset
    {
        public Scene() : base() {}
        public Scene(ulong handle) : base(handle) {}
        
        static public void OpenScene(Scene scene)
        {
            InternalCalls.Scene_OpenScene(scene.Handle);
        }
    }
}
