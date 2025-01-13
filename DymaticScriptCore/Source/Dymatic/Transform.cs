using System.Runtime.InteropServices;

namespace Dymatic
{
    public struct Transform
    {
        public Vector3 Translation, Rotation, Scale;

        public static Transform Zero => new Transform(new Vector3(0.0f), new Vector3(0.0f), new Vector3(1.0f));

        public Transform(Vector3 translation)
        {
            Translation = translation;
            Rotation = new Vector3(0.0f);
            Scale = new Vector3(1.0f);
        }

        public Transform(Vector3 translation, Vector3 rotation, Vector3 scale)
        {
            Translation = translation;
            Rotation = rotation;
            Scale = scale;
        }
    }
}
