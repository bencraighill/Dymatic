#include "dypch.h"
#include "Platform/OpenGL/OpenGLShader.h"

#include "Dymatic/Renderer/Renderer.h"
#include "Dymatic/Renderer/Utils/ShaderCompressor.h"

#include <fstream>
#include <regex>
#include <glad/glad.h>

#include <glm/gtc/type_ptr.hpp>

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#include "Dymatic/Core/Timer.h"

namespace Dymatic {

	namespace Utils {

		static GLenum ShaderTypeFromString(const std::string& type)
		{
			if (type == "vertex")
				return GL_VERTEX_SHADER;
			if (type == "fragment" || type == "pixel")
				return GL_FRAGMENT_SHADER;
			if (type == "geometry")
				return GL_GEOMETRY_SHADER;
			if (type == "compute")
				return GL_COMPUTE_SHADER;
			if (type == "control")
				return GL_TESS_CONTROL_SHADER;
			if (type == "evaluation")
				return GL_TESS_EVALUATION_SHADER;

			DY_CORE_ASSERT(false, "Unknown shader type!");
			return 0;
		}

		static shaderc_shader_kind GLShaderStageToShaderC(GLenum stage)
		{
			switch (stage)
			{
			case GL_VERTEX_SHADER:   return shaderc_glsl_vertex_shader;
			case GL_GEOMETRY_SHADER: return shaderc_glsl_geometry_shader;
			case GL_COMPUTE_SHADER: return shaderc_glsl_compute_shader;
			case GL_FRAGMENT_SHADER: return shaderc_glsl_fragment_shader;
			case GL_TESS_CONTROL_SHADER: return shaderc_glsl_tess_control_shader;
			case GL_TESS_EVALUATION_SHADER: return shaderc_glsl_tess_evaluation_shader;
			}
			DY_CORE_ASSERT(false);
			return (shaderc_shader_kind)0;
		}

		static const char* GLShaderStageToString(GLenum stage)
		{
			switch (stage)
			{
			case GL_VERTEX_SHADER:   return "GL_VERTEX_SHADER";
			case GL_GEOMETRY_SHADER: return "GL_GEOMETRY_SHADER";
			case GL_COMPUTE_SHADER: return "GL_COMPUTE_SHADER";
			case GL_FRAGMENT_SHADER: return "GL_FRAGMENT_SHADER";
			case GL_TESS_CONTROL_SHADER: return "GL_TESS_CONTROL_SHADER";
			case GL_TESS_EVALUATION_SHADER: return "GL_TESS_EVALUATION_SHADER";
			}
			DY_CORE_ASSERT(false);
			return nullptr;
		}

		static const char* GetCacheDirectory()
		{
			// TODO: make sure the assets directory is valid
			return "Resources/Cache/Shader/OpenGL";
		}

		static void CreateCacheDirectoryIfNeeded()
		{
			std::string cacheDirectory = GetCacheDirectory();
			if (!std::filesystem::exists(cacheDirectory))
				std::filesystem::create_directories(cacheDirectory);
		}

		static const char* GLShaderStageCachedOpenGLFileExtension(uint32_t stage)
		{
			switch (stage)
			{
			case GL_VERTEX_SHADER:    return ".cached_opengl.vert";
			case GL_GEOMETRY_SHADER:  return ".cached_opengl.geom";
			case GL_COMPUTE_SHADER:  return ".cached_opengl.comp";
			case GL_FRAGMENT_SHADER:  return ".cached_opengl.frag";
			case GL_TESS_CONTROL_SHADER:  return ".cached_opengl.cont";
			case GL_TESS_EVALUATION_SHADER:  return ".cached_opengl.eval";
			}
			DY_CORE_ASSERT(false);
			return "";
		}

		static const char* GLShaderStageCachedVulkanFileExtension(uint32_t stage)
		{
			switch (stage)
			{
			case GL_VERTEX_SHADER:    return ".cached_vulkan.vert";
			case GL_GEOMETRY_SHADER:  return ".cached_vulkan.geom";
			case GL_COMPUTE_SHADER:  return ".cached_vulkan.comp";
			case GL_FRAGMENT_SHADER:  return ".cached_vulkan.frag";
			case GL_TESS_CONTROL_SHADER:  return ".cached_vulkan.cont";
			case GL_TESS_EVALUATION_SHADER:  return ".cached_vulkan.eval";
			}
			DY_CORE_ASSERT(false);
			return "";
		}

