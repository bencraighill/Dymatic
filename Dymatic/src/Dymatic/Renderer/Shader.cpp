#include "dypch.h"
#include "Dymatic/Renderer/Shader.h"

#include "Dymatic/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLShader.h"

#include "Dymatic/Core/FileStream.h"
#include "Dymatic/Core/Filesystem.h"

namespace Dymatic {

	Ref<Shader> Shader::Create(const std::filesystem::path& filepath, const bool useCache)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    DY_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLShader>(filepath, useCache);
		}

		DY_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Shader> Shader::Create(const std::string& name, const std::unordered_map<uint32_t, Ref<ScopedBuffer>>& packages)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    DY_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLShader>(name, packages);
		}

		DY_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Shader> Shader::Create(const std::string& name, const std::unordered_map<uint32_t, std::vector<uint32_t>>& binaries)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    DY_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLShader>(name, binaries);
		}

		DY_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Shader> Shader::Create(const std::string& name, const std::string& source, bool useCache)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    DY_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLShader>(name, source, useCache);
		}

		DY_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
	
	Ref<Shader> Shader::Create(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    DY_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLShader>(name, vertexSrc, fragmentSrc);
		}

		DY_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	void ShaderLibrary::Add(const std::string& name, const Ref<Shader>& shader)
	{
		DY_CORE_ASSERT(!Exists(name), "Shader already exists!");
		m_Shaders[name] = shader;
	}

	void ShaderLibrary::Add(const Ref<Shader>& shader)
	{
		auto& name = shader->GetName();
		Add(name, shader);
	}

	Ref<Shader> ShaderLibrary::Load(const std::filesystem::path& filepath)
	{
		Ref<Shader> shader = Shader::Create(filepath);
		Add(shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::Load(const std::string& name, const std::filesystem::path& filepath)
	{
		Ref<Shader> shader = Shader::Create(filepath);
		Add(name, shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::Get(const std::string& name)
	{
		DY_CORE_ASSERT(Exists(name), "Shader not found!");
		return m_Shaders[name];
	}

	void ShaderLibrary::LoadShaderPack(const std::filesystem::path& filepath)
	{
		if (!std::filesystem::exists(filepath))
		{
			DY_CORE_ERROR("Specified shader pack '{}' does not exist", filepath.string());
			return;
		}

		FileStreamReader reader(filepath);

		if (!reader.IsStreamGood())
		{
			DY_CORE_ERROR("Failed to open shader pack '{}'", filepath.string());
			return;
		}
	
		// Validate the file header
		const char header[] = { 'D', 'Y', 'S', 'P' };
		char fileHeader[sizeof(header)];
		reader.ReadData(fileHeader, sizeof(header));
		
		if (memcmp(fileHeader, header, sizeof(header)) != 0)
		{
			DY_CORE_ERROR("Invalid shader pack file '{}'", filepath.string());
			return;
		}

		// Validate the Engine Version
		uint32_t engineVersionMajor, engineVersionMinor, engineVersionPatch;
		reader.ReadRaw(engineVersionMajor);
		reader.ReadRaw(engineVersionMinor);
		reader.ReadRaw(engineVersionPatch);
		
		if (engineVersionMajor != DY_VERSION_MAJOR || engineVersionMinor != DY_VERSION_MINOR || engineVersionPatch != DY_VERSION_PATCH)
		{
			DY_CORE_ERROR("Shader Pack was built with a different version of the engine. Expected {}.{}.{} but got {}.{}.{}",
			DY_VERSION_MAJOR, DY_VERSION_MINOR, DY_VERSION_PATCH, engineVersionMajor, engineVersionMinor, engineVersionPatch);
			DY_CORE_VERIFY(false, "Shader pack version does not match engine build!");
			return;
		}

		size_t shaderCount;
		reader.ReadRaw(shaderCount);
		
		m_Shaders.clear();
		m_Shaders.reserve(shaderCount);
		
		DY_CORE_TRACE("Loading shader pack '{}' with {} shaders", filepath.string(), shaderCount);

		for (size_t i = 0; i < shaderCount; i++)
		{
			std::string name;
			reader.ReadString(name);

			size_t binaryCount;
			reader.ReadRaw(binaryCount);

			std::unordered_map<uint32_t, Ref<ScopedBuffer>> binaries(binaryCount);
			
			for (size_t binaryIndex = 0; binaryIndex < binaryCount; binaryIndex++)
			{
				uint32_t stage;
				reader.ReadRaw(stage);

				size_t binarySize;
				reader.ReadRaw(binarySize);

				Ref<ScopedBuffer> buffer = ScopedBuffer::Create(binarySize);
				reader.ReadData((char*)buffer->Data, buffer->Size);
				binaries[stage] = buffer;
			}

			m_Shaders[name] = Shader::Create(name, binaries);
		}

		DY_CORE_TRACE("Shader Pack deserialization completed");
	}

	void ShaderLibrary::SerializeShaderPack(const std::filesystem::path& filepath)
	{
		FileSystem::CreateDirectory(filepath.parent_path());

		FileStreamWriter writer(filepath);

		// Write the file header
		const char header[] = { 'D', 'Y', 'S', 'P' };
		writer.WriteData(header, sizeof(header));

		// Write the engine version
		writer.WriteRaw<uint32_t>(DY_VERSION_MAJOR);
		writer.WriteRaw<uint32_t>(DY_VERSION_MINOR);
		writer.WriteRaw<uint32_t>(DY_VERSION_PATCH);

		DY_CORE_TRACE("Serializing shader pack '{}' with {} shaders", filepath.string(), m_Shaders.size());
		
		writer.WriteRaw<size_t>(m_Shaders.size());
		for (auto& [name, shader] : m_Shaders)
		{
			writer.WriteString(name);

			const auto& packages = shader->GetPackagedShaderBuffers();
			writer.WriteRaw<size_t>(packages.size());

			DY_CORE_INFO("Serializing shader '{}' to Shader Pack with {} bound stages", name, packages.size());
			
			for (auto& [stage, binary] : packages)
			{
				writer.WriteRaw<uint32_t>(stage);
				
				// Write the binary buffer
				writer.WriteRaw<size_t>(binary->Size);
				writer.WriteData((const char*)binary->Data, binary->Size);
			}
		}

		DY_CORE_TRACE("Shader Pack serialization completed");
	}

	bool ShaderLibrary::Exists(const std::string& name)
	{
		return m_Shaders.find(name) != m_Shaders.end();
	}

}