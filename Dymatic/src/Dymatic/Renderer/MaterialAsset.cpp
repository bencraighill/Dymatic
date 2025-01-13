#include "dypch.h"
#include "Dymatic/Renderer/MaterialAsset.h"

#include "Dymatic/Renderer/RendererConstants.h"

namespace Dymatic {

	namespace Utils {
	
		static void ValidateBufferSize(const Buffer& reference, Buffer& source)
		{
			if (reference.Size == source.Size)
				return;

			if (source)
				source.Release();

			if (reference.Size != 0)
				source.Allocate(reference.Size);
		}

		static uint32_t GetParameterSize(MaterialAsset::MaterialParameterType type)
		{
			switch (type)
			{
			case MaterialAsset::MaterialParameterType::Bool: return sizeof(uint32_t);
			case MaterialAsset::MaterialParameterType::Float: return sizeof(float);
			case MaterialAsset::MaterialParameterType::Vector2: return sizeof(glm::vec2);
			case MaterialAsset::MaterialParameterType::Vector3: return sizeof(glm::vec3);
			case MaterialAsset::MaterialParameterType::Vector4: return sizeof(glm::vec4);
			}
			return 0;
		}
	
	}

	const char* MaterialAsset::AlphaBlendModeToString(const AlphaBlendMode mode)
	{
		switch (mode)
		{
		case Opaque: return "Opaque";
		case Masked: return "Masked";
		case Translucent: return "Translucent";
		case Dithered: return "Dithered";
		}
		return "Unknown";
	}

	MaterialAsset::AlphaBlendMode MaterialAsset::GetMaterialAlphaBlendMode(const std::string& string)
	{
		if (string == "Opaque") return AlphaBlendMode::Opaque;
		if (string == "Masked") return AlphaBlendMode::Masked;
		if (string == "Translucent") return AlphaBlendMode::Translucent;
		if (string == "Dithered") return AlphaBlendMode::Dithered;

		return AlphaBlendMode::Opaque;
	}

	const char* MaterialAsset::MaterialRenderStageToString(const MaterialRenderStage stage)
	{
		switch (stage)
		{
		case None: return "None";
		case Default: return "Default";
		case PreDepth: return "PreDepth";
		case VXGI: return "VXGI";
		case Shadow: return "Shadow";
		case ShadowCSM: return "ShadowCSM";
		}
		return "None";
	}

	MaterialAsset::MaterialRenderStage MaterialAsset::GetMaterialRenderStage(const std::string& string)
	{
		if (string == "None") return None;
		if (string == "Default") return Default;
		if (string == "PreDepth") return PreDepth;
		if (string == "VXGI") return VXGI;
		if (string == "Shadow") return Shadow;
		if (string == "ShadowCSM") return ShadowCSM;
		
		return None;
	}

	const char* MaterialAsset::MaterialUsageToString(const MaterialUsage usage)
	{
		switch (usage)
		{
		case MaterialUsage::Surface: return "Surface";
		case MaterialUsage::PostProcessing: return "Post Processing";
		case MaterialUsage::Volumetric: return "Volumetric";
		case MaterialUsage::Particle: return "Particle";
		}
		return "None";
	}

	MaterialAsset::MaterialUsage MaterialAsset::GetMaterialUsage(const std::string& string)
	{
		if (string == "Surface") return MaterialUsage::Surface;
		if (string == "Post Processing") return MaterialUsage::PostProcessing;
		if (string == "Volumetric") return MaterialUsage::Volumetric;
		if (string == "Particle") return MaterialUsage::Particle;
		
		return Surface;
	}

	const char* MaterialAsset::MaterialParameterData::MaterialParameterTypeToString(const MaterialParameterType type)
	{
		switch (type)
		{
		case MaterialParameterType::Bool:    return "Bool";
		case MaterialParameterType::Float:   return "Float";
		case MaterialParameterType::Vector2: return "Vector2";
		case MaterialParameterType::Vector3: return "Vector3";
		case MaterialParameterType::Vector4: return "Vector4";
		case MaterialParameterType::Texture: return "Texture";
		}

		return "None";
	}

	const MaterialAsset::MaterialParameterType MaterialAsset::MaterialParameterData::MaterialParameterTypeFromString(const std::string& typeString)
	{
		if (typeString == "Bool")	return MaterialParameterType::Bool;
		if (typeString == "Float")	return MaterialParameterType::Float;
		if (typeString == "Vector2")	return MaterialParameterType::Vector2;
		if (typeString == "Vector3")	return MaterialParameterType::Vector3;
		if (typeString == "Vector4")	return MaterialParameterType::Vector4;
		if (typeString == "Texture")	return MaterialParameterType::Texture;

		return MaterialParameterType::Float;
	}

