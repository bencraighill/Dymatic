#pragma once

#include "Dymatic/Scene/Scene.h"
#include "Dymatic/Scene/Entity.h"

#include <vector>
#include <filesystem>
#include <string>
#include <map>

extern "C" {
	typedef struct _MonoClass MonoClass;
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoMethod MonoMethod;
	typedef struct _MonoAssembly MonoAssembly;
	typedef struct _MonoImage MonoImage;
	typedef struct _MonoClassField MonoClassField;
	typedef struct _MonoDomain MonoDomain;
}

namespace Dymatic {

	enum class ScriptFieldType
	{
		None = 0,
		Float, Double,
		Bool, Char, Byte, Short, Int, Long,
		UShort, UInt, ULong,
		Vector2, Vector3, Vector4,
		Entity,
		Asset, Scene, Texture, VirtualTexture, Mesh, Animation, Audio, Material, VideoPlayer
	};

	struct ScriptField
	{
		ScriptFieldType Type;
		std::string Name;

		MonoClassField* ClassField;
	};

	// ScriptField + data storage
	struct ScriptFieldInstance
	{
		ScriptField Field;

		ScriptFieldInstance()
		{
			memset(m_Buffer, 0, sizeof(m_Buffer));
		}

		template<typename T>
		T GetValue() const
		{
			static_assert(sizeof(T) <= 16, "Type too large!");
			return *(T*)m_Buffer;
		}

		template<typename T>
		void SetValue(T value)
		{
			static_assert(sizeof(T) <= 16, "Type too large!");
			memcpy(m_Buffer, &value, sizeof(T));
		}
	private:
		uint8_t m_Buffer[16];

		friend class ScriptEngine;
		friend class ScriptInstance;
	};

	using ScriptFieldMap = std::unordered_map<std::string, ScriptFieldInstance>;

	class ScriptClass
	{
	public:
		ScriptClass() = default;
		ScriptClass(const std::string& classNamespace, const std::string& className, bool isCore = false);

		MonoObject* Instantiate();
		MonoMethod* GetMethod(const std::string& name, int parameterCount);

		const std::map<std::string, ScriptField>& GetFields() const { return m_Fields; }
	private:
		std::string m_ClassNamespace;
		std::string m_ClassName;

		std::map<std::string, ScriptField> m_Fields;

		MonoClass* m_MonoClass = nullptr;

		friend class ScriptEngine;
		friend class ScriptInstance;
	};

	class ScriptInstance
	{
	public:
		ScriptInstance(Ref<ScriptClass> scriptClass, Entity entity);

		void InvokeOnCreate();
		void InvokeOnUpdate(float ts);
		void InvokeOnPrePhysicsUpdate(float ts);
		void InvokeOnDestroy();
		void InvokeOnContact(Entity other, const glm::vec3& hitPosition, const glm::vec3& normal);
		void InvokeOnContactPersisted(Entity other, const glm::vec3& hitPosition, const glm::vec3& normal);
		void InvokeOnContactRemoved(Entity other);

		Ref<ScriptClass> GetScriptClass() { return m_ScriptClass; }

		template<typename T>
		T GetFieldValue(const std::string& name)
		{
			static_assert(sizeof(T) <= 16, "Type too large!");

			bool success = GetFieldValueInternal(name, s_FieldValueBuffer);
			if (!success)
				return T();

			return *(T*)s_FieldValueBuffer;
		}

		template<typename T>
		void SetFieldValue(const std::string& name, T value)
		{
			static_assert(sizeof(T) <= 16, "Type too large!");

			SetFieldValueInternal(name, &value);
		}

		MonoObject* GetManagedObject() { return m_Instance; }
	private:
		bool GetFieldValueInternal(const std::string& name, void* buffer);
		bool SetFieldValueInternal(const std::string& name, const void* value);
	private:
		Ref<ScriptClass> m_ScriptClass;

