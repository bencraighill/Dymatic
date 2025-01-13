#include "dypch.h"
#include "Dymatic/Editor/Material/MaterialGraph.h"

namespace Dymatic::Editor {

	const char* MaterialPin::GetPinTypeString(const MaterialPinType type)
	{
		switch (type)
		{
		case MaterialPinType::None: return "None";
		case MaterialPinType::Bool: return "Bool";
		case MaterialPinType::Float: return "Float";
		case MaterialPinType::Float2: return "Float2";
		case MaterialPinType::Float3: return "Float3";
		case MaterialPinType::Float4: return "Float4";
		case MaterialPinType::FloatFamily: return "Float Family";
		case MaterialPinType::String: return "String";
		case MaterialPinType::StringMultiline: return "String Multiline";
		case MaterialPinType::Texture: return "Texture";
		}

		DY_CORE_ASSERT(false, "Unknown Material Node Function");
		return "Unknown";
	}

	void MaterialPin::SetType(const MaterialPinType type)
	{
		if (type == Type)
			return;

		Type = type;
		Data = MaterialPinData();
	}

	MaterialGraph::MaterialGraph()
	{
	}

	const char* MaterialGraph::GetNodeFunctionName(MaterialNodeFunction function)
	{
		switch (function)
		{
			case MaterialNodeFunction::None: return "None";
			case MaterialNodeFunction::Result: return "Result";
			case MaterialNodeFunction::Comment: return "Comment";
			case MaterialNodeFunction::CustomExpression: return "Custom Expression";
			case MaterialNodeFunction::TextureSample: return "Texture Sample";
			case MaterialNodeFunction::Constant: return "Constant";
			case MaterialNodeFunction::Parameter: return "Parameter";
			case MaterialNodeFunction::ComponentMask: return "Mask";
			case MaterialNodeFunction::ComponentAppend: return "Append";
			case MaterialNodeFunction::WorldPosition: return "World Position";
			case MaterialNodeFunction::WorldNormal: return "World Normal";
			case MaterialNodeFunction::VertexColor: return "Vertex Color";
			case MaterialNodeFunction::VertexDepth: return "Vertex Depth";
			case MaterialNodeFunction::VertexLinearDepth: return "Vertex Linear Depth";
			case MaterialNodeFunction::TextureCoordinates: return "Texture Coordinates";
			case MaterialNodeFunction::ObjectPosition: return "Object Position";
			case MaterialNodeFunction::EntityID: return "Entity ID";
			case MaterialNodeFunction::SubmeshIndex: return "Submesh Index";
			case MaterialNodeFunction::VertexIndex: return "Vertex Index";
			case MaterialNodeFunction::CameraPosition: return "Camera Position";
			case MaterialNodeFunction::CameraDirection: return "Camera Direction";
			case MaterialNodeFunction::CameraForward: return "Camera Forward";
			case MaterialNodeFunction::CameraRight: return "Camera Right";
			case MaterialNodeFunction::CameraUp: return "Camera Up";
			case MaterialNodeFunction::CameraNear: return "Camera Near";
			case MaterialNodeFunction::CameraFar: return "Camera Far";
			case MaterialNodeFunction::PixelPosition: return "Pixel Position";
			case MaterialNodeFunction::ViewSize: return "View Size";
			case MaterialNodeFunction::Time: return "Time";
			case MaterialNodeFunction::ParticleLifetime: return "Particle Lifetime";
			case MaterialNodeFunction::ParticleLifeRemaining: return "Particle Life Remaining";
			case MaterialNodeFunction::ParticleLifeWeight: return "Particle Life Weight";
			case MaterialNodeFunction::ParticleVelocity: return "Particle Velocity";
			case MaterialNodeFunction::SceneColor: return "Scene Color";
			case MaterialNodeFunction::SceneAlbedo: return "Scene Albedo";
			case MaterialNodeFunction::ScenePosition: return "Scene Position";
			case MaterialNodeFunction::SceneDepth: return "Scene Depth";
			case MaterialNodeFunction::SceneLinearDepth: return "Scene Linear Depth";
			case MaterialNodeFunction::SceneNormal: return "Scene Normal";
			case MaterialNodeFunction::SceneEmissive: return "Scene Emissive";
			case MaterialNodeFunction::SceneRoughness: return "Scene Roughness";
			case MaterialNodeFunction::SceneMetallic: return "Scene Metallic";
			case MaterialNodeFunction::SceneSpecular: return "Scene Specular";
			case MaterialNodeFunction::SceneAmbientOcclusion: return "Scene Ambient Occlusion";
			case MaterialNodeFunction::SceneEntityID: return "Scene Entity ID";
			case MaterialNodeFunction::SceneSubmeshIndex: return "Scene Submesh Index";
			case MaterialNodeFunction::Add: return "Add";
			case MaterialNodeFunction::Subtract: return "Subtract";
			case MaterialNodeFunction::Multiply: return "Multiply";
			case MaterialNodeFunction::Divide: return "Divide";
			case MaterialNodeFunction::Pi: return "Pi";
			case MaterialNodeFunction::Infinity: return "Infinity";
			case MaterialNodeFunction::Euler: return "Euler";
			case MaterialNodeFunction::Tau: return "Tau";
			case MaterialNodeFunction::Abs: return "Abs";
			case MaterialNodeFunction::Length: return "Length";
			case MaterialNodeFunction::Distance: return "Distance";
			case MaterialNodeFunction::Radians: return "Radians";
			case MaterialNodeFunction::Degrees: return "Degrees";
			case MaterialNodeFunction::Sine: return "Sine";
			case MaterialNodeFunction::Cosine: return "Cosine";
			case MaterialNodeFunction::Tangent: return "Tangent";
			case MaterialNodeFunction::ArcSine: return "Arc Sine";
			case MaterialNodeFunction::ArcCosine: return "Arc Cosine";
			case MaterialNodeFunction::ArcTangent: return "Arc Tangent";
			case MaterialNodeFunction::HyperbolicSine: return "Hyperbolic Sine";
			case MaterialNodeFunction::HyperbolicCosine: return "Hyperbolic Cosine";
			case MaterialNodeFunction::HyperbolicTangent: return "Hyperbolic Tangent";
			case MaterialNodeFunction::ArcHyperbolicSine: return "Arc Hyperbolic Sine";
			case MaterialNodeFunction::ArcHyperbolicCosine: return "Arc Hyperbolic Cosine";
			case MaterialNodeFunction::ArcHyperbolicTangent: return "Arc Hyperbolic Tangent";
			case MaterialNodeFunction::Ceil: return "Ceil";
			case MaterialNodeFunction::Floor: return "Floor";
			case MaterialNodeFunction::Clamp: return "Clamp";
			case MaterialNodeFunction::Truncate: return "Truncate";
			case MaterialNodeFunction::SquareRoot: return "Square Root";
			case MaterialNodeFunction::InverseSquareRoot: return "Inverse Square Root";
			case MaterialNodeFunction::CrossProduct: return "Cross Product";
			case MaterialNodeFunction::DotProduct: return "Dot Product";
			case MaterialNodeFunction::Reflect: return "Reflect Vector";
			case MaterialNodeFunction::Refract: return "Refract Vector";
			case MaterialNodeFunction::Min: return "Min";
			case MaterialNodeFunction::Max: return "Max";
			case MaterialNodeFunction::Normalize: return "Normalize";
			case MaterialNodeFunction::FMod: return "FMod";
			case MaterialNodeFunction::Fract: return "Fract";
			case MaterialNodeFunction::Step: return "Step";
			case MaterialNodeFunction::SmoothStep: return "Smooth Step";
			case MaterialNodeFunction::Round: return "Round";
			case MaterialNodeFunction::RoundEven: return "Round Even";
			case MaterialNodeFunction::Power: return "Power";
			case MaterialNodeFunction::Exponential: return "Exponental";
			case MaterialNodeFunction::Exponential2: return "Exponental2";
			case MaterialNodeFunction::Log: return "Log";
			case MaterialNodeFunction::Log2: return "Log2";
			case MaterialNodeFunction::Sign: return "Sign";
			case MaterialNodeFunction::OneMinus: return "One Minus";
			case MaterialNodeFunction::Negate: return "Negate";
			case MaterialNodeFunction::Saturate: return "Saturate";
			case MaterialNodeFunction::Desaturate: return "Desaturate";
			case MaterialNodeFunction::Mix: return "Mix";
			case MaterialNodeFunction::If: return "If";
			case MaterialNodeFunction::Switch: return "Switch";
			case MaterialNodeFunction::ConstructVector2: return "Construct Vector 2";
			case MaterialNodeFunction::ConstructVector3: return "Construct Vector 3";
			case MaterialNodeFunction::ConstructVector4: return "Construct Vector 4";
			case MaterialNodeFunction::SplitVector2: return "Split Vector 2";
			case MaterialNodeFunction::SplitVector3: return "Split Vector 3";
			case MaterialNodeFunction::SplitVector4: return "Split Vector 4";
		}

		DY_CORE_ASSERT(false, "Unknown Material Node Function");
		return "Unknown";
	}