	MaterialSource::MaterialSource(const Ref<Editor::MaterialGraph> editorGraph)
		: m_EditorGraph(editorGraph), m_IsLoaded(false)
	{}

	MaterialSource::MaterialSource(const std::unordered_map<MaterialRenderStage, Ref<Shader>>& shaders, const std::vector<MaterialTexture>& textures, const std::unordered_map<std::string, MaterialParameterData>& parameters, Ref<Editor::MaterialGraph> editorGraph)
		: m_Shaders(shaders), m_EditorGraph(editorGraph)
	{
		for (uint32_t i = 0; i < MaterialRenderStage::MATERIAL_RENDER_STAGE_SIZE; i++)
		{
			if (m_Shaders.find((MaterialRenderStage)i) == m_Shaders.end())
				m_Shaders[(MaterialRenderStage)i] = nullptr;
		}

		SetParameters(parameters);
		SetTextures(textures);

		m_IsLoaded = true;
	}

	MaterialSource::~MaterialSource()
	{
		m_ParameterBuffer.Release();
		m_TextureBuffer.Release();
	}

	MaterialSource& MaterialSource::operator=(const MaterialSource& other)
	{
		if (this == &other)
			return *this;

		m_IsLoaded = other.m_IsLoaded;
		m_Properties = other.m_Properties;

		m_Shaders = other.m_Shaders;
		m_PathTraceSource = other.m_PathTraceSource;
		SetTextures(other.m_Textures);
		SetParameters(other.m_Parameters);

		m_InvalidationID = UUID();

		return *this;
	}

	void MaterialSource::SetTextures(const std::vector<MaterialTexture>& textures)
	{
		m_Textures = textures;

		if (m_TextureBuffer)
			m_TextureBuffer.Release();

		if (m_Textures.empty())
			return;

		m_TextureBuffer = Buffer(m_Textures.size() * sizeof(uint64_t));
		size_t offset = 0;
		for (const auto& [id, texture] : m_Textures)
		{
			uint64_t handle = texture->GetHandle();
			m_TextureBuffer.Write(&handle, sizeof(uint64_t), offset);
			offset += sizeof(uint64_t);
		}
	}

	void MaterialSource::SetParameters(const std::unordered_map<std::string, MaterialParameterData>& parameters)
	{
		m_Parameters = parameters;

		if (m_Parameters.empty())
		{
			m_ParameterBuffer.Release();
			return;
		}

		// Ensure upload order is correct for alignment with std140
		std::unordered_map<MaterialParameterType, std::vector<std::string>> parameterTypes;
		for (const auto& [name, parameter] : parameters)
			parameterTypes[parameter.Type].push_back(name);


		// Calculate the buffer size
		uint32_t bufferSize = 0;
		for (const auto& [name, parameter] : m_Parameters)
		{
			switch (parameter.Type)
			{
			case MaterialParameterType::Bool:    bufferSize += sizeof(uint32_t);  break;
			case MaterialParameterType::Float:   bufferSize += sizeof(float);	  break;
			case MaterialParameterType::Vector2: bufferSize += sizeof(glm::vec2); break;
			case MaterialParameterType::Vector3: bufferSize += sizeof(glm::vec4); break;
			case MaterialParameterType::Vector4: bufferSize += sizeof(glm::vec4); break;
			}
		}

		DY_CORE_VERIFY(bufferSize <= RendererConstants::MaxMaterialBufferSize, "Max user material buffer size exceeded!");
		
		// Ensured appropriate memory is allocated
		if (bufferSize != m_ParameterBuffer.Size)
		{
			if (m_ParameterBuffer)
				m_ParameterBuffer.Release();
			
			if (bufferSize != 0)
				m_ParameterBuffer.Allocate(bufferSize);
		}

		// Copy the data into the buffer (in order to avoid alignment issues with std140 packing)
		uint32_t offset = 0;

		for (const auto& name : parameterTypes[MaterialParameterType::Vector4])
		{
			MaterialParameterData& parameter = m_Parameters[name];
			parameter.Offset = offset;
			memcpy(m_ParameterBuffer.Data + offset, &parameter.Value.Vector4, sizeof(glm::vec4));
			offset += sizeof(glm::vec4);
		}

		for (const auto& name : parameterTypes[MaterialParameterType::Vector3])
		{
			MaterialParameterData& parameter = m_Parameters[name];
			parameter.Offset = offset;
			memcpy(m_ParameterBuffer.Data + offset, &parameter.Value.Vector3, sizeof(glm::vec3));

			// Packed as vec4 to avoid alignment issues
			offset += sizeof(glm::vec4);
		}

		for (const auto& name : parameterTypes[MaterialParameterType::Vector2])
		{
			MaterialParameterData& parameter = m_Parameters[name];
			parameter.Offset = offset;
			memcpy(m_ParameterBuffer.Data + offset, &parameter.Value.Vector2, sizeof(glm::vec2));
			offset += sizeof(glm::vec2);
		}

		for (const auto& name : parameterTypes[MaterialParameterType::Float])
		{
			MaterialParameterData& parameter = m_Parameters[name];
			parameter.Offset = offset;
			memcpy(m_ParameterBuffer.Data + offset, &parameter.Value.Float, sizeof(float));
			offset += sizeof(float);
		}

		for (const auto& name : parameterTypes[MaterialParameterType::Bool])
		{
			MaterialParameterData& parameter = m_Parameters[name];
			parameter.Offset = offset;
			memcpy(m_ParameterBuffer.Data + offset, &parameter.Value.Bool, sizeof(uint32_t));
			offset += sizeof(uint32_t);
		}
	}

