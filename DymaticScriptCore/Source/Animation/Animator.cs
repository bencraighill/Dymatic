using System;

namespace Dymatic
{
    public abstract class Animator
    {
        protected abstract Pose OnUpdate(float ts);
    }
}