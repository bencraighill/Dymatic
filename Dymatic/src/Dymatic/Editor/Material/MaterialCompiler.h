#pragma once

#include "Dymatic/Renderer/MaterialAsset.h"
#include "Dymatic/Editor/Compiler.h"

#include "Dymatic/Editor/CompilerWriter.h"

#include <unordered_set>

namespace Dymatic::Editor {

	typedef std::unordered_set<NodeHandle> RenderStageNodeSet;
	typedef std::unordered_map<std::string, std::string> CompilerDefinitionTable;

	class MaterialCompiler
	{
	public:
		MaterialCompiler(Ref<MaterialGraph> editorGraph);
		void Compile();

		inline Ref<MaterialSource> GetMaterial() const { return m_Material; }
		inline const CompilerResult& GetCompilerResult() const { return m_CompilerResult; }
		inline std::unordered_map<MaterialAsset::MaterialRenderStage, std::string>& GetCompilerOutput() { return m_CompilerOutput; }

	private:
		void CompileGraph();
		std::string GetCompilationExpression(const CompilerDefinitionTable& definitions, const std::string& identifier, Ref<MaterialNode> resultNode);
		void RecursivePinWrite(Ref<MaterialPin> pin, CompilerWriter& compilerWriter, RenderStageNodeSet& written, const CompilerDefinitionTable& definitions, std::string& line);
		void RecursiveNodeWrite(Ref<MaterialNode> node, CompilerWriter& compilerWriter, const CompilerDefinitionTable& definitions, RenderStageNodeSet& written);

		MaterialPinType DerivePinType(const Ref<MaterialPin> input);
		MaterialPinType GetHighestPinValue(const Ref<MaterialPin> pin);
		Ref<MaterialPin> GetConnectedPin(const Ref<MaterialPin> pin);

	public:
		AssetHandle TargetHandle;
		MaterialAsset::MaterialProperties TargetProperties;
	private:
		// Inputs
		Ref<MaterialGraph> m_EditorGraph;

		// Outputs
		Ref<MaterialSource> m_Material;
		CompilerResult m_CompilerResult;
		std::unordered_map<MaterialAsset::MaterialRenderStage, std::string> m_CompilerOutput;
	};

}