	MaterialInstance::~MaterialInstance()
	{
		m_ParameterBuffer.Release();
		m_TextureBuffer.Release();
	}

	const Buffer& MaterialInstance::GetParameterBuffer()
	{
		CheckParentInvalidation();
		return m_ParameterBuffer;
	}

	void MaterialInstance::SetParameterOverride(const std::string& name, const MaterialParameterData::Data& value)
	{
		m_ParameterOverrides[name] = value;
		InvalidateParameters();
	}

	void MaterialInstance::RemoveParameterOverride(const std::string& name)
	{
		if (m_ParameterOverrides.find(name) == m_ParameterOverrides.end())
			return;

		m_ParameterOverrides.erase(name);
		InvalidateParameters();
	}

	const Buffer& MaterialInstance::GetTextureBuffer()
	{
		CheckParentInvalidation();
		return m_TextureBuffer;
	}

	void MaterialInstance::SetTextureOverride(const uint64_t id, const Ref<Texture2D> texture)
	{
		m_TextureOverrides[id] = texture;
		InvalidateTextures();
	}

	void MaterialInstance::RemoveTextureOverride(const uint64_t id)
	{
		if (m_TextureOverrides.find(id) == m_TextureOverrides.end())
			return;

		m_TextureOverrides.erase(id);
		InvalidateTextures();
	}

	void MaterialInstance::CheckParentInvalidation()
	{
		if (m_ParentInvalidationID == m_Parent->GetInvalidationID())
			return;

		m_ParentInvalidationID = m_Parent->GetInvalidationID();
		InvalidateParameters();
		InvalidateTextures();
	}

	void MaterialInstance::InvalidateParameters()
	{
		m_InvalidationID = UUID();

		const Buffer& parentBuffer = m_Parent->GetParameterBuffer();

		// Ensure instance and parent buffer sizes match
		Utils::ValidateBufferSize(parentBuffer, m_ParameterBuffer);

		// Copy the parent buffer
		m_ParameterBuffer.Copy(parentBuffer);

		// Apply instance overrides
		const auto& parameters = m_Parent->GetParameters();
		for (const auto& [name, data] : m_ParameterOverrides)
		{
			if (parameters.find(name) == parameters.end())
				continue;

			const auto& parameter = parameters.at(name);
			std::memcpy(m_ParameterBuffer.Data + parameter.Offset, &data, Utils::GetParameterSize(parameter.Type));
		}
	}

	void MaterialInstance::InvalidateTextures()
	{
		m_InvalidationID = UUID();

		const Buffer& parentBuffer = m_Parent->GetTextureBuffer();

		// Ensure instance and parent buffer sizes match
		Utils::ValidateBufferSize(parentBuffer, m_TextureBuffer);

		// Copy the parent buffer
		m_TextureBuffer.Copy(parentBuffer);

		// Apply instance overrides
		const auto& textures = m_Parent->GetTextures();
		size_t offset = 0;
		for (const auto& [id, texture] : textures)
		{
			if (m_TextureOverrides.find(id) != m_TextureOverrides.end())
			{
				uint64_t handle = m_TextureOverrides.at(id)->GetHandle();
				m_TextureBuffer.Write(&handle, sizeof(uint64_t), offset);
			}

			offset += sizeof(uint64_t);
		}
	}

}
