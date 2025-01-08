#include "dypch.h"
#include "Dymatic/Editor/Material/MaterialCompiler.h"

#include "Dymatic/Asset/AssetManager.h"
#include "Dymatic/Math/StringUtils.h"

#include <stack>

namespace Dymatic::Editor {

	namespace Utils {
		
		std::string ReadFile(const std::filesystem::path& filepath)
		{
			std::ifstream stream(filepath);
			std::stringstream stringStream;
			stringStream << stream.rdbuf();
			return stringStream.str();
		}

		const char* GetTypeString(const MaterialPinType type)
		{
			switch (type)
			{
			case MaterialPinType::Bool: return "bool";
			case MaterialPinType::Float: return "float";
			case MaterialPinType::Float2: return "vec2";
			case MaterialPinType::Float3: return "vec3";
			case MaterialPinType::Float4: return "vec4";
			case MaterialPinType::FloatFamily: return "float";
			case MaterialPinType::Texture: return "uvec2";
			}

			DY_CORE_ASSERT(false, "Unknown type string for specified material pin type");
			return "Unknown";
		}

		std::string GetFloatString(const float value)
		{
			return glm::fract(value) == 0.0f ? fmt::format("{}.0", value) : fmt::format("{}", value);
		}

		std::string GetDataString(const MaterialPinType type, const MaterialPinData& data)
		{
			switch (type)
			{
			case MaterialPinType::Bool: return data.Bool ? "true" : "false";
			case MaterialPinType::FloatFamily:
			case MaterialPinType::Float: return fmt::format("{}", GetFloatString(data.Float));
			case MaterialPinType::Float2: return fmt::format("vec2({}, {})", GetFloatString(data.Float2.x), GetFloatString(data.Float2.y));
			case MaterialPinType::Float3: return fmt::format("vec3({}, {}, {})", GetFloatString(data.Float3.x), GetFloatString(data.Float2.y), GetFloatString(data.Float3.z));
			case MaterialPinType::Float4: return fmt::format("vec4({}, {}, {}, {})", GetFloatString(data.Float4.x), GetFloatString(data.Float4.y), GetFloatString(data.Float4.z), GetFloatString(data.Float4.w));
			}

			DY_CORE_ASSERT(false, "Unknown material pin type for data generation");
			return std::string();
		}

		static void CorrectArgumentType(MaterialPinType target, MaterialPinType source, std::string& argument)
		{
			if (target == source)
				return;

			switch (source)
			{
			case MaterialPinType::Float: argument = fmt::format("{}({})", GetTypeString(target), argument); break;
			case MaterialPinType::Float2:
				switch (target)
				{
				case MaterialPinType::Float: argument = fmt::format("{}.x", argument); break;
				case MaterialPinType::Float3: argument = fmt::format("vec3({}, 0.0)", argument); break;
				case MaterialPinType::Float4: argument = fmt::format("vec4({}, 0.0, 1.0)", argument); break;
				}
				break;
			case MaterialPinType::Float3:
				switch (target)
				{
				case MaterialPinType::Float: argument = fmt::format("{}.x", argument); break;
				case MaterialPinType::Float2: argument = fmt::format("{}.xy", argument); break;
				case MaterialPinType::Float4: argument = fmt::format("vec4({}, 1.0)", argument); break;
				}
				break;
			case MaterialPinType::Float4:
				switch (target)
				{
				case MaterialPinType::Float: argument = fmt::format("{}.x", argument); break;
				case MaterialPinType::Float2: argument = fmt::format("{}.xy", argument); break;
				case MaterialPinType::Float3: argument = fmt::format("{}.xyz", argument); break;
				}
				break;
			}
		}

		static std::string GetCompilerSafeName(std::string name)
		{
			for (auto& character : name)
				if (!isalnum(character) && character != '_')
					character = '_';

			return name;
		}

		bool FindNearestSubstring(const std::string& source, const std::string& str1, const std::string& str2, std::string& nearestSubstring, size_t& position)
		{
			size_t pos1 = source.find(str1, position);
			size_t pos2 = source.find(str2, position);

			if (pos1 == std::string::npos && pos2 == std::string::npos)
				return false;

			if (pos1 == std::string::npos || (pos2 != std::string::npos && pos2 < pos1))
			{
				nearestSubstring = str2;
				position = pos2;
			}
			else
			{
				nearestSubstring = str1;
				position = pos1;
			}

			return true;
		}

		static std::string GenerateNodeSignature(const Ref<MaterialNode> node)
		{
			return fmt::format("mnf_{}_{}_pf", GetCompilerSafeName(MaterialGraph::GetNodeFunctionName(node->Function)), node->ID);
		}

		static bool IsComponentFunction(const MaterialNodeFunction function)
		{
			return function == MaterialNodeFunction::ComponentAppend || function == MaterialNodeFunction::ComponentMask;
		}

		static const char* GetOperatorString(const MaterialNodeFunction function)
		{
			switch (function)
			{
			case MaterialNodeFunction::Add: return "+";
			case MaterialNodeFunction::Subtract: return "-";
			case MaterialNodeFunction::Multiply: return "*";
			case MaterialNodeFunction::Divide: return "/";
			}

			return nullptr;
		}

		static bool IsOperatorFunction(const MaterialNodeFunction function)
		{
			return GetOperatorString(function) != nullptr;
		}

		static const char* GetInternalFunctionName(const MaterialNodeFunction function)
		{
			switch (function)
			{
				case MaterialNodeFunction::Abs: return "abs";
				case MaterialNodeFunction::Length: return "length";
				case MaterialNodeFunction::Distance: return "distance";
				case MaterialNodeFunction::Radians: return "radians";
				case MaterialNodeFunction::Degrees: return "degrees";
				case MaterialNodeFunction::Sine: return "sin";
				case MaterialNodeFunction::Cosine: return "cos";
				case MaterialNodeFunction::Tangent: return "tan";
				case MaterialNodeFunction::ArcSine: return "asin";
				case MaterialNodeFunction::ArcCosine: return "acos";
				case MaterialNodeFunction::ArcTangent: return "atan";
				case MaterialNodeFunction::HyperbolicSine: return "sinh";
				case MaterialNodeFunction::HyperbolicCosine: return "cosh";
				case MaterialNodeFunction::HyperbolicTangent: return "tanh";
				case MaterialNodeFunction::ArcHyperbolicSine: return "asinh";
				case MaterialNodeFunction::ArcHyperbolicCosine: return "acosh";
				case MaterialNodeFunction::ArcHyperbolicTangent: return "atanh";
				case MaterialNodeFunction::Ceil: return "ceil";
				case MaterialNodeFunction::Floor: return "floor";
				case MaterialNodeFunction::Clamp: return "clamp";
				case MaterialNodeFunction::Truncate: return "trunc";
				case MaterialNodeFunction::SquareRoot: return "sqrt";
				case MaterialNodeFunction::InverseSquareRoot: return "inversesqrt";
				case MaterialNodeFunction::CrossProduct: return "cross";
				case MaterialNodeFunction::DotProduct: return "dot";
				case MaterialNodeFunction::Reflect: return "reflect";
				case MaterialNodeFunction::Refract: return "refract";
				case MaterialNodeFunction::Min: return "min";
				case MaterialNodeFunction::Max: return "max";
				case MaterialNodeFunction::Normalize: return "normalize";
				case MaterialNodeFunction::FMod: return "mod";
				case MaterialNodeFunction::Fract: return "fract";
				case MaterialNodeFunction::Step: return "step";
				case MaterialNodeFunction::SmoothStep: return "smoothstep";
				case MaterialNodeFunction::Round: return "round";
				case MaterialNodeFunction::RoundEven: return "roundEven";
				case MaterialNodeFunction::Power: return "pow";
				case MaterialNodeFunction::Exponential: return "exp";
				case MaterialNodeFunction::Exponential2: return "exp2";
				case MaterialNodeFunction::Log: return "log";
				case MaterialNodeFunction::Log2: return "log2";
				case MaterialNodeFunction::Sign: return "sign";
				case MaterialNodeFunction::OneMinus: return "oneminus";
				case MaterialNodeFunction::Negate: return "negate";
				case MaterialNodeFunction::Saturate: return "saturate";
				case MaterialNodeFunction::Desaturate: return "desaturate";
				case MaterialNodeFunction::Mix: return "mix";
				case MaterialNodeFunction::If: return "blendif";
				case MaterialNodeFunction::Switch: return "boolswitch";
				case MaterialNodeFunction::ConstructVector2: return "vec2";
				case MaterialNodeFunction::ConstructVector3: return "vec3";
				case MaterialNodeFunction::ConstructVector4: return "vec4";
			}

			return nullptr;
		}

