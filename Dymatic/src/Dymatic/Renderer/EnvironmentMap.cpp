#include "dypch.h"
#include "Dymatic/Renderer/EnvironmentMap.h"

namespace Dymatic {

	EnvironmentMap::EnvironmentMap(const std::filesystem::path& filepath)
	{
		// Assume the environment map is in an RGB floating point format
		TextureSpecification textureSpecification;
		textureSpecification.Format = TextureFormat::RGB16F;

		m_Texture = Texture2D::Create(filepath, textureSpecification);
	}

	EnvironmentMap::EnvironmentMap(const Buffer& fileData)
	{
		// Assume the environment map is in an RGB floating point format
		TextureSpecification textureSpecification;
		textureSpecification.Format = TextureFormat::RGB16F;

		m_Texture = Texture2D::Create(fileData, textureSpecification);
	}

}