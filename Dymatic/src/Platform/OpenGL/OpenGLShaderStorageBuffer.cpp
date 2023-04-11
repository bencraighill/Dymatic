#include "dypch.h"
#include "OpenGLShaderStorageBuffer.h"

#include <glad/glad.h>

namespace Dymatic {

	namespace Utils
	{
		static GLenum DymaticSSBOUsageToGL(ShaderStorageBufferUsage usage)
		{
			switch (usage)
			{
			case ShaderStorageBufferUsage::STATIC_READ:		return GL_STATIC_READ;
			case ShaderStorageBufferUsage::STATIC_DRAW:		return GL_STATIC_DRAW;
			case ShaderStorageBufferUsage::STATIC_COPY:		return GL_STATIC_COPY;

			case ShaderStorageBufferUsage::DYNAMIC_READ:	return GL_DYNAMIC_READ;
			case ShaderStorageBufferUsage::DYNAMIC_DRAW:	return GL_DYNAMIC_DRAW;
			case ShaderStorageBufferUsage::DYNAMIC_COPY:	return GL_DYNAMIC_COPY;
			}

			DY_CORE_ASSERT(false);
			return 0;
		}

		static GLenum DymaticStorageAccessToGL(StorageAccess access)
		{
			switch (access)
			{
			case StorageAccess::READ_ONLY:	return GL_READ_ONLY;
			case StorageAccess::WRITE_ONLY:	return GL_WRITE_ONLY;
			case StorageAccess::READ_WRITE:	return GL_READ_WRITE;
			}

			DY_CORE_ASSERT(false);
			return 0;
		}
	}

	OpenGLShaderStorageBuffer::OpenGLShaderStorageBuffer(uint32_t size, uint32_t binding, ShaderStorageBufferUsage usage)
		: m_Usage(usage)
	{
		glCreateBuffers(1, &m_RendererID);
		glNamedBufferData(m_RendererID, size, nullptr, Utils::DymaticSSBOUsageToGL(usage)); // TODO: investigate usage hint
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, m_RendererID);
	}

	OpenGLShaderStorageBuffer::~OpenGLShaderStorageBuffer()
	{
		glDeleteBuffers(1, &m_RendererID);
	}

	// Copies all data into a new buffer
	// Only use if MapBuffer() is not viable, or unnessisary - don't use for large data retrievals
	void OpenGLShaderStorageBuffer::GetData(void* data, uint32_t size, uint32_t offset)
	{
		glGetNamedBufferSubData(m_RendererID, offset, size, data);
	}

	void OpenGLShaderStorageBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		glNamedBufferSubData(m_RendererID, offset, size, data);
	}

	// Maps data into client address space (more efficient than GetData() for large data retrievals)
	void* OpenGLShaderStorageBuffer::MapBuffer(StorageAccess access) const
	{
		return glMapNamedBuffer(m_RendererID, Utils::DymaticStorageAccessToGL(access));
	}

	void* OpenGLShaderStorageBuffer::MapBufferRange(uint32_t offset, uint32_t length, StorageAccess access) const
	{
		return glMapNamedBufferRange(m_RendererID, offset, length, Utils::DymaticStorageAccessToGL(access));
	}

	void OpenGLShaderStorageBuffer::UnmapBuffer() const
	{
		glUnmapNamedBuffer(m_RendererID);
	}

}