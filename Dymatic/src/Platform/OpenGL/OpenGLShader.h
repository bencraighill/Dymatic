#pragma once

#include "Dymatic/Renderer/Shader.h"
#include <glm/glm.hpp>

#include <unordered_set>

// TODO: REMOVE!
typedef unsigned int GLenum;

namespace Dymatic {

	class OpenGLShader : public Shader
	{
	public:
		OpenGLShader(const std::filesystem::path& filepath, const bool useCache);
		OpenGLShader(const std::string& name, const std::unordered_map<uint32_t, Ref<ScopedBuffer>>& packages);
		OpenGLShader(const std::string& name, const std::unordered_map<uint32_t, std::vector<uint32_t>>& binaries);
		OpenGLShader(const std::string& name, const std::string& source, bool useCache);
		OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
		virtual ~OpenGLShader();

		virtual void Bind() const override;
		virtual void Unbind() const override;
		virtual uint32_t GetRendererID() const override { return m_RendererID; }

		virtual bool IsLoaded() const { return m_IsLoaded; }

		virtual void Dispatch(int numGroupsX, int numGroupsY, int numGroupsZ) const override;

		virtual void SetMacro(const std::string& macro) override;
		virtual void SetMacro(const std::string& macro, const std::string& value) override;
		virtual void SetMacro(const std::string& macro, bool flag) override;
		virtual bool GetMacroFlag(const std::string& name) const override;

		virtual void Reload() override;
		virtual void Regenerate() override;

		virtual const std::string& GetName() const override { return m_Name; }
		virtual const std::string& GetSourceCode() const override { return m_SourceCode; }
		virtual const std::string& GetError() const override { return m_Error; }
		virtual const std::unordered_map<uint32_t, std::vector<uint32_t>>& GetBinaries() const override { return m_OpenGLSPIRV; }
		virtual std::unordered_map<uint32_t, Ref<ScopedBuffer>> GetPackagedShaderBuffers() const override;

		virtual void SetInt(const std::string& name, int value) override;
		virtual void SetIntArray(const std::string& name, int* values, uint32_t count) override;
		virtual void SetFloat(const std::string& name, float value) override;
		virtual void SetFloat2(const std::string& name, const glm::vec2& value) override;
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) override;
		virtual void SetFloat4(const std::string& name, const glm::vec4& value) override;
		virtual void SetMat4(const std::string& name, const glm::mat4& value) override;

		void UploadUniformInt(const std::string& name, int value);
		void UploadUniformIntArray(const std::string& name, int* values, uint32_t count);

		void UploadUniformFloat(const std::string& name, float value);
		void UploadUniformFloat2(const std::string& name, const glm::vec2& value);
		void UploadUniformFloat3(const std::string& name, const glm::vec3& value);
		void UploadUniformFloat4(const std::string& name, const glm::vec4& value);

		void UploadUniformMat3(const std::string& name, const glm::mat3& matrix);
		void UploadUniformMat4(const std::string& name, const glm::mat4& matrix);
	private:
		void DestroyShader();
		void CreateShaderFromPath(const bool useCache = true);

		std::string ReadFile(const std::filesystem::path& filepath);
		void GatherIncludes();
		void GatherIncludes(const std::filesystem::path& filepath, std::string& source, std::unordered_set<std::filesystem::path>& includedFiles, uint32_t parentFileNumber, uint32_t startingLine = 1);
		std::unordered_map<GLenum, std::string> PreProcess(std::string& source);
		bool VerifyRequirements(std::string& source);

		void DeleteCache();
		bool CompileOrGetVulkanBinaries(const std::unordered_map<GLenum, std::string>& shaderSources);
		bool CompileOrGetOpenGLBinaries();
		bool CreateProgram();
		void Reflect(GLenum stage, const std::vector<uint32_t>& shaderData);
	private:
		uint32_t m_RendererID;
		std::filesystem::path m_FilePath;
		std::string m_Name;
		std::string m_SourceCode;
		std::string m_Error;
		std::unordered_map<std::string, std::string> m_Macros;
		std::unordered_map<GLenum, std::string> m_ShaderSources;

		// Debug Only
		std::vector<std::filesystem::path> m_DebugFileMapping;

		bool m_IsLoaded = false;
		bool m_IsCompute = false;

		std::unordered_map<GLenum, std::vector<uint32_t>> m_VulkanSPIRV;
		std::unordered_map<GLenum, std::vector<uint32_t>> m_OpenGLSPIRV;

		std::unordered_map<GLenum, std::string> m_OpenGLSourceCode;
	};
}