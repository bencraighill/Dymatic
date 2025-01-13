#pragma once

#include "Dymatic/Scene/EntityRegistry.h"

namespace Dymatic {

	class Entity;

	class Prefab : public EntityRegistry
	{
	public:
		static Ref<Prefab> Create() { return CreateRef<Prefab>(); }
		static Ref<Prefab> Create(Entity root);

	public:
		Prefab() = default;
		Prefab(Entity root);

		Entity GetRootEntity();
		void UpdateRoot(Entity source);
		
		static AssetType GetStaticType() { return AssetType::Prefab; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	private:
		virtual void OnComponentAdded(Entity entity, const std::type_info& componentType) override;
		virtual void OnComponentRemoved(Entity entity, const std::type_info& componentType) override;

	private:
		friend class Scene;
	};

}