	const char* MaterialGraph::GetNodeFunctionName(Ref<Editor::EditorNode> node) const
	{
		return GetNodeFunctionName(As<MaterialNode>(node)->Function);
	}

	Ref<MaterialNode> MaterialGraph::CreateNode(const MaterialNodeFunction function)
	{
		Ref<MaterialNode> node = CreateRef<MaterialNode>(function);
		CreateNodeInternal(node);
		return node;
	}

	bool MaterialGraph::IsOperatorNode(const MaterialNodeFunction function)
	{
		switch (function)
		{
		case MaterialNodeFunction::Add:
		case MaterialNodeFunction::Subtract:
		case MaterialNodeFunction::Multiply:
		case MaterialNodeFunction::Divide:
			return true;
		}

		return false;
	}

	MaterialNodeFunction MaterialGraph::GetNodeFunction(const std::string& name)
	{
		if (name == "Result") return MaterialNodeFunction::Result;
		if (name == "Comment") return MaterialNodeFunction::Comment;
		if (name == "Custom Expression") return MaterialNodeFunction::CustomExpression;
		if (name == "Texture Sample") return MaterialNodeFunction::TextureSample;
		if (name == "Constant") return MaterialNodeFunction::Constant;
		if (name == "Parameter") return MaterialNodeFunction::Parameter;
		if (name == "Mask") return MaterialNodeFunction::ComponentMask;
		if (name == "Append") return MaterialNodeFunction::ComponentAppend;
		if (name == "World Position") return MaterialNodeFunction::WorldPosition;
		if (name == "World Normal") return MaterialNodeFunction::WorldNormal;
		if (name == "Vertex Color") return MaterialNodeFunction::VertexColor;
		if (name == "Vertex Depth") return MaterialNodeFunction::VertexDepth;
		if (name == "Vertex Linear Depth") return MaterialNodeFunction::VertexLinearDepth;
		if (name == "Texture Coordinates") return MaterialNodeFunction::TextureCoordinates;
		if (name == "Object Position") return MaterialNodeFunction::ObjectPosition;
		if (name == "Entity ID") return MaterialNodeFunction::EntityID;
		if (name == "Submesh Index") return MaterialNodeFunction::SubmeshIndex;
		if (name == "Vertex Index") return MaterialNodeFunction::VertexIndex;
		if (name == "Camera Position") return MaterialNodeFunction::CameraPosition;
		if (name == "Camera Direction") return MaterialNodeFunction::CameraDirection;
		if (name == "Camera Forward") return MaterialNodeFunction::CameraForward;
		if (name == "Camera Right") return MaterialNodeFunction::CameraRight;
		if (name == "Camera Up") return MaterialNodeFunction::CameraUp;
		if (name == "Camera Near") return MaterialNodeFunction::CameraNear;
		if (name == "Camera Far") return MaterialNodeFunction::CameraFar;
		if (name == "Pixel Position") return MaterialNodeFunction::PixelPosition;
		if (name == "View Size") return MaterialNodeFunction::ViewSize;
		if (name == "Time") return MaterialNodeFunction::Time;
		if (name == "Particle Lifetime") return MaterialNodeFunction::ParticleLifetime;
		if (name == "Particle Life Remaining") return MaterialNodeFunction::ParticleLifeRemaining;
		if (name == "Particle Life Weight") return MaterialNodeFunction::ParticleLifeWeight;
		if (name == "Particle Velocity") return MaterialNodeFunction::ParticleVelocity;
		if (name == "Scene Color") return MaterialNodeFunction::SceneColor;
		if (name == "Scene Albedo") return MaterialNodeFunction::SceneAlbedo;
		if (name == "Scene Position") return MaterialNodeFunction::ScenePosition;
		if (name == "Scene Depth") return MaterialNodeFunction::SceneDepth;
		if (name == "Scene Linear Depth") return MaterialNodeFunction::SceneLinearDepth;
		if (name == "Scene Normal") return MaterialNodeFunction::SceneNormal;
		if (name == "Scene Emissive") return MaterialNodeFunction::SceneEmissive;
		if (name == "Scene Roughness") return MaterialNodeFunction::SceneRoughness;
		if (name == "Scene Metallic") return MaterialNodeFunction::SceneMetallic;
		if (name == "Scene Specular") return MaterialNodeFunction::SceneSpecular;
		if (name == "Scene Ambient Occlusion") return MaterialNodeFunction::SceneAmbientOcclusion;
		if (name == "Scene Entity ID") return MaterialNodeFunction::SceneEntityID;
		if (name == "Scene Submesh Index") return MaterialNodeFunction::SceneSubmeshIndex;
		if (name == "Add") return MaterialNodeFunction::Add;
		if (name == "Subtract") return MaterialNodeFunction::Subtract;
		if (name == "Multiply") return MaterialNodeFunction::Multiply;
		if (name == "Divide") return MaterialNodeFunction::Divide;
		if (name == "Pi") return MaterialNodeFunction::Pi;
		if (name == "Infinity") return MaterialNodeFunction::Infinity;
		if (name == "Euler") return MaterialNodeFunction::Euler;
		if (name == "Tau") return MaterialNodeFunction::Tau;
		if (name == "Abs") return MaterialNodeFunction::Abs;
		if (name == "Length") return MaterialNodeFunction::Length;
		if (name == "Distance") return MaterialNodeFunction::Distance;
		if (name == "Radians") return MaterialNodeFunction::Radians;
		if (name == "Degrees") return MaterialNodeFunction::Degrees;
		if (name == "Sine") return MaterialNodeFunction::Sine;
		if (name == "Cosine") return MaterialNodeFunction::Cosine;
		if (name == "Tangent") return MaterialNodeFunction::Tangent;
		if (name == "Arc Sine") return MaterialNodeFunction::ArcSine;
		if (name == "Arc Cosine") return MaterialNodeFunction::ArcCosine;
		if (name == "Arc Tangent") return MaterialNodeFunction::ArcTangent;
		if (name == "Hyperbolic Sine") return MaterialNodeFunction::HyperbolicSine;
		if (name == "Hyperbolic Cosine") return MaterialNodeFunction::HyperbolicCosine;
		if (name == "Hyperbolic Tangent") return MaterialNodeFunction::HyperbolicTangent;
		if (name == "Arc Hyperbolic Sine") return MaterialNodeFunction::ArcHyperbolicSine;
		if (name == "Arc Hyperbolic Cosine") return MaterialNodeFunction::ArcHyperbolicCosine;
		if (name == "Arc Hyperbolic Tangent") return MaterialNodeFunction::ArcHyperbolicTangent;
		if (name == "Ceil") return MaterialNodeFunction::Ceil;
		if (name == "Floor") return MaterialNodeFunction::Floor;
		if (name == "Clamp") return MaterialNodeFunction::Clamp;
		if (name == "Truncate") return MaterialNodeFunction::Truncate;
		if (name == "Square Root") return MaterialNodeFunction::SquareRoot;
		if (name == "Inverse Square Root") return MaterialNodeFunction::InverseSquareRoot;
		if (name == "Cross Product") return MaterialNodeFunction::CrossProduct;
		if (name == "Dot Product") return MaterialNodeFunction::DotProduct;
		if (name == "Reflect Vector") return MaterialNodeFunction::Reflect;
		if (name == "Refract Vector") return MaterialNodeFunction::Refract;
		if (name == "Min") return MaterialNodeFunction::Min;
		if (name == "Max") return MaterialNodeFunction::Max;
		if (name == "Normalize") return MaterialNodeFunction::Normalize;
		if (name == "FMod") return MaterialNodeFunction::FMod;
		if (name == "Fract") return MaterialNodeFunction::Fract;
		if (name == "Step") return MaterialNodeFunction::Step;
		if (name == "Smooth Step") return MaterialNodeFunction::SmoothStep;
		if (name == "Round") return MaterialNodeFunction::Round;
		if (name == "Round Even") return MaterialNodeFunction::RoundEven;
		if (name == "Power") return MaterialNodeFunction::Power;
		if (name == "Exponential") return MaterialNodeFunction::Exponential;
		if (name == "Exponential2") return MaterialNodeFunction::Exponential2;
		if (name == "Log") return MaterialNodeFunction::Log;
		if (name == "Log2") return MaterialNodeFunction::Log2;
		if (name == "Sign") return MaterialNodeFunction::Sign;
		if (name == "One Minus") return MaterialNodeFunction::OneMinus;
		if (name == "Negate") return MaterialNodeFunction::Negate;
		if (name == "Saturate") return MaterialNodeFunction::Saturate;
		if (name == "Desaturate") return MaterialNodeFunction::Desaturate;
		if (name == "Mix") return MaterialNodeFunction::Mix;
		if (name == "If") return MaterialNodeFunction::If;
		if (name == "Switch") return MaterialNodeFunction::Switch;
		if (name == "Construct Vector 2") return MaterialNodeFunction::ConstructVector2;
		if (name == "Construct Vector 3") return MaterialNodeFunction::ConstructVector3;
		if (name == "Construct Vector 4") return MaterialNodeFunction::ConstructVector4;
		if (name == "Split Vector 2") return MaterialNodeFunction::SplitVector2;
		if (name == "Split Vector 3") return MaterialNodeFunction::SplitVector3;
		if (name == "Split Vector 4") return MaterialNodeFunction::SplitVector4;

		return MaterialNodeFunction::None;
	}

