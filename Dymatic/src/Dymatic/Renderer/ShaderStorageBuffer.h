#pragma once

#include "Dymatic/Core/Base.h"

namespace Dymatic {

	enum class ShaderStorageBufferUsage
	{
		None = 0,

		// Static
		STATIC_READ,
		STATIC_DRAW,
		STATIC_COPY,

		// Dynamic
		DYNAMIC_READ,
		DYNAMIC_DRAW,
		DYNAMIC_COPY, // Equivalent to Read and Write
	};

	enum StorageAccess
	{
		READ_ONLY = 0,
		WRITE_ONLY,
		READ_WRITE
	};

	class ShaderStorageBuffer
	{
	public:
		virtual ~ShaderStorageBuffer() {}

		virtual uint32_t GetRendererID() const = 0;

		virtual void GetData(void* data, uint32_t size, uint32_t offset = 0) = 0;
		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;

		virtual void* MapBuffer(StorageAccess access) const = 0;
		virtual void* MapBufferRange(uint32_t offset, uint32_t length, StorageAccess access) const = 0;
		virtual void UnmapBuffer() const = 0;

		static Ref<ShaderStorageBuffer> Create(uint32_t size, uint32_t binding, ShaderStorageBufferUsage usage);
	};

}