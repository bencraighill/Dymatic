#pragma once
#include "Dymatic/Editor/EditorGraph.h"

#include "Dymatic/Asset/Asset.h"

namespace Dymatic::Editor {

	// Similar to material properties but are editor/compilation only as not needed by runtime system
	// Hence these are stored along with the editor graph
	struct MaterialConfig
	{
		enum class TessellationSpacingMode { Equal, EvenFractional, OddFractional };

		TessellationSpacingMode TessellationSpacing = TessellationSpacingMode::Equal;
	};

	enum MaterialResultPinType
	{
		Albedo,
		Normal,
		Emissive,
		Roughness,
		Metallic,
		Specular,
		AmbientOcclusion,
		Alpha,
		IOR,
		WorldDisplacement,
		TessellationMultiplier,
		DepthOffset,
		ParticleSize,

		MaterialResultPinCount
	};

	enum class MaterialNodeFunction
	{
		None = 0,

		Result,
		Comment,

		CustomExpression,
		TextureSample,
		Constant,
		Parameter,
		ComponentMask,
		ComponentAppend,

		// Inputs
		WorldPosition,
		WorldNormal,
		VertexColor,
		VertexDepth,
		VertexLinearDepth,
		TextureCoordinates,
		EntityID,
		SubmeshIndex,
		VertexIndex,
		ObjectPosition,
		CameraPosition,
		CameraDirection,
		CameraForward,
		CameraRight,
		CameraUp,
		CameraNear,
		CameraFar,
		PixelPosition,
		ViewSize,
		Time,

		ParticleLifetime,
		ParticleLifeRemaining,
		ParticleLifeWeight,
		ParticleVelocity,

		SceneColor,
		SceneAlbedo,
		ScenePosition,
		SceneDepth,
		SceneLinearDepth,
		SceneNormal,
		SceneEmissive,
		SceneRoughness,
		SceneMetallic,
		SceneSpecular,
		SceneAmbientOcclusion,
		SceneEntityID,
		SceneSubmeshIndex,

		// Math
		Add,
		Subtract,
		Multiply,
		Divide,

		Pi,
		Infinity,
		Euler,
		Tau,

		Radians,
		Degrees,
		Sine,
		Cosine,
		Tangent,
		ArcSine,
		ArcCosine,
		ArcTangent,
		HyperbolicSine,
		HyperbolicCosine,
		HyperbolicTangent,
		ArcHyperbolicSine,
		ArcHyperbolicCosine,
		ArcHyperbolicTangent,

		Abs,
		Length,
		Distance,
		Ceil,
		Floor,
		Clamp,
		Truncate,
		SquareRoot,
		InverseSquareRoot,
		CrossProduct,
		DotProduct,
		Reflect,
		Refract,
		Min,
		Max,
		Normalize,
		FMod,
		Fract,
		Step,
		SmoothStep,
		Round,
		RoundEven,
		Power,
		Exponential,
		Exponential2,
		Log,
		Log2,
		Sign,
		OneMinus,
		Negate,
		Saturate,
		Desaturate,
		Mix,
		If,

		// Boolean Logic
		Switch,

		// Constructors
		ConstructVector2,
		ConstructVector3,
		ConstructVector4,

		// Splitters
		SplitVector2,
		SplitVector3,
		SplitVector4,
	};

	enum class MaterialPinType : uint32_t
	{
		None = 0,

		Bool,

		FloatFamily,
		Float,
		Float2,
		Float3,
		Float4,

		String,
		StringMultiline,

		Texture
	};

	struct MaterialPinData
	{
	public:
		union
		{
			bool Bool;

			float Float;
			glm::vec2 Float2;
			glm::vec3 Float3;
			glm::vec4 Float4 = glm::vec4(0.0f);

			AssetHandle Handle;
		};

		// This string takes up a lot of space. Should we heap allocate it so it can be part of the union?
		std::string String;
	};

	struct MaterialPin : public EditorPin
	{
		MaterialPinType Type = MaterialPinType::Float;
		MaterialPinData Data;

		MaterialPin() = default;
		MaterialPin(const std::string& name, MaterialPinType type, const bool hidden)
			: EditorPin(name, hidden), Type(type)
		{}

		void SetType(const MaterialPinType type);
		static const char* GetPinTypeString(const MaterialPinType type);
	};

	struct MaterialNode : public EditorNode
	{
		MaterialNode(const MaterialNodeFunction function)
			: Function(function)
		{}

