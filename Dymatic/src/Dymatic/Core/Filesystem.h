#pragma once

#include "Dymatic/Core/Buffer.h"

namespace Dymatic {

	class FileSystem
	{
	public:
		static Buffer ReadFileBinary(const std::filesystem::path& filepath);
	};

}