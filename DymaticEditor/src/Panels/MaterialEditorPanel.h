#pragma once

#include "Dymatic/Renderer/Material.h"

namespace Dymatic {

	class MaterialEditorPanel
	{
	public:
		MaterialEditorPanel();

		void OnImGuiRender();
		
		void Open(const std::filesystem::path& path);

	private:
		void DrawMaterialTexture(const char* name, const Ref<Material>& material, Ref<Texture2D>& texture);

	private:
		std::vector<Ref<Material>> m_ActiveMaterials;
		
		Ref<Texture2D> m_CheckerboardTexture;
	};

}