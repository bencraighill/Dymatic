#include "dypch.h"
#include "Dymatic/Scripting/ScriptEngine.h"

#include "Dymatic/Scripting/ScriptGlue.h"

#include "mono/jit/jit.h"
#include "mono/metadata/assembly.h"
#include "mono/metadata/object.h"
#include "mono/metadata/tabledefs.h"
#include "mono/metadata/mono-debug.h"
#include "mono/metadata/threads.h"
#include <mono/metadata/threads.h>

#include "FileWatch.h"

#include "Dymatic/Core/Application.h"
#include "Dymatic/Core/Timer.h"
#include "Dymatic/Core/Buffer.h"
#include "Dymatic/Core/FileSystem.h"

namespace Dymatic {

	static std::unordered_map<std::string, ScriptFieldType> s_ScriptFieldTypeMap =
	{
		{ "System.Single", ScriptFieldType::Float },
		{ "System.Double", ScriptFieldType::Double },
		{ "System.Boolean", ScriptFieldType::Bool },
		{ "System.Char", ScriptFieldType::Char },
		{ "System.Int16", ScriptFieldType::Short },
		{ "System.Int32", ScriptFieldType::Int },
		{ "System.Int64", ScriptFieldType::Long },
		{ "System.Byte", ScriptFieldType::Byte },
		{ "System.UInt16", ScriptFieldType::UShort },
		{ "System.UInt32", ScriptFieldType::UInt },
		{ "System.UInt64", ScriptFieldType::ULong },

		{ "Dymatic.Vector2", ScriptFieldType::Vector2 },
		{ "Dymatic.Vector3", ScriptFieldType::Vector3 },
		{ "Dymatic.Vector4", ScriptFieldType::Vector4 },

		{ "Dymatic.Entity", ScriptFieldType::Entity },

		{ "Dymatic.Asset", ScriptFieldType::Asset },
		{ "Dymatic.Scene", ScriptFieldType::Scene },
		{ "Dymatic.Texture", ScriptFieldType::Texture },
		{ "Dymatic.VirtualTexture", ScriptFieldType::VirtualTexture },
		{ "Dymatic.Mesh", ScriptFieldType::Mesh },
		{ "Dymatic.Animation", ScriptFieldType::Animation },
		{ "Dymatic.Material", ScriptFieldType::Material },
		{ "Dymatic.Audio", ScriptFieldType::Audio },
		{ "Dymatic.VideoPlayer", ScriptFieldType::VideoPlayer }
	};

	namespace Utils {

		static MonoAssembly* LoadMonoAssembly(const std::filesystem::path& assemblyPath, bool loadPDB = false)
		{
			ScopedBuffer fileData = FileSystem::ReadBytes(assemblyPath);

			// NOTE: We can't use this image for anything other than loading the assembly because this image doesn't have a reference to the assembly
			MonoImageOpenStatus status;
			MonoImage* image = mono_image_open_from_data_full(fileData.As<char>(), fileData.Size, 1, &status, 0);

			if (status != MONO_IMAGE_OK)
			{
				const char* errorMessage = mono_image_strerror(status);

				// Log some error message using the errorMessage data
				DY_CORE_ERROR("Failed to load C# mono image: {}", errorMessage);
				
				return nullptr;
			}

			if (loadPDB)
			{
				std::filesystem::path pdbPath = assemblyPath;
				pdbPath.replace_extension(".pdb");

				if (std::filesystem::exists(pdbPath))
				{
					ScopedBuffer pdbFileData = FileSystem::ReadBytes(pdbPath);
					mono_debug_open_image_from_memory(image, pdbFileData.As<const mono_byte>(), pdbFileData.Size);
					DY_CORE_INFO("Loaded PDB {}", pdbPath);
				}
			}

			std::string pathString = assemblyPath.string();
			MonoAssembly* assembly = mono_assembly_load_from_full(image, pathString.c_str(), &status, 0);
			mono_image_close(image);

			return assembly;
		}

		void PrintAssemblyTypes(MonoAssembly* assembly)
		{
			MonoImage* image = mono_assembly_get_image(assembly);
			const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
			int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);

