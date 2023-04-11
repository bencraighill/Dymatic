using System;

namespace Dymatic
{
    public class Pose
    {
        protected Pose() { Handle = 0; }
        internal Pose(ulong handle)
        {
            Handle = handle;
        }

        private readonly ulong Handle;

        public static Pose GetAnimationPose(string animation, float time) { return new Pose(InternalCalls.Animation_GetAnimationPose(animation, time)); }
        public static Pose BlendPoses(Pose basePose, Pose blendPose, float weight) { return new Pose(InternalCalls.Animation_BlendPoses(basePose.Handle, blendPose.Handle, weight)); }
    }
}