#include "dypch.h"
#include "Entity.h"

namespace Dymatic {

	Entity::Entity(entt::entity handle, Scene* scene)
		: m_EntityHandle(handle), m_Scene(scene)
	{
	}

}