	NodeHandle MaterialGraph::SpawnNode(const MaterialNodeFunction function)
	{
		switch (function)
		{
		case MaterialNodeFunction::Result: return SpawnResultNode();
		case MaterialNodeFunction::Comment: return SpawnCommentNode();
		case MaterialNodeFunction::CustomExpression: return SpawnCustomExpressionNode();
		case MaterialNodeFunction::TextureSample: return SpawnTextureSampleNode();
		case MaterialNodeFunction::Constant: return SpawnConstantNode(Editor::MaterialPinType::Float);
		case MaterialNodeFunction::Parameter: return SpawnParameterNode(Editor::MaterialPinType::Float);
		case MaterialNodeFunction::ComponentMask: return SpawnComponentMaskNode();
		case MaterialNodeFunction::ComponentAppend: return SpawnComponentAppendNode();
		case MaterialNodeFunction::WorldPosition: return SpawnWorldPositionNode();
		case MaterialNodeFunction::WorldNormal: return SpawnWorldNormalNode();
		case MaterialNodeFunction::VertexColor: return SpawnVertexColorNode();
		case MaterialNodeFunction::VertexDepth: return SpawnVertexDepthNode();
		case MaterialNodeFunction::VertexLinearDepth: return SpawnVertexLinearDepthNode();
		case MaterialNodeFunction::TextureCoordinates: return SpawnTextureCoordinatesNode();
		case MaterialNodeFunction::ObjectPosition: return SpawnObjectPositionNode();
		case MaterialNodeFunction::EntityID: return SpawnEntityIDNode();
		case MaterialNodeFunction::SubmeshIndex: return SpawnSubmeshIndexNode();
		case MaterialNodeFunction::VertexIndex: return SpawnVertexIndexNode();
		case MaterialNodeFunction::CameraPosition: return SpawnCameraPositionNode();
		case MaterialNodeFunction::CameraDirection: return SpawnCameraDirectionNode();
		case MaterialNodeFunction::CameraForward: return SpawnCameraForwardNode();
		case MaterialNodeFunction::CameraRight: return SpawnCameraRightNode();
		case MaterialNodeFunction::CameraUp: return SpawnCameraUpNode();
		case MaterialNodeFunction::CameraNear: return SpawnCameraNearNode();
		case MaterialNodeFunction::CameraFar: return SpawnCameraFarNode();
		case MaterialNodeFunction::PixelPosition: return SpawnPixelPositionNode();
		case MaterialNodeFunction::ViewSize: return SpawnViewSizeNode();
		case MaterialNodeFunction::Time: return SpawnTimeNode();
		case MaterialNodeFunction::ParticleLifetime: return SpawnParticleLifetimeNode();
		case MaterialNodeFunction::ParticleLifeRemaining: return SpawnParticleLifeRemainingNode();
		case MaterialNodeFunction::ParticleLifeWeight: return SpawnParticleLifeWeightNode();
		case MaterialNodeFunction::ParticleVelocity: return SpawnParticleVelocityNode();
		case MaterialNodeFunction::SceneColor: return SpawnSceneColorNode();
		case MaterialNodeFunction::SceneAlbedo: return SpawnSceneAlbedoNode();
		case MaterialNodeFunction::ScenePosition: return SpawnScenePositionNode();
		case MaterialNodeFunction::SceneDepth: return SpawnSceneDepthNode();
		case MaterialNodeFunction::SceneLinearDepth: return SpawnSceneLinearDepthNode();
		case MaterialNodeFunction::SceneNormal: return SpawnSceneNormalNode();
		case MaterialNodeFunction::SceneEmissive: return SpawnSceneEmissiveNode();
		case MaterialNodeFunction::SceneRoughness: return SpawnSceneRoughnessNode();
		case MaterialNodeFunction::SceneMetallic: return SpawnSceneMetallicNode();
		case MaterialNodeFunction::SceneSpecular: return SpawnSceneSpecularNode();
		case MaterialNodeFunction::SceneAmbientOcclusion: return SpawnSceneAmbientOcclusionNode();
		case MaterialNodeFunction::SceneEntityID: return SpawnSceneEntityIDNode();
		case MaterialNodeFunction::SceneSubmeshIndex: return SpawnSceneSubmeshIndexNode();
		case MaterialNodeFunction::Add: return SpawnAddNode();
		case MaterialNodeFunction::Subtract: return SpawnSubtractNode();
		case MaterialNodeFunction::Multiply: return SpawnMultiplyNode();
		case MaterialNodeFunction::Divide: return SpawnDivideNode();
		case MaterialNodeFunction::Pi: return SpawnPiNode();
		case MaterialNodeFunction::Infinity: return SpawnInfinityNode();
		case MaterialNodeFunction::Euler: return SpawnEulerNode();
		case MaterialNodeFunction::Tau: return SpawnTauNode();
		case MaterialNodeFunction::Abs: return SpawnAbsNode();
		case MaterialNodeFunction::Length: return SpawnLengthNode();
		case MaterialNodeFunction::Distance: return SpawnDistanceNode();
		case MaterialNodeFunction::Radians: return SpawnRadiansNode();
		case MaterialNodeFunction::Degrees: return SpawnDegreesNode();
		case MaterialNodeFunction::Sine: return SpawnSineNode();
		case MaterialNodeFunction::Cosine: return SpawnCosineNode();
		case MaterialNodeFunction::Tangent: return SpawnTangentNode();
		case MaterialNodeFunction::ArcSine: return SpawnArcSineNode();
		case MaterialNodeFunction::ArcCosine: return SpawnArcCosineNode();
		case MaterialNodeFunction::ArcTangent: return SpawnArcTangentNode();
		case MaterialNodeFunction::HyperbolicSine: return SpawnHyperbolicSineNode();
		case MaterialNodeFunction::HyperbolicCosine: return SpawnHyperbolicCosineNode();
		case MaterialNodeFunction::HyperbolicTangent: return SpawnHyperbolicTangentNode();
		case MaterialNodeFunction::ArcHyperbolicSine: return SpawnArcHyperbolicSineNode();
		case MaterialNodeFunction::ArcHyperbolicCosine: return SpawnArcHyperbolicCosineNode();
		case MaterialNodeFunction::ArcHyperbolicTangent: return SpawnArcHyperbolicTangentNode();
		case MaterialNodeFunction::Ceil: return SpawnCeilNode();
		case MaterialNodeFunction::Floor: return SpawnFloorNode();
		case MaterialNodeFunction::Clamp: return SpawnClampNode();
		case MaterialNodeFunction::Truncate: return SpawnTruncateNode();
		case MaterialNodeFunction::SquareRoot: return SpawnSquareRootNode();
		case MaterialNodeFunction::InverseSquareRoot: return SpawnInverseSquareRootNode();
		case MaterialNodeFunction::CrossProduct: return SpawnCrossProductNode();
		case MaterialNodeFunction::DotProduct: return SpawnDotProductNode();
		case MaterialNodeFunction::Reflect: return SpawnReflectVectorNode();
		case MaterialNodeFunction::Refract: return SpawnRefractVectorNode();
		case MaterialNodeFunction::Min: return SpawnMinNode();
		case MaterialNodeFunction::Max: return SpawnMaxNode();
		case MaterialNodeFunction::Normalize: return SpawnNormalizeNode();
		case MaterialNodeFunction::FMod: return SpawnFModNode();
		case MaterialNodeFunction::Fract: return SpawnFractNode();
		case MaterialNodeFunction::Step: return SpawnStepNode();
		case MaterialNodeFunction::SmoothStep: return SpawnSmoothStepNode();
		case MaterialNodeFunction::Round: return SpawnRoundNode();
		case MaterialNodeFunction::RoundEven: return SpawnRoundEvenNode();
		case MaterialNodeFunction::Power: return SpawnPowerNode();
		case MaterialNodeFunction::Exponential: return SpawnExponentialNode();
		case MaterialNodeFunction::Exponential2: return SpawnExponential2Node();
		case MaterialNodeFunction::Log: return SpawnLogNode();
		case MaterialNodeFunction::Log2: return SpawnLog2Node();
		case MaterialNodeFunction::Sign: return SpawnSignNode();
		case MaterialNodeFunction::OneMinus: return SpawnOneMinusNode();
		case MaterialNodeFunction::Negate: return SpawnNegateNode();
		case MaterialNodeFunction::Saturate: return SpawnSaturateNode();
		case MaterialNodeFunction::Desaturate: return SpawnDesaturateNode();
		case MaterialNodeFunction::Mix: return SpawnMixNode();
		case MaterialNodeFunction::If: return SpawnIfNode();
		case MaterialNodeFunction::Switch: return SpawnSwitchNode();
		case MaterialNodeFunction::ConstructVector2: return SpawnConstructVector2Node();
		case MaterialNodeFunction::ConstructVector3: return SpawnConstructVector3Node();
		case MaterialNodeFunction::ConstructVector4: return SpawnConstructVector4Node();
		case MaterialNodeFunction::SplitVector2: return SpawnSplitVector2Node();
		case MaterialNodeFunction::SplitVector3: return SpawnSplitVector3Node();
		case MaterialNodeFunction::SplitVector4: return SpawnSplitVector4Node();
		}

		DY_CORE_ASSERT(false);
		return 0;
	}

