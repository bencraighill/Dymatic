#include "dypch.h"
#include "Dymatic/Editor/Material/MaterialBuilder.h"

#include "Dymatic/Editor/Material/MaterialCompiler.h"

namespace Dymatic::Editor {

	typedef std::unordered_map<Ref<MaterialBuilderNode>, Ref<MaterialNode>> BuilderNodeMap;

	namespace Utils {
	
		static NodeHandle RecursiveNodeBuild(const Ref<MaterialBuilderNode> builder, Ref<MaterialGraph> editorGraph, BuilderNodeMap& builderNodeMap)
		{
			const NodeHandle nodeHandle = editorGraph->SpawnNode(builder->Function);

			if (!builder->Inputs.empty())
			{
				const Ref<MaterialNode> node = editorGraph->FindNode(nodeHandle);

				for (const auto& [index, input] : builder->Inputs)
				{
					const Ref<MaterialPin> pin = node->GetInput(index);

					if (input.IsData())
					{
						pin->Data = input.Data;

						if (node->Function == MaterialNodeFunction::Constant || node->Function == MaterialNodeFunction::Parameter)
						{
							pin->Type = input.Type;
							node->GetOutput(0)->Type = input.Type;
						}
					}
					else
					{
						if (builderNodeMap.find(input.Node) == builderNodeMap.end())
						{
							const NodeHandle childHandle = RecursiveNodeBuild(input.Node, editorGraph, builderNodeMap);
							const Ref<MaterialNode> childNode = editorGraph->FindNode(childHandle);
							builderNodeMap[input.Node] = childNode;
						}

						const Ref<MaterialPin> outputPin = builderNodeMap.at(input.Node)->GetOutput(input.Pin);
						DY_CORE_ASSERT(editorGraph->CanCreateLink(outputPin, pin), "Illegal material node pin connection detected during build step!");
						editorGraph->CreateLink(outputPin->ID, pin->ID);
					}
				}
			}

			return nodeHandle;
		}

	}

	MaterialBuilderInput::MaterialBuilderInput(Ref<MaterialBuilderNode> node, const uint32_t pin)
		: Node(node), Pin(pin), m_IsData(false) {}

	MaterialBuilderInput::MaterialBuilderInput(const MaterialPinData& data)
		: Data(data), m_IsData(true) {}

	MaterialBuilderNode::MaterialBuilderNode(const MaterialNodeFunction function, const std::unordered_map<uint32_t, MaterialBuilderInput>& inputs)
		: Function(function), Inputs(inputs) {}

	Ref<MaterialBuilderNode> MaterialBuilderNode::Create(const MaterialNodeFunction function, const std::vector<MaterialBuilderInput>& inputs)
	{
		std::unordered_map<uint32_t, MaterialBuilderInput> inputMap(inputs.size());

		uint32_t index = 0;
		for (const auto& input : inputs)
		{
			inputMap[index] = inputs[index];
			index++;
		}

		return MaterialBuilderNode::CreateMapped(function, inputMap);
	}

	Ref<MaterialBuilderNode> MaterialBuilderNode::CreateSampler(const AssetHandle handle)
	{
		return MaterialBuilderNode::Create(MaterialNodeFunction::TextureSample, { { (AssetHandle)1, MaterialBuilderInput(handle) } });
	}

	Ref<MaterialGraph> MaterialBuilder::BuildGraph()
	{
		// Create a new editor graph and upload our desired compiler configuration
		const Ref<MaterialGraph> editorGraph = CreateRef<MaterialGraph>();
		editorGraph->GetConfig() = Config;

		const NodeHandle nodeHandle = editorGraph->SpawnResultNode();
		const Ref<MaterialNode> resultNode = editorGraph->FindNode(nodeHandle);

		BuilderNodeMap builderNodeMap;

		for (const auto& [type, node] : Results)
		{
			const NodeHandle nodeHandle = Utils::RecursiveNodeBuild(node, editorGraph, builderNodeMap);
			const Ref<MaterialPin> outputPin = editorGraph->FindNode(nodeHandle)->GetOutput(0);
			editorGraph->CreateLink(outputPin, resultNode->GetInput(type));
		}

		return editorGraph;
	}

	Ref<MaterialAsset> MaterialBuilder::BuildMaterial()
	{
		const Ref<MaterialGraph> editorGraph = BuildGraph();
		MaterialCompiler compiler(editorGraph);
		compiler.TargetProperties = Properties;
		compiler.TargetHandle = UUID();

		compiler.Compile();

		if (!compiler.GetCompilerResult().Success)
		{
			DY_CORE_ASSERT(false);
			DY_CORE_ERROR("Material Builder failed to generate material asset from built graph!");
			DY_CORE_TRACE("Material Compiler Log:");
			
			const auto& messages = compiler.GetCompilerResult().Messages;
			for (const auto& message : messages)
			{
				switch (message.Type)
				{
				case CompilerMessageType::Info: DY_CORE_INFO(message.Message); break;
				case CompilerMessageType::Compile: DY_CORE_TRACE(message.Message); break;
				case CompilerMessageType::Warning: DY_CORE_WARN(message.Message); break;
				case CompilerMessageType::Error: DY_CORE_ERROR(message.Message); break;
				}
			}
		}

		return compiler.GetMaterial();
	}

}