		static std::string MakeStringMacroSafe(const std::string& string)
		{
			std::string macroString(string.length(), '_');

			std::transform(string.begin(), string.end(), macroString.begin(), [](char c)
			{
				if (std::isalnum(c))
					return (char)std::toupper(c);

				return '_';
			});

			return macroString;
		}

		static void FormatCompilationError(std::string& errorMessage, const std::vector<std::filesystem::path>& debugFileMappings, std::regex pattern)
		{
			auto it = errorMessage.cbegin();
			auto end = errorMessage.cend();
			std::string result;

			while (std::regex_search(it, end, std::smatch(), pattern))
			{
				std::smatch match;
				std::regex_search(it, end, match, pattern);

				result.append(it, match[0].first);

				const size_t fileNumber = std::atoll(match[1].str().c_str());
				result.append((fileNumber >= 0 && fileNumber < debugFileMappings.size()) ? debugFileMappings[fileNumber].string() : "[Unknown File Identifier]");
				result.append(fmt::format(" (line {})", match[2].str()));

				it = match[0].second;
			}

			result.append(it, end);
			errorMessage = std::move(result);
		}

		static void FormatCompilationError(std::string& errorMessage, const std::vector<std::filesystem::path>& debugFileMappings)
		{
			if (debugFileMappings.empty())
				return;

			std::regex nvidiaRegex(R"((\d+)\((\d+)\))");
			std::regex atiRegex(R"(: (\d+):(\d+):)");

			FormatCompilationError(errorMessage, debugFileMappings, nvidiaRegex);
			FormatCompilationError(errorMessage, debugFileMappings, atiRegex);
		}
	}

	static std::filesystem::path s_DefaultShaderDirectory = "Resources/Shaders";

	static const std::unordered_map<std::string, std::string> s_VulkanKeywordToGLMap = {
		{ "gl_InstanceIndex", "gl_InstanceID" }
	};

	OpenGLShader::OpenGLShader(const std::filesystem::path& filepath, const bool useCache)
		: m_FilePath(filepath)
	{
		DY_PROFILE_FUNCTION();

		CreateShaderFromPath();
	}

	OpenGLShader::OpenGLShader(const std::string& name, const std::unordered_map<uint32_t, Ref<ScopedBuffer>>& packages)
	{
		DY_PROFILE_FUNCTION();

		DY_CORE_TRACE("Creating shader '{}' from packaged binaries with {} bound stages.", name, packages.size());

		Timer timer;

		// Unpackage
		m_ShaderSources.clear();
		for (const auto& [stage, data] : packages)
			ShaderCompressor::HuffmanDecode(data, m_ShaderSources[stage]);

		// Create program
		CreateProgram();
		m_IsLoaded = true;

		DY_CORE_WARN("Shader creation took {} ms", timer.ElapsedMillis());
	}

	OpenGLShader::OpenGLShader(const std::string& name, const std::unordered_map<uint32_t, std::vector<uint32_t>>& binaries)
		: m_Name(name)
	{
		DY_PROFILE_FUNCTION();
		
		DY_CORE_TRACE("Creating shader '{}' from binaries with {} bound stages.", name, binaries.size());
		
		Timer timer;
		m_OpenGLSPIRV = binaries;
		CreateProgram();
		m_IsLoaded = true;
		
		DY_CORE_WARN("Shader creation took {} ms", timer.ElapsedMillis());
	}

	OpenGLShader::OpenGLShader(const std::string& name, const std::string& source, bool useCache)
		: m_Name(name), m_FilePath(s_DefaultShaderDirectory / name), m_SourceCode(source)
	{
		if (!useCache)
			DeleteCache();
		
		m_ShaderSources = PreProcess(std::string(source));

		{
			Timer timer;
			m_IsLoaded = CreateProgram();
			DY_CORE_WARN("Shader creation took {} ms", timer.ElapsedMillis());
		}
	}

	OpenGLShader::OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
		: m_Name(name)
	{
		DY_PROFILE_FUNCTION();

		m_ShaderSources[GL_VERTEX_SHADER] = vertexSrc;
		m_ShaderSources[GL_FRAGMENT_SHADER] = fragmentSrc;

		if (CreateProgram())
			m_IsLoaded = true;
	}