	NodeHandle MaterialGraph::SpawnNodeFromName(const std::string& name)
	{
		return SpawnNode(GetNodeFunction(name));
	}

	bool MaterialGraph::DoesGraphContainNodeFunction(const MaterialNodeFunction function)
	{
		for (auto& [nodeID, node] : Nodes)
			if (As<Editor::MaterialNode>(node)->Function == function)
				return true;

		return false;
	}

	Ref<MaterialNode> MaterialGraph::FindNode(const NodeHandle id) const
	{
		return As<MaterialNode>(FindNodeInternal(id));
	}

	Ref<MaterialPin> MaterialGraph::FindPin(const PinHandle id) const
	{
		return As<MaterialPin>(FindPinInternal(id));
	}

	bool MaterialGraph::CanDeleteNode(NodeHandle id) const
	{
		Ref<MaterialNode> node = FindNode(id);
		return node && node->Function != MaterialNodeFunction::Result;
	}

	Ref<MaterialPin> MaterialGraph::AddPin(std::vector<Ref<EditorPin>>& target, const std::string& name, MaterialPinType type, const bool hidden /*= false*/)
	{
		Ref<MaterialPin> node = CreateRef<MaterialPin>(name, type, hidden);
		target.emplace_back(node);
		return node;
	}

	NodeHandle MaterialGraph::SpawnCommentNode(const glm::vec2 size)
	{
		Ref<MaterialNode> node = CreateNode(MaterialNodeFunction::Comment);
		EditorGraph::SetupCommentNode(node, size);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle MaterialGraph::SpawnResultNode()
	{
		// Note: Adding/alterning inputs (including order) here requires replicated changes to `MaterialResultPinType`
		Ref<MaterialNode> node = CreateNode(MaterialNodeFunction::Result);
		AddPin(node->Inputs, "Albedo", MaterialPinType::Float3)->Data.Float3 = glm::vec3(1.0f);
		AddPin(node->Inputs, "Normal", MaterialPinType::Float3);
		AddPin(node->Inputs, "Emissive", MaterialPinType::Float3);
		AddPin(node->Inputs, "Roughness", MaterialPinType::Float)->Data.Float = 0.5f;
		AddPin(node->Inputs, "Metallic", MaterialPinType::Float)->Data.Float = 0.0f;
		AddPin(node->Inputs, "Specular", MaterialPinType::Float)->Data.Float = 1.0f;
		AddPin(node->Inputs, "Ambient Occlusion", MaterialPinType::Float)->Data.Float = 1.0f;
		AddPin(node->Inputs, "Alpha", MaterialPinType::Float)->Data.Float = 1.0f;
		AddPin(node->Inputs, "IOR", MaterialPinType::Float)->Data.Float = 1.25f;
		AddPin(node->Inputs, "World Displacement", MaterialPinType::Float3)->Editable = false;
		AddPin(node->Inputs, "Tessellation Multiplier", MaterialPinType::Float)->Data.Float = 1.0f;
		AddPin(node->Inputs, "Depth Offset", MaterialPinType::Float)->Editable = false;
		AddPin(node->Inputs, "Particle Size", MaterialPinType::Float2);
		EditorGraph::BuildNode(node);

		node->Comment = "Result node of the Material.";
		node->CommentEnabled = true;
		node->CommentPinned = true;

		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle MaterialGraph::SpawnCustomExpressionNode()
	{
		Ref<MaterialNode> node = CreateNode(MaterialNodeFunction::CustomExpression);
		node->Name = "Custom Expression";
		AddPin(node->Inputs, "Include Paths", MaterialPinType::StringMultiline, true);
		AddPin(node->Inputs, "Additional Defines", MaterialPinType::StringMultiline, true);
		AddPin(node->Inputs, "Shader Expression", MaterialPinType::StringMultiline, true);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle MaterialGraph::SpawnTextureSampleNode(AssetHandle handle)
	{
		Ref<MaterialNode> node = CreateNode(MaterialNodeFunction::TextureSample);
		AddPin(node->Inputs, "UVs", MaterialPinType::Float2)->Editable = false;
		AddPin(node->Inputs, "Tex", MaterialPinType::Texture)->Data.Handle = handle;
		AddPin(node->Outputs, "RGB", MaterialPinType::Float3);
		AddPin(node->Outputs, "R", MaterialPinType::Float);
		AddPin(node->Outputs, "G", MaterialPinType::Float);
		AddPin(node->Outputs, "B", MaterialPinType::Float);
		AddPin(node->Outputs, "A", MaterialPinType::Float);
		AddPin(node->Outputs, "RGBA", MaterialPinType::Float4);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle MaterialGraph::SpawnConstantNode(MaterialPinType type)
	{
		Ref<MaterialNode> node = CreateNode(MaterialNodeFunction::Constant);
		AddPin(node->Inputs, "Default", type, true);
		AddPin(node->Outputs, std::string(), type);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle MaterialGraph::SpawnParameterNode(MaterialPinType type)
	{
		Ref<MaterialNode> node = CreateNode(MaterialNodeFunction::Parameter);
		node->Name = "New Parameter";
		AddPin(node->Inputs, "Default", type, true);
		AddPin(node->Outputs, std::string(), type);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle MaterialGraph::SpawnComponentMaskNode()
	{
		Ref<MaterialNode> node = CreateNode(MaterialNodeFunction::ComponentMask);
		AddPin(node->Inputs, std::string(), MaterialPinType::FloatFamily);
		AddPin(node->Inputs, "R", MaterialPinType::Bool, true);
		AddPin(node->Inputs, "G", MaterialPinType::Bool, true);
		AddPin(node->Inputs, "B", MaterialPinType::Bool, true);
		AddPin(node->Inputs, "A", MaterialPinType::Bool, true);
		AddPin(node->Outputs, std::string(), MaterialPinType::FloatFamily);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle MaterialGraph::SpawnComponentAppendNode()
	{
		Ref<MaterialNode> node = CreateNode(MaterialNodeFunction::ComponentAppend);
		AddPin(node->Inputs, "A", MaterialPinType::FloatFamily);
		AddPin(node->Inputs, "B", MaterialPinType::FloatFamily);
		AddPin(node->Outputs, std::string(), MaterialPinType::FloatFamily);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle MaterialGraph::SpawnWorldPositionNode()
	{
		return SpawnInputNode(MaterialNodeFunction::WorldPosition, MaterialPinType::Float3);
	}

	NodeHandle MaterialGraph::SpawnWorldNormalNode()
	{
		return SpawnInputNode(MaterialNodeFunction::WorldNormal, MaterialPinType::Float3);
	}

	NodeHandle MaterialGraph::SpawnVertexColorNode()
	{
		return SpawnInputNode(MaterialNodeFunction::VertexColor, MaterialPinType::Float4);
	}

	NodeHandle MaterialGraph::SpawnVertexDepthNode()
	{
		return SpawnInputNode(MaterialNodeFunction::VertexDepth, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnVertexLinearDepthNode()
	{
		return SpawnInputNode(MaterialNodeFunction::VertexLinearDepth, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnTextureCoordinatesNode()
	{
		return SpawnInputNode(MaterialNodeFunction::TextureCoordinates, MaterialPinType::Float2);
	}

	NodeHandle MaterialGraph::SpawnObjectPositionNode()
	{
		return SpawnInputNode(MaterialNodeFunction::ObjectPosition, MaterialPinType::Float3);
	}

	NodeHandle MaterialGraph::SpawnEntityIDNode()
	{
		return SpawnInputNode(MaterialNodeFunction::EntityID, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnSubmeshIndexNode()
	{
		return SpawnInputNode(MaterialNodeFunction::SubmeshIndex, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnVertexIndexNode()
	{
		return SpawnInputNode(MaterialNodeFunction::VertexIndex, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnCameraPositionNode()
	{
		return SpawnInputNode(MaterialNodeFunction::CameraPosition, MaterialPinType::Float3);
	}

	NodeHandle MaterialGraph::SpawnCameraDirectionNode()
	{
		return SpawnInputNode(MaterialNodeFunction::CameraDirection, MaterialPinType::Float3);
	}

	
	NodeHandle MaterialGraph::SpawnCameraForwardNode()
	{
		return SpawnInputNode(MaterialNodeFunction::CameraForward, MaterialPinType::Float3);
	}

	NodeHandle MaterialGraph::SpawnCameraRightNode()
	{
		return SpawnInputNode(MaterialNodeFunction::CameraRight, MaterialPinType::Float3);
	}

	NodeHandle MaterialGraph::SpawnCameraUpNode()
	{
		return SpawnInputNode(MaterialNodeFunction::CameraUp, MaterialPinType::Float3);
	}

	NodeHandle MaterialGraph::SpawnCameraNearNode()
	{
		return SpawnInputNode(MaterialNodeFunction::CameraNear, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnCameraFarNode()
	{
		return SpawnInputNode(MaterialNodeFunction::CameraFar, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnPixelPositionNode()
	{
		return SpawnInputNode(MaterialNodeFunction::PixelPosition, MaterialPinType::Float2);
	}

	NodeHandle MaterialGraph::SpawnViewSizeNode()
	{
		return SpawnInputNode(MaterialNodeFunction::ViewSize, MaterialPinType::Float2);
	}

	NodeHandle MaterialGraph::SpawnTimeNode()
	{
		return SpawnInputNode(MaterialNodeFunction::Time, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnParticleLifetimeNode()
	{
		return SpawnInputNode(MaterialNodeFunction::ParticleLifetime, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnParticleLifeRemainingNode()
	{
		return SpawnInputNode(MaterialNodeFunction::ParticleLifeRemaining, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnParticleLifeWeightNode()
	{
		return SpawnInputNode(MaterialNodeFunction::ParticleLifeWeight, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnParticleVelocityNode()
	{
		return SpawnInputNode(MaterialNodeFunction::ParticleVelocity, MaterialPinType::Float3);
	}

	NodeHandle MaterialGraph::SpawnSceneColorNode()
	{
		return SpawnInputSamplerNode(MaterialNodeFunction::SceneColor, MaterialPinType::Float4);
	}

	NodeHandle MaterialGraph::SpawnSceneAlbedoNode()
	{
		return SpawnInputSamplerNode(MaterialNodeFunction::SceneAlbedo, MaterialPinType::Float4);
	}

	NodeHandle MaterialGraph::SpawnScenePositionNode()
	{
		return SpawnInputSamplerNode(MaterialNodeFunction::ScenePosition, MaterialPinType::Float3);
	}

	NodeHandle MaterialGraph::SpawnSceneDepthNode()
	{
		return SpawnInputSamplerNode(MaterialNodeFunction::SceneDepth, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnSceneLinearDepthNode()
	{
		return SpawnInputSamplerNode(MaterialNodeFunction::SceneLinearDepth, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnSceneNormalNode()
	{
		return SpawnInputSamplerNode(MaterialNodeFunction::SceneNormal, MaterialPinType::Float3);
	}

	NodeHandle MaterialGraph::SpawnSceneEmissiveNode()
	{
		return SpawnInputSamplerNode(MaterialNodeFunction::SceneEmissive, MaterialPinType::Float3);
	}

	NodeHandle MaterialGraph::SpawnSceneRoughnessNode()
	{
		return SpawnInputSamplerNode(MaterialNodeFunction::SceneRoughness, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnSceneMetallicNode()
	{
		return SpawnInputSamplerNode(MaterialNodeFunction::SceneMetallic, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnSceneSpecularNode()
	{
		return SpawnInputSamplerNode(MaterialNodeFunction::SceneSpecular, MaterialPinType::Float3);
	}

	NodeHandle MaterialGraph::SpawnSceneAmbientOcclusionNode()
	{
		return SpawnInputSamplerNode(MaterialNodeFunction::SceneAmbientOcclusion, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnSceneEntityIDNode()
	{
		return SpawnInputSamplerNode(MaterialNodeFunction::SceneEntityID, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnSceneSubmeshIndexNode()
	{
		return SpawnInputSamplerNode(MaterialNodeFunction::SceneSubmeshIndex, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnAddNode()
	{
		return SpawnDoubleMathNode(MaterialNodeFunction::Add);
	}

	NodeHandle MaterialGraph::SpawnSubtractNode()
	{
		return SpawnDoubleMathNode(MaterialNodeFunction::Subtract);
	}

	NodeHandle MaterialGraph::SpawnMultiplyNode()
	{
		return SpawnDoubleMathNode(MaterialNodeFunction::Multiply);
	}

	NodeHandle MaterialGraph::SpawnDivideNode()
	{
		return SpawnDoubleMathNode(MaterialNodeFunction::Divide);
	}

	NodeHandle MaterialGraph::SpawnPiNode()
	{
		return SpawnInputNode(MaterialNodeFunction::Pi, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnInfinityNode()
	{
		return SpawnInputNode(MaterialNodeFunction::Infinity, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnEulerNode()
	{
		return SpawnInputNode(MaterialNodeFunction::Euler, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnTauNode()
	{
		return SpawnInputNode(MaterialNodeFunction::Tau, MaterialPinType::Float);
	}

	NodeHandle MaterialGraph::SpawnAbsNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::Abs);
	}

	NodeHandle MaterialGraph::SpawnLengthNode()
	{
		Ref<MaterialNode> node = CreateNode(MaterialNodeFunction::Length);
		AddPin(node->Inputs, std::string(), MaterialPinType::FloatFamily);
		AddPin(node->Outputs, std::string(), MaterialPinType::Float);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle MaterialGraph::SpawnDistanceNode()
	{
		Ref<MaterialNode> node = CreateNode(MaterialNodeFunction::Distance);
		AddPin(node->Inputs, "A", MaterialPinType::FloatFamily);
		AddPin(node->Inputs, "B", MaterialPinType::FloatFamily);
		AddPin(node->Outputs, std::string(), MaterialPinType::Float);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle MaterialGraph::SpawnRadiansNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::Radians);
	}

	NodeHandle MaterialGraph::SpawnDegreesNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::Degrees);
	}

	NodeHandle MaterialGraph::SpawnSineNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::Sine);
	}

	NodeHandle MaterialGraph::SpawnCosineNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::Cosine);
	}

	NodeHandle MaterialGraph::SpawnTangentNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::Tangent);
	}

	NodeHandle MaterialGraph::SpawnArcSineNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::ArcSine);
	}

	NodeHandle MaterialGraph::SpawnArcCosineNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::ArcCosine);
	}

	NodeHandle MaterialGraph::SpawnArcTangentNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::ArcTangent);
	}

	NodeHandle MaterialGraph::SpawnHyperbolicSineNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::HyperbolicSine);
	}

	NodeHandle MaterialGraph::SpawnHyperbolicCosineNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::HyperbolicCosine);
	}

	NodeHandle MaterialGraph::SpawnHyperbolicTangentNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::HyperbolicTangent);
	}

	NodeHandle MaterialGraph::SpawnArcHyperbolicSineNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::ArcHyperbolicSine);
	}

	NodeHandle MaterialGraph::SpawnArcHyperbolicCosineNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::ArcHyperbolicCosine);
	}

	NodeHandle MaterialGraph::SpawnArcHyperbolicTangentNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::ArcHyperbolicTangent);
	}

	NodeHandle MaterialGraph::SpawnCeilNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::Ceil);
	}

	NodeHandle MaterialGraph::SpawnFloorNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::Floor);
	}

	NodeHandle MaterialGraph::SpawnClampNode()
	{
		Ref<MaterialNode> node = CreateNode(MaterialNodeFunction::Clamp);
		AddPin(node->Inputs, std::string(), MaterialPinType::FloatFamily);
		AddPin(node->Inputs, "Min", MaterialPinType::FloatFamily);
		AddPin(node->Inputs, "Max", MaterialPinType::FloatFamily);
		AddPin(node->Outputs, std::string(), MaterialPinType::FloatFamily);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle MaterialGraph::SpawnTruncateNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::Truncate);
	}

	NodeHandle MaterialGraph::SpawnSquareRootNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::SquareRoot);
	}

	NodeHandle MaterialGraph::SpawnInverseSquareRootNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::InverseSquareRoot);
	}

	NodeHandle MaterialGraph::SpawnCrossProductNode()
	{
		Ref<MaterialNode> node = CreateNode(MaterialNodeFunction::CrossProduct);
		AddPin(node->Inputs, "A", MaterialPinType::Float3);
		AddPin(node->Inputs, "B", MaterialPinType::Float3);
		AddPin(node->Outputs, std::string(), MaterialPinType::Float3);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle MaterialGraph::SpawnDotProductNode()
	{
		Ref<MaterialNode> node = CreateNode(MaterialNodeFunction::DotProduct);
		AddPin(node->Inputs, "A", MaterialPinType::FloatFamily);
		AddPin(node->Inputs, "B", MaterialPinType::FloatFamily);
		AddPin(node->Outputs, std::string(), MaterialPinType::Float);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle MaterialGraph::SpawnReflectVectorNode()
	{
		return SpawnDoubleMathNode(MaterialNodeFunction::Reflect, "Incident", "Normal");
	}

	NodeHandle MaterialGraph::SpawnRefractVectorNode()
	{
		Ref<MaterialNode> node = CreateNode(MaterialNodeFunction::Refract);
		AddPin(node->Inputs, "Incident", MaterialPinType::FloatFamily);
		AddPin(node->Inputs, "Normal", MaterialPinType::FloatFamily);
		AddPin(node->Inputs, "IOR", MaterialPinType::Float);
		AddPin(node->Outputs, std::string(), MaterialPinType::FloatFamily);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle MaterialGraph::SpawnMinNode()
	{
		return SpawnDoubleMathNode(MaterialNodeFunction::Min);
	}

	NodeHandle MaterialGraph::SpawnMaxNode()
	{
		return SpawnDoubleMathNode(MaterialNodeFunction::Max);
	}

	NodeHandle MaterialGraph::SpawnNormalizeNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::Normalize);
	}

	NodeHandle MaterialGraph::SpawnFModNode()
	{
		return SpawnDoubleMathNode(MaterialNodeFunction::FMod);
	}

	NodeHandle MaterialGraph::SpawnFractNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::Fract);
	}

	NodeHandle MaterialGraph::SpawnStepNode()
	{
		return SpawnDoubleMathNode(MaterialNodeFunction::Step);
	}

	NodeHandle MaterialGraph::SpawnSmoothStepNode()
	{
		return SpawnDoubleMathNode(MaterialNodeFunction::SmoothStep);
	}

	NodeHandle MaterialGraph::SpawnRoundNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::Round);
	}

	NodeHandle MaterialGraph::SpawnRoundEvenNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::RoundEven);
	}

	NodeHandle MaterialGraph::SpawnPowerNode()
	{
		return SpawnDoubleMathNode(MaterialNodeFunction::Power);
	}

	NodeHandle MaterialGraph::SpawnExponentialNode()
	{
		return SpawnDoubleMathNode(MaterialNodeFunction::Exponential);
	}

	NodeHandle MaterialGraph::SpawnExponential2Node()
	{
		return SpawnDoubleMathNode(MaterialNodeFunction::Exponential2);
	}

	NodeHandle MaterialGraph::SpawnLogNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::Log);
	}

	NodeHandle MaterialGraph::SpawnLog2Node()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::Log2);
	}

	NodeHandle MaterialGraph::SpawnSignNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::Sign);
	}

	NodeHandle MaterialGraph::SpawnOneMinusNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::OneMinus);
	}