		Ref<MaterialPin> GetInput(uint32_t index) { return As<MaterialPin>(Inputs[index]); }
		Ref<MaterialPin> GetOutput(uint32_t index) { return As<MaterialPin>(Outputs[index]); }

		MaterialNodeFunction Function;
	};

	class MaterialGraph : public EditorGraph
	{
	public:
		static Ref<MaterialGraph> Create() { return Ref<MaterialGraph>(); }
		MaterialGraph();

		const MaterialConfig& GetConfig() const { return m_Config; }
		MaterialConfig& GetConfig() { return m_Config; }

		static const char* GetNodeFunctionName(MaterialNodeFunction function);
		virtual const char* GetNodeFunctionName(Ref<Editor::EditorNode> node) const override;

		static bool IsOperatorNode(const MaterialNodeFunction function);
		static MaterialNodeFunction GetNodeFunction(const std::string& name);

		Ref<MaterialNode> CreateNode(const MaterialNodeFunction function);
		NodeHandle SpawnNode(const MaterialNodeFunction function);
		virtual NodeHandle SpawnNodeFromName(const std::string& name) override;

		bool DoesGraphContainNodeFunction(const MaterialNodeFunction function);

		Ref<MaterialNode> FindNode(const NodeHandle id) const;
		Ref<MaterialPin> FindPin(const PinHandle id) const;
		virtual bool CanDeleteNode(NodeHandle id) const override;
		Ref<MaterialPin> AddPin(std::vector<Ref<EditorPin>>& target, const std::string& name, MaterialPinType type, const bool hidden = false);

		virtual bool PerformLinkCheck(Ref<EditorPin> a, Ref<EditorPin> b) override;
		inline virtual void OnCreateLink(LinkHandle linkID, Ref<EditorPin> a, Ref<EditorPin> b) override {}
		inline virtual bool CanAddPin(Ref<EditorNode> node) const override;
		inline virtual void OnAddPin(Ref<EditorNode> node) override;

		static bool AreTypesCompatible(MaterialPinType a, MaterialPinType b);
		virtual bool AreTypesCompatible(PinHandle a, PinHandle b) const override;
		static bool CanCreateLink(Ref<MaterialPin> a, Ref<MaterialPin> b);
		virtual bool CanCreateLink(PinHandle a, PinHandle b) const override;

		// Node Creation
		NodeHandle SpawnCommentNode(const glm::vec2 size = DefaultCommentSize);
		NodeHandle SpawnResultNode();

		NodeHandle SpawnCustomExpressionNode();
		NodeHandle SpawnTextureSampleNode(AssetHandle handle = 0);
		NodeHandle SpawnConstantNode(MaterialPinType type);
		NodeHandle SpawnParameterNode(MaterialPinType type);
		NodeHandle SpawnComponentMaskNode();
		NodeHandle SpawnComponentAppendNode();

		// Inputs
		NodeHandle SpawnWorldPositionNode();
		NodeHandle SpawnWorldNormalNode();
		NodeHandle SpawnVertexColorNode();
		NodeHandle SpawnVertexDepthNode();
		NodeHandle SpawnVertexLinearDepthNode();
		NodeHandle SpawnTextureCoordinatesNode();
		NodeHandle SpawnObjectPositionNode();
		NodeHandle SpawnEntityIDNode();
		NodeHandle SpawnSubmeshIndexNode();
		NodeHandle SpawnVertexIndexNode();
		NodeHandle SpawnCameraPositionNode();
		NodeHandle SpawnCameraDirectionNode();
		NodeHandle SpawnCameraForwardNode();
		NodeHandle SpawnCameraRightNode();
		NodeHandle SpawnCameraUpNode();
		NodeHandle SpawnCameraNearNode();
		NodeHandle SpawnCameraFarNode();
		NodeHandle SpawnPixelPositionNode();
		NodeHandle SpawnViewSizeNode();
		NodeHandle SpawnTimeNode();

		NodeHandle SpawnParticleLifetimeNode();
		NodeHandle SpawnParticleLifeRemainingNode();
		NodeHandle SpawnParticleLifeWeightNode();
		NodeHandle SpawnParticleVelocityNode();

