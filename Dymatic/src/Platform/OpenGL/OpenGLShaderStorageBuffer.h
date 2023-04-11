#pragma once

#include "Dymatic/Renderer/ShaderStorageBuffer.h"

namespace Dymatic {

	class OpenGLShaderStorageBuffer : public ShaderStorageBuffer
	{
	public:
		OpenGLShaderStorageBuffer(uint32_t size, uint32_t binding, ShaderStorageBufferUsage usage);
		virtual ~OpenGLShaderStorageBuffer();

		virtual uint32_t GetRendererID() const override { return m_RendererID; }

		virtual void GetData(void* data, uint32_t size, uint32_t offset = 0) override;
		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;

		virtual void* MapBuffer(StorageAccess access) const override;
		virtual void* MapBufferRange(uint32_t offset, uint32_t length, StorageAccess access) const override;
		virtual void UnmapBuffer() const override;
	private:
		uint32_t m_RendererID = 0;
		ShaderStorageBufferUsage m_Usage;
	};
}