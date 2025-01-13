#pragma once

#include "Dymatic/Core/TransactionManager.h"

#include "Dymatic/Scene/Entity.h"

namespace Dymatic {

	class TransformTransaction : public Transaction
	{
	public:
		TransformTransaction(Entity entity, const TransformComponent& originalTransform, const TransformComponent& modifiedTransform)
			: m_Entity(entity), m_OriginalTransform(originalTransform), m_ModifiedTransform(modifiedTransform)
		{
		}

		virtual void Execute() override
		{
			m_Entity.GetComponent<TransformComponent>() = m_ModifiedTransform;
		}

		virtual void Undo() override
		{
			m_Entity.GetComponent<TransformComponent>() = m_OriginalTransform;
		}

		virtual const std::string GetName() const override { return "Modify Entity Transform"; }

	private:
		Entity m_Entity;
		TransformComponent m_OriginalTransform;
		TransformComponent m_ModifiedTransform;
	};

}