		static const char* GetInternalConstantName(const MaterialNodeFunction function)
		{
			switch (function)
			{
			case MaterialNodeFunction::Pi: return "PI";
			case MaterialNodeFunction::Infinity: return "FLT_MAX";
			case MaterialNodeFunction::Euler: return "EULER";
			case MaterialNodeFunction::Tau: return "TAU";
			}

			return nullptr;
		}

		static bool IsDirectScenePinRead(const MaterialNodeFunction function)
		{
			switch (function)
			{
			case MaterialNodeFunction::SceneColor:
			case MaterialNodeFunction::SceneAlbedo:
			case MaterialNodeFunction::ScenePosition:
			case MaterialNodeFunction::SceneDepth:
			case MaterialNodeFunction::SceneLinearDepth:
			case MaterialNodeFunction::SceneNormal:
			case MaterialNodeFunction::SceneEmissive:
			case MaterialNodeFunction::SceneRoughness:
			case MaterialNodeFunction::SceneMetallic:
			case MaterialNodeFunction::SceneSpecular:
			case MaterialNodeFunction::SceneAmbientOcclusion:
			case MaterialNodeFunction::SceneEntityID:
			case MaterialNodeFunction::SceneSubmeshIndex:
				return true;
			}

			return false;
		}

		static bool IsDirectPinRead(const MaterialNodeFunction function)
		{
			// These pins do not need a separate variable assigned to them.
			// Their data can be directly accessed/computed in place as an argument for subsequent calls.
			switch (function)
			{
			case MaterialNodeFunction::WorldPosition:
			case MaterialNodeFunction::WorldNormal:
			case MaterialNodeFunction::VertexColor:
			case MaterialNodeFunction::VertexDepth:
			case MaterialNodeFunction::VertexLinearDepth:
			case MaterialNodeFunction::TextureCoordinates:
			case MaterialNodeFunction::ObjectPosition:
			case MaterialNodeFunction::EntityID:
			case MaterialNodeFunction::SubmeshIndex:
			case MaterialNodeFunction::VertexIndex:
			case MaterialNodeFunction::CameraPosition:
			case MaterialNodeFunction::CameraDirection:
			case MaterialNodeFunction::CameraForward:
			case MaterialNodeFunction::CameraRight:
			case MaterialNodeFunction::CameraUp:
			case MaterialNodeFunction::CameraNear:
			case MaterialNodeFunction::CameraFar:
			case MaterialNodeFunction::ViewSize:
			case MaterialNodeFunction::Time:
			case MaterialNodeFunction::ParticleLifetime:
			case MaterialNodeFunction::ParticleLifeRemaining:
			case MaterialNodeFunction::ParticleLifeWeight:
			case MaterialNodeFunction::ParticleVelocity:
			case MaterialNodeFunction::PixelPosition:
				return true;
			}

			return IsDirectScenePinRead(function);
		}

		static bool IsInternalFunction(const MaterialNodeFunction function)
		{
			return GetInternalFunctionName(function) != nullptr;
		}

		static bool IsSurfaceOrParticleUsage(MaterialAsset::MaterialUsage usage)
		{
			return usage == MaterialAsset::MaterialUsage::Surface || usage == MaterialAsset::MaterialUsage::Particle;
		}

		static bool IsSplitComponentFunction(const MaterialNodeFunction function)
		{
			return function == MaterialNodeFunction::SplitVector2 || function == MaterialNodeFunction::SplitVector3 || function == MaterialNodeFunction::SplitVector4;
		}

	}

	MaterialCompiler::MaterialCompiler(Ref<MaterialGraph> editorGraph)
		: m_EditorGraph(editorGraph)
	{}

	void MaterialCompiler::Compile()
	{
		// Reset
		m_CompilerResult = CompilerResult();

		CompileGraph();

		const std::string materialName = AssetManager::GetMetadata(TargetHandle).FilePath.string();

		if (m_CompilerResult.ErrorCount == 0)
			m_CompilerResult.Add(fmt::format("Compile of '{}' completed [Vulkan GLSL] - Dymatic Material Nodes Version " DY_VERSION_STRING, materialName));
		else
			m_CompilerResult.Add(fmt::format("Compile of '{}' failed [Vulkan GLSL] - {} Error(s) {} Warnings(s) - Dymatic Material Nodes Version " DY_VERSION_STRING, materialName, m_CompilerResult.ErrorCount, m_CompilerResult.WarningCount));
	}