	NodeHandle MaterialGraph::SpawnNegateNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::Negate);
	}

	NodeHandle MaterialGraph::SpawnSaturateNode()
	{
		return SpawnSingleMathNode(MaterialNodeFunction::Saturate);
	}

	NodeHandle MaterialGraph::SpawnDesaturateNode()
	{
		return SpawnDoubleMathNode(MaterialNodeFunction::Desaturate);
	}

	NodeHandle MaterialGraph::SpawnMixNode()
	{
		Ref<MaterialNode> node = CreateNode(MaterialNodeFunction::Mix);
		AddPin(node->Inputs, "A", MaterialPinType::FloatFamily);
		AddPin(node->Inputs, "B", MaterialPinType::FloatFamily);
		AddPin(node->Inputs, "Factor", MaterialPinType::Float);
		AddPin(node->Outputs, std::string(), MaterialPinType::FloatFamily);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle MaterialGraph::SpawnIfNode()
	{
		Ref<MaterialNode> node = CreateNode(MaterialNodeFunction::If);
		AddPin(node->Inputs, "A", MaterialPinType::Float);
		AddPin(node->Inputs, "B", MaterialPinType::Float);
		AddPin(node->Inputs, "A > B", MaterialPinType::FloatFamily);
		AddPin(node->Inputs, "A == B", MaterialPinType::FloatFamily);
		AddPin(node->Inputs, "A < B", MaterialPinType::FloatFamily);
		AddPin(node->Outputs, std::string(), MaterialPinType::FloatFamily);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle MaterialGraph::SpawnSwitchNode()
	{
		Ref<MaterialNode> node = CreateNode(MaterialNodeFunction::Switch);
		AddPin(node->Inputs, "True", MaterialPinType::FloatFamily);
		AddPin(node->Inputs, "False", MaterialPinType::FloatFamily);
		AddPin(node->Inputs, "Condition", MaterialPinType::Bool);
		AddPin(node->Outputs, std::string(), MaterialPinType::FloatFamily);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle MaterialGraph::SpawnConstructVector2Node()
	{
		return SpawnComponentNode(MaterialNodeFunction::ConstructVector2, MaterialPinType::Float2, 2, true);
	}

	NodeHandle MaterialGraph::SpawnConstructVector3Node()
	{
		return SpawnComponentNode(MaterialNodeFunction::ConstructVector3, MaterialPinType::Float3, 3, true);
	}

	NodeHandle MaterialGraph::SpawnConstructVector4Node()
	{
		return SpawnComponentNode(MaterialNodeFunction::ConstructVector4, MaterialPinType::Float4, 4, true);
	}

	NodeHandle MaterialGraph::SpawnSplitVector2Node()
	{
		return SpawnComponentNode(MaterialNodeFunction::SplitVector2, MaterialPinType::Float2, 2, false);
	}

	NodeHandle MaterialGraph::SpawnSplitVector3Node()
	{
		return SpawnComponentNode(MaterialNodeFunction::SplitVector3, MaterialPinType::Float3, 3, false);
	}

	NodeHandle MaterialGraph::SpawnSplitVector4Node()
	{
		return SpawnComponentNode(MaterialNodeFunction::SplitVector4, MaterialPinType::Float4, 4, false);
	}

	NodeHandle MaterialGraph::SpawnInputNode(const MaterialNodeFunction function, const MaterialPinType type)
	{
		Ref<MaterialNode> node = CreateNode(function);
		AddPin(node->Outputs, std::string(), type);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle MaterialGraph::SpawnInputSamplerNode(const MaterialNodeFunction function, const MaterialPinType type)
	{
		Ref<MaterialNode> node = CreateNode(function);
		AddPin(node->Inputs, "UVs", MaterialPinType::Float2);
		AddPin(node->Outputs, std::string(), type);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle MaterialGraph::SpawnSingleMathNode(const MaterialNodeFunction function)
	{
		Ref<MaterialNode> node = CreateNode(function);
		AddPin(node->Inputs, std::string(), MaterialPinType::FloatFamily);
		AddPin(node->Outputs, std::string(), MaterialPinType::FloatFamily);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	NodeHandle MaterialGraph::SpawnDoubleMathNode(const MaterialNodeFunction function)
	{
		return SpawnDoubleMathNode(function, "A", "B");
	}

	NodeHandle MaterialGraph::SpawnDoubleMathNode(const MaterialNodeFunction function, const std::string& a, const std::string& b)
	{
		Ref<MaterialNode> node = CreateNode(function);
		AddPin(node->Inputs, a, MaterialPinType::FloatFamily);
		AddPin(node->Inputs, b, MaterialPinType::FloatFamily);
		AddPin(node->Outputs, std::string(), MaterialPinType::FloatFamily);
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

	bool MaterialGraph::PerformLinkCheck(Ref<EditorPin> a, Ref<EditorPin> b)
	{
		return true;
	}

	bool MaterialGraph::CanAddPin(Ref<EditorNode> node) const
	{
		const MaterialNodeFunction function = As<MaterialNode>(node)->Function;
		return IsOperatorNode(function);
	}

	void MaterialGraph::OnAddPin(Ref<EditorNode> internalNode)
	{
		const Ref<MaterialNode> node = As<MaterialNode>(internalNode);
		
		if (!IsOperatorNode(node->Function))
			return;

		AddPin(node->Inputs, std::string(1, 'A' + node->Inputs.size()), MaterialPinType::FloatFamily);
		BuildNode(node);
	}

	bool MaterialGraph::AreTypesCompatible(MaterialPinType a, MaterialPinType b)
	{
		// All float/vector types are compatible with each other.
		if ((a == MaterialPinType::FloatFamily || a == MaterialPinType::Float || a == MaterialPinType::Float2 || a == MaterialPinType::Float3 || a == MaterialPinType::Float4) &&
			(b == MaterialPinType::FloatFamily || b == MaterialPinType::Float || b == MaterialPinType::Float2 || b == MaterialPinType::Float3 || b == MaterialPinType::Float4))
			return true;

		// All other types must match
		return a == b;
	}

	bool MaterialGraph::AreTypesCompatible(PinHandle a, PinHandle b) const
	{
		Ref<MaterialPin> pinA = FindPin(a);
		Ref<MaterialPin> pinB = FindPin(b);

		if (pinA && pinB)
			return AreTypesCompatible(pinA->Type, pinB->Type);

		return false;
	}

	bool MaterialGraph::CanCreateLink(Ref<MaterialPin> a, Ref<MaterialPin> b)
	{
		if (a->Kind == PinKind::Input)
			std::swap(a, b);

		if (!a || !b || a == b || a->Kind == b->Kind || !AreTypesCompatible(a->Type, b->Type) || a->Node == b->Node)
			return false;

		return true;
	}

	bool MaterialGraph::CanCreateLink(PinHandle a, PinHandle b) const
	{
		Ref<MaterialPin> pinA = FindPin(a);
		Ref<MaterialPin> pinB = FindPin(b);

		if (pinA && pinB)
			return CanCreateLink(pinA, pinB);

		return false;
	}

	Ref<EditorPin> MaterialGraph::CreateNewPin(const std::string& name)
	{
		return CreateRef<MaterialPin>(name, MaterialPinType::Float, false);
	}

	NodeHandle MaterialGraph::SpawnComponentNode(const MaterialNodeFunction function, const MaterialPinType type, const uint8_t componentCount, const bool componentInputs)
	{
		Ref<MaterialNode> node = CreateNode(function);
		
		constexpr char* components[] = { "X", "Y", "Z", "W" };
		for (uint8_t componentIndex = 0; componentIndex < componentCount; componentIndex++)
			AddPin(componentInputs ? node->Inputs : node->Outputs, components[componentIndex], MaterialPinType::Float);

		AddPin(componentInputs ? node->Outputs : node->Inputs, std::string(), type)->Editable = false;
		EditorGraph::BuildNode(node);
		Nodes[node->ID] = node;
		return node->ID;
	}

}