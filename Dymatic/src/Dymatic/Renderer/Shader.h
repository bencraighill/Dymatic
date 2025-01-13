#pragma once

#include <string>
#include <unordered_map>

#include <glm/glm.hpp>

#include "Dymatic/Core/Buffer.h"

namespace Dymatic {

	class Shader
	{
	public:
		virtual ~Shader() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;
		virtual uint32_t GetRendererID() const = 0;

		virtual bool IsLoaded() const = 0;

		virtual void Dispatch(int numGroupsX, int numGroupsY, int numGroupsZ) const = 0;
		
		virtual void SetMacro(const std::string& macro) = 0;
		virtual void SetMacro(const std::string& macro, const std::string& value) = 0;
		virtual void SetMacro(const std::string& macro, bool flag) = 0;
		virtual bool GetMacroFlag(const std::string& name) const = 0;

		virtual void Reload() = 0;
		virtual void Regenerate() = 0;

		virtual const std::string& GetName() const = 0;
		virtual const std::string& GetSourceCode() const = 0;
		virtual const std::string& GetError() const = 0;
		virtual const std::unordered_map<uint32_t, std::vector<uint32_t>>& GetBinaries() const = 0;
		virtual std::unordered_map<uint32_t, Ref<ScopedBuffer>> GetPackagedShaderBuffers() const = 0;

		virtual void SetInt(const std::string& name, int value) = 0;
		virtual void SetIntArray(const std::string& name, int* values, uint32_t count) = 0;
		virtual void SetFloat(const std::string& name, float value) = 0;
		virtual void SetFloat2(const std::string& name, const glm::vec2& value) = 0;
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) = 0;
		virtual void SetFloat4(const std::string& name, const glm::vec4& value) = 0;
		virtual void SetMat4(const std::string& name, const glm::mat4& value) = 0;

		static Ref<Shader> Create(const std::filesystem::path& filepath, const bool useCache = true);
		static Ref<Shader> Create(const std::string& name, const std::unordered_map<uint32_t, Ref<ScopedBuffer>>& packages);
		static Ref<Shader> Create(const std::string& name, const std::unordered_map<uint32_t, std::vector<uint32_t>>& binaries);
		static Ref<Shader> Create(const std::string& name, const std::string& source, bool useCache = true);
		static Ref<Shader> Create(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
	};

	class ShaderLibrary
	{
	public:
		void Add(const std::string& name, const Ref<Shader>& shader);
		void Add(const Ref<Shader>& shader);
		Ref<Shader> Load(const std::filesystem::path& filepath);
		Ref<Shader> Load(const std::string& name, const std::filesystem::path& filepath);

		Ref<Shader> Get(const std::string& name);

		void LoadShaderPack(const std::filesystem::path& filepath);
		void SerializeShaderPack(const std::filesystem::path& filepath);

		bool Exists(const std::string& name);

		inline std::unordered_map<std::string, Ref<Shader>>& GetShaders() { return m_Shaders; }
	private:
		std::unordered_map<std::string, Ref<Shader>> m_Shaders;
	};

}