	void MaterialCompiler::CompileGraph()
	{
		Ref<MaterialGraph> editorGraph = m_EditorGraph;
		m_Material = nullptr;

		m_CompilerResult.Add("Build Started [Vulkan GLSL] - Dymatic Material Nodes Version " DY_VERSION_STRING);

		// Verify root node integrity
		Ref<MaterialNode> resultNode = nullptr;
		for (const auto& [nodeID, node] : editorGraph->Nodes)
		{
			const Ref<MaterialNode> materialNode = As<MaterialNode>(node);
			if (materialNode->Function == MaterialNodeFunction::Result)
			{
				resultNode = materialNode;
				break;
			}
		}

		if (!resultNode)
		{
			m_CompilerResult.Add("Could not find material result node : Aborting material compilation.", CompilerMessageType::Error);
			return;
		}

		// Verify all connections are legal (an issue here implies the graph was incorrectly constructed)
		m_CompilerResult.Add("Initializing Pre Compile Link Checks...");
		for (const auto& [linkID, link] : editorGraph->Links)
			if (!editorGraph->AreTypesCompatible(link.StartPinID, link.EndPinID))
				m_CompilerResult.Add("Cannot connect pins : Data types are not compatible.", CompilerMessageType::Error, link.StartPinID);

		// Verify all assets referenced are valid
		m_CompilerResult.Add("Initializing Asset Checks...");
		for (const auto& [nodeID, node] : editorGraph->Nodes)
		{
			const Ref<MaterialNode> materialNode = As<MaterialNode>(node);
			if (materialNode->Function == MaterialNodeFunction::TextureSample)
			{
				const AssetHandle textureHandle = materialNode->GetInput(1)->Data.Handle;

				if (AssetManager::DoesAssetExist(textureHandle) && AssetManager::IsAssetTypeCompatible(AssetType::Texture, AssetManager::GetMetadata(textureHandle).Type))
					continue;

				m_CompilerResult.Add("Invalid texture asset for sampler.", CompilerMessageType::Error, materialNode->ID);
			}
		}

		m_CompilerResult.Add("Initializing Node Tree Parse...");

		// Load appropriate shader templates
		std::unordered_map<MaterialAsset::MaterialRenderStage, std::string> renderStageSources;
		if (TargetProperties.Usage == MaterialAsset::MaterialUsage::Surface)
		{
			if (TargetProperties.AlphaBlendMode == MaterialAsset::AlphaBlendMode::Translucent)
				renderStageSources[MaterialAsset::MaterialRenderStage::Default] = Utils::ReadFile("Resources/Shaders/Template/Template_Translucent.glsl");
			else
			{
				renderStageSources[MaterialAsset::MaterialRenderStage::Default] = Utils::ReadFile("Resources/Shaders/Template/Template_Opaque.glsl");
				renderStageSources[MaterialAsset::MaterialRenderStage::PreDepth] = Utils::ReadFile("Resources/Shaders/Template/Template_PreDepth.glsl");
				renderStageSources[MaterialAsset::MaterialRenderStage::Shadow] = Utils::ReadFile("Resources/Shaders/Template/Template_ShadowDepth.glsl");
				renderStageSources[MaterialAsset::MaterialRenderStage::ShadowCSM] = Utils::ReadFile("Resources/Shaders/Template/Template_ShadowDepthCSM.glsl");
			}

			renderStageSources[MaterialAsset::MaterialRenderStage::VXGI] = Utils::ReadFile("Resources/Shaders/Template/Template_VXGIVoxelize.glsl");
		}
		else if (TargetProperties.Usage == MaterialAsset::MaterialUsage::PostProcessing)
		{
			renderStageSources[MaterialAsset::MaterialRenderStage::Default] = Utils::ReadFile("Resources/Shaders/Template/Template_PostProcess.glsl");
		}
		else if (TargetProperties.Usage == MaterialAsset::MaterialUsage::Particle)
		{
			renderStageSources[MaterialAsset::MaterialRenderStage::Default] = Utils::ReadFile("Resources/Shaders/Template/Template_Particle.glsl");
		}

		// Process template compiler operations

		// Apply include operations
		for (auto& [stage, source] : renderStageSources)
		{
			std::string includeString = "CompilerInclude(";
			while (source.find(includeString) != std::string::npos)
			{
				const size_t start = source.find(includeString);
				const size_t pathStart = start + includeString.length();
				const size_t end = source.find(")", pathStart);
				const std::string includePath = source.substr(pathStart, end - pathStart);
				source.replace(start, end - start + 1, Utils::ReadFile(fmt::format("Resources/Shaders/Template/{}.glsl", includePath)));
			}
		}

		// Read and build definition table
		std::unordered_map<MaterialAsset::MaterialRenderStage, CompilerDefinitionTable> renderStageDefinitions;
		for (auto& [stage, source] : renderStageSources)
		{
			const std::string definitionStartString = "CompilerDefine(";

			while (source.find(definitionStartString) != std::string::npos)
			{
				const size_t start = source.find(definitionStartString);
				const size_t identifierStart = start + definitionStartString.length();
				const size_t identifierEnd = source.find(",", identifierStart);
				const size_t definitionStart = source.find_first_not_of(" \t", identifierEnd + 1);
				const size_t definitionEnd = source.find(");", definitionStart);

				const std::string identifier = source.substr(identifierStart, identifierEnd - identifierStart);
				const std::string definition = source.substr(definitionStart, definitionEnd - definitionStart);

				renderStageDefinitions[stage][identifier] = definition;

				source.erase(start, (definitionEnd + 2) - start);
			}
		}

		// Apply conditionals
		std::unordered_map<std::string, bool> compilerConditionals;
		for (auto& [stage, source] : renderStageSources)
		{
			const std::string conditionalStartString = "CompilerIf(";
			const std::string conditionalEndString = "CompilerEndIf()";
			const std::string conditionalElseString = "CompilerElse()";

			size_t position = 0;
			while ((position = source.find(conditionalStartString, position)) != std::string::npos)
			{
				const size_t ifStart = position;
				const size_t conditionStart = ifStart + conditionalStartString.length();
				const size_t conditionEnd = source.find(")", conditionStart);
				std::string condition = source.substr(conditionStart, conditionEnd - conditionStart);

				// Stack to track nested if-else blocks
				size_t conditionalStack = 1;

				size_t currentPos = conditionEnd + 1;
				size_t elsePos = std::string::npos;
				size_t endIfPos = std::string::npos;

				while (conditionalStack)
				{
					const size_t nextIf = source.find(conditionalStartString, currentPos);
					const size_t nextElse = source.find(conditionalElseString, currentPos);
					const size_t nextEndIf = source.find(conditionalEndString, currentPos);

					// Determine the next keyword
					if (nextIf != std::string::npos && nextIf < nextElse && nextIf < nextEndIf)
					{
						conditionalStack++;
						currentPos = nextIf + conditionalStartString.length();
					}
					else if (nextElse != std::string::npos && nextElse < nextEndIf)
					{
						if (conditionalStack == 1)
							elsePos = nextElse;

						currentPos = nextElse + conditionalElseString.length();
					}
					else if (nextEndIf != std::string::npos)
					{
						conditionalStack--;

						if (!conditionalStack)
							endIfPos = nextEndIf;

						currentPos = nextEndIf + conditionalEndString.length();
					}
					else
					{
						DY_CORE_ASSERT(false, "Malformed if-else block detected for material compiler conditional");
						break;
					}
				}

				DY_CORE_ASSERT(endIfPos != std::string::npos, "Mismatched CompilerIf and CompilerEndIf");

				// Extract true/false branches
				std::string trueStatement, falseStatement;
				if (elsePos != std::string::npos && elsePos < endIfPos)
				{
					// We have an else statement
					trueStatement = source.substr(conditionEnd + 1, elsePos - conditionEnd - 1);
					falseStatement = source.substr(elsePos + conditionalElseString.length(), endIfPos - elsePos - conditionalElseString.length());
				}
				else
				{
					// Otherwise, generic if statement
					trueStatement = source.substr(conditionEnd + 1, endIfPos - conditionEnd - 1);
				}

				const bool negate = !condition.empty() && condition[0] == '!';

				if (negate)
					condition.erase(0, 1);

				if (compilerConditionals.find(condition) == compilerConditionals.end())
				{
					bool value = false;

					// Evaluate conditional
					if (condition == "Normal")
						value = editorGraph->IsPinLinked(resultNode->GetInput(MaterialResultPinType::Normal)->ID);
					else if (condition == "DepthOffset")
						value = editorGraph->IsPinLinked(resultNode->GetInput(MaterialResultPinType::DepthOffset)->ID);
					else if (condition == "VertexIndex")
						value = editorGraph->DoesGraphContainNodeFunction(Editor::MaterialNodeFunction::VertexIndex);
					else if (condition == "VertexColor")
						value = editorGraph->DoesGraphContainNodeFunction(Editor::MaterialNodeFunction::VertexColor);
					else if (condition == "TranslucentLit")
						value = TargetProperties.Lit && (TargetProperties.AlphaBlendMode == MaterialAsset::AlphaBlendMode::Translucent);
					else if (condition == "DeferredLit")
						value = TargetProperties.Lit && (TargetProperties.AlphaBlendMode != MaterialAsset::AlphaBlendMode::Translucent);
					else if (condition == "Tessellation")
						value = TargetProperties.Tessellation;
					else
					{
						DY_CORE_ASSERT(false, "Unknown material compiler conditional!");
						value = false;
					}

					compilerConditionals[condition] = value;
				}

				const bool value = compilerConditionals.at(condition);
				source.replace(ifStart, endIfPos + conditionalEndString.length() - ifStart, (negate ^ value) ? trueStatement : falseStatement);
			}
		}

		auto& resultPins = resultNode->Inputs;

		// Insert generated code
		for (auto& [stage, source] : renderStageSources)
		{
			std::unordered_map<std::string, std::string> compilerInsertions;

			const std::string insertionString = "Compiler(";
			while (source.find(insertionString) != std::string::npos)
			{
				const size_t start = source.find(insertionString);
				const size_t identifierStart = start + insertionString.length();
				const size_t end = source.find(")", identifierStart);
				const std::string identifier = source.substr(identifierStart, end - identifierStart);

				if (compilerInsertions.find(identifier) == compilerInsertions.end())
					compilerInsertions[identifier] = GetCompilationExpression(renderStageDefinitions[stage], identifier, resultNode);

				source.replace(start, end - start + 1, compilerInsertions.at(identifier));
			}
		}

		// Generate Path Tracing Snippet
		std::string pathTraceSource;
		{
			CompilerWriter compilerWriter;
			RenderStageNodeSet written;
			const CompilerDefinitionTable& definitions = renderStageDefinitions.at(MaterialAsset::MaterialRenderStage::Default);

			// Albedo
			{
				std::string line = "albedoColor = ";
				RecursivePinWrite(resultNode->GetInput(MaterialResultPinType::Albedo), compilerWriter, written, definitions, line);
				line += ";";
				compilerWriter.WriteLine(line);

				// TODO: Should have separate specular color pin
				compilerWriter.WriteLine("specularColor = albedoColor;");
			}

			compilerWriter.NewLine();

			// Emissive
			{
				std::string line = "emissive = ";
				RecursivePinWrite(resultNode->GetInput(MaterialResultPinType::Emissive), compilerWriter, written, definitions, line);
				line += ";";
				compilerWriter.WriteLine(line);
			}

			compilerWriter.NewLine();

			{
				std::string line = "smoothness = 1.0 - (";
				RecursivePinWrite(resultNode->GetInput(MaterialResultPinType::Roughness), compilerWriter, written, definitions, line);
				line += ");";
				compilerWriter.WriteLine(line);
			}

			compilerWriter.NewLine();

			{
				std::string line = "specularProbability = ";
				RecursivePinWrite(resultNode->GetInput(MaterialResultPinType::Specular), compilerWriter, written, definitions, line);
				line += ";";
				compilerWriter.WriteLine(line);
			}

			pathTraceSource = compilerWriter.Contents();
		}

		// If an error occurred during compilation we will not build!
		if (m_CompilerResult.ErrorCount != 0)
		{
			m_CompilerResult.Add("Internal errors were detected during runtime shader compilation. See compiler output for details - Aborting...", CompilerMessageType::Error);
			return;
		}

		// Begin shader compilation (to verify final generated GLSL is valid)

		// Copy the generated shader code to the compiler output window (even if internal shader compilation fails, so we can debug internal errors)
		m_CompilerOutput = renderStageSources;

		// Compile the generated shaders
		std::unordered_map<MaterialAsset::MaterialRenderStage, Ref<Shader>> shaders;
		for (const auto& [stage, output] : m_CompilerOutput)
			shaders[stage] = Shader::Create(fmt::format("RuntimeUserShader_{}_{}", TargetHandle, MaterialAsset::MaterialRenderStageToString(stage)), output, false);

		// Verify the generated code compiled successfully
		bool success = true;
		for (const auto& [renderStage, shader] : shaders)
		{
			if (!shader->IsLoaded())
			{
				success = false;
				m_CompilerResult.Add(fmt::format("[Internal Shader Error]: Failed at Material Render Stage '{}'", MaterialAsset::MaterialRenderStageToString(renderStage)), CompilerMessageType::Error);
				
				const auto& error = shader->GetError();
				if (!error.empty())
					m_CompilerResult.Add(error, CompilerMessageType::Error);
			}
		}

		if (!success)
		{
			m_CompilerResult.Add("Internal errors were detected during runtime shader compilation. See compiler output for details - Aborting...", CompilerMessageType::Error);
			return;
		}

		// We can now create, assign and serialize the generated material

		// Gather required material textures
		std::vector<MaterialAsset::MaterialTexture> textures;
		for (auto& [nodeID, node] : editorGraph->Nodes)
		{
			const Ref<MaterialNode> materialNode = As<MaterialNode>(node);
			if (materialNode->Function == MaterialNodeFunction::TextureSample)
			{
				const AssetHandle textureHandle = materialNode->GetInput(1)->Data.Handle;
				const Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(textureHandle);

				if (!texture)
				{
					m_CompilerResult.Add(fmt::format("Failed to load texture {} during compilation. Aborting...", textureHandle), CompilerMessageType::Error, nodeID);
					return;
				}

				textures.emplace_back(node->ID, texture);
			}
		}

		// Generate a list of all available material parameters
		std::unordered_map<std::string, MaterialAsset::MaterialParameterData> parameters;
		for (const auto& [nodeID, node] : editorGraph->Nodes)
		{
			Ref<MaterialNode> materialNode = As<MaterialNode>(node);

			if (materialNode->Function == MaterialNodeFunction::Parameter)
			{
				MaterialAsset::MaterialParameterData parameter;
				parameter.Name = node->Name;

				Ref<MaterialPin> output = materialNode->GetInput(0);
				switch (output->Type)
				{
				case MaterialPinType::Bool:   parameter.Type = MaterialAsset::MaterialParameterType::Bool;    parameter.Value.Bool    = output->Data.Bool;   break;
				case MaterialPinType::Float:  parameter.Type = MaterialAsset::MaterialParameterType::Float;   parameter.Value.Float   = output->Data.Float;  break;
				case MaterialPinType::Float2: parameter.Type = MaterialAsset::MaterialParameterType::Vector2; parameter.Value.Vector2 = output->Data.Float2; break;
				case MaterialPinType::Float3: parameter.Type = MaterialAsset::MaterialParameterType::Vector3; parameter.Value.Vector3 = output->Data.Float3; break;
				case MaterialPinType::Float4: parameter.Type = MaterialAsset::MaterialParameterType::Vector4; parameter.Value.Vector4 = output->Data.Float4; break;
				}

				parameters[node->Name] = parameter;
			}
		}

		// Create the new material asset and override the original (Note that we now pass ownership of the active editor graph to the new material asset)
		m_Material = MaterialSource::Create(shaders, textures, parameters, editorGraph);
		m_Material->Handle = TargetHandle;
		m_Material->SetPathTraceSource(pathTraceSource);
		m_Material->SetProperties(TargetProperties);
	}

