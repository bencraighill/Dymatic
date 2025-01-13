#include "dypch.h"
#include "Dymatic/Utils/AssimpInfoUtils.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Dymatic {

	std::vector<AssimpInfo::AnimationInfo> AssimpInfo::GetAnimationInfo(const std::filesystem::path& filepath)
	{
		Assimp::Importer importer;

		// Import the file with minimal processing
		const aiScene* scene = importer.ReadFile(filepath.string(), aiProcess_ValidateDataStructure);

		if (!scene)
		{
			DY_CORE_ERROR("Failed to load Assimp Details for file '{}'", filepath.string());
			return {};
		}

		// Access infomation about the animations
		std::vector<AnimationInfo> animations;
		for (uint32_t i = 0; i < scene->mNumAnimations; i++)
		{
			AnimationInfo details;
			details.Name = scene->mAnimations[i]->mName.C_Str();
			animations.push_back(details);
		}

		return animations;
	}

}