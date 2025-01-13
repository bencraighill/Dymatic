#pragma once

#include <filesystem>

namespace Dymatic {

	class AssimpInfo
	{
	public:
		struct AnimationInfo
		{
			std::string Name;
		};

	public:
		static std::vector<AnimationInfo> GetAnimationInfo(const std::filesystem::path& filepath);
	};

}