	OpenGLShader::~OpenGLShader()
	{
		DY_PROFILE_FUNCTION();

		DestroyShader();
	}

	void OpenGLShader::DestroyShader()
	{
		glDeleteProgram(m_RendererID);
	}

	void OpenGLShader::CreateShaderFromPath(const bool useCache)
	{
		m_Name = m_FilePath.stem().string();

		DY_CORE_TRACE("Creating shader '{}' from source file '{}'.", m_Name, m_FilePath.string());

		Utils::CreateCacheDirectoryIfNeeded();

		if (!useCache)
			DeleteCache();

		std::string source = ReadFile(m_FilePath);
		m_ShaderSources = PreProcess(source);

		{
			Timer timer;
			m_IsLoaded = CreateProgram();
			DY_CORE_WARN("Shader creation took {} ms", timer.ElapsedMillis());
		}
	}

	std::string OpenGLShader::ReadFile(const std::filesystem::path& filepath)
	{
		DY_PROFILE_FUNCTION();

		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary); // ifstream closes itself due to RAII
		if (in)
		{
			in.seekg(0, std::ios::end);
			size_t size = in.tellg();
			if (size != -1)
			{
				result.resize(size);
				in.seekg(0, std::ios::beg);
				in.read(&result[0], size);
			}
			else
			{
				DY_CORE_ERROR("Could not read from file '{}'", filepath.string());
			}
		}
		else
		{
			DY_CORE_ERROR("Could not open file '{0}'", filepath.string());
		}

