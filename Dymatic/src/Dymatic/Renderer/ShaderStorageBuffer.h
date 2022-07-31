#pragma once

#include "Dymatic/Core/Base.h"

namespace Dymatic {

	enum class ShaderStorageBufferUsage
	{
		None = 0,

		// Static
		STATIC_COPY,

		// Dynamic
		DYNAMIC_READ,
		DYNAMIC_DRAW,
	};

	class ShaderStorageBuffer
	{
	public:
		virtual ~ShaderStorageBuffer() {}

		virtual uint32_t GetRendererID() const = 0;

		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;

		virtual void* MapBuffer() const = 0;
		virtual void UnmapBuffer() const = 0;

		static Ref<ShaderStorageBuffer> Create(uint32_t size, uint32_t binding, ShaderStorageBufferUsage usage);
	};

}