#include "dypch.h"
#include "Dymatic/Renderer/Material.h"

namespace Dymatic {

	const char* Material::AlphaBlendModeToString(AlphaBlendMode mode)
	{
		switch (mode)
		{
		case Opaque: return "Opaque";
		case Masked: return "Masked";
		case Translucent: return "Translucent";
		case Dithered: return "Dithered";
		}
		return "Unknown";
	}

	void Material::Bind()
	{
		if (m_MaterialData.UsingAlbedoMap = (bool)m_AlbedoMap) m_AlbedoMap->Bind(0);
		if (m_MaterialData.UsingNormalMap = (bool)m_NormalMap) m_NormalMap->Bind(1);
		if (m_MaterialData.UsingEmissiveMap = (bool)m_EmissiveMap) m_EmissiveMap->Bind(2);
		if (m_MaterialData.UsingSpecularMap = (bool)m_SpecularMap) m_SpecularMap->Bind(3);
		if (m_MaterialData.UsingMetalnessMap = (bool)m_MetalnessMap) m_MetalnessMap->Bind(4);
		if (m_MaterialData.UsingRougnessMap = (bool)m_RougnessMap) m_RougnessMap->Bind(5);
		if (m_MaterialData.UsingAlphaMap = (bool)m_AlphaMap) m_AlphaMap->Bind(6);
		if (m_MaterialData.UsingAmbientOcclusionMap = (bool)m_AmbientOcclusionMap) m_AmbientOcclusionMap->Bind(7);
	}

}
