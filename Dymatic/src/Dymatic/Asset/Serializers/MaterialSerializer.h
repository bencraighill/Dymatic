#pragma once

#include "Dymatic/Asset/AssetSerializer.h"
#include "Dymatic/Renderer/MaterialAsset.h"
#include "Dymatic/Editor/Material/MaterialCompiler.h"

#include "Dymatic/Asset/Serializers/Utils/EditorGraphSerializer.h"
#include "Dymatic/Asset/Serializers/Utils/SerializerUtils.h"

#include "Dymatic/Project/Project.h"

#include <fstream>
#include <numeric>

namespace Dymatic {

	namespace Utils {

		static bool IsStringType(const Editor::MaterialPinType type)
		{
			return type == Editor::MaterialPinType::String || type == Editor::MaterialPinType::StringMultiline;
		}

		static void MaterialNodePostBuild(const YAML::Node& nodeNode, Ref<Editor::EditorNode> node)
		{
			// Custom expressions and operators may insert new pins so we need to rebuild
			const Editor::MaterialNodeFunction function = As<Editor::MaterialNode>(node)->Function;
			if (function == Editor::MaterialNodeFunction::CustomExpression || Editor::MaterialGraph::IsOperatorNode(function))
				Editor::EditorGraph::BuildNode(node);
		}

		static const char* MaterialPinTypeToString(const Editor::MaterialPinType type)
		{
			switch (type)
			{
			case Editor::MaterialPinType::Bool: return "Bool";
			case Editor::MaterialPinType::FloatFamily: return "Float Family";
			case Editor::MaterialPinType::Float: return "Float";
			case Editor::MaterialPinType::Float2: return "Vector2";
			case Editor::MaterialPinType::Float3: return "Vector3";
			case Editor::MaterialPinType::Float4: return "Vector4";
			case Editor::MaterialPinType::Texture: return "Texture";
			case Editor::MaterialPinType::String: return "String";
			case Editor::MaterialPinType::StringMultiline: return "String Multiline";
			}

			DY_CORE_ASSERT(false, "Unknown material pin data type");
			return nullptr;
		}

		static Editor::MaterialPinType MaterialPinTypeFromString(const std::string& string)
		{
			if (string == "Bool") return Editor::MaterialPinType::Bool;
			if (string == "Float Family") return Editor::MaterialPinType::FloatFamily;
			if (string == "Float") return Editor::MaterialPinType::Float;
			if (string == "Vector2") return Editor::MaterialPinType::Float2;
			if (string == "Vector3") return Editor::MaterialPinType::Float3;
			if (string == "Vector4") return Editor::MaterialPinType::Float4;
			if (string == "Texture") return Editor::MaterialPinType::Texture;
			if (string == "String") return Editor::MaterialPinType::String;
			if (string == "String Multiline") return Editor::MaterialPinType::StringMultiline;

			DY_CORE_ASSERT(false, "Unknown material pin data type string");
			return Editor::MaterialPinType::None;
		}

		static void SerializeMaterialPin(YAML::Emitter& out, const Ref<Editor::EditorPin> internalPin, const bool includeData)
		{
			const Ref<Editor::MaterialPin> pin = As<Editor::MaterialPin>(internalPin);

			out << YAML::BeginMap;

			out << YAML::Key << "Pin" << YAML::Value << pin->ID;

			if (!pin->Name.empty())
				out << YAML::Key << "Name" << YAML::Value << pin->Name;

			out << YAML::Key << "Type" << YAML::Value << MaterialPinTypeToString(pin->Type);

			if (includeData)
			{
				out << YAML::Key << "Data" << YAML::Value;

				switch (pin->Type)
				{
				case Editor::MaterialPinType::Bool:				out << pin->Data.Bool;		break;
				case Editor::MaterialPinType::FloatFamily:
				case Editor::MaterialPinType::Float:			out << pin->Data.Float;		break;
				case Editor::MaterialPinType::Float2:			out << pin->Data.Float2;	break;
				case Editor::MaterialPinType::Float3:			out << pin->Data.Float3;	break;
				case Editor::MaterialPinType::Float4:			out << pin->Data.Float4;	break;
				case Editor::MaterialPinType::Texture:			out << pin->Data.Handle;	break;
				case Editor::MaterialPinType::String:
				case Editor::MaterialPinType::StringMultiline:	out << pin->Data.String;	break;
				}
			}

			out << YAML::EndMap;
		}