	std::string MaterialCompiler::GetCompilationExpression(const CompilerDefinitionTable& definitions, const std::string& identifier, Ref<MaterialNode> resultNode)
	{
		// Generate compiler insertions
		CompilerWriter compilerWriter;

		if (identifier == "Comment")
		{
			compilerWriter.WriteLine("// " DY_VERSION_COPYRIGHT_SAFE ", All Rights Reserved");
			compilerWriter.WriteLine("// Generated by Dymatic Material Node Editor Version " DY_VERSION_STRING);
			compilerWriter.WriteLine(fmt::format("// Target: Vulkan GLSL (Runtime Material {})", TargetHandle));
		}
		else if (identifier == "Displacement")
		{
			const Ref<MaterialPin> displacementPin = resultNode->GetInput(MaterialResultPinType::WorldDisplacement);
			if (m_EditorGraph->IsPinLinked(displacementPin->ID))
			{
				RenderStageNodeSet written;
				compilerWriter.WriteLine("// Displacement Calculation added to position in world space");

				
				std::string line = definitions.at("World Position") + " += vec3(";
				RecursivePinWrite(displacementPin, compilerWriter, written, definitions, line);
				line += ");";
				compilerWriter.WriteLine(line);
				compilerWriter.NewLine();
			}
		}
		else if (identifier == "TessellationMultiplier")
		{
			const Ref<MaterialPin> tessellationMultiplierPin = resultNode->GetInput(MaterialResultPinType::TessellationMultiplier);

			RenderStageNodeSet written;
			std::string line = "const float tessellationMultiplier = ";
			RecursivePinWrite(tessellationMultiplierPin, compilerWriter, written, definitions, line);
			line += ";";
			compilerWriter.WriteLine(line);
		}
		else if (identifier == "TessellationSpacing")
		{
			const auto& config = m_EditorGraph->GetConfig();

			switch (config.TessellationSpacing)
			{
			case MaterialConfig::TessellationSpacingMode::Equal:			return "equal_spacing";
			case MaterialConfig::TessellationSpacingMode::EvenFractional:	return "fractional_even_spacing";
			case MaterialConfig::TessellationSpacingMode::OddFractional:	return "fractional_odd_spacing";
			}
		}
		else if (identifier == "DepthOffset")
		{
			// Pixel Depth Offset
			const Ref<MaterialPin> depthOffsetPin = resultNode->GetInput(MaterialResultPinType::DepthOffset);
			if (TargetProperties.Usage == MaterialAsset::MaterialUsage::Surface && m_EditorGraph->IsPinLinked(depthOffsetPin->ID))
			{
				// TODO: This NEEDS to be shared with properties (Solution: Have RenderStageNodeSet members for both Vertex and Fragment shader and everything just uses one or the other)
				RenderStageNodeSet written;

				std::string line = "float depthOffset = ";
				RecursivePinWrite(depthOffsetPin, compilerWriter, written, definitions, line);
				line += ";";
				compilerWriter.WriteLine(line);
			}
		}
		else if (identifier == "ParticleSize")
		{
			const Ref<MaterialPin> particleSizePin = resultNode->GetInput(MaterialResultPinType::ParticleSize);
			if (TargetProperties.Usage == MaterialAsset::MaterialUsage::Particle && m_EditorGraph->IsPinLinked(particleSizePin->ID))
			{
				RenderStageNodeSet written;

				std::string line = "vec2 particleSize = ";
				RecursivePinWrite(particleSizePin, compilerWriter, written, definitions, line);
				line += ";";
				compilerWriter.WriteLine(line);
			}
			else
				compilerWriter.WriteLine("vec2 particleSize = vec2(1.0);");
		}
		else if (identifier == "Alpha")
		{
			// TODO: Should share with properties (see solution comment above for pixel depth offset)
			RenderStageNodeSet written;

			std::string line = "float alpha = ";
			RecursivePinWrite(resultNode->GetInput(MaterialResultPinType::Alpha), compilerWriter, written, definitions, line);
			line += ";";
			compilerWriter.WriteLine(line);
		}
		else if (identifier == "PreDepth")
		{
			switch (TargetProperties.AlphaBlendMode)
			{
			case MaterialAsset::AlphaBlendMode::Masked:		compilerWriter.WriteLine("if (alpha < 0.5) discard;"); break;
			case MaterialAsset::AlphaBlendMode::Dithered:	compilerWriter.WriteLine("if (alpha < noise()) discard;"); break;
			}
		}
		else if (identifier == "ExpressionHeader")
		{
			bool writeComment = true;
			for (const auto& [nodeID, internalNode] : m_EditorGraph->Nodes)
			{
				const Ref<MaterialNode> node = As<MaterialNode>(internalNode);

				if (node->Function == MaterialNodeFunction::CustomExpression)
				{
					const std::string& includes = node->GetInput(0)->Data.String;
					const std::string& defines = node->GetInput(1)->Data.String;

					if (!includes.empty() || !defines.empty())
					{
						if (writeComment)
						{
							writeComment = false;
							compilerWriter.NewLine();
							compilerWriter.WriteLine("// Custom Material Expression Includes and Defines");
						}

						compilerWriter.WriteLine(defines);
						compilerWriter.WriteLine(includes);
					}
				}
			}

			compilerWriter.NewLine();
		}
		else if (identifier == "Buffers")
		{
			// Add material buffer and parameter bindings
			uint32_t size = 0;
			std::unordered_map<MaterialPinType, std::vector<std::string>> materialBufferBindings;
			for (auto& [nodeID, internalNode] : m_EditorGraph->Nodes)
			{
				const Ref<MaterialNode> node = As<MaterialNode>(internalNode);

				if (node->Function == MaterialNodeFunction::Parameter)
				{
					if (size >= 1024)
					{
						m_CompilerResult.Add(fmt::format("Material parameter buffer size exceeded 1024 bytes, preventing parameter node '{}' from being written", node->Name), CompilerMessageType::Error);
						continue;
					}

					const MaterialPinType type = node->GetInput(0)->Type;
					materialBufferBindings[type].push_back(fmt::format("{} u_UserParameter_{};", Utils::GetTypeString(type), node->ID));

					switch (type)
					{
					case MaterialPinType::Bool:		size += sizeof(int);		break;
					case MaterialPinType::Float:	size += sizeof(float);		break;
					case MaterialPinType::Float2:	size += sizeof(glm::vec2);	break;
					case MaterialPinType::Float3:	size += sizeof(glm::vec4);	break;
					case MaterialPinType::Float4:	size += sizeof(glm::vec4);	break;
					case MaterialPinType::Texture:	size += sizeof(glm::uvec2);	break;
					}
				}
			}

			if (!materialBufferBindings.empty())
			{
				compilerWriter.WriteLine("layout (std140, binding = USER_PARAMETER_BUFFER_BINDING) uniform UserMaterialParameterBuffer");
				compilerWriter.OpenScope();
				for (auto& binding : materialBufferBindings[MaterialPinType::Float4])
					compilerWriter.WriteLine(binding);
				uint32_t paddingIndex = 0;
				for (auto& binding : materialBufferBindings[MaterialPinType::Float3])
				{
					compilerWriter.WriteLine(binding);
					compilerWriter.WriteLine(fmt::format("float USER_PADD_{};", paddingIndex++));
				}
				for (auto& binding : materialBufferBindings[MaterialPinType::Float2])
					compilerWriter.WriteLine(binding);
				for (auto& binding : materialBufferBindings[MaterialPinType::Texture])
					compilerWriter.WriteLine(binding);
				for (auto& binding : materialBufferBindings[MaterialPinType::Float])
					compilerWriter.WriteLine(binding);
				for (auto& binding : materialBufferBindings[MaterialPinType::Bool])
					compilerWriter.WriteLine(binding);

				compilerWriter.CloseScope(true);
				compilerWriter.NewLine();
			}

			compilerWriter.NewLine();

			std::vector<NodeHandle> textureNodeIDs;
			for (const auto& [nodeID, internalNode] : m_EditorGraph->Nodes)
			{
				const Ref<MaterialNode> node = As<MaterialNode>(internalNode);
				if (node->Function == MaterialNodeFunction::TextureSample)
					textureNodeIDs.push_back(node->ID);
			}

			if (!textureNodeIDs.empty())
			{
				compilerWriter.WriteLine("layout (std140, binding = USER_TEXTURE_SAMPLER_BUFFER_BINDING) uniform UserMaterialTextureBuffer");
				compilerWriter.OpenScope();

				for (const auto& textureNodeID : textureNodeIDs)
					compilerWriter.WriteLine(fmt::format("uvec2 u_UserTextureSampler_{};", textureNodeID));

				compilerWriter.CloseScope(true);
			}

			compilerWriter.NewLine();
		}
		else if (identifier == "Properties")
		{
			RenderStageNodeSet written;

			// Albedo / Color
			{
				std::string line = Utils::IsSurfaceOrParticleUsage(TargetProperties.Usage) ? "vec3 albedo = " : "vec3 color = ";
				RecursivePinWrite(resultNode->GetInput(MaterialResultPinType::Albedo), compilerWriter, written, definitions, line);
				line += ";";
				compilerWriter.WriteLine(line);
			}

			compilerWriter.NewLine();

			// Normal
			const Ref<MaterialPin> normalPin = resultNode->GetInput(MaterialResultPinType::Normal);
			if (Utils::IsSurfaceOrParticleUsage(TargetProperties.Usage) && m_EditorGraph->IsPinLinked(normalPin->ID))
			{
				std::string line = "vec3 normal = ";
				RecursivePinWrite(normalPin, compilerWriter, written, definitions, line);
				line += ";";
				compilerWriter.WriteLine(line);
			}

			compilerWriter.NewLine();

			// Emissive
			if (Utils::IsSurfaceOrParticleUsage(TargetProperties.Usage))
			{
				std::string line = "vec3 emissive = ";
				RecursivePinWrite(resultNode->GetInput(MaterialResultPinType::Emissive), compilerWriter, written, definitions, line);
				line += ";";
				compilerWriter.WriteLine(line);
				compilerWriter.NewLine();
			}

			// Roughness
			if (Utils::IsSurfaceOrParticleUsage(TargetProperties.Usage))
			{
				std::string line = "float roughness = ";
				RecursivePinWrite(resultNode->GetInput(MaterialResultPinType::Roughness), compilerWriter, written, definitions, line);
				line += ";";
				compilerWriter.WriteLine(line);
				compilerWriter.NewLine();
			}

			if (Utils::IsSurfaceOrParticleUsage(TargetProperties.Usage))
			{
				std::string line = "float metallic = ";
				RecursivePinWrite(resultNode->GetInput(MaterialResultPinType::Metallic), compilerWriter, written, definitions, line);
				line += ";";
				compilerWriter.WriteLine(line);
				compilerWriter.NewLine();
			}

			if (Utils::IsSurfaceOrParticleUsage(TargetProperties.Usage))
			{
				std::string line = "float specular = ";
				RecursivePinWrite(resultNode->GetInput(MaterialResultPinType::Specular), compilerWriter, written, definitions, line);
				line += ";";
				compilerWriter.WriteLine(line);
				compilerWriter.NewLine();
			}

			if (Utils::IsSurfaceOrParticleUsage(TargetProperties.Usage))
			{
				std::string line = "float ao = ";
				RecursivePinWrite(resultNode->GetInput(MaterialResultPinType::AmbientOcclusion), compilerWriter, written, definitions, line);
				line += ";";
				compilerWriter.WriteLine(line);
				compilerWriter.NewLine();
			}

			if (Utils::IsSurfaceOrParticleUsage(TargetProperties.Usage))
			{
				if (TargetProperties.AlphaBlendMode == MaterialAsset::AlphaBlendMode::Opaque)
					compilerWriter.WriteLine("const float alpha = 1.0;");
				else
				{
					std::string line = "float alpha = ";
					RecursivePinWrite(resultNode->GetInput(MaterialResultPinType::Alpha), compilerWriter, written, definitions, line);
					line += ";";
					compilerWriter.WriteLine(line);
				}
			}

			if (Utils::IsSurfaceOrParticleUsage(TargetProperties.Usage) && TargetProperties.AlphaBlendMode == MaterialAsset::AlphaBlendMode::Translucent)
			{
				std::string line = "float ior = ";
				RecursivePinWrite(resultNode->GetInput(MaterialResultPinType::IOR), compilerWriter, written, definitions, line);
				line += ";";
				compilerWriter.WriteLine(line);
			}
		}
		else
			DY_CORE_ASSERT(false, "Unknown material compiler identifier!");

		return compilerWriter.Contents();
	}

