#include "dypch.h"
#include "Dymatic/Renderer/Material.h"

namespace Dymatic {

	void Material::Bind()
	{
		m_Shader->Bind();

		if (m_UniformData)
			m_UniformBuffer->SetData(m_UniformData, m_UniformBufferSize);

		for (size_t i = 0; i < m_Textures.size(); i++)
		{
			if (m_Textures[i])
				m_Textures[i]->Bind(i);
		}
	}

	void Material::Unbind()
	{
	}

	void MaterialInstance::Bind()
	{
		m_Material->Bind();

		for (size_t i = 0; i < m_Textures.size(); i++)
		{
			if (m_Textures[i])
				m_Textures[i]->Bind(i);
		}
	}

	void MaterialInstance::Unbind()
	{
		m_Material->Unbind();
	}

}