		static Editor::MaterialPinData GetMaterialPinData(const Editor::MaterialPinType type, const YAML::Node& node)
		{
			Editor::MaterialPinData data;

			if (!node)
				return data;

			switch (type)
			{
			case Editor::MaterialPinType::Bool:				data.Bool = node.as<bool>();			break;
			case Editor::MaterialPinType::FloatFamily:
			case Editor::MaterialPinType::Float:			data.Float = node.as<float>();			break;
			case Editor::MaterialPinType::Float2:			data.Float2 = node.as<glm::vec2>();		break;
			case Editor::MaterialPinType::Float3:			data.Float3 = node.as<glm::vec3>();		break;
			case Editor::MaterialPinType::Float4:			data.Float4 = node.as<glm::vec4>();		break;
			case Editor::MaterialPinType::Texture:			data.Handle = node.as<AssetHandle>();	break;
			case Editor::MaterialPinType::String:
			case Editor::MaterialPinType::StringMultiline:	data.String = node.as<std::string>();	break;
			}

			return data;
		}

		static Editor::NodeHandle SpawnMaterialNodeFromName(const std::string& function, Ref<Editor::EditorGraph> editorGraphInternal)
		{

		}

		static void DeserializeMaterialPinData(Ref<Editor::EditorNode> internalNode, const Ref<Editor::EditorGraph> editorGraphInternal, const YAML::Node& pinListNode, std::vector<Ref<Editor::EditorPin>>& pins)
		{
			const Ref<Editor::MaterialNode> node = As<Editor::MaterialNode>(internalNode);
			const Editor::MaterialNodeFunction function = node->Function;
			const Ref<Editor::MaterialGraph> editorGraph = As<Editor::MaterialGraph>(editorGraphInternal);

			// Match up pin data where it still aligns and defaults otherwise (in case the engine has compiler/node changes)
			size_t pinIndex = 0;
			for (const auto pinNode : pinListNode)
			{
				const Editor::PinHandle pinID = pinNode["Pin"].as<Editor::PinHandle>();

				const auto nameNode = pinNode["Name"];
				const std::string name = nameNode ? nameNode.as<std::string>() : std::string();

				// Extract type and data
				const Editor::MaterialPinType type = Utils::MaterialPinTypeFromString(pinNode["Type"].as<std::string>());
				const auto dataNode = pinNode["Data"];

				// Additional custom expressions pins will be added regardless
				if (function == Editor::MaterialNodeFunction::CustomExpression && !Utils::IsStringType(type))
				{
					const Ref<Editor::MaterialPin> pin = editorGraph->AddPin(pins, name, type);
					pin->ID = pinID;
					pin->Data = Utils::GetMaterialPinData(type, dataNode);
					continue;
				}

				if (Editor::MaterialGraph::IsOperatorNode(function) && pinIndex >= 2)
				{
					editorGraph->OnAddPin(node);
					const Ref<Editor::MaterialPin> pin = node->GetInput(pinIndex);
					pin->ID = pinID;
					pin->Data = Utils::GetMaterialPinData(type, dataNode);
					continue;
				}

				// Constants and parameters will just accept the pin type (as this can vary)
				if (function == Editor::MaterialNodeFunction::Constant || function == Editor::MaterialNodeFunction::Parameter)
				{
					const Ref<Editor::MaterialPin> pin = As<Editor::MaterialPin>(pins[pinIndex]);
					pin->ID = pinID;
					pin->Type = type;
					pin->Data = Utils::GetMaterialPinData(type, dataNode);
					continue;
				}

				if (name.empty())
				{
					// If no name is provided use index to match
					const Ref<Editor::MaterialPin> pin = As<Editor::MaterialPin>(pins[pinIndex]);
					if (pin->Name.empty() && pin->Type == type)
					{
						pin->ID = pinID;
						pin->Data = Utils::GetMaterialPinData(type, dataNode);
					}
				}
				else
				{
					// If a name is provided match using the name
					for (const auto& internalPin : pins)
					{
						const Ref<Editor::MaterialPin> pin = As<Editor::MaterialPin>(internalPin);
						if (pin->Name == name && pin->Type == type)
						{
							pin->ID = pinID;
							pin->Data = Utils::GetMaterialPinData(type, dataNode);
							break;
						}
					}
				}

				pinIndex++;
			}
		}