		return result;
	}

	void OpenGLShader::GatherIncludes(const std::filesystem::path& filepath, std::string& source, std::unordered_set<std::filesystem::path>& includedFiles, uint32_t parentFileNumber, uint32_t startingLine)
	{
		DY_PROFILE_FUNCTION();

		size_t currentLine = startingLine;
		size_t currentReadingPosition = 0;

		const char* includeToken = "#include ";
		size_t includeTokenLength = strlen(includeToken);
		size_t pos = source.find(includeToken, 0); // Start of include declaration line
		while (pos != std::string::npos)
		{
			size_t eol = source.find_first_of("\r\n", pos); // End of include declaration line
			DY_CORE_ASSERT(eol != std::string::npos, "Syntax error");
			size_t begin = pos + includeTokenLength; // Start of shader include file (after "#include " keyword)
			std::string include = source.substr(begin, eol - begin);

			source.erase(pos, eol - pos); // Remove the token

			// Search the included path relative to the source file
			std::filesystem::path includePath = (filepath.parent_path() / include).lexically_normal();

			if (!std::filesystem::exists(includePath))
			{
				// If we cannot relatively find the file, look in the base directory
				includePath = s_DefaultShaderDirectory / include;

				if (!std::filesystem::exists(includePath))
				{
					DY_CORE_ERROR("Could not find included relative file '{}'", filepath.string());
					DY_CORE_VERIFY(false, "Included file does not exist");
				}
			}

			currentLine += std::count(source.begin() + currentReadingPosition, source.begin() + pos, '\n');

			// Check if this external/header file has already been included in this source file
			if (includedFiles.find(includePath) != includedFiles.end())
			{
				currentReadingPosition = pos;
				pos = source.find(includeToken, currentReadingPosition);
				continue;
			}

			std::string includeSource = ReadFile(includePath);

			// Insert #line directive from the included file
			uint32_t includeFileNumber = m_DebugFileMapping.size();
			m_DebugFileMapping.push_back(includePath);
			std::string lineDirectiveStart = fmt::format("#line 1 {}", includeFileNumber) + "\n";
			source.insert(pos, lineDirectiveStart);

			// Check for guard directive
			bool guardFile = false;
			const std::string guardDirective = "#pragma once";
			while (includeSource.find(guardDirective) != std::string::npos)
			{
				// Remove directive from source
				const size_t guardPos = includeSource.find(guardDirective);
				const size_t eol = includeSource.find_first_of("\r\n", guardPos);
				includeSource.erase(guardPos, eol - guardPos);

				guardFile = true;
			}

			if (guardFile)
				includedFiles.insert(includePath);

			GatherIncludes(includePath, includeSource, includedFiles, includeFileNumber);
			source.insert(pos + lineDirectiveStart.length(), includeSource);

			// Insert #line directive to return to the original file
			std::string lineDirectiveEnd = '\n' + fmt::format("#line {} {}", currentLine, parentFileNumber) + '\n';
			source.insert(pos + lineDirectiveStart.length() + includeSource.length(), lineDirectiveEnd);

			currentReadingPosition = pos + lineDirectiveStart.length() + includeSource.length() + lineDirectiveEnd.length();
			pos = source.find(includeToken, currentReadingPosition);
		}
	}

	std::unordered_map<GLenum, std::string> OpenGLShader::PreProcess(std::string& source)
	{
		DY_PROFILE_FUNCTION();

		std::unordered_map<GLenum, std::string> shaderSources;
		std::unordered_map<GLenum, size_t> stageLines;

		// Split sources by type directive
		const char* typeToken = "#type";
		size_t typeTokenLength = strlen(typeToken);
		size_t pos = source.find(typeToken, 0); // Start of shader type declaration line
		while (pos != std::string::npos)
		{

			size_t eol = source.find_first_of("\r\n", pos); // End of shader type declaration line
			DY_CORE_ASSERT(eol != std::string::npos, "Syntax error");
			size_t begin = pos + typeTokenLength + 1; // Start of shader type name (after "#type " keyword)
			std::string typeString = source.substr(begin, eol - begin);
			GLenum type = Utils::ShaderTypeFromString(typeString);
			DY_CORE_ASSERT(type, "Invalid shader type specified");

			// Count the number of lines into the original source file the type directive is (accounting for already stripped #type directives)
			stageLines[type] = 1 + std::count(source.begin(), source.begin() + pos, '\n') + 1;

			size_t nextLinePos = source.find_first_not_of("\r\n", eol); //Start of shader code after shader type declaration line
			DY_CORE_ASSERT(nextLinePos != std::string::npos, "Syntax error");
			pos = source.find(typeToken, nextLinePos); //Start of next shader type declaration line

			shaderSources[type] = (pos == std::string::npos) ? source.substr(nextLinePos) : source.substr(nextLinePos, pos - nextLinePos);
		}

		// Gather all includes for each shader stage
		m_DebugFileMapping.push_back(m_FilePath);
		for (auto& [type, source] : shaderSources)
		{
			std::unordered_set<std::filesystem::path> includedFiles;
			GatherIncludes(m_FilePath, source, includedFiles, 0, stageLines[type]);
		}

		return shaderSources;
	}

	bool OpenGLShader::VerifyRequirements(std::string& source)
	{
		const std::string requireDirective = "#require ";
		while (source.find(requireDirective) != std::string::npos)
		{
			const size_t requirePos = source.find(requireDirective);
			const size_t lineEndPos = source.find_first_of("\r\n", requirePos);
			const size_t extensionPos = requirePos + requireDirective.length();
			const size_t extensionLength = lineEndPos - extensionPos;
			const std::string requiredMacro = source.substr(extensionPos, extensionLength);
			source.erase(requirePos, lineEndPos);

			if (m_Macros.find(requiredMacro) == m_Macros.end() || m_Macros.at(requiredMacro) == "0")
			{
				DY_CORE_WARN("Macro binding requirement '{}' failed. Shader stage will not be bound!", requiredMacro);
				return false;
			}
		}

		return true;
	}

	void OpenGLShader::DeleteCache()
	{
		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();

		std::string filename = m_FilePath.filename().string();
		
		for (const auto& entry : std::filesystem::directory_iterator(cacheDirectory))
		{
			if (entry.path().filename().string().find(filename) != std::string::npos)
				std::filesystem::remove(entry.path());
		}
	}

	bool OpenGLShader::CompileOrGetVulkanBinaries(const std::unordered_map<GLenum, std::string>& shaderSources)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		const bool optimize = false;
		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_performance);

		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();

		auto& shaderData = m_VulkanSPIRV;
		shaderData.clear();
		for (auto&& [stage, source] : shaderSources)
		{
			std::filesystem::path shaderFilePath = m_FilePath;
			std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.filename().string() + Utils::GLShaderStageCachedVulkanFileExtension(stage));

			std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
			if (in.is_open())
			{
				in.seekg(0, std::ios::end);
				auto size = in.tellg();
				in.seekg(0, std::ios::beg);

				auto& data = shaderData[stage];
				data.resize(size / sizeof(uint32_t));
				in.read((char*)data.data(), size);
			}
			else
			{
				shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, Utils::GLShaderStageToShaderC(stage), m_FilePath.string().c_str(), options);
				if (module.GetCompilationStatus() != shaderc_compilation_status_success)
				{
					m_Error = module.GetErrorMessage();
					DY_CORE_ERROR(m_Error);
					DY_CORE_ASSERT(false);
					return false;
				}

				shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());

				std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
				if (out.is_open())
				{
					auto& data = shaderData[stage];
					out.write((char*)data.data(), data.size() * sizeof(uint32_t));
					out.flush();
					out.close();
				}
			}
		}

		for (auto&& [stage, data] : shaderData)
			if (stage != GL_COMPUTE_SHADER)
				Reflect(stage, data);

		return true;
	}

	bool OpenGLShader::CompileOrGetOpenGLBinaries()
	{
		auto& shaderData = m_OpenGLSPIRV;

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);
		const bool optimize = true;
		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_performance);

		//spirv_cross::CompilerGLSL::Options glslOptions;
		//glslOptions.es = false;
		//glslOptions.version = 450;
		//glslOptions.enable_420pack_extension = true;

		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();

		shaderData.clear();
		m_OpenGLSourceCode.clear();
		for (auto&& [stage, spirv] : m_VulkanSPIRV)
		{
			if (spirv.empty())
				continue;

			std::filesystem::path shaderFilePath = m_FilePath;
			std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.filename().string() + Utils::GLShaderStageCachedOpenGLFileExtension(stage));

			std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
			if (in.is_open())
			{
				in.seekg(0, std::ios::end);
				auto size = in.tellg();
				in.seekg(0, std::ios::beg);

				auto& data = shaderData[stage];
				data.resize(size / sizeof(uint32_t));
				in.read((char*)data.data(), size);
			}
			else
			{
				spirv_cross::CompilerGLSL glslCompiler(spirv);
				//glslCompiler.set_common_options(glslOptions);
				m_OpenGLSourceCode[stage] = glslCompiler.compile();

				// Take a copy of the OpenGL source code (so we can modify it locally)
				std::string source = m_OpenGLSourceCode[stage];

				// Replace all Vulkan keywords with their OpenGL counterparts
				for (const auto& [vulkanKeyword, openglKeyword] : s_VulkanKeywordToGLMap)
				{
					size_t pos = source.find(vulkanKeyword);
					while (pos != std::string::npos)
					{
						// Replace the substring with the new string
						source.replace(pos, vulkanKeyword.length(), openglKeyword);

						// Search for the next occurrence of the substring
						pos = source.find(vulkanKeyword, pos + openglKeyword.length());
					}
				}

				shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, Utils::GLShaderStageToShaderC(stage), m_FilePath.string().c_str(), options);
				if (module.GetCompilationStatus() != shaderc_compilation_status_success)
				{
					m_Error = module.GetErrorMessage();
					DY_CORE_ERROR(m_Error);
					DY_CORE_ASSERT(false);
					return false;
				}

				shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());

				std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
				if (out.is_open())
				{
					auto& data = shaderData[stage];
					out.write((char*)data.data(), data.size() * sizeof(uint32_t));
					out.flush();
					out.close();
				}
			}
		}

		return true;
	}

	// Direct OpenGL shader compilation (not using SpirV)
	bool OpenGLShader::CreateProgram()
	{
		bool success = true;

		GLuint program = glCreateProgram();

		std::vector<GLuint> shaderIDs;
		for (auto&& [stage, source] : m_ShaderSources)
		{
			std::string modifiedSource = source;

			// Verify require statements
			if (!VerifyRequirements(modifiedSource))
				continue;

			// Add header directives
			size_t versionPos = modifiedSource.find("#version");
			if (versionPos == std::string::npos)
			{
				versionPos = 0;
				DY_CORE_WARN("#version directive was not found in shader source");
			}

			const size_t headerPos = modifiedSource.find("\n", versionPos) + 1;

			// Insert macros/values
			for (const auto& [macro, value] : m_Macros)
				modifiedSource.insert(headerPos, fmt::format("#define {} {}", macro, value) + "\n");

			// Insert other header defines/extensions
			modifiedSource.insert(headerPos, "#extension GL_KHR_vulkan_glsl: enable\n");

			// Translate vulkan features
			// Replace all Vulkan keywords with their OpenGL counterparts
			for (const auto& [vulkanKeyword, openglKeyword] : s_VulkanKeywordToGLMap)
			{
				size_t pos = modifiedSource.find(vulkanKeyword);
				while (pos != std::string::npos)
				{
					// Replace the substring with the new string
					modifiedSource.replace(pos, vulkanKeyword.length(), openglKeyword);

					// Search for the next occurrence of the substring
					pos = modifiedSource.find(vulkanKeyword, pos + openglKeyword.length());
				}
			}

			GLuint shaderID = shaderIDs.emplace_back(glCreateShader(stage));
			GLchar* sourceData = (GLchar*)modifiedSource.c_str();
			glShaderSource(shaderID, 1, &sourceData, nullptr);

			glSpecializeShader(shaderID, "main", 0, nullptr, nullptr);
			glCompileShader(shaderID);

			// Check compilation status
			GLint isCompiled;
			glGetShaderiv(shaderID, GL_COMPILE_STATUS, &isCompiled);
			if (isCompiled == GL_FALSE) {
				GLint maxLength;
				glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &maxLength);

				std::vector<GLchar> infoLog(maxLength);
				glGetShaderInfoLog(shaderID, maxLength, &maxLength, infoLog.data());
				std::string infoLogString(infoLog.begin(), infoLog.end());

				Utils::FormatCompilationError(infoLogString, m_DebugFileMapping);
				m_Error = fmt::format("Shader compilation failed at stage {} ({}):\n{}", Utils::GLShaderStageToString(stage), m_FilePath, infoLogString);
				DY_CORE_ERROR(m_Error);
				DY_CORE_ASSERT(false);

				// Cleanup
				glDeleteProgram(program);

				for (auto id : shaderIDs)
					glDeleteShader(id);

				success = false;
			}

			if (stage == GL_COMPUTE_SHADER)
				m_IsCompute = true;

			glAttachShader(program, shaderID);
		}

		// Ensure we can retrieve the binary

		glProgramParameteri(program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);

		glLinkProgram(program);

		// Check linking status
		GLint isLinked;
		glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, infoLog.data());
			m_Error = fmt::format("Shader linking failed ({}):\n{}", m_FilePath, infoLog.data());
			DY_CORE_ERROR(m_Error);
			DY_CORE_ASSERT(false);

			glDeleteProgram(program);

			for (auto id : shaderIDs)
				glDeleteShader(id);

			success = false;
		}

		for (auto id : shaderIDs)
		{
			glDetachShader(program, id);
			glDeleteShader(id);
		}

		if (success)
		{
			GLint binaryLength;
			glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &binaryLength);

			std::vector<GLubyte> binaryData(binaryLength);
			GLenum binaryFormat;
			glGetProgramBinary(program, binaryLength, nullptr, &binaryFormat, binaryData.data());

			const std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();
			const std::filesystem::path cachedPath = cacheDirectory / (m_FilePath.filename().string() + ".cached_opengl");
			std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
			if (out.is_open())
			{
				out.write((char*)binaryData.data(), binaryData.size() * sizeof(GLubyte));
				out.flush();
				out.close();
			}
		}

		m_RendererID = program;

		return success;
	}

	void OpenGLShader::Reflect(GLenum stage, const std::vector<uint32_t>& shaderData)
	{
		spirv_cross::Compiler compiler(shaderData);
		spirv_cross::ShaderResources resources = compiler.get_shader_resources();

		DY_CORE_TRACE("OpenGLShader::Reflect - {} {}", Utils::GLShaderStageToString(stage), m_FilePath);
		DY_CORE_TRACE("    {} uniform buffers", resources.uniform_buffers.size());
		DY_CORE_TRACE("    {} storage buffers", resources.storage_buffers.size());
		DY_CORE_TRACE("    {} resources", resources.sampled_images.size());

		DY_CORE_TRACE("Uniform buffers:");
		for (const auto& resource : resources.uniform_buffers)
		{
			const auto& bufferType = compiler.get_type(resource.base_type_id);
			uint32_t bufferSize = compiler.get_declared_struct_size(bufferType);
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			int memberCount = bufferType.member_types.size();

			DY_CORE_TRACE("  {}", resource.name);
			DY_CORE_TRACE("    Size = {0}", bufferSize);
			DY_CORE_TRACE("    Binding = {0}", binding);
			DY_CORE_TRACE("    Members = {0}", memberCount);
		}
	}

	void OpenGLShader::Bind() const
	{
		DY_PROFILE_FUNCTION();

		glUseProgram(m_RendererID);
	}

	void OpenGLShader::Unbind() const
	{
		DY_PROFILE_FUNCTION();

		glUseProgram(0);
	}

	void OpenGLShader::Dispatch(int numGroupsX, int numGroupsY, int numGroupsZ) const
	{
		DY_PROFILE_FUNCTION();

		Bind();

		if (m_IsCompute)
		{
			glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
		}
	}

	void OpenGLShader::Reload()
	{
		if (m_FilePath.empty())
			return;

		DestroyShader();

		CreateShaderFromPath(false);
	}

	void OpenGLShader::Regenerate()
	{
		Timer timer;
		DestroyShader();
		CreateProgram();
		DY_CORE_WARN("Regenerating shader took {} ms", timer.ElapsedMillis());
	}

	void OpenGLShader::SetMacro(const std::string& macro, const std::string& value)
	{
		DY_CORE_INFO("Setting macro {} as {}", macro, value);
		m_Macros[macro] = value;
		Regenerate();
	}

	void OpenGLShader::SetMacro(const std::string& macro)
	{
		SetMacro(macro, std::string());
	}

	void OpenGLShader::SetMacro(const std::string& macro, bool flag)
	{
		SetMacro(macro, std::string(flag ? "1" : "0"));
	}

	bool OpenGLShader::GetMacroFlag(const std::string& name) const
	{
		if (m_Macros.find(name) == m_Macros.end())
			return false;

		return m_Macros.at(name) != "0";
	}

	std::unordered_map<uint32_t, Ref<ScopedBuffer>> OpenGLShader::GetPackagedShaderBuffers() const
	{
		// Minify the GLSL source code of each stage
		std::unordered_map<uint32_t, std::string> minifiedSources;
		ShaderCompressor::Minify(m_ShaderSources, minifiedSources);

		std::unordered_map<uint32_t, Ref<ScopedBuffer>> result;
		for (const auto& [stage, source] : minifiedSources)
			result[stage] = ShaderCompressor::HuffmanEncode(source);

		return result;
	}

	void OpenGLShader::SetInt(const std::string& name, int value)
	{
		DY_PROFILE_FUNCTION();

		UploadUniformInt(name, value);
	}

	void OpenGLShader::SetIntArray(const std::string& name, int* values, uint32_t count)
	{
		UploadUniformIntArray(name, values, count);
	}

	void OpenGLShader::SetFloat(const std::string& name, float value)
	{
		DY_PROFILE_FUNCTION();

		UploadUniformFloat(name, value);
	}

	void OpenGLShader::SetFloat2(const std::string& name, const glm::vec2& value)
	{
		DY_PROFILE_FUNCTION();

		UploadUniformFloat2(name, value);
	}

	void OpenGLShader::SetFloat3(const std::string& name, const glm::vec3& value)
	{
		DY_PROFILE_FUNCTION();

		UploadUniformFloat3(name, value);
	}

	void OpenGLShader::SetFloat4(const std::string& name, const glm::vec4& value)
	{
		DY_PROFILE_FUNCTION();

		UploadUniformFloat4(name, value);
	}

	void OpenGLShader::SetMat4(const std::string& name, const glm::mat4& value)
	{
		DY_PROFILE_FUNCTION();

		UploadUniformMat4(name, value);
	}

	void OpenGLShader::UploadUniformInt(const std::string& name, int value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1i(location, value);
	}

	void OpenGLShader::UploadUniformIntArray(const std::string& name, int* values, uint32_t count)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1iv(location, count, values);
	}

	void OpenGLShader::UploadUniformFloat(const std::string& name, float value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1f(location, value);
	}

	void OpenGLShader::UploadUniformFloat2(const std::string& name, const glm::vec2& value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform2f(location, value.x, value.y);
	}

	void OpenGLShader::UploadUniformFloat3(const std::string& name, const glm::vec3& value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform3f(location, value.x, value.y, value.z);
	}

	void OpenGLShader::UploadUniformFloat4(const std::string& name, const glm::vec4& value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform4f(location, value.x, value.y, value.z, value.w);
	}

	void OpenGLShader::UploadUniformMat3(const std::string& name, const glm::mat3& matrix)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void OpenGLShader::UploadUniformMat4(const std::string& name, const glm::mat4& matrix)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

}