		MonoObject* m_Instance = nullptr;
		MonoMethod* m_Constructor = nullptr;
		MonoMethod* m_OnCreateMethod = nullptr;
		MonoMethod* m_OnUpdateMethod = nullptr;
		MonoMethod* m_OnPrePhysicsUpdateMethod = nullptr;
		MonoMethod* m_OnDestroyMethod = nullptr;
		MonoMethod* m_OnContactMethod = nullptr;
		MonoMethod* m_OnContactPersistedMethod = nullptr;
		MonoMethod* m_OnContactRemovedMethod = nullptr;

		inline static char s_FieldValueBuffer[16];

		friend class ScriptEngine;
		friend struct ScriptFieldInstance;
	};
	
	struct ParameterDeclaration
	{
		std::string Name;
		std::string Type;
		std::string Value;

		bool ByRef = false;
		bool IsOut = false;
		
		bool NoPinLabel = false;
	};

	struct MethodDeclaration
	{
		std::string FullName;
		std::string Name;
		std::string DisplayName;
		
		bool IsPure = false;
		bool IsConversion = false;
		bool IsCompactNode = false;
		bool NoPinLabels = false;
		
		std::vector<ParameterDeclaration> Parameters;
		std::string ReturnType;


		std::string Category;
		std::string Keywords;
	};

	class ScriptEngine
	{
	public:
		static void Init();
		static void Shutdown();

		static bool LoadAssembly(const std::filesystem::path& filepath);
		static bool LoadAppAssembly(const std::filesystem::path& filepath);

		static void SetCoreAssemblyPath(const std::filesystem::path& filepath);
		static void SetAppAssemblyPath(const std::filesystem::path& filepath);
		static void ReloadAssembly();

		static void OnRuntimeStart(Scene* scene);
		static void OnRuntimeStop();

		static bool EntityClassExists(const std::string& fullClassName);
		static void OnCreateEntity(Entity entity);
		static void OnPrePhysicsUpdateEntity(Entity entity, Timestep ts);
		static void OnUpdateEntity(Entity entity, Timestep ts);
		static void OnDestroyEntity(Entity entity);
		static void OnContactEntity(Entity entity, Entity other, const glm::vec3& hitPosition, const glm::vec3& normal);
		static void OnContactPersistedEntity(Entity entity, Entity other, const glm::vec3& hitPosition, const glm::vec3& normal);
		static void OnContactRemovedEntity(Entity entity, Entity other);

		static Scene* GetSceneContext();
		static Ref<ScriptInstance> GetEntityScriptInstance(UUID entityID);

		static Ref<ScriptClass> GetEntityClass(const std::string& name);
		static const std::unordered_map<std::string, Ref<ScriptClass>>& GetEntityClasses();
		static ScriptFieldMap& GetScriptFieldMap(Entity entity);

		static void* RegisterThread();
		static void UnregisterThread(void* context);

		static MonoImage* GetCoreAssemblyImage();
		static MonoDomain* GetRootDomain();
		static MonoDomain* GetApplicationDomain();

		static MonoObject* GetManagedInstance(UUID uuid);
		static MonoObject* GetManagedInstanceOrDefaultEntity(UUID uuid);

		static void ExecuteEntityMethod(UUID entityID, const std::string& methodName, const Buffer& parameterData);

		// Editor only
		static bool IsDebuggerAttached();
		static void SetAssemblyReloadCallback(const std::function<void()> callback);

		static std::vector<std::string> GetEntityClassOverridableMethods(const std::string& className);
		static const std::unordered_map<std::string, std::vector<MethodDeclaration>> GetEntityClassOverridableMethods();
		static const std::unordered_map<std::string, MethodDeclaration>& GetStaticMethodDeclarations();
		static bool IsConversionAvalible(const std::string& typeA, const std::string& typeB);
		static std::string GetConversionName(const std::string& typeA, const std::string& typeB);

	private:
		static void InitMono();
		static void ShutdownMono();

		static MonoObject* InstantiateClass(MonoClass* monoClass);
		static void LoadAssemblyClasses();

		static MonoObject* InvokeMethod(MonoObject* instance, MonoMethod* method, void** params = nullptr);