		static const char* TessellationSpacingToString(const Editor::MaterialConfig::TessellationSpacingMode tessellationSpacing)
		{
			switch (tessellationSpacing)
			{
			case Editor::MaterialConfig::TessellationSpacingMode::Equal:			return "Equal";
			case Editor::MaterialConfig::TessellationSpacingMode::EvenFractional:	return "Even Fractional";
			case Editor::MaterialConfig::TessellationSpacingMode::OddFractional:	return "Odd Fractional";
			}

			DY_CORE_ASSERT(false, "Unknown tessellation spacing mode!");
			return "Unknown";
		}

		static Editor::MaterialConfig::TessellationSpacingMode TessellationSpacingFromString(const std::string& tessellationSpacingString)
		{
			if (tessellationSpacingString == "Equal")			return Editor::MaterialConfig::TessellationSpacingMode::Equal;
			if (tessellationSpacingString == "Even Fractional")	return Editor::MaterialConfig::TessellationSpacingMode::EvenFractional;
			if (tessellationSpacingString == "Odd Fractional")	return Editor::MaterialConfig::TessellationSpacingMode::OddFractional;

			DY_CORE_ASSERT(false, "Unknown tessellation spacing mode!");
			return Editor::MaterialConfig::TessellationSpacingMode::Equal;
		}

	}

	class MaterialSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override
		{			
			YAML::Emitter out;
			out << YAML::BeginMap << YAML::Key << "Material" << YAML::Value << YAML::BeginMap;
			
			Ref<MaterialAsset> material = std::dynamic_pointer_cast<MaterialAsset>(asset);

			if (material->IsInstance())
			{
				// Material Instance
				const Ref<MaterialInstance> instance = As<MaterialInstance>(material);
				Ref<MaterialAsset> parent = instance->GetParent();
				out << YAML::Key << "Parent" << YAML::Value << (parent ? parent->Handle : 0);

				if (parent)
				{
					out << YAML::Key << "ParameterOverrides" << YAML::Value << YAML::BeginSeq; // ParameterOverrides
					const auto& parameters = instance->GetParameters();
					const auto& parameterOverrides = instance->GetParameterOverrides();

					for (const auto& [name, parameterOverride] : parameterOverrides)
					{
						if (parameters.find(name) == parameters.end())
							continue;

						out << YAML::BeginMap;

						out << YAML::Key << "Name" << YAML::Value << name;
						out << YAML::Key << "Type" << YAML::Value << MaterialAsset::MaterialParameterData::MaterialParameterTypeToString(parameters.at(name).Type);
						out << YAML::Key << "Value" << YAML::Value;

						switch (parameters.at(name).Type)
						{
						case MaterialAsset::MaterialParameterType::Bool:    out << (bool)parameterOverride.Bool;	break;
						case MaterialAsset::MaterialParameterType::Float:   out << parameterOverride.Float;			break;
						case MaterialAsset::MaterialParameterType::Vector2: out << parameterOverride.Vector2;		break;
						case MaterialAsset::MaterialParameterType::Vector3: out << parameterOverride.Vector3;		break;
						case MaterialAsset::MaterialParameterType::Vector4: out << parameterOverride.Vector4;		break;
						}

						out << YAML::EndMap;
					}

					out << YAML::EndSeq; // ParameterOverrides

					out << YAML::Key << "TextureOverrides" << YAML::Value << YAML::BeginSeq; // TextureOverrides
					const auto& textures = instance->GetTextures();
					const auto& textureOverrides = instance->GetTextureOverrides();

					for (const auto& [id, texture] : textures)
					{
						if (textureOverrides.find(id) == textureOverrides.end())
							continue;

						const Ref<Texture2D> textureOverride = textureOverrides.at(id);
						if (!textureOverride)
							continue;

						out << YAML::BeginMap;
						out << YAML::Key << "ID" << YAML::Value << id;
						out << YAML::Key << "Texture" << textureOverride->Handle;
						out << YAML::EndMap;
					}

					out << YAML::EndSeq; // TextureOverrides
				}
			}
			else
			{
				// Material Source
				const MaterialAsset::MaterialProperties& properties = material->GetProperties();
				out << YAML::Key << "Usage" << YAML::Value << MaterialAsset::MaterialUsageToString(properties.Usage);
				out << YAML::Key << "AlphaBlendMode" << YAML::Value << MaterialAsset::AlphaBlendModeToString(properties.AlphaBlendMode);
				out << YAML::Key << "CastShadows" << YAML::Value << properties.CastShadows;
				out << YAML::Key << "TwoSided" << YAML::Value << properties.TwoSided;
				out << YAML::Key << "Lit" << YAML::Value << properties.Lit;
				out << YAML::Key << "Tessellation" << YAML::Value << properties.Tessellation;

				if (const Ref<Editor::MaterialGraph> editorGraph = As<MaterialSource>(material)->m_EditorGraph)
				{
					const auto& config = editorGraph->GetConfig();
					out << YAML::Key << "Tessellation Spacing" << YAML::Value << Utils::TessellationSpacingToString(config.TessellationSpacing);

					Utils::SerializeEditorGraph(out, editorGraph, &Utils::SerializeMaterialPin, nullptr);
				}
			}
			
