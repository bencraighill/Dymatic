#pragma once

// Utilities for compressing/packaging GLSL shader files for runtime distribution

#include "Dymatic/Core/Buffer.h"

namespace Dymatic {

	class ShaderCompressor
	{
	public:
		static void Minify(const std::unordered_map<uint32_t, std::string>& sources, std::unordered_map<uint32_t, std::string>& minifiedSources);
		static Ref<ScopedBuffer> HuffmanEncode(const std::string& source);
		static void HuffmanDecode(Ref<Buffer> data, std::string& target);
	};

}