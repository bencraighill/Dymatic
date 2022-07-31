#pragma once

#include "Dymatic/Renderer/Shader.h"
#include "Dymatic/Renderer/Texture.h"
#include "Dymatic/Renderer/UniformBuffer.h"

namespace Dymatic {

	class Material
	{
	public:
		static Ref<Material> Create(Ref<Shader> shader) { return CreateRef<Material>(shader); }

		Material(Ref<Shader> shader)
			: m_Shader(shader)
		{
		}

		void Bind();
		void Unbind();

		inline Ref<Shader> GetShader() const { return m_Shader; }

	private:
		friend class MaterialInstance;

	protected:
		Ref<Shader> m_Shader;
		std::vector<Ref<Texture2D>> m_Textures;

		Ref<UniformBuffer> m_UniformBuffer;
		size_t m_UniformBufferSize;
		void* m_UniformData = nullptr;
	};

	class MaterialInstance
	{
	public:
		static Ref<MaterialInstance> Create(Ref<Material> material) { return CreateRef<MaterialInstance>(material); }

		MaterialInstance(Ref<Material> material)
			: m_Material(material)
		{
		}

		void Bind();
		void Unbind();

		inline Ref<Material> GetMaterial() const { return m_Material; }

	private:
		Ref<Material> m_Material;
		std::vector<Ref<Texture2D>> m_Textures;

	};

}