	void MaterialCompiler::RecursivePinWrite(Ref<MaterialPin> pin, CompilerWriter& compilerWriter, RenderStageNodeSet& written, const CompilerDefinitionTable& definitions, std::string& line)
	{
		auto& editorGraph = m_EditorGraph;

		Link* link = editorGraph->GetPinLink(pin->ID);

		if (!link)
		{
			line += Utils::GetDataString(pin->Type, pin->Data);
			return;
		}

		const Ref<MaterialPin> otherPin = editorGraph->FindPin(link->StartPinID == pin->ID ? link->EndPinID : link->StartPinID);
		const Ref<MaterialNode> otherNode = editorGraph->FindNode(otherPin->Node);

		std::string argument;

		if (written.find(otherNode->ID) == written.end())
			RecursiveNodeWrite(otherNode, compilerWriter, definitions, written);

		if (Utils::IsDirectPinRead(otherNode->Function))
		{
			// Look in definition table for render stage and write the corresponding one (if it exists)
			const std::string identifier = MaterialGraph::GetNodeFunctionName(otherNode->Function);
			if (definitions.find(identifier) == definitions.end())
			{
				m_CompilerResult.Add(fmt::format("Direct node function '{}' is not available for the active material render stage", identifier), CompilerMessageType::Error, otherNode->ID);
				argument += "Unknown";
			}
			else
			{
				std::string definition = definitions.at(identifier);

				if (Utils::IsDirectScenePinRead(otherNode->Function))
				{
					const Ref<MaterialPin> uvPin = otherNode->GetInput(0);

					if (m_EditorGraph->IsPinLinked(uvPin->ID))
					{
						std::string argument;
						RecursivePinWrite(uvPin, compilerWriter, written, definitions, argument);
						String::ReplaceAll(definition, "__TEX__", argument);
					}
					else
					{
						String::ReplaceAll(definition, "__TEX__", definitions.at("Pixel Position"));
					}
				}

				argument += definition;
			}
		}
		else if (const char* constant = Utils::GetInternalConstantName(otherNode->Function))
			argument += constant;
		else if (otherNode->Function == MaterialNodeFunction::TextureSample)
		{
			argument += Utils::GenerateNodeSignature(otherNode);

			if (otherPin->Name == "RGB") argument += ".rgb";
			else if (otherPin->Name == "R") argument += ".r";
			else if (otherPin->Name == "G") argument += ".g";
			else if (otherPin->Name == "B") argument += ".b";
			else if (otherPin->Name == "A") argument += ".a";
			else if (otherPin->Name == "RGBA") argument += ".rgba";
		}
		else if (otherNode->Function == MaterialNodeFunction::Parameter)
		{
			argument += fmt::format("u_UserParameter_{}", otherNode->ID);
		}
		else if (otherNode->Function == MaterialNodeFunction::CustomExpression)
		{
			argument += fmt::format("{}_o{}", Utils::GenerateNodeSignature(otherNode), otherPin->ID);
		}
		else if (Utils::IsSplitComponentFunction(otherNode->Function))
		{
			// Split nodes have no need to store their own state, just access the components as required
			// Hence we 'pass through' this node a grab the next one
			const Ref<MaterialPin> passthroughPin = GetConnectedPin(otherNode->GetInput(0));
			const Ref<MaterialNode> passthroughNode = passthroughPin ? editorGraph->FindNode(passthroughPin->Node) : nullptr;

			if (!passthroughNode)
			{
				m_CompilerResult.Add("Split component node is not linked!", CompilerMessageType::Error, otherNode->ID);
				return;
			}

			if (written.find(passthroughNode->ID) == written.end())
				RecursiveNodeWrite(passthroughNode, compilerWriter, definitions, written);

			argument += Utils::GenerateNodeSignature(passthroughNode);

			if (otherPin->Name == "X") argument += ".x";
			else if (otherPin->Name == "Y") argument += ".y";
			else if (otherPin->Name == "Z") argument += ".z";
			else if (otherPin->Name == "W") argument += ".w";
		}
		else
			argument += Utils::GenerateNodeSignature(otherNode);

		const Ref<MaterialNode> node = editorGraph->FindNode(pin->Node);
		if (!Utils::IsComponentFunction(node->Function))
			Utils::CorrectArgumentType(GetHighestPinValue(pin), GetHighestPinValue(otherPin), argument);

		line += argument;
	}

