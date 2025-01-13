namespace Dymatic
{
    public class VideoPlayer : Asset
    {
        public VideoPlayer() : base() {}
        public VideoPlayer(ulong handle) : base(handle) {}

        public void SetTime(float time)
        {
            InternalCalls.VideoPlayer_SetTime(Handle, time);
        }

        public void Update(float ts)
        {
            InternalCalls.VideoPlayer_Update(Handle, ts);
        }

        public string GetSubtitle()
        {
            InternalCalls.VideoPlayer_GetSubtitle(Handle, out string subtitle);
            return subtitle;
        }

    }
}