			for (int32_t i = 0; i < numTypes; i++)
			{
				uint32_t cols[MONO_TYPEDEF_SIZE];
				mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

				const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
				const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);
				DY_CORE_TRACE("{}.{}", nameSpace, name);
			}
		}

		ScriptFieldType MonoTypeToScriptFieldType(MonoType* monoType)
		{
			std::string typeName = mono_type_get_name(monoType);

			auto it = s_ScriptFieldTypeMap.find(typeName);
			if (it == s_ScriptFieldTypeMap.end())
			{
				DY_CORE_ERROR("Unknown type: {}", typeName);
				return ScriptFieldType::None;
			}

			return it->second;
		}

	}

	struct ScriptEngineData
	{
		MonoDomain* RootDomain = nullptr;
		MonoDomain* AppDomain = nullptr;

		MonoAssembly* CoreAssembly = nullptr;
		MonoImage* CoreAssemblyImage = nullptr;

		MonoAssembly* AppAssembly = nullptr;
		MonoImage* AppAssemblyImage = nullptr;

		std::filesystem::path CoreAssemblyFilepath;
		std::filesystem::path AppAssemblyFilepath;

		ScriptClass EntityClass;

		std::unordered_map<std::string, Ref<ScriptClass>> EntityClasses;
		std::unordered_map<UUID, Ref<ScriptInstance>> EntityInstances;
		std::unordered_map<UUID, ScriptFieldMap> EntityScriptFields;

		Scope<filewatch::FileWatch<std::string>> AppAssemblyFileWatcher;
		bool AssemblyReloadPending = false;

		bool EnableDebugging = true;

		// Runtime
		Scene* SceneContext = nullptr;

		// Editor only
		std::unordered_map<std::string, MethodDeclaration> StaticMethods;
		std::unordered_map<std::string, std::vector<MethodDeclaration>> EntityClassOverridableMethods;
		std::function<void()> AssemblyReloadCallback;
		
		MonoClass* IsEditorCallableAttributeClass;
		MonoClass* ParameterNameAttributeClass;
		MonoClass* NoPinLabelAttributeClass;
		MonoClass* DisplayNameAttributeClass;
		MonoClass* IsPureAttributeClass;
		MonoClass* IsConversionAttributeClass;
		MonoClass* IsCompactNodeAttributeClass;
		MonoClass* NoPinLabelsAttributeClass;
		MonoClass* MethodCategoryAttributeClass;
		MonoClass* MethodKeywordsAttributeClass;
	};

	static ScriptEngineData* s_Data = nullptr;

	static void OnAppAssemblyFileSystemEvent(const std::string& path, const filewatch::Event change_type)
	{
		if (!s_Data->AssemblyReloadPending && change_type == filewatch::Event::modified)
		{
			s_Data->AssemblyReloadPending = true;

			Application::Get().SubmitToMainThread([]()
			{
				s_Data->AssemblyReloadCallback();
				s_Data->AppAssemblyFileWatcher.reset();
				ScriptEngine::ReloadAssembly();
			});
		}
	}

	void ScriptEngine::Init()
	{
		s_Data = new ScriptEngineData();

		InitMono();
		ScriptGlue::RegisterFunctions();
	}

	void ScriptEngine::Shutdown()
	{
		ShutdownMono();
		delete s_Data;
	}

	void ScriptEngine::InitMono()
	{
		mono_set_assemblies_path("mono/lib");

#ifdef DY_DEBUG
		if (s_Data->EnableDebugging)
		{
			const char* argv[2] = {
				"--debugger-agent=transport=dt_socket,address=127.0.0.1:2550,server=y,suspend=n,loglevel=3,logfile=logs/MonoDebugger.log",
				"--soft-breakpoints"
			};

			mono_jit_parse_options(2, (char**)argv);
			mono_debug_init(MONO_DEBUG_FORMAT_MONO);
		}
#endif

		MonoDomain* rootDomain = mono_jit_init("DymaticJITRuntime");
		DY_CORE_ASSERT(rootDomain);

		// Store the root domain pointer
		s_Data->RootDomain = rootDomain;

#ifdef DY_DEBUG
		if (s_Data->EnableDebugging)
			mono_debug_domain_create(s_Data->RootDomain);
#endif DY_DEBUG

		mono_thread_set_main(mono_thread_current());
	}

	void ScriptEngine::ShutdownMono()
	{
		mono_domain_set(mono_get_root_domain(), false);

		if (s_Data->AppDomain)
		{
			mono_domain_unload(s_Data->AppDomain);
			s_Data->AppDomain = nullptr;
		}

		if (s_Data->RootDomain)
		{
			mono_jit_cleanup(s_Data->RootDomain);
			s_Data->RootDomain = nullptr;
		}
	}

	bool ScriptEngine::LoadAssembly(const std::filesystem::path& filepath)
	{
		// Create an App Domain
		s_Data->AppDomain = mono_domain_create_appdomain("DymaticScriptRuntime", nullptr);
		mono_domain_set(s_Data->AppDomain, true);

		if (!std::filesystem::exists(filepath))
			return false;

		s_Data->CoreAssemblyFilepath = filepath;
		s_Data->CoreAssembly = Utils::LoadMonoAssembly(filepath, s_Data->EnableDebugging);
		if (s_Data->CoreAssembly == nullptr)
			return false;

		s_Data->CoreAssemblyImage = mono_assembly_get_image(s_Data->CoreAssembly);
		return true;
	}

	bool ScriptEngine::LoadAppAssembly(const std::filesystem::path& filepath)
	{
		if (!std::filesystem::exists(filepath))
			return false;

		s_Data->AppAssemblyFilepath = filepath;
		s_Data->AppAssembly = Utils::LoadMonoAssembly(filepath, s_Data->EnableDebugging);
		if (s_Data->AppAssembly == nullptr)
			return false;

		s_Data->AppAssemblyImage = mono_assembly_get_image(s_Data->AppAssembly);

		s_Data->AppAssemblyFileWatcher = CreateScope<filewatch::FileWatch<std::string>>(filepath.string(), OnAppAssemblyFileSystemEvent);
		s_Data->AssemblyReloadPending = false;
		return true;
	}

	void ScriptEngine::SetCoreAssemblyPath(const std::filesystem::path& filepath)
	{
		s_Data->CoreAssemblyFilepath = filepath;
	}

	void ScriptEngine::SetAppAssemblyPath(const std::filesystem::path& filepath)
	{
		s_Data->AppAssemblyFilepath = filepath;
	}

	void ScriptEngine::ReloadAssembly()
	{
		DY_CORE_INFO("Reloading Script Assembly...");

		mono_domain_set(mono_get_root_domain(), false);

		if (s_Data->AppDomain)
			mono_domain_unload(s_Data->AppDomain);

		DY_CORE_INFO("Loading Core Assembly '{}'", s_Data->CoreAssemblyFilepath);
		s_Data->CoreAssembly = nullptr;
		s_Data->CoreAssemblyImage = nullptr;
		LoadAssembly(s_Data->CoreAssemblyFilepath);

		DY_CORE_INFO("Loading App Assembly '{}'", s_Data->AppAssemblyFilepath);
		s_Data->AppAssembly = nullptr;
		s_Data->AppAssemblyImage = nullptr;
		LoadAppAssembly(s_Data->AppAssemblyFilepath);

		LoadAssemblyClasses();

		GenerateScriptMetadata();

		ScriptGlue::RegisterComponents();

		// Retrieve and instantiate class
		if (s_Data->CoreAssemblyImage)
			s_Data->EntityClass = ScriptClass("Dymatic", "Entity", true);
	}

	bool ScriptEngine::EntityClassExists(const std::string& fullClassName)
	{
		return s_Data->EntityClasses.find(fullClassName) != s_Data->EntityClasses.end();
	}

	void ScriptEngine::OnCreateEntity(Entity entity)
	{
		if (!s_Data->SceneContext)
			return;

		const auto& sc = entity.GetComponent<ScriptComponent>();
		if (ScriptEngine::EntityClassExists(sc.ClassName))
		{
			UUID entityID = entity.GetUUID();

			Ref<ScriptInstance> instance = CreateRef<ScriptInstance>(s_Data->EntityClasses[sc.ClassName], entity);
			s_Data->EntityInstances[entityID] = instance;

			// Copy field values
			if (s_Data->EntityScriptFields.find(entityID) != s_Data->EntityScriptFields.end())
			{
				const ScriptFieldMap& fieldMap = s_Data->EntityScriptFields.at(entityID);
				for (const auto& [name, fieldInstance] : fieldMap)
					instance->SetFieldValueInternal(name, fieldInstance.m_Buffer);
			}

			instance->InvokeOnCreate();
		}
	}

	void ScriptEngine::OnUpdateEntity(Entity entity, Timestep ts)
	{
		UUID entityUUID = entity.GetUUID();

		if (s_Data->EntityInstances.find(entityUUID) == s_Data->EntityInstances.end())
		{
			DY_CORE_ERROR("Could not find ScriptInstance for entity {}", entityUUID);
			return;
		}

		Ref<ScriptInstance> instance = s_Data->EntityInstances.at(entityUUID);
		instance->InvokeOnUpdate((float)ts);
	}

	void ScriptEngine::OnPrePhysicsUpdateEntity(Entity entity, Timestep ts)
	{
		EntityHandle entityUUID = entity.GetUUID();

		if (s_Data->EntityInstances.find(entityUUID) == s_Data->EntityInstances.end())
		{
			DY_CORE_ERROR("Could not find ScriptInstance for entity {}", entityUUID);
			return;
		}

		Ref<ScriptInstance> instance = s_Data->EntityInstances.at(entityUUID);
		instance->InvokeOnPrePhysicsUpdate((float)ts);
	}

	void ScriptEngine::OnDestroyEntity(Entity entity)
	{
		if (!s_Data->SceneContext)
			return;

		UUID entityUUID = entity.GetUUID();
		if (s_Data->EntityInstances.find(entityUUID) != s_Data->EntityInstances.end())
		{
			Ref<ScriptInstance> instance = s_Data->EntityInstances.at(entityUUID);
			instance->InvokeOnDestroy();
		}
		else
			DY_CORE_ERROR("Could not find ScriptInstance for entity {}", entityUUID);
	}

	void ScriptEngine::OnContactEntity(Entity entity, Entity other, const glm::vec3& hitPosition, const glm::vec3& normal)
	{
		if (Ref<ScriptInstance> instance = GetEntityScriptInstance(entity.GetUUID()))
			instance->InvokeOnContact(other, hitPosition, normal);
	}

	void ScriptEngine::OnContactPersistedEntity(Entity entity, Entity other, const glm::vec3& hitPosition, const glm::vec3& normal)
	{
		if (Ref<ScriptInstance> instance = GetEntityScriptInstance(entity.GetUUID()))
			instance->InvokeOnContactPersisted(other, hitPosition, normal);
	}

	void ScriptEngine::OnContactRemovedEntity(Entity entity, Entity other)
	{
		if (Ref<ScriptInstance> instance = GetEntityScriptInstance(entity.GetUUID()))
			instance->InvokeOnContactRemoved(other);
	}

	Scene* ScriptEngine::GetSceneContext()
	{
		return s_Data->SceneContext;
	}

	Ref<ScriptInstance> ScriptEngine::GetEntityScriptInstance(UUID entityID)
	{
		auto it = s_Data->EntityInstances.find(entityID);
		if (it == s_Data->EntityInstances.end())
			return nullptr;

		return it->second;
	}


	Ref<ScriptClass> ScriptEngine::GetEntityClass(const std::string& name)
	{
		if (s_Data->EntityClasses.find(name) == s_Data->EntityClasses.end())
			return nullptr;

		return s_Data->EntityClasses.at(name);
	}

	void ScriptEngine::OnRuntimeStart(Scene* scene)
	{
		s_Data->SceneContext = scene;
		
		ScriptGlue::OnRuntimeStart();
	}

	void ScriptEngine::OnRuntimeStop()
	{
		s_Data->SceneContext = nullptr;
		s_Data->EntityInstances.clear();

		ScriptGlue::OnRuntimeStop();
	}

	const std::unordered_map<std::string, Ref<ScriptClass>>& ScriptEngine::GetEntityClasses()
	{
		return s_Data->EntityClasses;
	}

	ScriptFieldMap& ScriptEngine::GetScriptFieldMap(Entity entity)
	{
		DY_CORE_ASSERT(entity);

		UUID entityID = entity.GetUUID();
		return s_Data->EntityScriptFields[entityID];
	}

	std::vector<std::string> ScriptEngine::GetEntityClassOverridableMethods(const std::string& className)
	{
		std::vector<std::string> methods;
		
		Ref<ScriptClass> entityClass = GetEntityClass(className);
		ScriptClass scriptClass = entityClass ? *entityClass : s_Data->EntityClass;
		
		{
			MonoClass* klass = scriptClass.m_MonoClass;
			while (klass)
			{
				void* iter = nullptr;
				MonoMethod* method;
				while (method = mono_class_get_methods(klass, &iter))
				{
					auto flags = mono_method_get_flags(method, nullptr);

					if (flags & (METHOD_ATTRIBUTE_VIRTUAL | METHOD_ATTRIBUTE_NEW_SLOT))
						methods.push_back(mono_method_get_name(method));
				}

				klass = mono_class_get_parent(klass);
			}
		}
		
		return methods;
	}

	const std::unordered_map<std::string, std::vector<Dymatic::MethodDeclaration>> ScriptEngine::GetEntityClassOverridableMethods()
	{
		return s_Data->EntityClassOverridableMethods;
	}

	const std::unordered_map<std::string, MethodDeclaration>& ScriptEngine::GetStaticMethodDeclarations()
	{
		return s_Data->StaticMethods;
	}

	bool ScriptEngine::IsConversionAvalible(const std::string& typeA, const std::string& typeB)
	{
		for (auto& [name, method] : s_Data->StaticMethods)
			if (method.IsConversion)
				if (method.Parameters[0].Type == typeA && method.ReturnType == typeB)
					return true;
		return false;
	}

	std::string ScriptEngine::GetConversionName(const std::string& typeA, const std::string& typeB)
	{
		for (auto& [name, method] : s_Data->StaticMethods)
			if (method.IsConversion)
				if (method.Parameters[0].Type == typeA && method.ReturnType == typeB)
					return name;
		return "";
	}

	void ScriptEngine::LoadAssemblyClasses()
	{
		s_Data->EntityClasses.clear();

		if (!s_Data->CoreAssemblyImage || !s_Data->AppAssemblyImage)
			return;

		const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(s_Data->AppAssemblyImage, MONO_TABLE_TYPEDEF);
		int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);
		MonoClass* entityClass = mono_class_from_name(s_Data->CoreAssemblyImage, "Dymatic", "Entity");

		for (int32_t i = 0; i < numTypes; i++)
		{
			uint32_t cols[MONO_TYPEDEF_SIZE];
			mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

			const char* nameSpace = mono_metadata_string_heap(s_Data->AppAssemblyImage, cols[MONO_TYPEDEF_NAMESPACE]);
			const char* className = mono_metadata_string_heap(s_Data->AppAssemblyImage, cols[MONO_TYPEDEF_NAME]);
			std::string fullName;
			if (strlen(nameSpace) != 0)
				fullName = fmt::format("{}.{}", nameSpace, className);
			else
				fullName = className;

			MonoClass* monoClass = mono_class_from_name(s_Data->AppAssemblyImage, nameSpace, className);

			if (!monoClass)
				continue;

			if (monoClass == entityClass)
				continue;

			bool isEntity = mono_class_is_subclass_of(monoClass, entityClass, false);
			if (!isEntity)
				continue;

			Ref<ScriptClass> scriptClass = CreateRef<ScriptClass>(nameSpace, className);
			s_Data->EntityClasses[fullName] = scriptClass;


			// This routine is an iterator routine for retrieving the fields in a class.
			// You must pass a gpointer that points to zero and is treated as an opaque handle
			// to iterate over all of the elements. When no more values are available, the return value is NULL.

			int fieldCount = mono_class_num_fields(monoClass);
			DY_CORE_WARN("{} has {} fields:", className, fieldCount);
			void* iterator = nullptr;
			while (MonoClassField* field = mono_class_get_fields(monoClass, &iterator))
			{
				const char* fieldName = mono_field_get_name(field);
				uint32_t flags = mono_field_get_flags(field);
				if (flags & FIELD_ATTRIBUTE_PUBLIC)
				{
					MonoType* type = mono_field_get_type(field);
					ScriptFieldType fieldType = Utils::MonoTypeToScriptFieldType(type);
					DY_CORE_WARN("  {} ({})", fieldName, Utils::ScriptFieldTypeToString(fieldType));

					scriptClass->m_Fields[fieldName] = { fieldType, fieldName, field };
				}
			}

		}

		auto& entityClasses = s_Data->EntityClasses;

		//mono_field_get_value()
	}
	
	static MethodDeclaration GenerateMethodDeclaration(MonoMethod* monoMethod, MonoImage* image, MonoCustomAttrInfo* methodAttributes, const std::string& fullClassName)
	{
		std::string methodName = mono_method_get_name(monoMethod);
		std::string methodFullName = fmt::format("{}.{}", fullClassName, methodName);
		MethodDeclaration method;
		method.Name = methodName;
		method.FullName = methodFullName;

		// Get the return type
		MonoMethodSignature* monoMethodSignature = mono_method_get_signature(monoMethod, image, 0);
		method.ReturnType = mono_type_get_name(mono_signature_get_return_type(monoMethodSignature));

		// If we have a reference, we will track that separately. The data type string should not include this.
		method.ReturnType.erase(std::remove(method.ReturnType.begin(), method.ReturnType.end(), '&'), method.ReturnType.end());

		// Get the parameters
		void* iter = nullptr;
		MonoType* monoType;
		uint32_t index = 0;
		uint32_t inputIndex = 0;
		uint32_t outputIndex = 0;
		while (monoType = mono_signature_get_params(monoMethodSignature, &iter))
		{
			std::string type = mono_type_get_name(monoType);
			// If we have a reference, we will track that separately. The data type string should not include this.
			type.erase(std::remove(type.begin(), type.end(), '&'), type.end());

			bool byRef = mono_type_is_byref(monoType);
			bool isOut = mono_signature_param_is_out(monoMethodSignature, index);

			// Check if this method should be a compact node
			bool noPinLabel = methodAttributes && mono_custom_attrs_has_attr(methodAttributes, s_Data->NoPinLabelAttributeClass);

			std::string name;
			if (MonoCustomAttrInfo* paramAttributes = mono_custom_attrs_from_param(monoMethod, index + 1))
			{
				if (MonoObject* attr = mono_custom_attrs_get_attr(paramAttributes, s_Data->ParameterNameAttributeClass))
				{
					MonoProperty* prop = mono_class_get_property_from_name(mono_object_get_class(attr), "Name");
					MonoString* value = (MonoString*)mono_property_get_value(prop, attr, nullptr, nullptr);
					name = mono_string_to_utf8(value);
				}
			}
			if (name.empty())
			{
				noPinLabel = true;
				name = fmt::format("{}_{}", isOut ? "Output" : "Input", isOut ? outputIndex++ : inputIndex++);
			}

			method.Parameters.push_back({ name, type, "", byRef, isOut });

			index++;
		}

		// Check if this method is pure
		if (methodAttributes && mono_custom_attrs_has_attr(methodAttributes, s_Data->IsPureAttributeClass))
			method.IsPure = true;

		// Check if this method is a conversion
		if (methodAttributes && mono_custom_attrs_has_attr(methodAttributes, s_Data->IsConversionAttributeClass))
			method.IsConversion = true;

		// Check if this method should be a compact node
		if (methodAttributes && mono_custom_attrs_has_attr(methodAttributes, s_Data->IsCompactNodeAttributeClass))
			method.IsCompactNode = true;

		// Check if this method should not display pin labels
		if (methodAttributes && mono_custom_attrs_has_attr(methodAttributes, s_Data->NoPinLabelsAttributeClass))
			method.NoPinLabels = true;

		// Get the method display name
		if (methodAttributes && mono_custom_attrs_has_attr(methodAttributes, s_Data->DisplayNameAttributeClass))
		{
			if (MonoObject* attr = mono_custom_attrs_get_attr(methodAttributes, s_Data->DisplayNameAttributeClass))
			{
				MonoProperty* prop = mono_class_get_property_from_name(mono_object_get_class(attr), "Name");
				MonoString* value = (MonoString*)mono_property_get_value(prop, attr, nullptr, nullptr);
				method.DisplayName = mono_string_to_utf8(value);
			}
		}

		// Get the method category
		if (methodAttributes && mono_custom_attrs_has_attr(methodAttributes, s_Data->MethodCategoryAttributeClass))
		{
			if (MonoObject* attr = mono_custom_attrs_get_attr(methodAttributes, s_Data->MethodCategoryAttributeClass))
			{
				MonoProperty* prop = mono_class_get_property_from_name(mono_object_get_class(attr), "Category");
				MonoString* value = (MonoString*)mono_property_get_value(prop, attr, nullptr, nullptr);
				method.Category = mono_string_to_utf8(value);
			}
		}

		// Get the method keywords
		if (methodAttributes && mono_custom_attrs_has_attr(methodAttributes, s_Data->MethodKeywordsAttributeClass))
		{
			if (MonoObject* attr = mono_custom_attrs_get_attr(methodAttributes, s_Data->MethodKeywordsAttributeClass))
			{
				MonoProperty* prop = mono_class_get_property_from_name(mono_object_get_class(attr), "Keywords");
				MonoString* value = (MonoString*)mono_property_get_value(prop, attr, nullptr, nullptr);
				method.Keywords = mono_string_to_utf8(value);
			}
		}

		return method;
	}

	void ScriptEngine::GenerateScriptMetadata()
	{
		s_Data->StaticMethods.clear();
		s_Data->EntityClassOverridableMethods.clear();

		if (!s_Data->CoreAssemblyImage || !s_Data->AppAssemblyImage)
			return;

		// Get the internal attribute classes used to provide method metadata
		s_Data->IsEditorCallableAttributeClass = mono_class_from_name(s_Data->CoreAssemblyImage, "Dymatic", "IsEditorCallable");
		s_Data->ParameterNameAttributeClass = mono_class_from_name(s_Data->CoreAssemblyImage, "Dymatic", "ParameterName");
		s_Data->NoPinLabelAttributeClass = mono_class_from_name(s_Data->CoreAssemblyImage, "Dymatic", "NoPinLabel");
		s_Data->DisplayNameAttributeClass = mono_class_from_name(s_Data->CoreAssemblyImage, "Dymatic", "DisplayName");
		s_Data->IsPureAttributeClass = mono_class_from_name(s_Data->CoreAssemblyImage, "Dymatic", "IsPure");
		s_Data->IsConversionAttributeClass = mono_class_from_name(s_Data->CoreAssemblyImage, "Dymatic", "IsConversion");
		s_Data->IsCompactNodeAttributeClass = mono_class_from_name(s_Data->CoreAssemblyImage, "Dymatic", "IsCompactNode");
		s_Data->NoPinLabelsAttributeClass = mono_class_from_name(s_Data->CoreAssemblyImage, "Dymatic", "NoPinLabels");
		s_Data->MethodCategoryAttributeClass = mono_class_from_name(s_Data->CoreAssemblyImage, "Dymatic", "MethodCategory");
		s_Data->MethodKeywordsAttributeClass = mono_class_from_name(s_Data->CoreAssemblyImage, "Dymatic", "MethodKeywords");

		for (auto& image : { s_Data->CoreAssemblyImage, s_Data->AppAssemblyImage })
		{
			const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
			int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);

			for (int32_t i = 0; i < numTypes; i++)
			{
				uint32_t cols[MONO_TYPEDEF_SIZE];
				mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

				const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
				const char* className = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);
				std::string fullClassName;
				if (strlen(nameSpace) != 0)
					fullClassName = fmt::format("{}.{}", nameSpace, className);
				else
					fullClassName = className;

				MonoClass* monoClass = mono_class_from_name(image, nameSpace, className);

				if (!monoClass)
					continue;

				// Retrieve all the methods this class contains
				void* iter = nullptr;
				MonoMethod* monoMethod;
				while (monoMethod = mono_class_get_methods(monoClass, &iter))
				{
					// Check if this function has been marked for editor interpretations (or is App assembly in which case all static functions are callable by default)
					MonoCustomAttrInfo* monoCustomAttrInfo = mono_custom_attrs_from_method(monoMethod);
					if (image == s_Data->AppAssemblyImage || (monoCustomAttrInfo && mono_custom_attrs_has_attr(monoCustomAttrInfo, s_Data->IsEditorCallableAttributeClass)))
					{
						if (mono_method_get_flags(monoMethod, nullptr) & METHOD_ATTRIBUTE_STATIC)
						{
							MethodDeclaration method = GenerateMethodDeclaration(monoMethod, image, monoCustomAttrInfo, fullClassName);
							s_Data->StaticMethods[method.FullName] = method;
						}
					}
				}

				// Retrieve all the overidable functions available for this class
				{
					MonoClass* klass = monoClass;
					while (klass)
					{
						void* iter = nullptr;
						MonoMethod* monoMethod;
						while (monoMethod = mono_class_get_methods(klass, &iter))
						{
							MonoCustomAttrInfo* monoCustomAttrInfo = mono_custom_attrs_from_method(monoMethod);
							if (image == s_Data->AppAssemblyImage || (monoCustomAttrInfo && mono_custom_attrs_has_attr(monoCustomAttrInfo, s_Data->IsEditorCallableAttributeClass)))
							{
								auto flags = mono_method_get_flags(monoMethod, nullptr);
								if ((flags & METHOD_ATTRIBUTE_VIRTUAL) && (flags & METHOD_ATTRIBUTE_NEW_SLOT))
								{
									MethodDeclaration method = GenerateMethodDeclaration(monoMethod, image, monoCustomAttrInfo, fullClassName);
									s_Data->EntityClassOverridableMethods[fullClassName].push_back(method);
								}
							}
						}

						klass = mono_class_get_parent(klass);
					}
				}
			}
		}
	}

	void* ScriptEngine::RegisterThread()
	{
		if (s_Data->RootDomain)
			return mono_thread_attach(s_Data->RootDomain);

		return nullptr;
	}

	void ScriptEngine::UnregisterThread(void* context)
	{
		if (context)
			mono_thread_detach((MonoThread*)context);
	}

	MonoImage* ScriptEngine::GetCoreAssemblyImage()
	{
		return s_Data->CoreAssemblyImage;
	}


	MonoDomain* ScriptEngine::GetRootDomain()
	{
		return s_Data->RootDomain;
	}

	MonoDomain* ScriptEngine::GetApplicationDomain()
	{
		return s_Data->AppDomain;
	}

	MonoObject* ScriptEngine::GetManagedInstance(UUID uuid)
	{
		if (s_Data->EntityInstances.find(uuid) != s_Data->EntityInstances.end())
			return s_Data->EntityInstances.at(uuid)->GetManagedObject();
		return nullptr;
	}

	MonoObject* ScriptEngine::GetManagedInstanceOrDefaultEntity(UUID uuid)
	{
		// Get the existing instance if this entity has a script
		MonoObject* instance = ScriptEngine::GetManagedInstance(uuid);

		if (!instance)
		{
			if (uuid == 0 || !s_Data->SceneContext->DoesEntityExist(uuid))
				return nullptr;

			// Create a new entity instance if it does not exist (we don't need to have access to any class specific fields/functions for a non-script entity)
			instance = s_Data->EntityClass.Instantiate();

			// Call constructor for new entity
			MonoMethod* constructor = s_Data->EntityClass.GetMethod(".ctor", 1);
			void* param = &uuid;
			ScriptEngine::InvokeMethod(instance, constructor, &param);
		}

		return instance;
	}

	void ScriptEngine::ExecuteEntityMethod(UUID entityID, const std::string& methodName, const Buffer& parameterData)
	{
		if (s_Data->EntityInstances.find(entityID) == s_Data->EntityInstances.end())
			return;

		Ref<ScriptInstance> instance = s_Data->EntityInstances.at(entityID);
		MonoMethod* method = instance->GetScriptClass()->GetMethod(methodName, 0);
		
		if (!method)
			return;

		InvokeMethod(instance->GetManagedObject(), method);
	}

	bool ScriptEngine::IsDebuggerAttached()
	{
#ifdef DY_DEBUG
		return mono_is_debugger_attached();
#else
		return false;
#endif
	}

	void ScriptEngine::SetAssemblyReloadCallback(const std::function<void()> callback)
	{
		s_Data->AssemblyReloadCallback = callback;
	}

	MonoObject* ScriptEngine::InstantiateClass(MonoClass* monoClass)
	{
		MonoObject* instance = mono_object_new(s_Data->AppDomain, monoClass);
		mono_runtime_object_init(instance);
		return instance;
	}

	MonoObject* ScriptEngine::InvokeMethod(MonoObject* instance, MonoMethod* method, void** params)
	{
		try
		{
			// Crash Note: If your debugger breaks here that is because a C# exception was thrown.
			// Forcing your debugger to continue should be fine. To avoid this, do not attach a debugger
			// or configure your IDE to not break when exceptions (e.g. null reference) are thrown.
			MonoObject* exception = nullptr;
			MonoObject* result = mono_runtime_invoke(method, instance, params, &exception);
		
			if (exception)
			{
				MonoString* message = mono_object_to_string((MonoObject*)exception, nullptr);
				const char* message_cstr = mono_string_to_utf8(message);
				DY_CRITICAL(message_cstr);
				mono_free((void*)message_cstr);
		
				return nullptr;
			}
		
			return result;
		}
		catch (const std::exception& e)
		{
			DY_CRITICAL(e.what());
		}

		return nullptr;
	}

	ScriptClass::ScriptClass(const std::string& classNamespace, const std::string& className, bool isCore)
		: m_ClassNamespace(classNamespace), m_ClassName(className)
	{
		m_MonoClass = mono_class_from_name(isCore ? s_Data->CoreAssemblyImage : s_Data->AppAssemblyImage, classNamespace.c_str(), className.c_str());
	}

	MonoObject* ScriptClass::Instantiate()
	{
		return ScriptEngine::InstantiateClass(m_MonoClass);
	}

	MonoMethod* ScriptClass::GetMethod(const std::string& name, int parameterCount)
	{
		return mono_class_get_method_from_name(m_MonoClass, name.c_str(), parameterCount);
	}

	ScriptInstance::ScriptInstance(Ref<ScriptClass> scriptClass, Entity entity)
		: m_ScriptClass(scriptClass)
	{
		m_Instance = scriptClass->Instantiate();

		m_Constructor = s_Data->EntityClass.GetMethod(".ctor", 1);
		m_OnCreateMethod = scriptClass->GetMethod("OnCreate", 0);
		m_OnUpdateMethod = scriptClass->GetMethod("OnUpdate", 1);
		m_OnPrePhysicsUpdateMethod = scriptClass->GetMethod("OnPrePhysicsUpdate", 1);
		m_OnDestroyMethod = scriptClass->GetMethod("OnDestroy", 0);
		m_OnContactMethod = scriptClass->GetMethod("OnContact", 3);
		m_OnContactPersistedMethod = scriptClass->GetMethod("OnContactPersisted", 3);
		m_OnContactRemovedMethod = scriptClass->GetMethod("OnContactRemoved", 1);

		// Call Entity constructor
		{
			UUID entityID = entity.GetUUID();
			void* param = &entityID;
			ScriptEngine::InvokeMethod(m_Instance, m_Constructor, &param);
		}
	}

	void ScriptInstance::InvokeOnCreate()
	{
		if (m_OnCreateMethod)
			ScriptEngine::InvokeMethod(m_Instance, m_OnCreateMethod);
	}

	void ScriptInstance::InvokeOnUpdate(float ts)
	{
		if (m_OnUpdateMethod)
		{
			void* param = &ts;
			ScriptEngine::InvokeMethod(m_Instance, m_OnUpdateMethod, &param);
		}
	}

	void ScriptInstance::InvokeOnPrePhysicsUpdate(float ts)
	{
		if (m_OnPrePhysicsUpdateMethod)
		{
			void* param = &ts;
			ScriptEngine::InvokeMethod(m_Instance, m_OnPrePhysicsUpdateMethod, &param);
		}
	}

	void ScriptInstance::InvokeOnDestroy()
	{
		if (m_OnDestroyMethod)
			ScriptEngine::InvokeMethod(m_Instance, m_OnDestroyMethod);
	}

	void ScriptInstance::InvokeOnContact(Entity other, const glm::vec3& hitPosition, const glm::vec3& normal)
	{
		if (m_OnContactMethod)
		{
			MonoObject* instance = ScriptEngine::GetManagedInstanceOrDefaultEntity(other.GetUUID());
			void* params[] = { instance, (void*)&hitPosition, (void*)&normal};
			ScriptEngine::InvokeMethod(m_Instance, m_OnContactMethod, params);
		}
	}

	void ScriptInstance::InvokeOnContactPersisted(Entity other, const glm::vec3& hitPosition, const glm::vec3& normal)
	{
		if (m_OnContactPersistedMethod)
		{
			MonoObject* instance = ScriptEngine::GetManagedInstanceOrDefaultEntity(other.GetUUID());
			void* params[] = { instance, (void*)&hitPosition, (void*)&normal };
			ScriptEngine::InvokeMethod(m_Instance, m_OnContactPersistedMethod, params);
		}
	}

	void ScriptInstance::InvokeOnContactRemoved(Entity other)
	{
		if (m_OnContactRemovedMethod)
		{
			MonoObject* instance = ScriptEngine::GetManagedInstanceOrDefaultEntity(other.GetUUID());
			ScriptEngine::InvokeMethod(m_Instance, m_OnContactRemovedMethod, (void**)&instance);
		}
	}

	bool ScriptInstance::GetFieldValueInternal(const std::string& name, void* buffer)
	{
		const auto& fields = m_ScriptClass->GetFields();
		auto it = fields.find(name);
		if (it == fields.end())
			return false;

		const ScriptField& field = it->second;
		mono_field_get_value(m_Instance, field.ClassField, buffer);
		return true;
	}

	bool ScriptInstance::SetFieldValueInternal(const std::string& name, const void* value)
	{
		const auto& fields = m_ScriptClass->GetFields();
		auto it = fields.find(name);
		if (it == fields.end())
			return false;

		const ScriptField& field = it->second;
		
		MonoType* fieldType = mono_field_get_type(field.ClassField);
		if (mono_type_is_reference(fieldType))
		{
			// If the field is a class then we assume it is some form of asset or entity that takes in a handle/id as a constructor
			if (*(uint64_t*)value == 0)
				mono_field_set_value(m_Instance, field.ClassField, nullptr);
			else
			{
				MonoClass* fieldClass = mono_type_get_class(fieldType);
				MonoObject* instance = mono_object_new(s_Data->AppDomain, fieldClass);

				MonoMethod* constructor = mono_class_get_method_from_name(fieldClass, ".ctor", 1);
				void* param = (void*)value;
				ScriptEngine::InvokeMethod(instance, constructor, &param);

				mono_field_set_value(m_Instance, field.ClassField, (void*)instance);
			}
		}
		else
			mono_field_set_value(m_Instance, field.ClassField, (void*)value);
		
		return true;
	}

}