	void MaterialCompiler::RecursiveNodeWrite(Ref<MaterialNode> node, CompilerWriter& compilerWriter, const CompilerDefinitionTable& definitions, RenderStageNodeSet& written)
	{
		if (written.find(node->ID) != written.end())
			return;

		if (node->Function == MaterialNodeFunction::TextureSample)
		{
			const Ref<MaterialPin> texCoordPin = node->GetInput(0);

			std::string argument;
			if (m_EditorGraph->IsPinLinked(texCoordPin->ID))
				RecursivePinWrite(texCoordPin, compilerWriter, written, definitions, argument);
			else
				argument = definitions.at("Texture Coordinates");

			compilerWriter.WriteLine(fmt::format("vec4 {} = texture(sampler2D(u_UserTextureSampler_{}), {});", Utils::GenerateNodeSignature(node), node->ID, argument));
		}
		else if (Utils::IsOperatorFunction(node->Function))
		{
			MaterialPinType type = GetHighestPinValue(node->GetOutput(0));

			std::string line = fmt::format("{} {} = ", Utils::GetTypeString(type), Utils::GenerateNodeSignature(node));

			const uint32_t argumentCount = node->Inputs.size();
			const std::string operatorString = fmt::format(" {} ", Utils::GetOperatorString(node->Function));

			for (uint32_t argumentIndex = 0; argumentIndex < argumentCount; argumentIndex++)
			{
				if (argumentIndex != 0)
					line += operatorString;

				const Ref<MaterialPin> pin = node->GetInput(argumentIndex);
				std::string argument;
				RecursivePinWrite(pin, compilerWriter, written, definitions, argument);
				Utils::CorrectArgumentType(type, pin->Type, argument);
				line += argument;
			}

			compilerWriter.WriteLine(line + ";");
		}
		else if (node->Function == MaterialNodeFunction::ComponentMask)
		{
			uint32_t componentCount = 0;
			for (auto& pin : node->Inputs)
			{
				const Ref<MaterialPin> input = As<MaterialPin>(pin);
				if (input->Type == MaterialPinType::Bool && input->Data.Bool)
					componentCount++;
			}

			const MaterialPinType type = componentCount == 0 ? GetHighestPinValue(node->GetOutput(0)) : (MaterialPinType)((uint32_t)MaterialPinType::Float + componentCount - 1u);

			std::string argument;
			RecursivePinWrite(node->GetInput(0), compilerWriter, written, definitions, argument);

			std::string components;
			if (componentCount != 0)
			{
				components = ".";
				if (node->GetInput(1)->Data.Bool) components += "r";
				if (node->GetInput(2)->Data.Bool) components += "g";
				if (node->GetInput(3)->Data.Bool) components += "b";
				if (node->GetInput(4)->Data.Bool) components += "a";
			}

			compilerWriter.WriteLine(fmt::format("{} {} = {}{};", Utils::GetTypeString(type), Utils::GenerateNodeSignature(node), argument, components));
		}
		else if (node->Function == MaterialNodeFunction::ComponentAppend)
		{
			const Ref<MaterialPin> pinA = node->GetInput(0);
			const Ref<MaterialPin> pinB = node->GetInput(1);

			const Ref<MaterialPin> otherPinA = GetConnectedPin(pinA);
			const Ref<MaterialPin> otherPinB = GetConnectedPin(pinB);

			MaterialPinType aType = otherPinA ? GetHighestPinValue(otherPinA) : MaterialPinType::Float;
			MaterialPinType bType = otherPinB ? GetHighestPinValue(otherPinB) : MaterialPinType::Float;

			const uint32_t aComponentCount = (uint32_t)aType - (uint32_t)MaterialPinType::Float + 1u;
			const uint32_t bComponentCount = (uint32_t)bType - (uint32_t)MaterialPinType::Float + 1u;
			const uint32_t totalComponentCount = aComponentCount + bComponentCount;

			const MaterialPinType outType = (MaterialPinType)(std::min((uint32_t)MaterialPinType::Float + (totalComponentCount - 1u), (uint32_t)MaterialPinType::Float4));

			std::string argumentA;
			RecursivePinWrite(pinA, compilerWriter, written, definitions, argumentA);

			std::string argumentB;
			RecursivePinWrite(pinB, compilerWriter, written, definitions, argumentB);

			compilerWriter.WriteLine(fmt::format("{} {} = {}({}, {});", Utils::GetTypeString(outType), Utils::GenerateNodeSignature(node), Utils::GetTypeString(outType), argumentA, argumentB));
		}
		else if (node->Function == MaterialNodeFunction::Constant)
		{
			const Ref<MaterialPin> constantPin = node->GetInput(0);
			compilerWriter.WriteLine(fmt::format("const {} {} = {};", Utils::GetTypeString(constantPin->Type), Utils::GenerateNodeSignature(node), Utils::GetDataString(constantPin->Type, constantPin->Data)));
		}
		else if (node->Function == MaterialNodeFunction::CustomExpression)
		{
			compilerWriter.WriteLine(fmt::format("// Begin Custom Material Expression '{}' ({})", node->Name, node->ID));

			std::string expression = node->GetInput(2)->Data.String;

			compilerWriter.WriteLine("// Custom Expression Inputs");
			for (uint32_t i = 3; i < node->Inputs.size(); i++)
			{
				const Ref<MaterialPin> input = node->GetInput(i);

				const std::string variableName = fmt::format("{}_i{}", Utils::GenerateNodeSignature(node), input->ID);
				std::string argument;
				RecursivePinWrite(input, compilerWriter, written, definitions, argument);
				compilerWriter.WriteLine(fmt::format("{} {} = {};", Utils::GetTypeString(input->Type), variableName, argument));

				String::ReplaceAll(expression, input->Name, variableName);
			}

			// Declare all output nodes and ensure that the expression has the correctly formatted name
			compilerWriter.WriteLine("// Custom Expression Outputs");
			for (auto& pin : node->Outputs)
			{
				const Ref<MaterialPin> output = As<MaterialPin>(pin);

				const std::string variableName = fmt::format("{}_o{}", Utils::GenerateNodeSignature(node), output->ID);
				compilerWriter.WriteLine(fmt::format("{} {};", Utils::GetTypeString(output->Type), variableName));

				String::ReplaceAll(expression, output->Name, variableName);
			}

			compilerWriter.WriteLine("// Custom Expression Body");
			compilerWriter.WriteLine(expression);
			compilerWriter.WriteLine("// End Custom Material Expression");
			compilerWriter.NewLine();
		}
		else if (Utils::IsInternalFunction(node->Function))
		{
			// Otherwise we attempt to map this as a normal function

			const MaterialPinType resultType = GetHighestPinValue(node->GetOutput(0));

			// Generate the argument string for the function call
			std::string arguments;
			bool first = true;
			for (auto& pin : node->Inputs)
			{
				const Ref<MaterialPin> input = As<MaterialPin>(pin);

				if (first)
					first = false;
				else
					arguments += ", ";

				RecursivePinWrite(input, compilerWriter, written, definitions, arguments);
			}


			compilerWriter.WriteLine(fmt::format("{} {} = {}({});", Utils::GetTypeString(resultType), Utils::GenerateNodeSignature(node), Utils::GetInternalFunctionName(node->Function), arguments));
		}

		written.insert(node->ID);
	}

