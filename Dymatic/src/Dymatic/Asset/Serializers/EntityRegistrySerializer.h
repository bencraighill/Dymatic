#pragma once

#include "Dymatic/Scene/EntityRegistry.h"

#include "Dymatic/Scene/Components.h"

#include <yaml-cpp/yaml.h>
#include "Dymatic/Utils/YAMLUtils.h"

#include "Dymatic/Core/FileStream.h"

namespace Dymatic {

	class EntityRegistrySerializer
	{
	public:
		// Editor
		static void SerializeRegistry(YAML::Emitter& out, Ref<EntityRegistry> scene);
		static void DeserializeRegistry(YAML::Node entities, Ref<EntityRegistry> scene);

		static void SerializeEntity(YAML::Emitter& out, Entity entity);
		static void DeserializeEntity(YAML::Node entity, Ref<EntityRegistry> scene);

		// Runtime
		static void SerializeRegistry(Ref<EntityRegistry> scene, FileStreamWriter& stream);
		static void DeserializeRegistry(FileStreamReader& stream, Ref<EntityRegistry> scene);

		static void SerializeEntity(Entity entity, FileStreamWriter& stream);
		static void DeserializeEntity(FileStreamReader& stream, Ref<EntityRegistry> scene);
	};

}