		NodeHandle SpawnSceneColorNode();
		NodeHandle SpawnSceneAlbedoNode();
		NodeHandle SpawnScenePositionNode();
		NodeHandle SpawnSceneDepthNode();
		NodeHandle SpawnSceneLinearDepthNode();
		NodeHandle SpawnSceneNormalNode();
		NodeHandle SpawnSceneEmissiveNode();
		NodeHandle SpawnSceneRoughnessNode();
		NodeHandle SpawnSceneMetallicNode();
		NodeHandle SpawnSceneSpecularNode();
		NodeHandle SpawnSceneAmbientOcclusionNode();
		NodeHandle SpawnSceneEntityIDNode();
		NodeHandle SpawnSceneSubmeshIndexNode();

		// Math
		NodeHandle SpawnAddNode();
		NodeHandle SpawnSubtractNode();
		NodeHandle SpawnMultiplyNode();
		NodeHandle SpawnDivideNode();

		NodeHandle SpawnPiNode();
		NodeHandle SpawnInfinityNode();
		NodeHandle SpawnEulerNode();
		NodeHandle SpawnTauNode();

		NodeHandle SpawnAbsNode();
		NodeHandle SpawnLengthNode();
		NodeHandle SpawnDistanceNode();
		NodeHandle SpawnRadiansNode();
		NodeHandle SpawnDegreesNode();
		NodeHandle SpawnSineNode();
		NodeHandle SpawnCosineNode();
		NodeHandle SpawnTangentNode();
		NodeHandle SpawnArcSineNode();
		NodeHandle SpawnArcCosineNode();
		NodeHandle SpawnArcTangentNode();
		NodeHandle SpawnHyperbolicSineNode();
		NodeHandle SpawnHyperbolicCosineNode();
		NodeHandle SpawnHyperbolicTangentNode();
		NodeHandle SpawnArcHyperbolicSineNode();
		NodeHandle SpawnArcHyperbolicCosineNode();
		NodeHandle SpawnArcHyperbolicTangentNode();
		NodeHandle SpawnCeilNode();
		NodeHandle SpawnFloorNode();
		NodeHandle SpawnClampNode();
		NodeHandle SpawnTruncateNode();
		NodeHandle SpawnSquareRootNode();
		NodeHandle SpawnInverseSquareRootNode();
		NodeHandle SpawnCrossProductNode();
		NodeHandle SpawnDotProductNode();
		NodeHandle SpawnReflectVectorNode();
		NodeHandle SpawnRefractVectorNode();
		NodeHandle SpawnMinNode();
		NodeHandle SpawnMaxNode();
		NodeHandle SpawnNormalizeNode();
		NodeHandle SpawnFModNode();
		NodeHandle SpawnFractNode();
		NodeHandle SpawnStepNode();
		NodeHandle SpawnSmoothStepNode();
		NodeHandle SpawnRoundNode();
		NodeHandle SpawnRoundEvenNode();
		NodeHandle SpawnPowerNode();
		NodeHandle SpawnExponentialNode();
		NodeHandle SpawnExponential2Node();
		NodeHandle SpawnLogNode();
		NodeHandle SpawnLog2Node();
		NodeHandle SpawnSignNode();
		NodeHandle SpawnOneMinusNode();
		NodeHandle SpawnNegateNode();
		NodeHandle SpawnSaturateNode();
		NodeHandle SpawnDesaturateNode();
		NodeHandle SpawnMixNode();
		NodeHandle SpawnIfNode();

		NodeHandle SpawnSwitchNode();

		NodeHandle SpawnConstructVector2Node();
		NodeHandle SpawnConstructVector3Node();
		NodeHandle SpawnConstructVector4Node();
		NodeHandle SpawnSplitVector2Node();
		NodeHandle SpawnSplitVector3Node();
		NodeHandle SpawnSplitVector4Node();

	private:
		NodeHandle SpawnInputNode(const MaterialNodeFunction function, const MaterialPinType type);
		NodeHandle SpawnInputSamplerNode(const MaterialNodeFunction function, const MaterialPinType type);
		NodeHandle SpawnSingleMathNode(const MaterialNodeFunction function);
		NodeHandle SpawnDoubleMathNode(const MaterialNodeFunction function);
		NodeHandle SpawnDoubleMathNode(const MaterialNodeFunction function, const std::string& a, const std::string& b);

		NodeHandle SpawnComponentNode(const MaterialNodeFunction function, const MaterialPinType type, const uint8_t componentCount, const bool componentInputs);

		virtual Ref<EditorPin> CreateNewPin(const std::string& name) override;

	private:
		MaterialConfig m_Config;
	};

}