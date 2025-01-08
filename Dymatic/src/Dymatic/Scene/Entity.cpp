#include "dypch.h"
#include "Entity.h"

namespace Dymatic {

	Entity::Entity(entt::entity handle, EntityRegistry* scene)
		: m_EntityHandle(handle), m_Scene(scene)
	{
	}

}