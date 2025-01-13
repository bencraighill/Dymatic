using System;

namespace Dymatic
{
    public class Entity
    {
        protected virtual void OnCreate() {}
        protected virtual void OnUpdate(float ts) {}
        protected virtual void OnPrePhysicsUpdate(float ts) {}
        protected virtual void OnDestroy() {}

        // Events from engine systems (e.g. physics and animation systems)
        protected virtual void OnContact(Entity other, Vector3 hitPosition, Vector3 normal) {}
        protected virtual void OnContactPersisted(Entity other, Vector3 hitPosition, Vector3 normal) {}
        protected virtual void OnContactRemoved(Entity other) {}
        protected virtual void OnTrigger(string name) {}
        protected Entity()
        {
            ID = 0;
        }

        internal Entity(ulong id)
        {
            ID = id;
        }

        public ulong ID { get; private set; }

        public static bool operator ==(Entity a, Entity b)
        {
            if (ReferenceEquals(a, b))
            {
                return true;
            }

            if (ReferenceEquals(a, null) || ReferenceEquals(b, null))
            {
                return false;
            }

            return a.ID == b.ID;
        }

        public static bool operator !=(Entity a, Entity b)
        {
            return !(a == b);
        }

        public string Tag
        {
            get
            {
                InternalCalls.TagComponent_GetTag(ID, out string name);
                return name;
            }
            set
            {
                InternalCalls.TagComponent_SetTag(ID, value);
            }
        }

        public Vector3 Translation
        {
            get
            {
                InternalCalls.TransformComponent_GetTranslation(ID, out Vector3 result);
                return result;
            }
            set
            {
                InternalCalls.TransformComponent_SetTranslation(ID, ref value);
            }
        }

        public Vector3 Rotation
        {
            get
            {
                InternalCalls.TransformComponent_GetRotation(ID, out Vector3 rotation);
                return rotation;
            }
            set
            {
                InternalCalls.TransformComponent_SetRotation(ID, ref value);
            }
        }

        public Vector3 Scale
        {
            get
            {
                InternalCalls.TransformComponent_GetScale(ID, out Vector3 scale);
                return scale;
            }
            set
            {
                InternalCalls.TransformComponent_SetScale(ID, ref value);
            }
        }

        public T AddComponent<T>() where T : Component, new()
        {
            if (HasComponent<T>())
                return null;

            Type componentType = typeof(T);
            InternalCalls.Entity_AddComponent(ID, componentType);

            T component = new T() { Entity = this };
            return component;
        }

        public T AddComponent<T>(params object[] args) where T : Component
        {
            if (HasComponent<T>())
                return null;

            var constructorArguments = new object[args.Length + 1];
            constructorArguments[0] = ID;
            for (int i = 0; i < args.Length; i++)
                constructorArguments[i + 1] = args[i];

            Type[] constructorArgumentTypes = new Type[args.Length + 1];
            constructorArgumentTypes[0] = typeof(ulong);
            for (int i = 0; i < args.Length; i++)
                constructorArgumentTypes[i + 1] = args[i].GetType();

            Type componentType = typeof(T);
            var constructor =  componentType.GetConstructor(constructorArgumentTypes);
            if (constructor == null)
                throw new ArgumentException($"No matching constructor found for {componentType.Name} with the provided arguments.");

            T component = (T)constructor.Invoke(constructorArguments);
            component.Entity = this;

            return component;
        }

        public void Parent(Entity parent)
        {
            InternalCalls.Entity_Parent(ID, parent.ID);
        }

        public void Unparent()
        {
            InternalCalls.Entity_Unparent(ID);
        }

        public bool HasComponent<T>() where T : Component
        {
            Type componentType = typeof(T);
            return InternalCalls.Entity_HasComponent(ID, componentType);
        }

        public T GetComponent<T>() where T : Component, new()
        {
            if (!HasComponent<T>())
                return null;

            T component = new T() { Entity = this };
            return component;
        }

        public void RemoveComponent<T>() where T : Component, new()
        {
            if (!HasComponent<T>())
                return;

            Type componentType = typeof(T);
            InternalCalls.Entity_RemoveComponent(ID, componentType);
        }

        public Entity Duplicate()
        {
            ulong entityID = InternalCalls.Entity_Duplicate(ID);
            return InternalCalls.Entity_GetEntityByID(entityID);
        }

        public void Destroy()
        {
            InternalCalls.Entity_Destroy(ID);
            ID = 0;
        }

        public bool IsAlive()
        {
            return ID != 0;
        }

        public bool HasParent()
        {
            return InternalCalls.Entity_HasParent(ID);
        }

        public Entity GetParent()
        {
            return InternalCalls.Entity_GetParent(ID);
        }

        public bool HasChildren()
        {
            return InternalCalls.Entity_GetChildCount(ID) > 0;
        }

        public uint GetChildCount()
        {
            return InternalCalls.Entity_GetChildCount(ID);
        }

        public Entity[] GetChildren()
        {
            return InternalCalls.Entity_GetChildren(ID);
        }

        public void Attach(string boneName)
        {
            InternalCalls.Entity_Attach(ID, boneName);
        }

        public string GetAttachment()
        {
            InternalCalls.Entity_GetAttachment(ID, out string boneName);
            return boneName;
        }

        public static Entity GetEntityByID(ulong entityID)
        {
            return InternalCalls.Entity_GetEntityByID(entityID);
        }

        public static Entity FindEntityByName(string name)
        {
            return InternalCalls.Entity_FindEntityByName(name);
        }

        public static Entity Create(string name)
        {
            return InternalCalls.Entity_Create(name);
        }

        public static Entity Create<T>(string name) where T : Entity
        {
            return InternalCalls.Entity_CreateWithScript(name, typeof(T));
        }

        public T As<T>() where T : Entity, new()
        {
            object instance = InternalCalls.GetScriptInstance(ID);
            return instance as T;
        }

        public void ReplicateMethod(string methodName, params object[] args)
        {
            InternalCalls.Network_ReplicateMethod(ID, methodName, args);
        }
    }

}