			out << YAML::EndMap; // Material
			out << YAML::EndMap;

			std::ofstream fout(Project::GetAssetFileSystemPath(metadata.FilePath));
			fout << out.c_str();

			return;
		}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset, bool multithreaded) const override
		{
			YAML::Node data;
			if (!Utils::TryLoadYAMLFromFile(metadata, data))
				return false;

			auto materialNode = data["Material"];
			if (!materialNode)
				return false;

			if (auto parentNode = materialNode["Parent"])
			{
				// This material has a source, therefore it is a material instance

				// Retrieve the parent material asset
				UUID handle = parentNode.as<UUID>();
				Ref<MaterialAsset> parent = handle ? AssetManager::GetAsset<MaterialAsset>(handle) : nullptr;

				if (!parent)
					return false;

				// Retrieve the parameter overrides
				std::unordered_map<std::string, MaterialAsset::MaterialParameterData::Data> parameterOverrides;
				auto parameterOverridesNode = materialNode["ParameterOverrides"];
				if (parent && parameterOverridesNode)
				{
					parameterOverrides.reserve(parameterOverridesNode.size());
					auto& parameters = parent->GetParameters();
					for (auto& parameterOverrideNode : parameterOverridesNode)
					{
						const std::string name = parameterOverrideNode["Name"].as<std::string>();

						if (parameters.find(name) == parameters.end())
						{
							DY_CORE_WARN("Material instance '{}' overrides parameter '{}' which does not exist in parent material '{}'", metadata.FilePath.string(), name, AssetManager::GetMetadata(parent->Handle).FilePath.string());
							continue;
						}

						const auto& parameter = parameters.at(name);
						const MaterialAsset::MaterialParameterType type = MaterialAsset::MaterialParameterData::MaterialParameterTypeFromString(parameterOverrideNode["Type"].as<std::string>());

						if (type != parameter.Type)
						{
							DY_CORE_WARN("Material instance '{}' has mismatched type {} overriding parameter '{}' of type {}", metadata.FilePath.string(), MaterialAsset::MaterialParameterData::MaterialParameterTypeToString(type), name, MaterialAsset::MaterialParameterData::MaterialParameterTypeToString(parameter.Type));
							continue;
						}

						MaterialAsset::MaterialParameterData::Data parameterOverride;
						auto parameterOverrideValueNode = parameterOverrideNode["Value"];
						switch (type)
						{
						case MaterialAsset::MaterialParameterType::Bool:    parameterOverride.Bool  =	parameterOverrideValueNode.as<bool>();     break;
						case MaterialAsset::MaterialParameterType::Float:   parameterOverride.Float =	parameterOverrideValueNode.as<float>();     break;
						case MaterialAsset::MaterialParameterType::Vector2: parameterOverride.Vector2 = parameterOverrideValueNode.as<glm::vec2>(); break;
						case MaterialAsset::MaterialParameterType::Vector3: parameterOverride.Vector3 = parameterOverrideValueNode.as<glm::vec3>(); break;
						case MaterialAsset::MaterialParameterType::Vector4: parameterOverride.Vector4 = parameterOverrideValueNode.as<glm::vec4>(); break;
						}
						parameterOverrides[name] = parameterOverride;
					}
				}

				// Retrieve Texture Overrides
				std::unordered_map<uint64_t, Ref<Texture2D>> textureOverrides;
				auto textureOverridesNode = materialNode["TextureOverrides"];
				if (parent && textureOverridesNode)
				{
					textureOverrides.reserve(textureOverridesNode.size());
					auto& textures = parent->GetTextures();
					for (auto& textureOverrideNode : textureOverridesNode)
					{
						const uint64_t id = textureOverrideNode["ID"].as<uint64_t>();

						if (!std::any_of(textures.begin(), textures.end(), [id](const MaterialAsset::MaterialTexture& texture) { return texture.CompilerNodeID == id; }))
						{
							DY_CORE_WARN("Material instance '{}' overrides texture '{}' which does not exist in parent material '{}'", metadata.FilePath.string(), id, AssetManager::GetMetadata(parent->Handle).FilePath.string());
							continue;
						}

						const AssetHandle textureHandle = textureOverrideNode["Texture"].as<AssetHandle>();
						textureOverrides[id] = AssetManager::GetAsset<Texture2D>(textureHandle);
					}
				}

				Ref<MaterialInstance> instance = MaterialInstance::Create(parent, parameterOverrides, textureOverrides);
				asset = instance;
			}
			else
			{
				// No material source target, hence this is itself a source material

				Ref<Editor::MaterialGraph> editorGraph = CreateRef<Editor::MaterialGraph>();

				// Setup the editor graph
				if (auto graphNode = materialNode["Graph"])
				{
					Utils::DeserializeEditorGraph(graphNode, editorGraph, &Utils::DeserializeMaterialPinData, &Utils::MaterialNodePostBuild);
				}
				else
				{
					editorGraph->SpawnResultNode();
				}

				// Load editor graph configuration
				auto& config = editorGraph->GetConfig();
				if (auto tessellationSpacingNode = materialNode["Tessellation Spacing"])
					config.TessellationSpacing = Utils::TessellationSpacingFromString(tessellationSpacingNode.as<std::string>());

				// Compile the editor graph
				Editor::MaterialCompiler compiler(editorGraph);
				compiler.TargetHandle = metadata.Handle;

				auto& properties = compiler.TargetProperties;

				if (auto usageNode = materialNode["Usage"])
					properties.Usage = MaterialAsset::GetMaterialUsage(usageNode.as<std::string>());

				if (auto alphaBlendModeNode = materialNode["AlphaBlendMode"])
					properties.AlphaBlendMode = MaterialAsset::GetMaterialAlphaBlendMode(alphaBlendModeNode.as<std::string>());

				if (auto castsShadowsNode = materialNode["CastShadows"])
					properties.CastShadows = castsShadowsNode.as<bool>();

				if (auto twoSidedNode = materialNode["TwoSided"])
					properties.TwoSided = twoSidedNode.as<bool>();

				if (auto litNode = materialNode["Lit"])
					properties.Lit = litNode.as<bool>();

				if (auto tessellationNode = materialNode["Tessellation"])
					properties.Tessellation = tessellationNode.as<bool>();

				compiler.Compile();
				Ref<MaterialSource> material = compiler.GetMaterial();

				// If material compilation failed create a shaderless material (with the same properties)
				if (!material)
				{
					material = MaterialSource::Create(editorGraph);
					material->SetProperties(compiler.TargetProperties);
				}

				asset = material;
			}

			return true;
		}

		virtual bool SerializeToAssetPack(const AssetMetadata& metadata, const AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const override
		{
			Ref<MaterialAsset> material = AssetManager::GetAsset<MaterialAsset>(handle);

			const bool isInstance = material->IsInstance();
			stream.WriteRaw(isInstance);

			if (isInstance)
			{
				const Ref<MaterialInstance> instance = As<MaterialInstance>(material);
				
				stream.WriteRaw(instance->GetParent()->Handle);

				const auto& parameters = instance->GetParameters();
				const auto& textures = instance->GetTextures();

				// Remove any overridden elements that do not exist on the parent material
				std::unordered_map<std::string, MaterialAsset::MaterialParameterData::Data> parameterOverrides = instance->GetParameterOverrides();
				for (auto it = parameterOverrides.begin(); it != parameterOverrides.end();)
				{
					if (parameters.find(it->first) == parameters.end())
						it = parameterOverrides.erase(it);
					else
						it++;
				}

				// Write valid parameter overrides
				stream.WriteRaw<uint32_t>(parameterOverrides.size());
				for (const auto& [name, data] : parameterOverrides)
				{
					stream.WriteString(name);

					const MaterialAsset::MaterialParameterType type = parameters.at(name).Type;
					stream.WriteRaw<uint8_t>((uint8_t)type);

					switch (type)
					{
					case MaterialAsset::MaterialParameterType::Bool:	stream.WriteRaw<bool>(data.Bool); break;
					case MaterialAsset::MaterialParameterType::Float:	stream.WriteRaw<float>(data.Float); break;
					case MaterialAsset::MaterialParameterType::Vector2: stream.WriteRaw<glm::vec2>(data.Vector2); break;
					case MaterialAsset::MaterialParameterType::Vector3: stream.WriteRaw<glm::vec3>(data.Vector3); break;
					case MaterialAsset::MaterialParameterType::Vector4: stream.WriteRaw<glm::vec4>(data.Vector4); break;
					case MaterialAsset::MaterialParameterType::Texture: stream.WriteRaw<AssetHandle>(data.Texture); break;
					}
				}

				// Calculate the total number of texture overrides (excluding ones that do not have a corresponding parent parameter)
				const auto& textureOverrides = instance->GetTextureOverrides();
				uint32_t textureOverrideCount = 0;
				for (const auto& [id, texture] : textures)
					if (textureOverrides.find(id) != textureOverrides.end())
						textureOverrideCount++;

				// Write texture override data to the pack stream
				stream.WriteRaw<uint32_t>(textureOverrideCount);
				for (const auto& [id, texture] : textures)
				{
					if (textureOverrides.find(id) == textureOverrides.end())
						continue;

					stream.WriteRaw<uint64_t>(id);
					stream.WriteRaw<AssetHandle>(texture ? texture->Handle : 0);
				}
			}
			else
			{
				const Ref<MaterialSource> source = As<MaterialSource>(material);

				// Write Parameters
				const auto& parameters = source->GetParameters();
				stream.WriteRaw<uint32_t>(parameters.size());
				for (const auto& [name, parameter] : parameters)
				{
					stream.WriteString(name);
					stream.WriteRaw<uint8_t>((uint8_t)parameter.Type);

					switch (parameter.Type)
					{
					case MaterialAsset::MaterialParameterType::Bool:	stream.WriteRaw<bool>(parameter.Value.Bool); break;
					case MaterialAsset::MaterialParameterType::Float:	stream.WriteRaw<float>(parameter.Value.Float); break;
					case MaterialAsset::MaterialParameterType::Vector2: stream.WriteRaw<glm::vec2>(parameter.Value.Vector2); break;
					case MaterialAsset::MaterialParameterType::Vector3: stream.WriteRaw<glm::vec3>(parameter.Value.Vector3); break;
					case MaterialAsset::MaterialParameterType::Vector4: stream.WriteRaw<glm::vec4>(parameter.Value.Vector4); break;
					case MaterialAsset::MaterialParameterType::Texture: stream.WriteRaw<AssetHandle>(parameter.Value.Texture); break;
					}
				}

				// Write textures
				const auto& textures = source->GetTextures();
				stream.WriteRaw<uint32_t>(textures.size());
				for (const auto& materialTexture : textures)
				{
					stream.WriteRaw<uint64_t>(materialTexture.CompilerNodeID);
					stream.WriteRaw<AssetHandle>(materialTexture.Texture->Handle);
				}

				// Write shader data
				const auto& shaders = source->GetShaders();

				uint32_t shaderCount = 0;
				for (const auto& [stage, shader] : shaders)
				{
					if (shader)
						shaderCount++;
				}
				stream.WriteRaw<uint32_t>(shaderCount);

				for (const auto& [stage, shader] : shaders)
				{
					if (!shader)
						continue;

					stream.WriteRaw<uint8_t>((uint8_t)stage);

					// Write the shader stage binaries
					const auto& packages = shader->GetPackagedShaderBuffers();
					stream.WriteRaw<uint32_t>(packages.size());
					for (auto& [shaderStage, binary] : packages)
					{
						stream.WriteRaw<uint32_t>(shaderStage);

						// Write the binary buffer
						stream.WriteRaw<size_t>(binary->Size);
						stream.WriteData((const char*)binary->Data, binary->Size);
					}
				}

				// Write other basic material properties
				const MaterialAsset::MaterialProperties& properties = source->GetProperties();
				stream.WriteRaw<uint8_t>(properties.Usage);

				if (properties.Usage == MaterialAsset::MaterialUsage::Surface || properties.Usage == MaterialAsset::MaterialUsage::Particle)
				{
					stream.WriteRaw<uint8_t>(properties.AlphaBlendMode);
					stream.WriteRaw<bool>(properties.CastShadows);
				}

				if (properties.Usage == MaterialAsset::MaterialUsage::Surface)
					stream.WriteRaw<bool>(properties.TwoSided);

				if (properties.Usage == MaterialAsset::MaterialUsage::Particle)
					stream.WriteRaw<bool>(properties.Lit);

				// Write path tracing source
				stream.WriteString(source->GetPathTraceSource());
			}

			return true;
		}

		virtual bool DeserializeFromAssetPack(const AssetMetadata& metadata, Ref<Asset>& asset, FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const override
		{
			const bool isInstance = stream.ReadRaw<bool>();

			if (isInstance)
			{
				const AssetHandle parentHandle = stream.ReadRaw<AssetHandle>();
				const Ref<MaterialAsset> parent = AssetManager::GetAsset<MaterialAsset>(parentHandle);

				if (!parent)
					return false;

				const uint32_t parameterOverrideCount = stream.ReadRaw<uint32_t>();
				std::unordered_map<std::string, MaterialAsset::MaterialParameterData::Data> parameterOverrides(parameterOverrideCount);
				for (uint32_t parameterOverrideIndex = 0; parameterOverrideIndex < parameterOverrideCount; parameterOverrideIndex++)
				{
					std::string name;
					stream.ReadString(name);

					auto& parameterOverride = parameterOverrides[name];
					const MaterialAsset::MaterialParameterType type = (MaterialAsset::MaterialParameterType)stream.ReadRaw<uint8_t>();

					switch (type)
					{
					case MaterialAsset::MaterialParameterType::Bool:	parameterOverride.Bool = stream.ReadRaw<bool>(); break;
					case MaterialAsset::MaterialParameterType::Float:	stream.ReadRaw<float>(parameterOverride.Float); break;
					case MaterialAsset::MaterialParameterType::Vector2:	stream.ReadRaw<glm::vec2>(parameterOverride.Vector2); break;
					case MaterialAsset::MaterialParameterType::Vector3:	stream.ReadRaw<glm::vec3>(parameterOverride.Vector3); break;
					case MaterialAsset::MaterialParameterType::Vector4:	stream.ReadRaw<glm::vec4>(parameterOverride.Vector4); break;
					case MaterialAsset::MaterialParameterType::Texture:	stream.ReadRaw<AssetHandle>(parameterOverride.Texture); break;
					}
				}

				const uint32_t textureOverrideCount = stream.ReadRaw<uint32_t>();
				std::unordered_map<uint64_t, Ref<Texture2D>> textureOverrides(textureOverrideCount);
				for (uint32_t textureOverrideIndex = 0; textureOverrideIndex < textureOverrideCount; textureOverrideIndex++)
				{
					const uint64_t id = stream.ReadRaw<uint64_t>();
					auto& textureOverride = textureOverrides[id];
					textureOverride = AssetManager::GetAsset<Texture2D>(stream.ReadRaw<AssetHandle>());
				}

				Ref<MaterialInstance> instance = MaterialInstance::Create(parent, parameterOverrides, textureOverrides);
				asset = instance;
			}
			else
			{
				// Parameters
				const uint32_t parameterCount = stream.ReadRaw<uint32_t>();
				std::unordered_map<std::string, MaterialAsset::MaterialParameterData> parameters(parameterCount);
				for (uint32_t parameterIndex = 0; parameterIndex < parameterCount; parameterIndex++)
				{
					std::string name;
					stream.ReadString(name);

					auto& parameter = parameters[name];
					parameter.Name = name;
					parameter.Type = (MaterialAsset::MaterialParameterType)stream.ReadRaw<uint8_t>();

					switch (parameter.Type)
					{
					case MaterialAsset::MaterialParameterType::Bool:	parameter.Value.Bool = stream.ReadRaw<bool>(); break;
					case MaterialAsset::MaterialParameterType::Float:	stream.ReadRaw<float>(parameter.Value.Float); break;
					case MaterialAsset::MaterialParameterType::Vector2:	stream.ReadRaw<glm::vec2>(parameter.Value.Vector2); break;
					case MaterialAsset::MaterialParameterType::Vector3:	stream.ReadRaw<glm::vec3>(parameter.Value.Vector3); break;
					case MaterialAsset::MaterialParameterType::Vector4:	stream.ReadRaw<glm::vec4>(parameter.Value.Vector4); break;
					case MaterialAsset::MaterialParameterType::Texture:	stream.ReadRaw<AssetHandle>(parameter.Value.Texture); break;
					}
				}

				// Textures
				const uint32_t textureCount = stream.ReadRaw<uint32_t>();
				std::vector<MaterialAsset::MaterialTexture> textures;
				textures.reserve(textureCount);
				for (uint32_t textureIndex = 0; textureIndex < textureCount; textureIndex++)
				{
					const uint64_t id = stream.ReadRaw<uint64_t>();
					const Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(stream.ReadRaw<AssetHandle>());
					textures.emplace_back(id, texture);
				}

				// Shader Data
				const uint32_t shaderCount = stream.ReadRaw<uint32_t>();
				std::unordered_map<MaterialAsset::MaterialRenderStage, Ref<Shader>> shaders(shaderCount);
				for (uint32_t shaderIndex = 0; shaderIndex < shaderCount; shaderIndex++)
				{
					const MaterialAsset::MaterialRenderStage stage = (MaterialAsset::MaterialRenderStage)stream.ReadRaw<uint8_t>();

					// Read shader stage binaries
					const uint32_t binaryCount = stream.ReadRaw<uint32_t>();
					std::unordered_map<uint32_t, Ref<ScopedBuffer>> binaries(binaryCount);

					for (uint32_t binaryIndex = 0; binaryIndex < binaryCount; binaryIndex++)
					{
						const uint32_t shaderStage = stream.ReadRaw<uint32_t>();
						const size_t binarySize = stream.ReadRaw<uint64_t>();

						Ref<ScopedBuffer> binary = ScopedBuffer::Create(binarySize);
						stream.ReadData((char*)binary->Data, binary->Size);

						binaries[shaderStage] = binary;
					}

					shaders[stage] = Shader::Create(fmt::format("RuntimeUserShader_{}_{}", metadata.Handle, MaterialAsset::MaterialRenderStageToString(stage)), binaries);
				}

				// Create the material
				Ref<MaterialSource> source = MaterialSource::Create(shaders, textures, parameters);

				// Basic Material Properties
				const MaterialAsset::MaterialUsage usage = (MaterialAsset::MaterialUsage)stream.ReadRaw<uint8_t>();
				source->SetUsage(usage);

				if (usage == MaterialAsset::MaterialUsage::Surface || usage == MaterialAsset::MaterialUsage::Particle)
				{
					source->SetAlphaBlendMode((MaterialAsset::AlphaBlendMode)stream.ReadRaw<uint8_t>());
					source->SetCastShadows(stream.ReadRaw<bool>());
				}

				if (usage == MaterialAsset::MaterialUsage::Surface)
					source->SetTwoSided(stream.ReadRaw<bool>());

				if (usage == MaterialAsset::MaterialUsage::Particle)
					source->SetLit(stream.ReadRaw<bool>());

				// Path Tracing Source
				std::string pathTraceSource;
				stream.ReadString(pathTraceSource);
				source->SetPathTraceSource(pathTraceSource);

				asset = source;
			}

			return true;
		}
	};

}