	MaterialPinType MaterialCompiler::DerivePinType(const Ref<MaterialPin> input)
	{
		auto& editorGraph = m_EditorGraph;

		if (!editorGraph->IsPinLinked(input->ID))
			return MaterialPinType::Float;

		Link* link = editorGraph->GetPinLink(input->ID);
		Ref<MaterialPin> otherPin = editorGraph->FindPin(link->StartPinID == input->ID ? link->EndPinID : link->StartPinID);
		return GetHighestPinValue(otherPin);
	}

	MaterialPinType MaterialCompiler::GetHighestPinValue(const Ref<MaterialPin> pin)
	{
		if (pin->Type != MaterialPinType::FloatFamily)
			return pin->Type;

		const Ref<MaterialNode> node = m_EditorGraph->FindNode(pin->Node);

		if (Utils::IsComponentFunction(node->Function))
		{
			uint32_t componentCount = 0;
			if (node->Function == MaterialNodeFunction::ComponentMask)
			{
				for (auto& pin : node->Inputs)
				{
					const Ref<MaterialPin> input = As<MaterialPin>(pin);
					if (input->Type == MaterialPinType::Bool && input->Data.Bool)
						componentCount++;
				}
			}
			else if (node->Function == MaterialNodeFunction::ComponentAppend)
				componentCount = std::min((uint32_t)DerivePinType(node->GetInput(0)) + (uint32_t)DerivePinType(node->GetInput(1)) - 2 * ((uint32_t)MaterialPinType::Float) + 2, 4u);

			if (componentCount != 0)
				return (MaterialPinType)((uint32_t)MaterialPinType::Float + componentCount - 1);
		}

		MaterialPinType highest = MaterialPinType::Float;
		for (auto& pin : node->Inputs)
		{
			const Ref<MaterialPin> input = As<MaterialPin>(pin);
			MaterialPinType type = DerivePinType(input);

			if (type > highest)
				highest = type;

			if (highest == MaterialPinType::Float4)
				break;
		}

		return highest;
	}

	Ref<MaterialPin> MaterialCompiler::GetConnectedPin(const Ref<MaterialPin> pin)
	{
		if (!m_EditorGraph->IsPinLinked(pin->ID))
			return nullptr;

		Link* link = m_EditorGraph->GetPinLink(pin->ID);
		return m_EditorGraph->FindPin(link->StartPinID == pin->ID ? link->EndPinID : link->StartPinID);
	}

}