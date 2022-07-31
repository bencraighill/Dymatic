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
			case ShaderStorageBufferUsage::STATIC_COPY:		return GL_STATIC_COPY;

			case ShaderStorageBufferUsage::DYNAMIC_READ:	return GL_DYNAMIC_READ;
			case ShaderStorageBufferUsage::DYNAMIC_DRAW:	return GL_DYNAMIC_DRAW;
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


	void OpenGLShaderStorageBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		glNamedBufferSubData(m_RendererID, offset, size, data);
	}

	void* OpenGLShaderStorageBuffer::MapBuffer() const
	{
		return glMapNamedBuffer(m_RendererID, GL_READ_WRITE);
	}

	void OpenGLShaderStorageBuffer::UnmapBuffer() const
	{
		glUnmapNamedBuffer(m_RendererID);
	}

}