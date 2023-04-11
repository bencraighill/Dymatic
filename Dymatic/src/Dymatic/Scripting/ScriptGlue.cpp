#include "dypch.h"
#include "ScriptGlue.h"
#include "ScriptEngine.h"

#include "Dymatic/Core/UUID.h"
#include "Dymatic/Core/KeyCodes.h"
#include "Dymatic/Core/Input.h"

#include "Dymatic/Scene/Scene.h"
#include "Dymatic/Scene/Entity.h"

#include "mono/metadata/object.h"
#include "mono/metadata/reflection.h"

#include "box2d/b2_body.h"

namespace Dymatic {

	static std::unordered_map<MonoType*, std::function<bool(Entity)>> s_EntityHasComponentFuncs;

#define DY_ADD_INTERNAL_CALL(Name) mono_add_internal_call("Dymatic.InternalCalls::" #Name, Name)

	static MonoObject* GetScriptInstance(UUID entityID)
	{
		return ScriptEngine::GetManagedInstance(entityID);
	}

	static bool Entity_HasComponent(UUID entityID, MonoReflectionType* componentType)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		MonoType* managedType = mono_reflection_type_get_type(componentType);
		DY_CORE_ASSERT(s_EntityHasComponentFuncs.find(managedType) != s_EntityHasComponentFuncs.end());
		return s_EntityHasComponentFuncs.at(managedType)(entity);
	}

	static uint64_t Entity_FindEntityByName(MonoString* name)
	{
		char* nameCStr = mono_string_to_utf8(name);

		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->FindEntityByName(nameCStr);
		mono_free(nameCStr);

		if (!entity)
			return 0;

		return entity.GetUUID();
	}

	static void TransformComponent_GetTranslation(UUID entityID, glm::vec3* outTranslation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		*outTranslation = entity.GetComponent<TransformComponent>().Translation;
	}

	static void TransformComponent_SetTranslation(UUID entityID, glm::vec3* translation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<TransformComponent>().Translation = *translation;
	}

	static void Rigidbody2DComponent_ApplyLinearImpulse(UUID entityID, glm::vec2* impulse, glm::vec2* point, bool wake)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
		b2Body* body = (b2Body*)rb2d.RuntimeBody;
		body->ApplyLinearImpulse(b2Vec2(impulse->x, impulse->y), b2Vec2(point->x, point->y), wake);
	}

	static void Rigidbody2DComponent_ApplyLinearImpulseToCenter(UUID entityID, glm::vec2* impulse, bool wake)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
		b2Body* body = (b2Body*)rb2d.RuntimeBody;
		body->ApplyLinearImpulseToCenter(b2Vec2(impulse->x, impulse->y), wake);
	}

	static bool Input_IsKeyDown(KeyCode keycode)
	{
		return Input::IsKeyPressed(keycode);
	}

	static bool Input_IsMouseButtonPressed(MouseCode button)
	{
		return Input::IsMouseButtonPressed(button);
	}
	
	static void Input_GetMousePosition(glm::vec2* outPosition)
	{
		*outPosition = Input::GetMousePosition();
	}

	static float Input_GetMouseX()
	{
		return Input::GetMouseX();
	}

	static float Input_GetMouseY()
	{
		return Input::GetMouseY();
	}

	static bool Input_IsGamepadConnected(int gamepad)
	{
		return Input::IsGamepadConnected(gamepad);
	}

	static bool Input_IsGamepadButtonPressed(int gamepad, GamepadButtonCode button)
	{
		return Input::IsGamepadButtonPressed(gamepad, button);
	}

	static float Input_GetGamepadAxis(int gamepad, GamepadAxisCode axis)
	{
		return Input::GetGamepadAxis(gamepad, axis);
	}

	static void Input_GetGamepadSensor(int gamepad, GamepadSensorCode sensor, glm::vec3* outValue)
	{
		*outValue = Input::GetGamepadSensor(gamepad, sensor);
	}

	static bool Input_SetGamepadRumble(int gamepad, float left, float right, float duration)
	{
		return Input::SetGamepadRumble(gamepad, left, right, duration);
	}

	static bool Input_SetGamepadLED(int gamepad, glm::vec3* color)
	{
		return Input::SetGamepadLED(gamepad, *color);
	}
	
	static void Log_Trace(MonoString* message) 
	{
		char* messageCStr = mono_string_to_utf8(message);
		DY_TRACE(messageCStr);
	}
	
	static void Log_Info(MonoString* message)
	{
		char* messageCStr = mono_string_to_utf8(message);
		DY_INFO(messageCStr);
	}

	static void Log_Warn(MonoString* message)
	{
		char* messageCStr = mono_string_to_utf8(message);
		DY_WARN(messageCStr);
	}
	
	static void Log_Error(MonoString* message)
	{
		char* messageCStr = mono_string_to_utf8(message);
		DY_ERROR(messageCStr);
	}

	static void Log_Critical(MonoString* message)
	{
		char* messageCStr = mono_string_to_utf8(message);
		DY_CRITICAL(messageCStr);
	}

	static void Core_Assert(bool condition, MonoString* message)
	{
		char* messageCStr = mono_string_to_utf8(message);
		DY_ASSERT(condition, messageCStr);
	}

	template<typename... Component>
	static void RegisterComponent()
	{
		([]()
			{
				std::string_view typeName = typeid(Component).name();
		size_t pos = typeName.find_last_of(':');
		std::string_view structName = typeName.substr(pos + 1);
		std::string managedTypename = fmt::format("Dymatic.{}", structName);

		MonoType* managedType = mono_reflection_type_from_name(managedTypename.data(), ScriptEngine::GetCoreAssemblyImage());
		if (!managedType)
		{
			DY_CORE_ERROR("Could not find component type {}", managedTypename);
			return;
		}
		s_EntityHasComponentFuncs[managedType] = [](Entity entity) { return entity.HasComponent<Component>(); };
			}(), ...);
	}

	template<typename... Component>
	static void RegisterComponent(ComponentGroup<Component...>)
	{
		RegisterComponent<Component...>();
	}

	void ScriptGlue::RegisterComponents()
	{
		s_EntityHasComponentFuncs.clear();
		RegisterComponent(AllComponents{});
	}

	void ScriptGlue::RegisterFunctions()
	{
		DY_ADD_INTERNAL_CALL(GetScriptInstance);

		DY_ADD_INTERNAL_CALL(Entity_HasComponent);
		DY_ADD_INTERNAL_CALL(Entity_FindEntityByName);

		DY_ADD_INTERNAL_CALL(TransformComponent_GetTranslation);
		DY_ADD_INTERNAL_CALL(TransformComponent_SetTranslation);

		DY_ADD_INTERNAL_CALL(Rigidbody2DComponent_ApplyLinearImpulse);
		DY_ADD_INTERNAL_CALL(Rigidbody2DComponent_ApplyLinearImpulseToCenter);
		
		DY_ADD_INTERNAL_CALL(Input_IsKeyDown);
		DY_ADD_INTERNAL_CALL(Input_IsMouseButtonPressed);
		DY_ADD_INTERNAL_CALL(Input_GetMousePosition);
		DY_ADD_INTERNAL_CALL(Input_GetMouseX);
		DY_ADD_INTERNAL_CALL(Input_GetMouseY);
		DY_ADD_INTERNAL_CALL(Input_IsGamepadConnected);
		DY_ADD_INTERNAL_CALL(Input_IsGamepadButtonPressed);
		DY_ADD_INTERNAL_CALL(Input_GetGamepadAxis);
		DY_ADD_INTERNAL_CALL(Input_GetGamepadSensor);
		DY_ADD_INTERNAL_CALL(Input_SetGamepadRumble);
		DY_ADD_INTERNAL_CALL(Input_SetGamepadLED);

		DY_ADD_INTERNAL_CALL(Log_Trace);
		DY_ADD_INTERNAL_CALL(Log_Info);
		DY_ADD_INTERNAL_CALL(Log_Warn);
		DY_ADD_INTERNAL_CALL(Log_Error);
		DY_ADD_INTERNAL_CALL(Log_Critical);

		DY_ADD_INTERNAL_CALL(Core_Assert);
	}

}