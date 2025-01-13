#pragma once

// Builder Interface to construct Dymatic Material Graphs concisely within the engine code-base
// E.g. used for Default Grid and Navigation Mesh engine materials

#include "Dymatic/Renderer/MaterialAsset.h"
#include <unordered_map>

namespace Dymatic::Editor {

	struct MaterialBuilderNode;

	struct MaterialBuilderInput
	{
		MaterialBuilderInput() = default;
		MaterialBuilderInput(Ref<MaterialBuilderNode> node, const uint32_t pin = 0);
		MaterialBuilderInput(const MaterialPinData& data);

		MaterialBuilderInput(bool value) : Type(MaterialPinType::Bool) { Data.Bool = value; }
		MaterialBuilderInput(float value) : Type(MaterialPinType::Float) { Data.Float = value; }
		MaterialBuilderInput(const glm::vec2& value) : Type(MaterialPinType::Float2) { Data.Float2 = value; }
		MaterialBuilderInput(const glm::vec3& value) : Type(MaterialPinType::Float3) { Data.Float3 = value; }
		MaterialBuilderInput(const glm::vec4& value) : Type(MaterialPinType::Float4) { Data.Float4 = value; }
		MaterialBuilderInput(AssetHandle value) : Type(MaterialPinType::Texture) { Data.Handle = value; }
		MaterialBuilderInput(const std::string& value) : Type(MaterialPinType::String) { Data.String = value; }

		MaterialPinData Data;
		MaterialPinType Type;

		Ref<MaterialBuilderNode> Node;
		uint32_t Pin = 0;

		inline bool IsData() const { return m_IsData; }

	private:
		bool m_IsData = true;
	};

	struct MaterialBuilderNode
	{
		static Ref<MaterialBuilderNode> CreateMapped(const MaterialNodeFunction function, const std::unordered_map<uint32_t, MaterialBuilderInput>& inputs = {}) { return CreateRef<MaterialBuilderNode>(function, inputs); }
		static Ref<MaterialBuilderNode> Create(const MaterialNodeFunction function, const std::vector<MaterialBuilderInput>& inputs = {});
		static Ref<MaterialBuilderNode> CreateSampler(const AssetHandle handle);

		MaterialBuilderNode(const MaterialNodeFunction function, const std::unordered_map<uint32_t, MaterialBuilderInput>& inputs);

		MaterialNodeFunction Function;
		std::unordered_map<uint32_t, MaterialBuilderInput> Inputs;
	};

	struct MaterialBuilder
	{
		// Setup
		MaterialAsset::MaterialProperties Properties;
		MaterialConfig Config;
		std::unordered_map<MaterialResultPinType, Ref<MaterialBuilderNode>> Results;

		// Build Steps
		Ref<MaterialGraph> BuildGraph();
		Ref<MaterialAsset> BuildMaterial();
	};

}