		// Editor Only
		static void GenerateScriptMetadata();

		friend class ScriptClass;
		friend class ScriptInstance;
		friend class ScriptGlue;
	};

	namespace Utils {

		inline const char* ScriptFieldTypeToString(ScriptFieldType fieldType)
		{
			switch (fieldType)
			{
			case ScriptFieldType::None:				return "None";
			case ScriptFieldType::Float:			return "Float";
			case ScriptFieldType::Double:			return "Double";
			case ScriptFieldType::Bool:				return "Bool";
			case ScriptFieldType::Char:				return "Char";
			case ScriptFieldType::Byte:				return "Byte";
			case ScriptFieldType::Short:			return "Short";
			case ScriptFieldType::Int:				return "Int";
			case ScriptFieldType::Long:				return "Long";
			case ScriptFieldType::UShort:			return "UShort";
			case ScriptFieldType::UInt:				return "UInt";
			case ScriptFieldType::ULong:			return "ULong";
			case ScriptFieldType::Vector2:			return "Vector2";
			case ScriptFieldType::Vector3:			return "Vector3";
			case ScriptFieldType::Vector4:			return "Vector4";
			case ScriptFieldType::Entity:			return "Entity";
			case ScriptFieldType::Asset:			return "Asset";
			case ScriptFieldType::Scene:			return "Scene";
			case ScriptFieldType::Texture:			return "Texture";
			case ScriptFieldType::VirtualTexture:	return "VirtualTexture";
			case ScriptFieldType::Mesh:				return "Mesh";
			case ScriptFieldType::Animation:		return "Animation";
			case ScriptFieldType::Material:			return "Material";
			case ScriptFieldType::Audio:			return "Audio";
			case ScriptFieldType::VideoPlayer:		return "VideoPlayer";
			}
			DY_CORE_ASSERT(false, "Unknown ScriptFieldType");
			return "None";
		}

		inline ScriptFieldType ScriptFieldTypeFromString(const std::string& fieldType)
		{
			if (fieldType == "None")			return ScriptFieldType::None;
			if (fieldType == "Float")			return ScriptFieldType::Float;
			if (fieldType == "Double")			return ScriptFieldType::Double;
			if (fieldType == "Bool")			return ScriptFieldType::Bool;
			if (fieldType == "Char")			return ScriptFieldType::Char;
			if (fieldType == "Byte")			return ScriptFieldType::Byte;
			if (fieldType == "Short")			return ScriptFieldType::Short;
			if (fieldType == "Int")				return ScriptFieldType::Int;
			if (fieldType == "Long")			return ScriptFieldType::Long;
			if (fieldType == "UShort")			return ScriptFieldType::UShort;
			if (fieldType == "UInt")			return ScriptFieldType::UInt;
			if (fieldType == "ULong")			return ScriptFieldType::ULong;
			if (fieldType == "Vector2")			return ScriptFieldType::Vector2;
			if (fieldType == "Vector3")			return ScriptFieldType::Vector3;
			if (fieldType == "Vector4")			return ScriptFieldType::Vector4;
			if (fieldType == "Entity")			return ScriptFieldType::Entity;
			if (fieldType == "Asset")			return ScriptFieldType::Asset;
			if (fieldType == "Scene")			return ScriptFieldType::Scene;
			if (fieldType == "Texture")			return ScriptFieldType::Texture;
			if (fieldType == "VirtualTexture")	return ScriptFieldType::VirtualTexture;
			if (fieldType == "Mesh")			return ScriptFieldType::Mesh;
			if (fieldType == "Animation")		return ScriptFieldType::Animation;
			if (fieldType == "Material")		return ScriptFieldType::Material;
			if (fieldType == "Audio")			return ScriptFieldType::Audio;
			if (fieldType == "VideoPlayer")		return ScriptFieldType::VideoPlayer;

			DY_CORE_ASSERT(false, "Unknown ScriptFieldType");
			return ScriptFieldType::None;
		}

	}

}