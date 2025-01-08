#include "dypch.h"
#include "Dymatic/Asset/Serializers/EntityRegistrySerializer.h"

#include "Dymatic/Scene/Entity.h"
#include "Dymatic/Scripting/ScriptEngine.h"
#include "Dymatic/Project/Project.h"

#include <queue>

#include "Dymatic/Utils/TypeTraits.h"

namespace Dymatic {

	#define WRITE_SCRIPT_FIELD(FieldType, Type)        \
			case ScriptFieldType::FieldType:           \
				out << scriptField.GetValue<Type>();   \
				break

#define READ_SCRIPT_FIELD(FieldType, Type)             \
	case ScriptFieldType::FieldType:                   \
	{                                                  \
		Type data = scriptField["Data"].as<Type>();    \
		fieldInstance.SetValue(data);                  \
		break;                                         \
	}

	// Type and Enum Stringification Utilities
	namespace Utils {

		static const char* CameraProjectionTypeToString(const SceneCamera::ProjectionType type)
		{
			switch (type)
			{
			case SceneCamera::ProjectionType::Perspective:	return "Perspective";
			case SceneCamera::ProjectionType::Orthographic:	return "Orthographic";
			}
			 
			DY_CORE_ASSERT(false, "Unknown camera projection type");
			return {};
		}

		static SceneCamera::ProjectionType CameraProjectionTypeFromString(const std::string& typeString)
		{
			if (typeString == "Perspective")	return SceneCamera::ProjectionType::Perspective;
			if (typeString == "Orthographic")	return SceneCamera::ProjectionType::Orthographic;

			DY_CORE_ASSERT(false, "Unknown camera projection type");
			return SceneCamera::ProjectionType::Perspective;
		}

		static const char* RigidBody2DBodyTypeToString(RigidBody2DComponent::BodyType bodyType)
		{
			switch (bodyType)
			{
			case RigidBody2DComponent::BodyType::Static:    return "Static";
			case RigidBody2DComponent::BodyType::Dynamic:   return "Dynamic";
			case RigidBody2DComponent::BodyType::Kinematic: return "Kinematic";
			}

			DY_CORE_ASSERT(false, "Unknown body type");
			return {};
		}

		static RigidBody2DComponent::BodyType RigidBody2DBodyTypeFromString(const std::string& bodyTypeString)
		{
			if (bodyTypeString == "Static")    return RigidBody2DComponent::BodyType::Static;
			if (bodyTypeString == "Dynamic")   return RigidBody2DComponent::BodyType::Dynamic;
			if (bodyTypeString == "Kinematic") return RigidBody2DComponent::BodyType::Kinematic;

			DY_CORE_ASSERT(false, "Unknown body type");
			return RigidBody2DComponent::BodyType::Static;
		}

		static const char* SkyLightComponentTypeToString(const SkyLightComponent::SkyType type)
		{
			switch (type)
			{
			case SkyLightComponent::SkyType::EnvironmentMap:	return "Environment Map";
			case SkyLightComponent::SkyType::DynamicSky:	return "Dynamic Sky";
			}

			DY_CORE_ASSERT(false, "Unknown sky light component type");
			return {};
		}

		static SkyLightComponent::SkyType SkyLightComponentTypeFromString(const std::string& typeString)
		{
			if (typeString == "Environment Map")	return SkyLightComponent::SkyType::EnvironmentMap;
			if (typeString == "Dynamic Sky")		return SkyLightComponent::SkyType::DynamicSky;

			DY_CORE_ASSERT(false, "Unknown sky light component type");
			return SkyLightComponent::SkyType::EnvironmentMap;
		}

		static const char* SplineTypeToString(SplineComponent::SplineType type)
		{
			switch (type)
			{
			case SplineComponent::SplineType::Curve:		return "Curve";
			case SplineComponent::SplineType::Linear:		return "Linear";
			case SplineComponent::SplineType::Constant:		return "Constant";
			}

			DY_CORE_ASSERT(false, "Unknown spline type");
			return {};
		}

		static SplineComponent::SplineType SplineTypeFromString(const std::string& typeString)
		{
			if (typeString == "Curve")		return SplineComponent::SplineType::Curve;
			if (typeString == "Linear")		return SplineComponent::SplineType::Linear;
			if (typeString == "Constant")	return SplineComponent::SplineType::Constant;

			DY_CORE_ASSERT(false, "Unknown spline type");
			return SplineComponent::SplineType::Curve;
		}

		static const char* RigidBodyBodyTypeToString(RigidBodyComponent::BodyType bodyType)
		{
			switch (bodyType)
			{
			case RigidBodyComponent::BodyType::Static:		return "Static";
			case RigidBodyComponent::BodyType::Dynamic:		return "Dynamic";
			case RigidBodyComponent::BodyType::Kinematic:	return "Kinematic";
			}

			DY_CORE_ASSERT(false, "Unknown body type");
			return {};
		}

		static RigidBodyComponent::BodyType RigidBodyBodyTypeFromString(const std::string& bodyTypeString)
		{
			if (bodyTypeString == "Static")		return RigidBodyComponent::BodyType::Static;
			if (bodyTypeString == "Dynamic")	return RigidBodyComponent::BodyType::Dynamic;
			if (bodyTypeString == "Kinematic")	return RigidBodyComponent::BodyType::Kinematic;

			DY_CORE_ASSERT(false, "Unknown body type");
			return RigidBodyComponent::BodyType::Static;
		}

		static const char* RigidBodyModeToString(RigidBodyComponent::MassMode mode)
		{
			switch (mode)
			{
			case RigidBodyComponent::MassMode::Density:		return "Density";
			case RigidBodyComponent::MassMode::Mass:		return "Mass";
			}

			DY_CORE_ASSERT(false, "Unknown body mode");
			return {};
		}

		static RigidBodyComponent::MassMode RigidBodyModeFromString(const std::string& modeString)
		{
			if (modeString == "Density")	return RigidBodyComponent::MassMode::Density;
			if (modeString == "Mass")		return RigidBodyComponent::MassMode::Mass;

			DY_CORE_ASSERT(false, "Unknown body mode");
			return RigidBodyComponent::MassMode::Density;
		}

		static const char* MeshColliderMeshTypeToString(MeshColliderComponent::MeshType meshType)
		{
			switch (meshType)
			{
			case MeshColliderComponent::MeshType::Triangle:    return "Triangle";
			case MeshColliderComponent::MeshType::Convex:   return "Convex";
			}

			DY_CORE_ASSERT(false, "Unknown mesh type");
			return {};
		}

		static MeshColliderComponent::MeshType MeshColliderMeshTypeFromString(const std::string& meshTypeString)
		{
			if (meshTypeString == "Triangle")	return MeshColliderComponent::MeshType::Triangle;
			if (meshTypeString == "Convex")		return MeshColliderComponent::MeshType::Convex;

			DY_CORE_ASSERT(false, "Unknown mesh type");
			return MeshColliderComponent::MeshType::Triangle;
		}

		static const char* TextAlignmentTypeToString(TextAlignment textAlignment)
		{
			switch (textAlignment)
			{
			case TextAlignment::Left:    return "Left";
			case TextAlignment::Center:  return "Center";
			case TextAlignment::Right:   return "Right";
			case TextAlignment::Justify: return "Justify";
			}

			DY_CORE_ASSERT(false, "Unknown text alignment type");
			return {};
		}

		static TextAlignment TextAlignmentTypeFromString(const std::string& textAlignmentString)
		{
			if (textAlignmentString == "Left")		return TextAlignment::Left;
			if (textAlignmentString == "Center")	return TextAlignment::Center;
			if (textAlignmentString == "Right")		return TextAlignment::Right;
			if (textAlignmentString == "Justify")	return TextAlignment::Justify;

			DY_CORE_ASSERT(false, "Unknown text alignment type");
			return TextAlignment::Left;
		}

		static const char* FieldTypeToString(FieldComponent::FieldType type)
		{
			switch (type)
			{
			case FieldComponent::FieldType::Directional: return "Directional";
			case FieldComponent::FieldType::Radial: return "Radial";
			case FieldComponent::FieldType::Buoyancy: return "Buoyancy";
			}

			DY_CORE_ASSERT(false, "Unknown field component type");
			return {};
		}

		static FieldComponent::FieldType FieldTypeFromString(const std::string& typeString)
		{
			if (typeString == "Directional") return FieldComponent::FieldType::Directional;
			if (typeString == "Radial") return FieldComponent::FieldType::Radial;
			if (typeString == "Buoyancy") return FieldComponent::FieldType::Buoyancy;

			DY_CORE_ASSERT(false, "Unknown field component type");
			return FieldComponent::FieldType::Directional;
		}

		static const char* CaptureMaskTypeToString(SceneRendererContext::RenderMaskType type)
		{
			switch (type)
			{
			case SceneRendererContext::RenderMaskType::None:		return "None";
			case SceneRendererContext::RenderMaskType::Exclusive:	return "Exclusive";
			case SceneRendererContext::RenderMaskType::Inclusive:	return "Inclusive";
			}

			DY_CORE_ASSERT(false, "Unknown capture component mask type");
			return {};
		}

		static SceneRendererContext::RenderMaskType CaptureMaskTypeFromString(const std::string& typeString)
		{
			if (typeString == "None")		return SceneRendererContext::RenderMaskType::None;
			if (typeString == "Exclusive")	return SceneRendererContext::RenderMaskType::Exclusive;
			if (typeString == "Inclusive")	return SceneRendererContext::RenderMaskType::Inclusive;

			DY_CORE_ASSERT(false, "Unknown capture component mask type");
			return SceneRendererContext::RenderMaskType::None;
		}

		static const char* VolumeBlendTypeToString(const VolumeComponent::BlendType type)
		{
			switch (type)
			{
			case VolumeComponent::BlendType::Set:	return "Set";
			case VolumeComponent::BlendType::Add:	return "Add";
			}

			DY_CORE_ASSERT(false, "Unknown volume component blend type");
			return {};
		}

		static VolumeComponent::BlendType VolumeBlendTypeFromString(const std::string& typeString)
		{
			if (typeString == "Set")	return VolumeComponent::BlendType::Set;
			if (typeString == "Add")	return VolumeComponent::BlendType::Add;

			DY_CORE_ASSERT(false, "Unknown volume component blend type");
			return VolumeComponent::BlendType::Set;
		}

		static const char* ConstraintSpaceToString(const ConstraintSpace space)
		{
			switch (space)
			{
			case ConstraintSpace::LocalSpace:	return "Local Space";
			case ConstraintSpace::WorldSpace:	return "World Space";
			case ConstraintSpace::Automatic:	return "Automatic";
			}

			DY_CORE_ASSERT(false, "Unknown constraint space");
			return {};
		}

		static ConstraintSpace ConstraintSpaceFromString(const std::string& spaceString)
		{
			if (spaceString == "Local Space")	return ConstraintSpace::LocalSpace;
			if (spaceString == "World Space")	return ConstraintSpace::WorldSpace;
			if (spaceString == "Automatic")		return ConstraintSpace::Automatic;

			DY_CORE_ASSERT(false, "Unknown constraint space");
			return ConstraintSpace::LocalSpace;
		}

		static void SerializeComponentSpace(YAML::Emitter& out, const ConstraintSpace space)
		{
			out << YAML::Key << "Space" << YAML::Value << ConstraintSpaceToString(space);
		}

		static void DeserializeConstraintSpace(const YAML::Node& node, ConstraintSpace& space)
		{
			if (auto spaceNode = node["Space"])
				space = Utils::ConstraintSpaceFromString(spaceNode.as<std::string>());
		}

		static const char* ConstraintSwingTypeToString(const ConstraintSwingType type)
		{
			switch (type)
			{
			case ConstraintSwingType::Cone:		return "Cone";
			case ConstraintSwingType::Pyramid:	return "Pyramid";
			}

			DY_CORE_ASSERT(false, "Unknown constraint swing type");
			return {};
		}

		static ConstraintSwingType ConstraintSwingTypeFromString(const std::string& typeString)
		{
			if (typeString == "Cone")		return ConstraintSwingType::Cone;
			if (typeString == "Pyramid")	return ConstraintSwingType::Pyramid;

			DY_CORE_ASSERT(false, "Unknown constraint swing type");
			return ConstraintSwingType::Cone;
		}

		static const char* DistanceConstraintTypeToString(const DistanceConstraintComponent::DistanceType type)
		{
			switch (type)
			{
			case DistanceConstraintComponent::DistanceType::Default:	return "Default";
			case DistanceConstraintComponent::DistanceType::Fixed:		return "Fixed";
			case DistanceConstraintComponent::DistanceType::Range:		return "Range";
			}

			DY_CORE_ASSERT(false, "Unknown distance constraint type");
			return {};
		}

		static DistanceConstraintComponent::DistanceType DistanceConstraintTypeFromString(const std::string& typeString)
		{
			if (typeString == "Default")	return DistanceConstraintComponent::DistanceType::Default;
			if (typeString == "Fixed")		return DistanceConstraintComponent::DistanceType::Fixed;
			if (typeString == "Range")		return DistanceConstraintComponent::DistanceType::Range;

			DY_CORE_ASSERT(false, "Unknown distance constraint type");
			return DistanceConstraintComponent::DistanceType::Default;
		}

		static const char* SpringConstraintTypeToString(const SpringConstraintComponent::SpringType type)
		{
			switch (type)
			{
			case SpringConstraintComponent::SpringType::FrequencyAndDamping:	return "Frequency And Damping";
			case SpringConstraintComponent::SpringType::StiffnessAndDamping:	return "Stiffness And Damping";
			}

			DY_CORE_ASSERT(false, "Unknown spring constraint type");
			return {};
		}

		static SpringConstraintComponent::SpringType SpringConstraintTypeFromString(const std::string& typeString)
		{
			if (typeString == "Frequency And Damping")	return SpringConstraintComponent::SpringType::FrequencyAndDamping;
			if (typeString == "Stiffness And Damping")	return SpringConstraintComponent::SpringType::StiffnessAndDamping;

			DY_CORE_ASSERT(false, "Unknown distance constraint type");
			return SpringConstraintComponent::SpringType::FrequencyAndDamping;
		}

		static const char* RackAndPinionConstraintModeToString(const RackAndPinionConstraintComponent::RatioMode mode)
		{
			switch (mode)
			{
			case RackAndPinionConstraintComponent::RatioMode::Properties:	return "Properties";
			case RackAndPinionConstraintComponent::RatioMode::Ratio:		return "Ratio";
			}

			DY_CORE_ASSERT(false, "Unknown rack and pinion constraint ratio mode");
			return {};
		}

		static RackAndPinionConstraintComponent::RatioMode RackAndPinionConstraintModeFromString(const std::string& modeString)
		{
			if (modeString == "Properties")	return RackAndPinionConstraintComponent::RatioMode::Properties;
			if (modeString == "Ratio")		return RackAndPinionConstraintComponent::RatioMode::Ratio;

			DY_CORE_ASSERT(false, "Unknown rack and pinion constraint ratio mode");
			return RackAndPinionConstraintComponent::RatioMode::Properties;
		}

		static const char* SixDOFConstraintAxisToString(const SixDOFConstraintComponent::Axis axis)
		{
			switch (axis)
			{
			case SixDOFConstraintComponent::Axis::TranslationX:		return "Translation X";
			case SixDOFConstraintComponent::Axis::TranslationY:		return "Translation Y";
			case SixDOFConstraintComponent::Axis::TranslationZ:		return "Translation Z";
			case SixDOFConstraintComponent::Axis::RotationX:		return "Rotation X";
			case SixDOFConstraintComponent::Axis::RotationY:		return "Rotation Y";
			case SixDOFConstraintComponent::Axis::RotationZ:		return "Rotation Z";
			}

			DY_CORE_ASSERT(false, "Unknown six DOF constraint axis");
			return {};
		}

		static SixDOFConstraintComponent::Axis SixDOFConstraintAxisFromString(const std::string& axisString)
		{
			if (axisString == "Translation X")		return SixDOFConstraintComponent::Axis::TranslationX;
			if (axisString == "Translation Y")		return SixDOFConstraintComponent::Axis::TranslationY;
			if (axisString == "Translation Z")		return SixDOFConstraintComponent::Axis::TranslationZ;
			if (axisString == "Rotation X")			return SixDOFConstraintComponent::Axis::RotationX;
			if (axisString == "Rotation Y")			return SixDOFConstraintComponent::Axis::RotationY;
			if (axisString == "Rotation Z")			return SixDOFConstraintComponent::Axis::RotationZ;

			DY_CORE_ASSERT(false, "Unknown six DOF constraint axis");
			return SixDOFConstraintComponent::Axis::AxisCount;
		}

		static const char* SixDOFConstraintAxisStatusToString(const SixDOFConstraintComponent::AxisStatus status)
		{
			switch (status)
			{
			case SixDOFConstraintComponent::AxisStatus::Free:	return "Free";
			case SixDOFConstraintComponent::AxisStatus::Locked:	return "Locked";
			case SixDOFConstraintComponent::AxisStatus::Custom:	return "Custom";
			}

			DY_CORE_ASSERT(false, "Unknown six DOF constraint axis status");
			return {};
		}

		static SixDOFConstraintComponent::AxisStatus SixDOFConstraintAxisStatusFromString(const std::string& statusString)
		{
			if (statusString == "Free")		return SixDOFConstraintComponent::AxisStatus::Free;
			if (statusString == "Locked")	return SixDOFConstraintComponent::AxisStatus::Locked;
			if (statusString == "Custom")	return SixDOFConstraintComponent::AxisStatus::Custom;

			DY_CORE_ASSERT(false, "Unknown six DOF constraint axis status");
			return SixDOFConstraintComponent::AxisStatus::Free;
		}

		static const char* FollowConstraintRotationTypeToString(const FollowConstraintComponent::RotationConstraintType type)
		{
			switch (type)
			{
			case FollowConstraintComponent::RotationConstraintType::Free:			return "Free";
			case FollowConstraintComponent::RotationConstraintType::AroundTangent:	return "Around Tangent";
			case FollowConstraintComponent::RotationConstraintType::AroundNormal:	return "Around Normal";
			case FollowConstraintComponent::RotationConstraintType::AroundBinormal:	return "Around Binormal";
			case FollowConstraintComponent::RotationConstraintType::ToPath:			return "To Path";
			case FollowConstraintComponent::RotationConstraintType::Constrained:	return "Constrained";
			}

			DY_CORE_ASSERT(false, "Unknown follow constraint rotation constraint type");
			return {};
		}

		static FollowConstraintComponent::RotationConstraintType FollowConstraintRotationTypeFromString(const std::string& typeString)
		{
			if (typeString == "Free")				return FollowConstraintComponent::RotationConstraintType::Free;
			if (typeString == "Around Tangent")		return FollowConstraintComponent::RotationConstraintType::AroundTangent;
			if (typeString == "Around Normal")		return FollowConstraintComponent::RotationConstraintType::AroundNormal;
			if (typeString == "Around Binormal")	return FollowConstraintComponent::RotationConstraintType::AroundBinormal;
			if (typeString == "To Path")			return FollowConstraintComponent::RotationConstraintType::ToPath;
			if (typeString == "Constrained")		return FollowConstraintComponent::RotationConstraintType::Constrained;

			DY_CORE_ASSERT(false, "Unknown follow constraint rotation constraint type");
			return FollowConstraintComponent::RotationConstraintType::Free;
		}

	}

	// Generic Component Utilities
	namespace Utils {
	
		static void SerializeConstraintComponent(YAML::Emitter& out, const ConstraintComponentBase* component)
		{
			out << YAML::Key << "Target" << YAML::Value << component->Target;
			out << YAML::Key << "Enabled" << YAML::Value << component->Enabled;
		}

		static void SerializeConstraintComponent(const ConstraintComponentBase* component, FileStreamWriter& stream)
		{
			stream.WriteRaw<EntityHandle>(component->Target);
			stream.WriteRaw<bool>(component->Enabled);
		}

		static void DeserializeConstraintComponent(const YAML::Node& node, ConstraintComponentBase* component)
		{
			if (auto targetNode = node["Target"])
				component->Target = targetNode.as<EntityHandle>();

			if (auto enabledNode = node["Enabled"])
				component->Enabled = enabledNode.as<bool>();
		}

		static void DeserializeConstraintComponent(FileStreamReader& stream, ConstraintComponentBase* component)
		{
			stream.ReadRaw<EntityHandle>(component->Target);
			stream.ReadRaw<bool>(component->Enabled);
		}

		template<typename ComponentType>
		static void SerializeReferenceFrame(YAML::Emitter& out, const ComponentType& component, const std::function<void(const decltype(component.LocalReferenceFrame)&)>& callback)
		{
			out << YAML::Key << "Local Reference Frame" << YAML::Value << YAML::BeginMap;
			callback(component.LocalReferenceFrame);
			out << YAML::EndMap;

			out << YAML::Key << "Target Reference Frame" << YAML::Value << YAML::BeginMap;
			callback(component.TargetReferenceFrame);
			out << YAML::EndMap;
		}

		template<typename ComponentType>
		static void DeserializeReferenceFrame(const YAML::Node& node, ComponentType& component, const std::function<void(const YAML::Node&, decltype(component.LocalReferenceFrame)&)>& callback)
		{
			if (auto localReferenceFrame = node["Local Reference Frame"])
				callback(localReferenceFrame, component.LocalReferenceFrame);

			if (auto targetReferenceFrame = node["Target Reference Frame"])
				callback(targetReferenceFrame, component.TargetReferenceFrame);
		}

		static void SerializeAxis(YAML::Emitter& out, const char* name, const glm::vec3& axis)
		{
			out << YAML::Key << name << YAML::Value << glm::vec3(
				axis.x == c_AxisMarker ? 1.0f : axis.x,
				axis.y == c_AxisMarker ? 1.0f : axis.y,
				axis.z == c_AxisMarker ? 1.0f : axis.z
			);
		}

		static void SerializeAxis(const glm::vec3& axis, FileStreamWriter& stream)
		{
			stream.WriteRaw<glm::vec3>(glm::normalize(glm::vec3(
				axis.x == c_AxisMarker ? 1.0f : axis.x,
				axis.y == c_AxisMarker ? 1.0f : axis.y,
				axis.z == c_AxisMarker ? 1.0f : axis.z
			)));
		}

		static void DeserializeAxis(const YAML::Node& node, glm::vec3& axis)
		{
			if (!node)
				return;

			axis = node.as<glm::vec3>();

			axis.x = (axis.x == 1.0f) ? c_AxisMarker : axis.x;
			axis.y = (axis.y == 1.0f) ? c_AxisMarker : axis.y;
			axis.z = (axis.z == 1.0f) ? c_AxisMarker : axis.z;
		}

		static void DeserializeAxis(FileStreamReader& stream, glm::vec3& axis)
		{
			stream.ReadRaw<glm::vec3>(axis);
		}

		template<typename T>
		static bool DeserializeOptional(const YAML::Node& node, const char* name, T& value)
		{
			if (!node)
				return false;

			try
			{
				if (auto optionalNode = node[name])
				{
					value = optionalNode.as<T>();
					return true;
				}
			}
			catch (const YAML::ParserException& e)
			{
				DY_CORE_ERROR("Failed to deserialize optional field '{}'. Error: {}", name, e.msg);
			}

			return false;
		}

		template <typename Callback>
		static bool DeserializeOptionalCallback(const YAML::Node& node, const char* name, Callback&& callback)
		{
			if (!node)
				return false;

			try
			{
				if (auto optionalNode = node[name])
				{
					using T = typename FunctionTraits<Callback>::template Argument<0>::Type;
					callback(optionalNode.as<T>());
					return true;
				}
			}
			catch (const YAML::ParserException& e)
			{
				DY_CORE_ERROR("Failed to deserialize optional field '{}'. Error: {}", name, e.msg);
			}

			return false;
		}

		template<typename T>
		static T DeserializeOptionalDefault(const YAML::Node& node, const char* name, T defaultValue = {})
		{
			DeserializeOptional(node, name, defaultValue);
			return defaultValue;
		}

		template<typename T>
		static T DeserializeOptional(const YAML::Node& node, const char* name)
		{
			return DeserializeOptionalDefault<T>(node, name);
		}

		template<typename T>
		static bool DeserializeOptionalEnum(const YAML::Node& node, const char* name, T& value, T (*converter)(const std::string&))
		{
			std::string valueString;
			if (!DeserializeOptional<std::string>(node, name, valueString))
				return false;

			value = converter(valueString);
			return true;
		}

		template<typename T>
		static bool DeserializeOptionalAsset(const YAML::Node& node, const char* name, Ref<T>& asset)
		{
			AssetHandle handle;
			if (!DeserializeOptional(node, name, handle))
				return false;

			asset = AssetManager::GetAsset<T>(handle);
			
			return (bool)asset;
		}
	}

	void EntityRegistrySerializer::SerializeRegistry(YAML::Emitter& out, Ref<EntityRegistry> scene)
	{
		// Gather all Entity's at the root of the scene (i.e. without a parent)
		scene->m_Registry.each([&](auto entityID)
		{
			Entity entity = { entityID, scene.get() };
			if (!entity || entity.HasComponent<SceneComponent>())
				return;

			SerializeEntity(out, entity);
		});
	}

	void EntityRegistrySerializer::SerializeRegistry(Ref<EntityRegistry> scene, FileStreamWriter& stream)
	{
		uint32_t entityCount = 0;
		scene->m_Registry.each([&](auto entityID)
		{
			Entity entity = { entityID, scene.get() };
			if (!entity || entity.HasComponent<SceneComponent>())
				return;

			entityCount++;
		});

		stream.WriteRaw<uint32_t>(entityCount);

		scene->m_Registry.each([&](auto entityID)
		{
			Entity entity = { entityID, scene.get() };
			if (!entity || entity.HasComponent<SceneComponent>())
				return;

			SerializeEntity(entity, stream);
		});
	}

	void EntityRegistrySerializer::DeserializeRegistry(YAML::Node entities, Ref<EntityRegistry> scene)
	{
		for (auto entity : entities)
			DeserializeEntity(entity, scene);
	}

	void EntityRegistrySerializer::DeserializeRegistry(FileStreamReader& stream, Ref<EntityRegistry> scene)
	{
		uint32_t entityCount;
		stream.ReadRaw<uint32_t>(entityCount);

		for (uint32_t i = 0; i < entityCount; i++)
			DeserializeEntity(stream, scene);
	}

	void EntityRegistrySerializer::SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		DY_CORE_ASSERT(entity.HasComponent<IDComponent>());

		out << YAML::BeginMap; // Entity
		out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();

		if (entity.HasComponent<TagComponent>())
		{
			out << YAML::Key << "TagComponent";
			out << YAML::BeginMap; // TagComponent

			auto& tag = entity.GetComponent<TagComponent>().Tag;
			out << YAML::Key << "Tag" << YAML::Value << tag;

			out << YAML::EndMap; // TagComponent
		}

		if (entity.HasComponent<RelationshipComponent>())
		{
			auto& rc = entity.GetComponent<RelationshipComponent>();
			out << YAML::Key << "Parent" << YAML::Value << rc.ParentHandle;

			out << YAML::Key << "Children";
			out << YAML::Value << YAML::BeginSeq; // Children
			
			for (auto& child : rc.Children)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Handle" << YAML::Value << child;
				out << YAML::EndMap;
			}
			
			out << YAML::EndSeq; // Children
		}

		if (entity.HasComponent<PrefabComponent>())
		{
			out << YAML::Key << "PrefabComponent";
			out << YAML::BeginMap; // PrefabComponent

			auto& pc = entity.GetComponent<PrefabComponent>();
			out << YAML::Key << "Prefab" << YAML::Value << pc.PrefabID;

			out << YAML::EndMap; // PrefabComponent
		}

		if (entity.HasComponent<AttachmentComponent>())
		{
			out << YAML::Key << "AttachmentComponent";
			out << YAML::BeginMap; // AttachmentComponent

			auto& ac = entity.GetComponent<AttachmentComponent>();
			out << YAML::Key << "Bone Name" << YAML::Value << ac.BoneName;

			out << YAML::EndMap; // AttachmentComponent
		}

		if (entity.HasComponent<FolderComponent>())
		{
			out << YAML::Key << "FolderComponent";
			out << YAML::BeginMap; // FolderComponent

			auto& fc = entity.GetComponent<FolderComponent>();
			out << YAML::Key << "Color" << YAML::Value << fc.Color;
			
			out << YAML::EndMap; // FolderComponent
		}

		if (entity.HasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap; // TransformComponent


			const auto& transform = entity.GetComponent<TransformComponent>().Transform;
			out << YAML::Key << "Translation" << YAML::Value << transform.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << transform.GetRotationDegrees();
			out << YAML::Key << "Scale" << YAML::Value << transform.Scale;

			out << YAML::EndMap; // TransformComponent
		}

		if (entity.HasComponent<CameraComponent>())
		{
			out << YAML::Key << "CameraComponent";
			out << YAML::BeginMap; // CameraComponent

			auto& cameraComponent = entity.GetComponent<CameraComponent>();
			auto& camera = cameraComponent.Camera;
			auto& settings = camera.GetCameraSettings();

			out << YAML::Key << "Camera" << YAML::Value;
			out << YAML::BeginMap; // Camera
			out << YAML::Key << "ProjectionType" << YAML::Value << Utils::CameraProjectionTypeToString(camera.GetProjectionType());
			out << YAML::Key << "PerspectiveFOV" << YAML::Value << camera.GetPerspectiveVerticalFOV();
			out << YAML::Key << "PerspectiveNear" << YAML::Value << camera.GetPerspectiveNearClip();
			out << YAML::Key << "PerspectiveFar" << YAML::Value << camera.GetPerspectiveFarClip();
			out << YAML::Key << "OrthographicSize" << YAML::Value << camera.GetOrthographicSize();
			out << YAML::Key << "OrthographicNear" << YAML::Value << camera.GetOrthographicNearClip();
			out << YAML::Key << "OrthographicFar" << YAML::Value << camera.GetOrthographicFarClip();
			out << YAML::EndMap; // Camera

			out << YAML::Key << "Primary" << YAML::Value << cameraComponent.Primary;
			out << YAML::Key << "FixedAspectRatio" << YAML::Value << cameraComponent.FixedAspectRatio;

			{
				out << YAML::Key << "Settings" << YAML::Value;
				out << YAML::BeginMap; // Settings

				out << YAML::Key << "Bloom Threshold" << YAML::Value << settings.BloomThreshold;

				if (settings.BloomDirtTexture)
					out << YAML::Key << "Bloom Dirt Texture" << YAML::Value << settings.BloomDirtTexture->Handle;

				out << YAML::Key << "DOFStrength" << YAML::Value << settings.DOFStrength;
				out << YAML::Key << "DOFTarget" << YAML::Value << settings.DOFTarget;
				out << YAML::Key << "DOFFocusRange" << YAML::Value << settings.DOFFocusRange;
				out << YAML::Key << "DOFFocusFalloff" << YAML::Value << settings.DOFFocusFalloff;

				if (settings.LUT)
					out << YAML::Key << "LUT" << YAML::Value << settings.LUT->Handle;

				out << YAML::EndMap; // Settings
			}

			out << YAML::EndMap; // CameraComponent
		}

		if (entity.HasComponent<CaptureComponent>())
		{
			auto& captureComponent = entity.GetComponent<CaptureComponent>();

			out << YAML::Key << "CaptureComponent";
			out << YAML::BeginMap; // CaptureComponent

			out << YAML::Key << "Capture" << YAML::Value << captureComponent.Capture;
			out << YAML::Key << "Cumulative" << YAML::Value << captureComponent.Cumulative;
			out << YAML::Key << "Target" << YAML::Value << (captureComponent.Target ? captureComponent.Target->Handle : 0);
			out << YAML::Key << "Type" << YAML::Value << SceneRendererContext::RenderVisualizationModeToString(captureComponent.Type);
			out << YAML::Key << "Mask" << YAML::Value << Utils::CaptureMaskTypeToString(captureComponent.MaskType);

			out << YAML::EndMap; // CaptureComponent
		}

		if (entity.HasComponent<ScriptComponent>())
		{
			auto& scriptComponent = entity.GetComponent<ScriptComponent>();

			out << YAML::Key << "ScriptComponent";
			out << YAML::BeginMap; // ScriptComponent
			out << YAML::Key << "ClassName" << YAML::Value << scriptComponent.ClassName;

			// Fields
			Ref<ScriptClass> entityClass = ScriptEngine::GetEntityClass(scriptComponent.ClassName);
			const auto& fields = entityClass->GetFields();
			if (fields.size() > 0)
			{
				out << YAML::Key << "ScriptFields" << YAML::Value;
				auto& entityFields = ScriptEngine::GetScriptFieldMap(entity);
				out << YAML::BeginSeq;
				for (const auto& [name, field] : fields)
				{
					if (entityFields.find(name) == entityFields.end())
						continue;

					out << YAML::BeginMap; // ScriptField
					out << YAML::Key << "Name" << YAML::Value << name;
					out << YAML::Key << "Type" << YAML::Value << Utils::ScriptFieldTypeToString(field.Type);

					out << YAML::Key << "Data" << YAML::Value;
					ScriptFieldInstance& scriptField = entityFields.at(name);

					switch (field.Type)
					{
						WRITE_SCRIPT_FIELD(Float, float);
						WRITE_SCRIPT_FIELD(Double, double);
						WRITE_SCRIPT_FIELD(Bool, bool);
						WRITE_SCRIPT_FIELD(Char, char);
						WRITE_SCRIPT_FIELD(Byte, int8_t);
						WRITE_SCRIPT_FIELD(Short, int16_t);
						WRITE_SCRIPT_FIELD(Int, int32_t);
						WRITE_SCRIPT_FIELD(Long, int64_t);
						WRITE_SCRIPT_FIELD(UShort, uint16_t);
						WRITE_SCRIPT_FIELD(UInt, uint32_t);
						WRITE_SCRIPT_FIELD(ULong, uint64_t);
						WRITE_SCRIPT_FIELD(Vector2, glm::vec2);
						WRITE_SCRIPT_FIELD(Vector3, glm::vec3);
						WRITE_SCRIPT_FIELD(Vector4, glm::vec4);
						WRITE_SCRIPT_FIELD(Entity, UUID);
						WRITE_SCRIPT_FIELD(Asset, UUID);
						WRITE_SCRIPT_FIELD(Scene, UUID);
						WRITE_SCRIPT_FIELD(Texture, UUID);
						WRITE_SCRIPT_FIELD(VirtualTexture, UUID);
						WRITE_SCRIPT_FIELD(Mesh, UUID);
						WRITE_SCRIPT_FIELD(Animation, UUID);
						WRITE_SCRIPT_FIELD(Material, UUID);
						WRITE_SCRIPT_FIELD(Audio, UUID);
						WRITE_SCRIPT_FIELD(VideoPlayer, UUID);
					}
					out << YAML::EndMap; // ScriptFields
				}
				out << YAML::EndSeq;
			}

			out << YAML::EndMap; // ScriptComponent
		}

		if (entity.HasComponent<SpriteRendererComponent>())
		{
			out << YAML::Key << "SpriteRendererComponent";
			out << YAML::BeginMap; // SpriteRendererComponent

			auto& spriteRendererComponent = entity.GetComponent<SpriteRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << spriteRendererComponent.Color;
			if (spriteRendererComponent.Texture)
				out << YAML::Key << "Texture" << YAML::Value << spriteRendererComponent.Texture->Handle;

			out << YAML::Key << "TilingFactor" << YAML::Value << spriteRendererComponent.TilingFactor;

			out << YAML::EndMap; // SpriteRendererComponent
		}

		if (entity.HasComponent<CircleRendererComponent>())
		{
			out << YAML::Key << "CircleRendererComponent";
			out << YAML::BeginMap; // CircleRendererComponent

			auto& circleRendererComponent = entity.GetComponent<CircleRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << circleRendererComponent.Color;
			out << YAML::Key << "Thickness" << YAML::Value << circleRendererComponent.Thickness;
			out << YAML::Key << "Fade" << YAML::Value << circleRendererComponent.Fade;

			out << YAML::EndMap; // CircleRendererComponent
		}

		if (entity.HasComponent<TextComponent>())
		{
			out << YAML::Key << "TextComponent";
			out << YAML::BeginMap; // TextComponent

			auto& textComponent = entity.GetComponent<TextComponent>();
			out << YAML::Key << "TextString" << YAML::Value << textComponent.TextString;
			out << YAML::Key << "Alignment" << YAML::Value << Utils::TextAlignmentTypeToString(textComponent.Alignment);
			out << YAML::Key << "Color" << YAML::Value << textComponent.Color;
			out << YAML::Key << "Font" << YAML::Value << (textComponent.Font ? textComponent.Font->Handle : 0);
			out << YAML::Key << "Kerning" << YAML::Value << textComponent.Kerning;
			out << YAML::Key << "LineSpacing" << YAML::Value << textComponent.LineSpacing;
			out << YAML::Key << "MaxWidth" << YAML::Value << textComponent.MaxWidth;

			out << YAML::EndMap; // TextComponent
		}

		if (entity.HasComponent<ParticleSystemComponent>())
		{
			out << YAML::Key << "ParticleSystemComponent";
			out << YAML::BeginMap; // ParticleSystemComponent

			const auto& particleSystemComponent = entity.GetComponent<ParticleSystemComponent>();
			const auto& player = particleSystemComponent.Player;
			const Ref<ParticleSystem> particleSystem = player ? player->GetParticleSystem() : nullptr;

			if (particleSystem)
				out << YAML::Key << "Particle System" << YAML::Value << particleSystem->Handle;

			if (particleSystemComponent.Material && (particleSystemComponent.Material->Handle != particleSystem->GetMaterialHandle()))
				out << YAML::Key << "Material" << YAML::Value << particleSystemComponent.Material->Handle;

			out << YAML::EndMap; // ParticleSystemComponent
		}

		if (entity.HasComponent<RigidBody2DComponent>())
		{
			out << YAML::Key << "Rigidbody2DComponent";
			out << YAML::BeginMap; // Rigidbody2DComponent

			auto& rb2dComponent = entity.GetComponent<RigidBody2DComponent>();
			out << YAML::Key << "BodyType" << YAML::Value << Utils::RigidBody2DBodyTypeToString(rb2dComponent.Type);
			out << YAML::Key << "FixedRotation" << YAML::Value << rb2dComponent.FixedRotation;

			out << YAML::EndMap; // Rigidbody2DComponent
		}

		if (entity.HasComponent<BoxCollider2DComponent>())
		{
			out << YAML::Key << "BoxCollider2DComponent";
			out << YAML::BeginMap; // BoxCollider2DComponent

			auto& bc2dComponent = entity.GetComponent<BoxCollider2DComponent>();
			out << YAML::Key << "Offset" << YAML::Value << bc2dComponent.Offset;
			out << YAML::Key << "Size" << YAML::Value << bc2dComponent.Size;
			out << YAML::Key << "Density" << YAML::Value << bc2dComponent.Density;
			out << YAML::Key << "Friction" << YAML::Value << bc2dComponent.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << bc2dComponent.Restitution;
			out << YAML::Key << "RestitutionThreshold" << YAML::Value << bc2dComponent.RestitutionThreshold;

			out << YAML::EndMap; // BoxCollider2DComponent
		}

		if (entity.HasComponent<CircleCollider2DComponent>())
		{
			out << YAML::Key << "CircleCollider2DComponent";
			out << YAML::BeginMap; // CircleCollider2DComponent

			auto& cc2dComponent = entity.GetComponent<CircleCollider2DComponent>();
			out << YAML::Key << "Offset" << YAML::Value << cc2dComponent.Offset;
			out << YAML::Key << "Radius" << YAML::Value << cc2dComponent.Radius;
			out << YAML::Key << "Density" << YAML::Value << cc2dComponent.Density;
			out << YAML::Key << "Friction" << YAML::Value << cc2dComponent.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << cc2dComponent.Restitution;
			out << YAML::Key << "RestitutionThreshold" << YAML::Value << cc2dComponent.RestitutionThreshold;

			out << YAML::EndMap; // CircleCollider2DComponent
		}
		
		if (entity.HasComponent<StaticMeshComponent>())
		{
			out << YAML::Key << "StaticMeshComponent";
			out << YAML::BeginMap; // StaticMeshComponent

			auto& staticMeshComponent = entity.GetComponent<StaticMeshComponent>();
			if (staticMeshComponent.m_Model)
			{
				out << YAML::Key << "Mesh" << YAML::Value << staticMeshComponent.m_Model->Handle;

				Ref<AnimationGraphPlayer> animationPlayer = staticMeshComponent.GetAnimationPlayer();
				if (animationPlayer && animationPlayer->GetAnimationGraph())
				{
					const AssetHandle animationGraphHandle = animationPlayer->GetAnimationGraph()->Handle;
					if (animationGraphHandle)
						out << YAML::Key << "Animation Graph" << YAML::Value << animationGraphHandle;
				}

				out << YAML::Key << "Materials" << YAML::Value << YAML::BeginSeq;
				for (auto& material : staticMeshComponent.m_Materials)
					out << YAML::Value << (material ? material->Handle : 0);
				out << YAML::EndSeq;
			}

			out << YAML::EndMap; // StaticMeshComponent
		}

		if (entity.HasComponent<DirectionalLightComponent>())
		{
			out << YAML::Key << "DirectionalLightComponent";
			out << YAML::BeginMap; // DirectionalLightComponent

			auto& directionalLightComponent = entity.GetComponent<DirectionalLightComponent>();
			out << YAML::Key << "Color" << YAML::Value << directionalLightComponent.Color;
			out << YAML::Key << "Intensity" << YAML::Value << directionalLightComponent.Intensity;

			out << YAML::EndMap; // DirectionalLightComponent
		}

		if (entity.HasComponent<PointLightComponent>())
		{
			out << YAML::Key << "PointLightComponent";
			out << YAML::BeginMap; // PointLightComponent

			auto& pointLightComponent = entity.GetComponent<PointLightComponent>();
			out << YAML::Key << "Color" << YAML::Value << pointLightComponent.Color;
			out << YAML::Key << "Intensity" << YAML::Value << pointLightComponent.Intensity;
			out << YAML::Key << "Radius" << YAML::Value << pointLightComponent.Radius;
			out << YAML::Key << "Casts Shadows" << YAML::Value << pointLightComponent.CastsShadows;

			out << YAML::EndMap; // PointLightComponent
		}

		if (entity.HasComponent<SkyLightComponent>())
		{
			out << YAML::Key << "SkyLightComponent";
			out << YAML::BeginMap; // SkyLightComponent

			auto& skyLightComponent = entity.GetComponent<SkyLightComponent>();
			out << YAML::Key << "Type" << YAML::Value << Utils::SkyLightComponentTypeToString(skyLightComponent.Type);
			
			if (skyLightComponent.Type == SkyLightComponent::SkyType::EnvironmentMap)
			{
				if (skyLightComponent.EnvironmentMap)
					out << YAML::Key << "Environment Map" << YAML::Value << skyLightComponent.EnvironmentMap->Handle;
				
				if (skyLightComponent.FlowMap)
					out << YAML::Key << "Flow Map" << YAML::Value << skyLightComponent.FlowMap->Handle;
			}
			
			out << YAML::Key << "Intensity" << YAML::Value << skyLightComponent.Intensity;

			out << YAML::EndMap; // SkyLightComponent
		}

		if (entity.HasComponent<DecalComponent>())
		{
			out << YAML::Key << "DecalComponent";
			out << YAML::BeginMap; // DecalComponent

			const auto& decalComponent = entity.GetComponent<DecalComponent>();
			out << YAML::Key << "Constrain Angle" << YAML::Value << decalComponent.ConstrainAngle;

			if (decalComponent.Texture)
				out << YAML::Key << "Texture" << YAML::Value << decalComponent.Texture->Handle;

			out << YAML::EndMap; // DecalComponent
		}

		if (entity.HasComponent<PostProcessVolumeComponent>())
		{
			out << YAML::Key << "PostProcessVolumeComponent";
			out << YAML::BeginMap; // PostProcessVolumeComponent

			const auto& postProcessVolumeComponent = entity.GetComponent<PostProcessVolumeComponent>();
			out << YAML::Key << "Bounded" << YAML::Value << postProcessVolumeComponent.Bounded;
			out << YAML::Key << "Enabled" << YAML::Value << postProcessVolumeComponent.Enabled;

			if (postProcessVolumeComponent.Material)
				out << YAML::Key << "Material" << YAML::Value << postProcessVolumeComponent.Material->Handle;

			out << YAML::EndMap; // PostProcessVolumeComponent
		}

		if (entity.HasComponent<VolumeComponent>())
		{
			out << YAML::Key << "VolumeComponent";
			out << YAML::BeginMap; // VolumeComponent

			const auto& volumeComponent = entity.GetComponent<VolumeComponent>();
			out << YAML::Key << "Blend" << YAML::Value << Utils::VolumeBlendTypeToString(volumeComponent.Blend);
			out << YAML::Key << "Color" << YAML::Value << volumeComponent.Color;
			out << YAML::Key << "Extinction Scale" << YAML::Value << volumeComponent.ExtinctionScale;
			out << YAML::Key << "Scattering Distribution" << YAML::Value << volumeComponent.ScatteringDistribution;
			out << YAML::Key << "Scattering Intensity" << YAML::Value << volumeComponent.ScatteringIntensity;

			out << YAML::EndMap; // VolumeComponent
		}

		if (entity.HasComponent<AudioComponent>())
		{
			out << YAML::Key << "AudioComponent";
			out << YAML::BeginMap; // AudioComponent

			auto& audioComponent = entity.GetComponent<AudioComponent>();
			if (audioComponent.AudioSound)
			{
				out << YAML::Key << "Sound" << YAML::Value << audioComponent.AudioSound->Handle;

				// Sound Parameters
				out << YAML::Key << "StartPosition" << YAML::Value << audioComponent.StartPosition;
				out << YAML::Key << "StartOnAwake" << YAML::Value << audioComponent.StartOnAwake;
				out << YAML::Key << "3D" << YAML::Value << audioComponent.AudioSound->Is3D();
				out << YAML::Key << "Looping" << YAML::Value << audioComponent.AudioSound->IsLooping();
				out << YAML::Key << "Volume" << YAML::Value << audioComponent.AudioSound->GetVolume();
				out << YAML::Key << "Pan" << YAML::Value << audioComponent.AudioSound->GetPan();
				out << YAML::Key << "Speed" << YAML::Value << audioComponent.AudioSound->GetSpeed();
				out << YAML::Key << "Radius" << YAML::Value << audioComponent.AudioSound->GetRadius();
				out << YAML::Key << "Echo" << YAML::Value << audioComponent.AudioSound->GetEcho();
			}

			out << YAML::EndMap; // AudioComponent
		}

		if (entity.HasComponent<SplineComponent>())
		{
			out << YAML::Key << "SplineComponent";
			out << YAML::BeginMap; // SplineComponent

			const auto& splineComponent = entity.GetComponent<SplineComponent>();
			out << YAML::Key << "Points" << YAML::Value;

			out << YAML::BeginSeq; // Points
			for (const auto& point : splineComponent.Points)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Position" << YAML::Value << point.Position;
				out << YAML::Key << "Tangent" << YAML::Value << point.Tangent;
				out << YAML::Key << "Type" << YAML::Value << Utils::SplineTypeToString(point.Type);
				out << YAML::EndMap;
			}
			out << YAML::EndSeq; // Points

			out << YAML::EndMap; // SplineComponent
		}

		if (entity.HasComponent<RigidBodyComponent>())
		{
			out << YAML::Key << "RigidbodyComponent";
			out << YAML::BeginMap; // RigidbodyComponent

			auto& rigidbodyComponent = entity.GetComponent<RigidBodyComponent>();
			out << YAML::Key << "BodyType" << YAML::Value << Utils::RigidBodyBodyTypeToString(rigidbodyComponent.Type);
			
			if (rigidbodyComponent.Layer)
				out << YAML::Key << "Layer" << YAML::Value << rigidbodyComponent.Layer;
			
			out << YAML::Key << "Sensor" << YAML::Value << rigidbodyComponent.Sensor;
			out << YAML::Key << "Mode" << YAML::Value << Utils::RigidBodyModeToString(rigidbodyComponent.Mode);

			if (rigidbodyComponent.Mode == RigidBodyComponent::MassMode::Mass)
				out << YAML::Key << "Mass" << YAML::Value << rigidbodyComponent.Mass;
			else
				out << YAML::Key << "Density" << YAML::Value << rigidbodyComponent.Density;

			out << YAML::Key << "Material Properties";
			out << YAML::BeginMap; // Material Properties
			out << YAML::Key << "Friction" << YAML::Value << rigidbodyComponent.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << rigidbodyComponent.Restitution;
			out << YAML::EndMap; // Material Properties

			out << YAML::EndMap; // RigidbodyComponent
		}

		if (entity.HasComponent<SoftBodyComponent>())
		{
			out << YAML::Key << "SoftBodyComponent";
			out << YAML::BeginMap; // SoftBodyComponent

			const auto& softBodyComponent = entity.GetComponent<SoftBodyComponent>();
			out << YAML::Key << "Friction" << YAML::Value << softBodyComponent.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << softBodyComponent.Restitution;
			out << YAML::Key << "Pressure" << YAML::Value << softBodyComponent.Pressure;

			out << YAML::Key << "VertexMass" << YAML::Value << softBodyComponent.VertexMass;
			out << YAML::Key << "VertexRadius" << YAML::Value << softBodyComponent.VertexRadius;
			out << YAML::Key << "UseVertexColorAsWeight" << YAML::Value << softBodyComponent.UseVertexColorAsWeight;

			out << YAML::EndMap; // SoftBodyComponent
		}

		if (entity.HasComponent<BoxColliderComponent>())
		{
			out << YAML::Key << "BoxColliderComponent";
			out << YAML::BeginMap; // BoxColliderComponent

			auto& boxColliderComponent = entity.GetComponent<BoxColliderComponent>();
			out << YAML::Key << "Size" << YAML::Value << boxColliderComponent.Size;

			out << YAML::EndMap; // BoxColliderComponent
		}

		if (entity.HasComponent<SphereColliderComponent>())
		{
			out << YAML::Key << "SphereColliderComponent";
			out << YAML::BeginMap; // SphereColliderComponent

			auto& sphereColliderComponent = entity.GetComponent<SphereColliderComponent>();
			out << YAML::Key << "Radius" << YAML::Value << sphereColliderComponent.Radius;

			out << YAML::EndMap; // SphereColliderComponent
		}

		if (entity.HasComponent<CapsuleColliderComponent>())
		{
			out << YAML::Key << "CapsuleColliderComponent";
			out << YAML::BeginMap; // CapsuleColliderComponent

			auto& capsuleColliderComponent = entity.GetComponent<CapsuleColliderComponent>();
			out << YAML::Key << "Radius" << YAML::Value << capsuleColliderComponent.Radius;
			out << YAML::Key << "HalfHeight" << YAML::Value << capsuleColliderComponent.HalfHeight;

			out << YAML::EndMap; // CapsuleColliderComponent
		}

		if (entity.HasComponent<MeshColliderComponent>())
		{
			out << YAML::Key << "MeshColliderComponent";
			out << YAML::BeginMap; // MeshColliderComponent

			auto& meshColliderComponent = entity.GetComponent<MeshColliderComponent>();
			out << YAML::Key << "MeshType" << YAML::Value << Utils::MeshColliderMeshTypeToString(meshColliderComponent.Type);

			out << YAML::EndMap; // MeshColliderComponent
		}

		if (entity.HasComponent<PointConstraintComponent>())
		{
			auto& pointConstraintComponent = entity.GetComponent<PointConstraintComponent>();
			
			out << YAML::Key << "PointConstraintComponent";
			out << YAML::BeginMap; // PointConstraintComponent

			Utils::SerializeConstraintComponent(out, &pointConstraintComponent);
			Utils::SerializeComponentSpace(out, pointConstraintComponent.Space);
			out << YAML::Key << "Local Point" << YAML::Value << pointConstraintComponent.LocalPoint;
			out << YAML::Key << "Target Point" << YAML::Value << pointConstraintComponent.TargetPoint;

			out << YAML::EndMap; // PointConstraintComponent
		}

		if (entity.HasComponent<ConeConstraintComponent>())
		{
			auto& coneConstraintComponent = entity.GetComponent<ConeConstraintComponent>();

			out << YAML::Key << "ConeConstraintComponent";
			out << YAML::BeginMap; // ConeConstraintComponent

			Utils::SerializeConstraintComponent(out, &coneConstraintComponent);
			Utils::SerializeComponentSpace(out, coneConstraintComponent.Space);
			
			Utils::SerializeReferenceFrame(out, coneConstraintComponent, [&](const ConeConstraintComponent::ReferenceFrame& frame)
			{
				out << YAML::Key << "Offset" << YAML::Value << frame.Offset;
				Utils::SerializeAxis(out, "Twist Axis", frame.TwistAxis);
			});
			
			out << YAML::Key << "Half Cone Angle" << YAML::Value << coneConstraintComponent.HalfConeAngle;

			out << YAML::EndMap; // ConeConstraintComponent
		}

		if (entity.HasComponent<DistanceConstraintComponent>())
		{
			const auto& distanceConstraintComponent = entity.GetComponent<DistanceConstraintComponent>();

			out << YAML::Key << "DistanceConstraintComponent";
			out << YAML::BeginMap; // DistanceConstraintComponent

			Utils::SerializeConstraintComponent(out, &distanceConstraintComponent);

			out << YAML::Key << "Type" << YAML::Value << Utils::DistanceConstraintTypeToString(distanceConstraintComponent.Type);

			if (distanceConstraintComponent.Type == DistanceConstraintComponent::DistanceType::Fixed)
				out << YAML::Key << "Distance" << YAML::Value << distanceConstraintComponent.Distance;
			else if (distanceConstraintComponent.Type == DistanceConstraintComponent::DistanceType::Range)
			{
				out << YAML::Key << "Min Distance" << YAML::Value << distanceConstraintComponent.MinDistance;
				out << YAML::Key << "Max Distance" << YAML::Value << distanceConstraintComponent.MaxDistance;
			}

			out << YAML::EndMap; // DistanceConstraintComponent
		}

		if (entity.HasComponent<SpringConstraintComponent>())
		{
			const auto& springConstraintComponent = entity.GetComponent<SpringConstraintComponent>();

			out << YAML::Key << "SpringConstraintComponent";
			out << YAML::BeginMap; // SpringConstraintComponent

			out << YAML::Key << "Type" << YAML::Value << Utils::SpringConstraintTypeToString(springConstraintComponent.Type);

			if (springConstraintComponent.Type == SpringConstraintComponent::SpringType::FrequencyAndDamping)
				out << YAML::Key << "Frequency" << YAML::Value << springConstraintComponent.Frequency;
			else if (springConstraintComponent.Type == SpringConstraintComponent::SpringType::StiffnessAndDamping)
				out << YAML::Key << "Stiffness" << YAML::Value << springConstraintComponent.Stiffness;

			out << YAML::Key << "Damping" << YAML::Value << springConstraintComponent.Damping;
			
			out << YAML::EndMap; // SpringConstraintComponent
		}

		if (entity.HasComponent<HingeConstraintComponent>())
		{
			const auto& hingeConstraintComponent = entity.GetComponent<HingeConstraintComponent>();

			out << YAML::Key << "HingeConstraintComponent";
			out << YAML::BeginMap; // HingeConstraintComponent

			Utils::SerializeConstraintComponent(out, &hingeConstraintComponent);
			Utils::SerializeComponentSpace(out, hingeConstraintComponent.Space);

			Utils::SerializeReferenceFrame(out, hingeConstraintComponent, [&](const HingeConstraintComponent::ReferenceFrame& frame)
			{
				out << YAML::Key << "Point" << YAML::Value << frame.Point;
				Utils::SerializeAxis(out, "Hinge Axis", frame.HingeAxis);
				Utils::SerializeAxis(out, "Normal Axis", frame.NormalAxis);
			});

			out << YAML::Key << "Min Rotation" << YAML::Value << hingeConstraintComponent.MinRotation;
			out << YAML::Key << "Max Rotation" << YAML::Value << hingeConstraintComponent.MaxRotation;
			out << YAML::Key << "Max Friction Torque" << YAML::Value << hingeConstraintComponent.MaxFrictionTorque;

			out << YAML::EndMap; // HingeConstraintComponent
		}

		if (entity.HasComponent<FixedConstraintComponent>())
		{
			const auto& fixedConstraintComponent = entity.GetComponent<FixedConstraintComponent>();

			out << YAML::Key << "FixedConstraintComponent";
			out << YAML::BeginMap; // FixedConstraintComponent

			Utils::SerializeConstraintComponent(out, &fixedConstraintComponent);
			Utils::SerializeComponentSpace(out, fixedConstraintComponent.Type);

			Utils::SerializeReferenceFrame(out, fixedConstraintComponent, [&](const FixedConstraintComponent::ReferenceFrame& frame)
			{
				out << YAML::Key << "Point" << YAML::Value << frame.Point;
				Utils::SerializeAxis(out, "Axis X", frame.AxisX);
				Utils::SerializeAxis(out, "Axis Y", frame.AxisY);
			});

			out << YAML::EndMap; // FixedConstraintComponent
		}

		if (entity.HasComponent<GearConstraintComponent>())
		{
			const auto& gearConstraintComponent = entity.GetComponent<GearConstraintComponent>();

			out << YAML::Key << "GearConstraintComponent";
			out << YAML::BeginMap; // GearConstraintComponent

			Utils::SerializeConstraintComponent(out, &gearConstraintComponent);
			Utils::SerializeComponentSpace(out, gearConstraintComponent.Space);

			Utils::SerializeAxis(out, "Local Hinge Axis", gearConstraintComponent.LocalHingeAxis);
			Utils::SerializeAxis(out, "Target Hinge Axis", gearConstraintComponent.TargetHingeAxis);

			out << YAML::Key << "Ratio" << YAML::Value << gearConstraintComponent.Ratio;

			out << YAML::EndMap; // GearConstraintComponent
		}

		if (entity.HasComponent<PulleyConstraintComponent>())
		{
			const auto& pulleyConstraintComponent = entity.GetComponent<PulleyConstraintComponent>();

			out << YAML::Key << "PulleyConstraintComponent";
			out << YAML::BeginMap; // PulleyConstraintComponent

			Utils::SerializeConstraintComponent(out, &pulleyConstraintComponent);
			Utils::SerializeComponentSpace(out, pulleyConstraintComponent.Space);
			
			Utils::SerializeReferenceFrame(out, pulleyConstraintComponent, [&](const PulleyConstraintComponent::ReferenceFrame& frame)
			{
				out << YAML::Key << "Body Point" << YAML::Value << frame.BodyPoint;
				out << YAML::Key << "Fixed Point" << YAML::Value << frame.FixedPoint;
			});

			out << YAML::Key << "Ratio" << YAML::Value << pulleyConstraintComponent.Ratio;
			out << YAML::Key << "Min Length" << YAML::Value << pulleyConstraintComponent.MinLength;
			out << YAML::Key << "Max Length" << YAML::Value << pulleyConstraintComponent.MaxLength;

			out << YAML::EndMap; // PulleyConstraintComponent
		}

		if (entity.HasComponent<RackAndPinionConstraintComponent>())
		{
			const auto& rackAndPinionConstraintComponent = entity.GetComponent<RackAndPinionConstraintComponent>();

			out << YAML::Key << "RackAndPinionConstraintComponent";
			out << YAML::BeginMap; // RackAndPinionConstraintComponent

			Utils::SerializeConstraintComponent(out, &rackAndPinionConstraintComponent);
			Utils::SerializeComponentSpace(out, rackAndPinionConstraintComponent.Space);

			out << YAML::Key << "Mode" << YAML::Value << Utils::RackAndPinionConstraintModeToString(rackAndPinionConstraintComponent.Mode);

			Utils::SerializeAxis(out, "Hinge Axis", rackAndPinionConstraintComponent.HingeAxis);
			Utils::SerializeAxis(out, "Slider Axis", rackAndPinionConstraintComponent.SliderAxis);

			if (rackAndPinionConstraintComponent.Mode == RackAndPinionConstraintComponent::RatioMode::Ratio)
				out << YAML::Key << "Ratio" << YAML::Value << rackAndPinionConstraintComponent.Ratio;
			else
			{
				out << YAML::Key << "Rack Teeth Count" << YAML::Value << rackAndPinionConstraintComponent.RackTeethCount;
				out << YAML::Key << "Pinion Teeth Count" << YAML::Value << rackAndPinionConstraintComponent.PinionTeethCount;
				out << YAML::Key << "Rack Length" << YAML::Value << rackAndPinionConstraintComponent.RackLength;
			}

			out << YAML::EndMap; // RackAndPinionConstraintComponent
		}

		if (entity.HasComponent<SwingTwistConstraintComponent>())
		{
			const auto& swingTwistConstraintComponent = entity.GetComponent<SwingTwistConstraintComponent>();

			out << YAML::Key << "SwingTwistConstraintComponent";
			out << YAML::BeginMap; // SwingTwistConstraintComponent

			Utils::SerializeConstraintComponent(out, &swingTwistConstraintComponent);
			Utils::SerializeComponentSpace(out, swingTwistConstraintComponent.Space);
			out << YAML::Key << "Swing Type" << YAML::Value << Utils::ConstraintSwingTypeToString(swingTwistConstraintComponent.SwingType);

			Utils::SerializeReferenceFrame(out, swingTwistConstraintComponent, [&](const SwingTwistConstraintComponent::ReferenceFrame& frame)
			{
				out << YAML::Key << "Position" << YAML::Value << frame.Position;
				Utils::SerializeAxis(out, "Twist Axis", frame.TwistAxis);
				Utils::SerializeAxis(out, "Plane Axis", frame.PlaneAxis);
			});

			out << YAML::Key << "Normal Half Cone Angle" << YAML::Value << swingTwistConstraintComponent.NormalHalfConeAngle;
			out << YAML::Key << "Plane Half Cone Angle" << YAML::Value << swingTwistConstraintComponent.PlaneHalfConeAngle;
			out << YAML::Key << "Twist Min Angle" << YAML::Value << swingTwistConstraintComponent.TwistMinAngle;
			out << YAML::Key << "Twist Max Angle" << YAML::Value << swingTwistConstraintComponent.TwistMaxAngle;
			out << YAML::Key << "Max Friction Torque" << YAML::Value << swingTwistConstraintComponent.MaxFrictionTorque;

			out << YAML::EndMap; // SwingTwistConstraintComponent
		}

		if (entity.HasComponent<SliderConstraintComponent>())
		{
			const auto& sliderConstraintComponent = entity.GetComponent<SliderConstraintComponent>();

			out << YAML::Key << "SliderConstraintComponent";
			out << YAML::BeginMap; // SliderConstraintComponent

			Utils::SerializeConstraintComponent(out, &sliderConstraintComponent);
			Utils::SerializeComponentSpace(out, sliderConstraintComponent.Space);

			Utils::SerializeReferenceFrame(out, sliderConstraintComponent, [&](const SliderConstraintComponent::ReferenceFrame& frame)
			{
				out << YAML::Key << "Point" << YAML::Value << frame.Point;
				Utils::SerializeAxis(out, "Slider Axis", frame.SliderAxis);
				Utils::SerializeAxis(out, "Normal Axis", frame.NormalAxis);
			});

			if (sliderConstraintComponent.SliderMin != (-SliderConstraintComponent::SliderMaxBound))
				out << YAML::Key << "Slider Min" << YAML::Value << sliderConstraintComponent.SliderMin;

			if (sliderConstraintComponent.SliderMax != SliderConstraintComponent::SliderMaxBound)
				out << YAML::Key << "Slider Max" << YAML::Value << sliderConstraintComponent.SliderMax;

			out << YAML::Key << "Max Friction Force" << YAML::Value << sliderConstraintComponent.MaxFrictionForce;

			out << YAML::EndMap; // SliderConstraintComponent
		}

		if (entity.HasComponent<SixDOFConstraintComponent>())
		{
			const auto& sixDOFConstraintComponent = entity.GetComponent<SixDOFConstraintComponent>();

			out << YAML::Key << "SixDOFConstraintComponent";
			out << YAML::BeginMap; // SixDOFConstraintComponent

			Utils::SerializeConstraintComponent(out, &sixDOFConstraintComponent);
			Utils::SerializeComponentSpace(out, sixDOFConstraintComponent.Space);

			Utils::SerializeReferenceFrame(out, sixDOFConstraintComponent, [&](const SixDOFConstraintComponent::ReferenceFrame& frame)
			{
				out << YAML::Key << "Position" << YAML::Value << frame.Position;
				Utils::SerializeAxis(out, "Axis X", frame.AxisX);
				Utils::SerializeAxis(out, "Axis Y", frame.AxisY);
			});

			out << YAML::Key << "Swing Type" << YAML::Value << Utils::ConstraintSwingTypeToString(sixDOFConstraintComponent.SwingType);

			out << YAML::Key << "Axes" << YAML::Value << YAML::BeginSeq;
			for (uint32_t axisIndex = 0; axisIndex < SixDOFConstraintComponent::Axis::AxisCount; axisIndex++)
			{
				const SixDOFConstraintComponent::Axis axis = (SixDOFConstraintComponent::Axis)axisIndex;
				const SixDOFConstraintComponent::AxisStatus status = sixDOFConstraintComponent.GetAxisStatus(axis);

				out << YAML::BeginMap;
				out << YAML::Key << "Name" << YAML::Value << Utils::SixDOFConstraintAxisToString(axis);
				out << YAML::Key << "Status" << YAML::Value << Utils::SixDOFConstraintAxisStatusToString(status);
				out << YAML::Key << "Max Friction" << YAML::Value << sixDOFConstraintComponent.MaxFriction[axisIndex];

				if (status == SixDOFConstraintComponent::AxisStatus::Custom)
				{
					out << YAML::Key << "Limit Min" << YAML::Value << sixDOFConstraintComponent.LimitMin[axisIndex];
					out << YAML::Key << "Limit Max" << YAML::Value << sixDOFConstraintComponent.LimitMax[axisIndex];
				}

				out << YAML::EndMap;
			}
			out << YAML::EndSeq; // Axes

			out << YAML::EndMap; // SixDOFConstraintComponent
		}

		if (entity.HasComponent<FollowConstraintComponent>())
		{
			const auto& followConstraintComponent = entity.GetComponent<FollowConstraintComponent>();

			out << YAML::Key << "FollowConstraintComponent";
			out << YAML::BeginMap; // FollowConstraintComponent

			Utils::SerializeConstraintComponent(out, &followConstraintComponent);

			out << YAML::Key << "Looping" << YAML::Value << followConstraintComponent.Looping;
			Utils::SerializeAxis(out, "Normal", followConstraintComponent.Normal);
			out << YAML::Key << "Start Fraction" << YAML::Value << followConstraintComponent.StartFraction;
			out << YAML::Key << "Max Friction Force" << YAML::Value << followConstraintComponent.MaxFrictionForce;
			out << YAML::Key << "Rotation Constraint" << YAML::Value << Utils::FollowConstraintRotationTypeToString(followConstraintComponent.RotationConstraint);
			
			if (followConstraintComponent.BaseTarget)
				out << YAML::Key << "Base Target" << YAML::Value << followConstraintComponent.BaseTarget;

			out << YAML::EndMap; // FollowConstraintComponent
		}

		if (entity.HasComponent<SpringArmComponent>())
		{
			auto& springArmComponent = entity.GetComponent<SpringArmComponent>();

			out << YAML::Key << "SpringArmComponent";
			out << YAML::BeginMap; // SpringArmComponent

			out << YAML::Key << "TargetLength" << YAML::Value << springArmComponent.TargetLength;
			out << YAML::Key << "ProbeRadius" << YAML::Value << springArmComponent.ProbeRadius;
			out << YAML::Key << "TargetOffset" << YAML::Value << springArmComponent.TargetOffset;
			out << YAML::Key << "SocketOffset" << YAML::Value << springArmComponent.SocketOffset;

			out << YAML::EndMap; // SpringArmComponent
		}

		if (entity.HasComponent<FieldComponent>())
		{
			auto& fieldComponent = entity.GetComponent<FieldComponent>();

			out << YAML::Key << "SpringArmComponent";
			out << YAML::BeginMap; // SpringArmComponent

			out << YAML::Key << "Type" << YAML::Value << Utils::FieldTypeToString(fieldComponent.Type);

			if (fieldComponent.Layer)
				out << YAML::Key << "Layer" << YAML::Value << fieldComponent.Layer;

			if (fieldComponent.Type == FieldComponent::FieldType::Directional)
			{
				// Directional
				out << YAML::Key << "Force" << YAML::Value << fieldComponent.Force;
			}
			else if (fieldComponent.Type == FieldComponent::FieldType::Radial)
			{
				// Radial
				out << YAML::Key << "Magnitude" << YAML::Value << fieldComponent.Magnitude;
				out << YAML::Key << "Radius" << YAML::Value << fieldComponent.Radius;
				out << YAML::Key << "Falloff" << YAML::Value << fieldComponent.Falloff;
			}
			else
			{
				// Buoyancy
				out << YAML::Key << "Buoyancy" << YAML::Value << fieldComponent.Buoyancy;
				out << YAML::Key << "Linear Drag" << YAML::Value << fieldComponent.LinearDrag;
				out << YAML::Key << "Angular Drag" << YAML::Value << fieldComponent.AngularDrag;
				out << YAML::Key << "Fluid Velocity" << YAML::Value << fieldComponent.FluidVelocity;
			}

			out << YAML::EndMap; // SpringArmComponent
		}

		if (entity.HasComponent<LandscapeComponent>())
		{
			const auto& landscapeComponent = entity.GetComponent<LandscapeComponent>();

			out << YAML::Key << "LandscapeComponent";
			out << YAML::BeginMap; // LandscapeComponent

			out << YAML::Key << "Resolution" << YAML::Value << landscapeComponent.Resolution;

			if (landscapeComponent.Material)
				out << YAML::Key << "Material" << YAML::Value << landscapeComponent.Material->Handle;

			// Heightfield Data
			{
				out << YAML::Key << "Height";
				out << YAML::BeginSeq;

				for (uint32_t y = 0; y < landscapeComponent.Resolution.y; y++)
					for (uint32_t x = 0; x < landscapeComponent.Resolution.x; x++)
						out << YAML::Value << landscapeComponent.Data->Get<float>(x + landscapeComponent.Resolution.x * y);

				out << YAML::EndSeq;
			}

			out << YAML::EndMap; // LandscapeComponent
		}

		out << YAML::EndMap; // Entity
	}

	template<typename T>
	static bool StreamWriteEntityHasComponent(Entity entity, FileStreamWriter& stream)
	{
		const bool hasComponent = entity.HasComponent<T>();
		stream.WriteRaw<bool>(hasComponent);
		return hasComponent;
	}

	void EntityRegistrySerializer::SerializeEntity(Entity entity, FileStreamWriter& stream)
	{
		DY_CORE_VERIFY(entity.HasComponent<IDComponent>());

		// Tag and ID Component
		stream.WriteRaw<EntityHandle>(entity.GetUUID());
		stream.WriteString(entity.GetComponent<TagComponent>().Tag);

		// Relationship Component
		if (StreamWriteEntityHasComponent<RelationshipComponent>(entity, stream))
		{
			const auto& rc = entity.GetComponent<RelationshipComponent>();
			stream.WriteRaw<EntityHandle>(rc.ParentHandle);

			stream.WriteRaw<uint32_t>(rc.Children.size());
			for (const auto& child : rc.Children)
				stream.WriteRaw<EntityHandle>(child);
		}

		// Attachment Component
		if (StreamWriteEntityHasComponent<AttachmentComponent>(entity, stream))
		{
			const auto& ac = entity.GetComponent<AttachmentComponent>();
			stream.WriteString(ac.BoneName);
		}

		// Prefab Component
		if (StreamWriteEntityHasComponent<PrefabComponent>(entity, stream))
		{
			const auto& pc = entity.GetComponent<PrefabComponent>();
			stream.WriteRaw<AssetHandle>(pc.PrefabID);
		}

		// Folder Component
		if (StreamWriteEntityHasComponent<FolderComponent>(entity, stream))
		{
			const auto& fc = entity.GetComponent<FolderComponent>();
			stream.WriteRaw<glm::vec4>(fc.Color);
		}

		// Transform Component
		if (StreamWriteEntityHasComponent<TransformComponent>(entity, stream))
		{
			const auto& transform = entity.GetComponent<TransformComponent>().Transform;
			stream.WriteRaw<glm::vec3>(transform.Translation);
			stream.WriteRaw<glm::quat>(transform.Rotation);
			stream.WriteRaw<glm::vec3>(transform.Scale);
		}

		// Camera Component
		if (StreamWriteEntityHasComponent<CameraComponent>(entity, stream))
		{
			const auto& cc = entity.GetComponent<CameraComponent>();
			const auto& camera = cc.Camera;
			const auto& settings = camera.GetCameraSettings();

			// Camera
			stream.WriteRaw<uint8_t>((uint8_t)camera.GetProjectionType());
			stream.WriteRaw<float>(camera.GetPerspectiveVerticalFOV());
			stream.WriteRaw<float>(camera.GetPerspectiveNearClip());
			stream.WriteRaw<float>(camera.GetPerspectiveFarClip());
			stream.WriteRaw<float>(camera.GetOrthographicSize());
			stream.WriteRaw<float>(camera.GetOrthographicNearClip());
			stream.WriteRaw<float>(camera.GetOrthographicFarClip());

			stream.WriteRaw<bool>(cc.Primary);
			stream.WriteRaw<bool>(cc.FixedAspectRatio);

			// Settings
			stream.WriteRaw<float>(settings.BloomThreshold);
			stream.WriteRaw<AssetHandle>(settings.BloomDirtTexture ? settings.BloomDirtTexture->Handle : 0);
			stream.WriteRaw<float>(settings.DOFStrength);
			stream.WriteRaw<float>(settings.DOFTarget);
			stream.WriteRaw<float>(settings.DOFFocusRange);
			stream.WriteRaw<float>(settings.DOFFocusFalloff);
			stream.WriteRaw<AssetHandle>(settings.LUT ? settings.LUT->Handle : 0);
		}

		// Capture Component
		if (StreamWriteEntityHasComponent<CaptureComponent>(entity, stream))
		{
			const auto& cc = entity.GetComponent<CaptureComponent>();
			stream.WriteRaw<bool>(cc.Capture);
			stream.WriteRaw<bool>(cc.Cumulative);
			stream.WriteRaw<uint8_t>((uint8_t)cc.Type);
			stream.WriteRaw<uint8_t>((uint8_t)cc.MaskType);
			stream.WriteRaw<AssetHandle>(cc.Target ? cc.Target->Handle : 0);
		}

		// Script Component
		if (StreamWriteEntityHasComponent<ScriptComponent>(entity, stream))
		{
			const auto& sc = entity.GetComponent<ScriptComponent>();
			stream.WriteString(sc.ClassName);

			// Fields
			Ref<ScriptClass> entityClass = ScriptEngine::GetEntityClass(sc.ClassName);
			const auto& fields = entityClass->GetFields();
			ScriptFieldMap entityFields = ScriptEngine::GetScriptFieldMap(entity);

			// Remove fields that do not exist on the entity class (hence why we copy the map)
			for (auto it = entityFields.begin(); it != entityFields.end();)
			{
				if (fields.find(it->first) == fields.end())
					it = entityFields.erase(it);
				else
					it++;
			}

			// Write all remaining classes
			stream.WriteRaw<uint32_t>(entityFields.size());
			for (const auto& [name, scriptField] : entityFields)
			{
				const auto& field = fields.at(name);

				stream.WriteString(name);
				stream.WriteRaw<uint8_t>((uint8_t)field.Type);

				#define WRITE_SCRIPT_FIELD_RUNTIME(FieldType, Type)				\
					case ScriptFieldType::FieldType:							\
						stream.WriteRaw<Type>(scriptField.GetValue<Type>());	\
						break;													\

				switch (field.Type)
				{
					WRITE_SCRIPT_FIELD_RUNTIME(Float, float);
					WRITE_SCRIPT_FIELD_RUNTIME(Double, double);
					WRITE_SCRIPT_FIELD_RUNTIME(Bool, bool);
					WRITE_SCRIPT_FIELD_RUNTIME(Char, char);
					WRITE_SCRIPT_FIELD_RUNTIME(Byte, int8_t);
					WRITE_SCRIPT_FIELD_RUNTIME(Short, int16_t);
					WRITE_SCRIPT_FIELD_RUNTIME(Int, int32_t);
					WRITE_SCRIPT_FIELD_RUNTIME(Long, int64_t);
					WRITE_SCRIPT_FIELD_RUNTIME(UShort, uint16_t);
					WRITE_SCRIPT_FIELD_RUNTIME(UInt, uint32_t);
					WRITE_SCRIPT_FIELD_RUNTIME(ULong, uint64_t);
					WRITE_SCRIPT_FIELD_RUNTIME(Vector2, glm::vec2);
					WRITE_SCRIPT_FIELD_RUNTIME(Vector3, glm::vec3);
					WRITE_SCRIPT_FIELD_RUNTIME(Vector4, glm::vec4);
					WRITE_SCRIPT_FIELD_RUNTIME(Entity, EntityHandle);
					WRITE_SCRIPT_FIELD_RUNTIME(Asset, AssetHandle);
					WRITE_SCRIPT_FIELD_RUNTIME(Scene, AssetHandle);
					WRITE_SCRIPT_FIELD_RUNTIME(Texture, AssetHandle);
					WRITE_SCRIPT_FIELD_RUNTIME(VirtualTexture, AssetHandle);
					WRITE_SCRIPT_FIELD_RUNTIME(Mesh, AssetHandle);
					WRITE_SCRIPT_FIELD_RUNTIME(Animation, AssetHandle);
					WRITE_SCRIPT_FIELD_RUNTIME(Material, AssetHandle);
					WRITE_SCRIPT_FIELD_RUNTIME(Audio, AssetHandle);
					WRITE_SCRIPT_FIELD_RUNTIME(VideoPlayer, AssetHandle);
				}
			}
		}

		// Sprite Renderer Component
		if (StreamWriteEntityHasComponent<SpriteRendererComponent>(entity, stream))
		{
			const auto& src = entity.GetComponent<SpriteRendererComponent>();
			stream.WriteRaw<glm::vec4>(src.Color);
			stream.WriteRaw<float>(src.TilingFactor);
			stream.WriteRaw<AssetHandle>(src.Texture ? src.Texture->Handle : 0);
		}

		// Circle Renderer Component
		if (StreamWriteEntityHasComponent<CircleRendererComponent>(entity, stream))
		{
			const auto& crc = entity.GetComponent<CircleRendererComponent>();
			stream.WriteRaw<glm::vec4>(crc.Color);
			stream.WriteRaw<float>(crc.Thickness);
			stream.WriteRaw<float>(crc.Fade);
		}

		// Text Component
		if (StreamWriteEntityHasComponent<TextComponent>(entity, stream))
		{
			const auto& tc = entity.GetComponent<TextComponent>();
			stream.WriteString(tc.TextString);
			stream.WriteRaw<uint8_t>((uint8_t)tc.Alignment);
			stream.WriteRaw<glm::vec4>(tc.Color);
			stream.WriteRaw<float>(tc.Kerning);
			stream.WriteRaw<float>(tc.LineSpacing);
			stream.WriteRaw<float>(tc.MaxWidth);
			stream.WriteRaw<AssetHandle>(tc.Font ? tc.Font->Handle : 0);
		}

		// Particle System Component
		if (StreamWriteEntityHasComponent<ParticleSystemComponent>(entity, stream))
		{
			const auto& psc = entity.GetComponent<ParticleSystemComponent>();
			const Ref<ParticleSystem> particleSystem = psc.Player ? psc.Player->GetParticleSystem() : nullptr;

			stream.WriteRaw<AssetHandle>(particleSystem ? particleSystem->Handle : 0);
			stream.WriteRaw<AssetHandle>(psc.Material ? psc.Material->Handle : 0);
		}

		// Rigidbody 2D Component
		if (StreamWriteEntityHasComponent<RigidBody2DComponent>(entity, stream))
		{
			const auto& rbc = entity.GetComponent<RigidBody2DComponent>();
			stream.WriteRaw<uint8_t>((uint8_t)rbc.Type);
			stream.WriteRaw<bool>(rbc.FixedRotation);
		}

		// Box Collider 2D Component
		if (StreamWriteEntityHasComponent<BoxCollider2DComponent>(entity, stream))
		{
			const auto& bcc = entity.GetComponent<BoxCollider2DComponent>();
			stream.WriteRaw<glm::vec2>(bcc.Offset);
			stream.WriteRaw<glm::vec2>(bcc.Size);
			stream.WriteRaw<float>(bcc.Density);
			stream.WriteRaw<float>(bcc.Friction);
			stream.WriteRaw<float>(bcc.Restitution);
			stream.WriteRaw<float>(bcc.RestitutionThreshold);
		}

		// Circle Collider 2D Component
		if (StreamWriteEntityHasComponent<CircleCollider2DComponent>(entity, stream))
		{
			const auto& ccc = entity.GetComponent<CircleCollider2DComponent>();
			stream.WriteRaw<glm::vec2>(ccc.Offset);
			stream.WriteRaw<float>(ccc.Radius);
			stream.WriteRaw<float>(ccc.Density);
			stream.WriteRaw<float>(ccc.Friction);
			stream.WriteRaw<float>(ccc.Restitution);
			stream.WriteRaw<float>(ccc.RestitutionThreshold);
		}

		// Static Mesh Component
		if (StreamWriteEntityHasComponent<StaticMeshComponent>(entity, stream))
		{
			const auto& smc = entity.GetComponent<StaticMeshComponent>();
			stream.WriteRaw<AssetHandle>(smc.m_Model ? smc.m_Model->Handle : 0);

			if (smc.m_Model)
			{
				Ref<AnimationGraphPlayer> animationPlayer = smc.GetAnimationPlayer();
				stream.WriteRaw<AssetHandle>((animationPlayer && animationPlayer->GetAnimationGraph()) ? animationPlayer->GetAnimationGraph()->Handle : 0);

				const auto& materials = smc.m_Materials;
				stream.WriteRaw<uint32_t>(materials.size());
				for (const auto& material : materials)
					stream.WriteRaw<AssetHandle>(material ? material->Handle : 0);
			}
		}

		// Directional Light Component
		if (StreamWriteEntityHasComponent<DirectionalLightComponent>(entity, stream))
		{
			const auto& dlc = entity.GetComponent<DirectionalLightComponent>();
			stream.WriteRaw<glm::vec3>(dlc.Color);
			stream.WriteRaw<float>(dlc.Intensity);
		}

		// Point Light Component
		if (StreamWriteEntityHasComponent<PointLightComponent>(entity, stream))
		{
			const auto& plc = entity.GetComponent<PointLightComponent>();
			stream.WriteRaw<glm::vec3>(plc.Color);
			stream.WriteRaw<float>(plc.Intensity);
			stream.WriteRaw<float>(plc.Radius);
			stream.WriteRaw<bool>(plc.CastsShadows);
		}

		// Sky Light Component
		if (StreamWriteEntityHasComponent<SkyLightComponent>(entity, stream))
		{
			const auto& slc = entity.GetComponent<SkyLightComponent>();
			stream.WriteRaw<uint8_t>((uint8_t)slc.Type);

			if (slc.Type == SkyLightComponent::SkyType::EnvironmentMap)
			{
				stream.WriteRaw<AssetHandle>(slc.EnvironmentMap ? slc.EnvironmentMap->Handle : 0);
				stream.WriteRaw<AssetHandle>(slc.FlowMap ? slc.FlowMap->Handle : 0);
			}

			stream.WriteRaw<float>(slc.Intensity);
		}

		// Decal Component
		if (StreamWriteEntityHasComponent<DecalComponent>(entity, stream))
		{
			const auto& dc = entity.GetComponent<DecalComponent>();
			stream.WriteRaw<bool>(dc.ConstrainAngle);
			stream.WriteRaw<AssetHandle>(dc.Texture ? dc.Texture->Handle : 0);
		}

		// Post Process Volume Component
		if (StreamWriteEntityHasComponent<PostProcessVolumeComponent>(entity, stream))
		{
			const auto& ppvc = entity.GetComponent<PostProcessVolumeComponent>();
			stream.WriteRaw<bool>(ppvc.Bounded);
			stream.WriteRaw<bool>(ppvc.Enabled);
			stream.WriteRaw<AssetHandle>(ppvc.Material ? ppvc.Material->Handle : 0);
		}

		// Volume Component
		if (StreamWriteEntityHasComponent<VolumeComponent>(entity, stream))
		{
			const auto& vc = entity.GetComponent<VolumeComponent>();
			stream.WriteRaw<uint8_t>((uint8_t)vc.Blend);
			stream.WriteRaw<glm::vec3>(vc.Color);
			stream.WriteRaw<float>(vc.ExtinctionScale);
			stream.WriteRaw<float>(vc.ScatteringDistribution);
			stream.WriteRaw<float>(vc.ScatteringIntensity);
		}

		// Audio Component
		if (StreamWriteEntityHasComponent<AudioComponent>(entity, stream))
		{
			const auto& ac = entity.GetComponent<AudioComponent>();
			stream.WriteRaw<AssetHandle>(ac.AudioSound ? ac.AudioSound->Handle : 0);

			if (ac.AudioSound)
			{
				stream.WriteRaw<uint32_t>(ac.StartPosition);
				stream.WriteRaw<bool>(ac.StartOnAwake);
				stream.WriteRaw<bool>(ac.AudioSound->Is3D());
				stream.WriteRaw<bool>(ac.AudioSound->IsLooping());
				stream.WriteRaw<float>(ac.AudioSound->GetVolume());
				stream.WriteRaw<float>(ac.AudioSound->GetPan());
				stream.WriteRaw<float>(ac.AudioSound->GetSpeed());
				stream.WriteRaw<float>(ac.AudioSound->GetRadius());
				stream.WriteRaw<bool>(ac.AudioSound->GetEcho());
			}
		}

		// Spline Component
		if (StreamWriteEntityHasComponent<SplineComponent>(entity, stream))
		{
			const auto& sc = entity.GetComponent<SplineComponent>();

			stream.WriteRaw<uint32_t>(sc.Points.size());
			for (const auto& point : sc.Points)
			{
				stream.WriteRaw<glm::vec3>(point.Position);
				stream.WriteRaw<glm::vec3>(point.Tangent);
				stream.WriteRaw<uint8_t>((uint8_t)point.Type);
			}
		}

		// Rigid Body Component
		if (StreamWriteEntityHasComponent<RigidBodyComponent>(entity, stream))
		{
			const auto& rbc = entity.GetComponent<RigidBodyComponent>();
			stream.WriteRaw<uint8_t>((uint8_t)rbc.Type);
			stream.WriteRaw<PhysicsLayerID>(rbc.Layer);
			stream.WriteRaw<bool>(rbc.Sensor);
			stream.WriteRaw<uint8_t>((uint8_t)rbc.Mode);
			stream.WriteRaw<float>(rbc.Density);
			stream.WriteRaw<float>(rbc.Friction);
			stream.WriteRaw<float>(rbc.Restitution);
		}

		// Soft Body Component
		if (StreamWriteEntityHasComponent<SoftBodyComponent>(entity, stream))
		{
			const auto& sbc = entity.GetComponent<SoftBodyComponent>();
			stream.WriteRaw<float>(sbc.Friction);
			stream.WriteRaw<float>(sbc.Restitution);
			stream.WriteRaw<float>(sbc.Pressure);

			stream.WriteRaw<float>(sbc.VertexMass);
			stream.WriteRaw<float>(sbc.VertexRadius);
			stream.WriteRaw<bool>(sbc.UseVertexColorAsWeight);
		}

		// Box Collider Component
		if (StreamWriteEntityHasComponent<BoxColliderComponent>(entity, stream))
		{
			const auto& bcc = entity.GetComponent<BoxColliderComponent>();
			stream.WriteRaw<glm::vec3>(bcc.Size);
		}

		// Sphere Collider Component
		if (StreamWriteEntityHasComponent<SphereColliderComponent>(entity, stream))
		{
			const auto& scc = entity.GetComponent<SphereColliderComponent>();
			stream.WriteRaw<float>(scc.Radius);
		}

		// Capsule Collider Component
		if (StreamWriteEntityHasComponent<CapsuleColliderComponent>(entity, stream))
		{
			const auto& ccc = entity.GetComponent<CapsuleColliderComponent>();
			stream.WriteRaw<float>(ccc.Radius);
			stream.WriteRaw<float>(ccc.HalfHeight);
		}

		// Mesh Collider Component
		if (StreamWriteEntityHasComponent<MeshColliderComponent>(entity, stream))
		{
			const auto& mcc = entity.GetComponent<MeshColliderComponent>();
			stream.WriteRaw<uint8_t>((uint8_t)mcc.Type);
		}

		// Point Constraint Component
		if (StreamWriteEntityHasComponent<PointConstraintComponent>(entity, stream))
		{
			const auto& pcc = entity.GetComponent<PointConstraintComponent>();
			Utils::SerializeConstraintComponent(&pcc, stream);
			stream.WriteRaw<uint8_t>((uint8_t)pcc.Space);
			stream.WriteRaw<glm::vec3>(pcc.LocalPoint);
			stream.WriteRaw<glm::vec3>(pcc.TargetPoint);
		}

		// Cone Constraint Component
		if (StreamWriteEntityHasComponent<ConeConstraintComponent>(entity, stream))
		{
			const auto& ccc = entity.GetComponent<ConeConstraintComponent>();
			Utils::SerializeConstraintComponent(&ccc, stream);
			stream.WriteRaw<uint8_t>((uint8_t)ccc.Space);
			stream.WriteRaw<glm::vec3>(ccc.LocalReferenceFrame.Offset);
			Utils::SerializeAxis(ccc.LocalReferenceFrame.TwistAxis, stream);
			stream.WriteRaw<glm::vec3>(ccc.TargetReferenceFrame.Offset);
			Utils::SerializeAxis(ccc.TargetReferenceFrame.TwistAxis, stream);
			stream.WriteRaw<float>(ccc.HalfConeAngle);
		}

		// Distance Constraint Component
		if (StreamWriteEntityHasComponent<DistanceConstraintComponent>(entity, stream))
		{
			const auto& dcc = entity.GetComponent<DistanceConstraintComponent>();
			Utils::SerializeConstraintComponent(&dcc, stream);
			stream.WriteRaw<uint8_t>((uint8_t)dcc.Type);

			if (dcc.Type == DistanceConstraintComponent::DistanceType::Fixed)
				stream.WriteRaw<float>(dcc.Distance);
			else if (dcc.Type == DistanceConstraintComponent::DistanceType::Range)
			{
				stream.WriteRaw<float>(dcc.MinDistance);
				stream.WriteRaw<float>(dcc.MaxDistance);
			}
		}

		// Spring Constraint Component
		if (StreamWriteEntityHasComponent<SpringConstraintComponent>(entity, stream))
		{
			const auto& scc = entity.GetComponent<SpringConstraintComponent>();
			stream.WriteRaw<uint8_t>((uint8_t)scc.Type);

			if (scc.Type == SpringConstraintComponent::SpringType::FrequencyAndDamping)
				stream.WriteRaw<float>(scc.Frequency);
			else if (scc.Type == SpringConstraintComponent::SpringType::StiffnessAndDamping)
				stream.WriteRaw<float>(scc.Stiffness);

			stream.WriteRaw<float>(scc.Damping);
		}

		// Hinge Constraint Component
		if (StreamWriteEntityHasComponent<HingeConstraintComponent>(entity, stream))
		{
			const auto& hcc = entity.GetComponent<HingeConstraintComponent>();
			Utils::SerializeConstraintComponent(&hcc, stream);
			stream.WriteRaw<uint8_t>((uint8_t)hcc.Space);

			stream.WriteRaw<glm::vec3>(hcc.LocalReferenceFrame.Point);
			Utils::SerializeAxis(hcc.LocalReferenceFrame.HingeAxis, stream);
			Utils::SerializeAxis(hcc.LocalReferenceFrame.NormalAxis, stream);
			stream.WriteRaw<glm::vec3>(hcc.TargetReferenceFrame.Point);
			Utils::SerializeAxis(hcc.TargetReferenceFrame.HingeAxis, stream);
			Utils::SerializeAxis(hcc.TargetReferenceFrame.NormalAxis, stream);

			stream.WriteRaw<float>(hcc.MinRotation);
			stream.WriteRaw<float>(hcc.MaxRotation);
			stream.WriteRaw<float>(hcc.MaxFrictionTorque);
		}

		// Fixed Constraint Component
		if (StreamWriteEntityHasComponent<FixedConstraintComponent>(entity, stream))
		{
			const auto& fcc = entity.GetComponent<FixedConstraintComponent>();
			Utils::SerializeConstraintComponent(&fcc, stream);
			stream.WriteRaw<uint8_t>((uint8_t)fcc.Type);

			stream.WriteRaw<glm::vec3>(fcc.LocalReferenceFrame.Point);
			Utils::SerializeAxis(fcc.LocalReferenceFrame.AxisX, stream);
			Utils::SerializeAxis(fcc.LocalReferenceFrame.AxisY, stream);
			stream.WriteRaw<glm::vec3>(fcc.TargetReferenceFrame.Point);
			Utils::SerializeAxis(fcc.TargetReferenceFrame.AxisX, stream);
			Utils::SerializeAxis(fcc.TargetReferenceFrame.AxisY, stream);
		}

		// Gear Constraint Component
		if (StreamWriteEntityHasComponent<GearConstraintComponent>(entity, stream))
		{
			const auto& gcc = entity.GetComponent<GearConstraintComponent>();
			Utils::SerializeConstraintComponent(&gcc, stream);
			stream.WriteRaw<uint8_t>((uint8_t)gcc.Space);

			Utils::SerializeAxis(gcc.LocalHingeAxis, stream);
			Utils::SerializeAxis(gcc.TargetHingeAxis, stream);
			stream.WriteRaw<Fraction>(gcc.Ratio);
		}

		// Pulley Constraint Component
		if (StreamWriteEntityHasComponent<PulleyConstraintComponent>(entity, stream))
		{
			const auto& pcc = entity.GetComponent<PulleyConstraintComponent>();
			Utils::SerializeConstraintComponent(&pcc, stream);
			stream.WriteRaw<uint8_t>((uint8_t)pcc.Space);

			stream.WriteRaw<glm::vec3>(pcc.LocalReferenceFrame.BodyPoint);
			stream.WriteRaw<glm::vec3>(pcc.LocalReferenceFrame.FixedPoint);
			stream.WriteRaw<glm::vec3>(pcc.TargetReferenceFrame.BodyPoint);
			stream.WriteRaw<glm::vec3>(pcc.TargetReferenceFrame.FixedPoint);

			stream.WriteRaw<Fraction>(pcc.Ratio);
			stream.WriteRaw<float>(pcc.MinLength);
			stream.WriteRaw<float>(pcc.MaxLength);
		}

		// Rack And Pinion Constraint Component
		if (StreamWriteEntityHasComponent<RackAndPinionConstraintComponent>(entity, stream))
		{
			const auto& rapcc = entity.GetComponent<RackAndPinionConstraintComponent>();
			Utils::SerializeConstraintComponent(&rapcc, stream);
			stream.WriteRaw<uint8_t>((uint8_t)rapcc.Space);

			stream.WriteRaw<uint8_t>((uint8_t)rapcc.Mode);
			Utils::SerializeAxis(rapcc.HingeAxis, stream);
			Utils::SerializeAxis(rapcc.SliderAxis, stream);

			if (rapcc.Mode == RackAndPinionConstraintComponent::RatioMode::Ratio)
				stream.WriteRaw<float>(rapcc.Ratio);
			else
			{
				stream.WriteRaw<uint32_t>(rapcc.RackTeethCount);
				stream.WriteRaw<uint32_t>(rapcc.PinionTeethCount);
				stream.WriteRaw<float>(rapcc.RackLength);
			}
		}

		// Swing Twist Constraint Component
		if (StreamWriteEntityHasComponent<SwingTwistConstraintComponent>(entity, stream))
		{
			const auto& stcc = entity.GetComponent<SwingTwistConstraintComponent>();
			Utils::SerializeConstraintComponent(&stcc, stream);
			stream.WriteRaw<uint8_t>((uint8_t)stcc.Space);
			stream.WriteRaw<uint8_t>((uint8_t)stcc.SwingType);

			stream.WriteRaw<glm::vec3>(stcc.LocalReferenceFrame.Position);
			Utils::SerializeAxis(stcc.LocalReferenceFrame.TwistAxis, stream);
			Utils::SerializeAxis(stcc.LocalReferenceFrame.PlaneAxis, stream);
			stream.WriteRaw<glm::vec3>(stcc.TargetReferenceFrame.Position);
			Utils::SerializeAxis(stcc.TargetReferenceFrame.TwistAxis, stream);
			Utils::SerializeAxis(stcc.TargetReferenceFrame.PlaneAxis, stream);

			stream.WriteRaw<float>(stcc.NormalHalfConeAngle);
			stream.WriteRaw<float>(stcc.PlaneHalfConeAngle);
			stream.WriteRaw<float>(stcc.TwistMinAngle);
			stream.WriteRaw<float>(stcc.TwistMaxAngle);
			stream.WriteRaw<float>(stcc.MaxFrictionTorque);
		}

		// Slider Constraint Component
		if (StreamWriteEntityHasComponent<SliderConstraintComponent>(entity, stream))
		{
			const auto& scc = entity.GetComponent<SliderConstraintComponent>();
			Utils::SerializeConstraintComponent(&scc, stream);
			stream.WriteRaw<uint8_t>((uint8_t)scc.Space);

			stream.WriteRaw<glm::vec3>(scc.LocalReferenceFrame.Point);
			Utils::SerializeAxis(scc.LocalReferenceFrame.SliderAxis, stream);
			Utils::SerializeAxis(scc.LocalReferenceFrame.NormalAxis, stream);
			stream.WriteRaw<glm::vec3>(scc.TargetReferenceFrame.Point);
			Utils::SerializeAxis(scc.TargetReferenceFrame.SliderAxis, stream);
			Utils::SerializeAxis(scc.TargetReferenceFrame.NormalAxis, stream);

			stream.WriteRaw<float>(scc.SliderMin);
			stream.WriteRaw<float>(scc.SliderMax);
			stream.WriteRaw<float>(scc.MaxFrictionForce);
		}

		// Six DOF Constraint Component
		if (StreamWriteEntityHasComponent<SixDOFConstraintComponent>(entity, stream))
		{
			const auto& sdcc = entity.GetComponent<SixDOFConstraintComponent>();
			Utils::SerializeConstraintComponent(&sdcc, stream);
			stream.WriteRaw<uint8_t>((uint8_t)sdcc.Space);

			stream.WriteRaw<glm::vec3>(sdcc.LocalReferenceFrame.Position);
			Utils::SerializeAxis(sdcc.LocalReferenceFrame.AxisX, stream);
			Utils::SerializeAxis(sdcc.LocalReferenceFrame.AxisY, stream);
			stream.WriteRaw<glm::vec3>(sdcc.TargetReferenceFrame.Position);
			Utils::SerializeAxis(sdcc.TargetReferenceFrame.AxisX, stream);
			Utils::SerializeAxis(sdcc.TargetReferenceFrame.AxisY, stream);

			stream.WriteRaw<uint8_t>((uint8_t)sdcc.SwingType);

			for (uint32_t axisIndex = 0; axisIndex < SixDOFConstraintComponent::Axis::AxisCount; axisIndex++)
			{
				const SixDOFConstraintComponent::Axis axis = (SixDOFConstraintComponent::Axis)axisIndex;
				const SixDOFConstraintComponent::AxisStatus status = sdcc.GetAxisStatus(axis);

				stream.WriteRaw<uint8_t>((uint8_t)status);
				stream.WriteRaw<float>(sdcc.MaxFriction[axisIndex]);

				if (status == SixDOFConstraintComponent::AxisStatus::Custom)
				{
					stream.WriteRaw<float>(sdcc.LimitMin[axisIndex]);
					stream.WriteRaw<float>(sdcc.LimitMax[axisIndex]);
				}
			}
		}

		// Follow Constraint Component
		if (StreamWriteEntityHasComponent<FollowConstraintComponent>(entity, stream))
		{
			const auto& fcc = entity.GetComponent<FollowConstraintComponent>();
			Utils::SerializeConstraintComponent(&fcc, stream);

			stream.WriteRaw<bool>(fcc.Looping);
			Utils::SerializeAxis(fcc.Normal, stream);
			stream.WriteRaw<float>(fcc.StartFraction);
			stream.WriteRaw<float>(fcc.MaxFrictionForce);
			stream.WriteRaw<uint8_t>((uint8_t)fcc.RotationConstraint);
			stream.WriteRaw<EntityHandle>(fcc.BaseTarget);
		}

		// Spring Arm Component
		if (StreamWriteEntityHasComponent<SpringArmComponent>(entity, stream))
		{
			const auto& sac = entity.GetComponent<SpringArmComponent>();
			stream.WriteRaw<float>(sac.TargetLength);
			stream.WriteRaw<float>(sac.ProbeRadius);
			stream.WriteRaw<glm::vec3>(sac.TargetOffset);
			stream.WriteRaw<glm::vec3>(sac.SocketOffset);
		}

		// Field Component
		if (StreamWriteEntityHasComponent<FieldComponent>(entity, stream))
		{
			const auto& fc = entity.GetComponent<FieldComponent>();
			stream.WriteRaw<uint8_t>((uint8_t)fc.Type);
			stream.WriteRaw<PhysicsLayerID>(fc.Layer);

			if (fc.Type == FieldComponent::FieldType::Directional)
			{
				// Directional
				stream.WriteRaw<glm::vec3>(fc.Force);
			}
			else if (fc.Type == FieldComponent::FieldType::Radial)
			{
				// Radial
				stream.WriteRaw<float>(fc.Magnitude);
				stream.WriteRaw<float>(fc.Radius);
				stream.WriteRaw<float>(fc.Falloff);
			}
			else
			{
				// Buoyancy
				stream.WriteRaw<float>(fc.Buoyancy);
				stream.WriteRaw<float>(fc.LinearDrag);
				stream.WriteRaw<float>(fc.AngularDrag);
				stream.WriteRaw<glm::vec3>(fc.FluidVelocity);
			}
		}

		// Landscape Component
		if (StreamWriteEntityHasComponent<LandscapeComponent>(entity, stream))
		{
			const auto& lc = entity.GetComponent<LandscapeComponent>();

			stream.WriteRaw<glm::uvec2>(lc.Resolution);
			stream.WriteRaw<AssetHandle>(lc.Material ? lc.Material->Handle : 0);

			stream.WriteBuffer(lc.Data);
		}
	}

	void EntityRegistrySerializer::DeserializeEntity(YAML::Node entity, Ref<EntityRegistry> scene)
	{
		uint64_t uuid = entity["Entity"].as<uint64_t>();

		std::string name;
		auto tagComponent = entity["TagComponent"];
		if (tagComponent)
			name = tagComponent["Tag"].as<std::string>();

		DY_CORE_TRACE("Deserialized entity '{}' with ID: {}", name, uuid);

		Entity deserializedEntity = scene->CreateEntityWithUUID(uuid, name);

		Utils::DeserializeOptionalCallback(entity, "Parent", [&](const EntityHandle parentHandle)
		{
			auto& rc = deserializedEntity.GetComponent<RelationshipComponent>();
			rc.ParentHandle = parentHandle;
		});

		if (auto children = entity["Children"])
		{
			auto& rc = deserializedEntity.GetComponent<RelationshipComponent>();

			rc.Children.reserve(children.size());
			for (auto child : children)
			{
				auto handle = child["Handle"];
				if (handle)
					rc.Children.push_back(handle.as<EntityHandle>());
			}
		}

		if (auto attachmentComponent = entity["AttachmentComponent"])
		{
			auto& ac = deserializedEntity.AddComponent<AttachmentComponent>();
			Utils::DeserializeOptional(attachmentComponent, "Bone Name", ac.BoneName);
		}

		if (auto prefabComponent = entity["PrefabComponent"])
		{
			auto& pc = deserializedEntity.AddComponent<PrefabComponent>();
			Utils::DeserializeOptional(prefabComponent, "Prefab", pc.PrefabID);
		}

		if (auto folderComponent = entity["FolderComponent"])
		{
			deserializedEntity.RemoveComponent<TransformComponent>();
			auto& fc = deserializedEntity.AddComponent<FolderComponent>();
			Utils::DeserializeOptional<glm::vec4>(folderComponent, "Color", fc.Color);
		}

		if (auto transformComponent = entity["TransformComponent"])
		{
			// Entities always have transforms
			auto& transform = deserializedEntity.GetComponent<TransformComponent>().Transform;
			Utils::DeserializeOptional<glm::vec3>(transformComponent, "Translation", transform.Translation);
			transform.SetRotationDegrees(Utils::DeserializeOptional<glm::vec3>(transformComponent, "Rotation"));
			Utils::DeserializeOptional<glm::vec3>(transformComponent, "Scale", transform.Scale);
		}

		if (auto cameraComponent = entity["CameraComponent"])
		{
			auto& cc = deserializedEntity.AddComponent<CameraComponent>();

			auto& cameraProps = cameraComponent["Camera"];

			SceneCamera::ProjectionType projectionType;
			Utils::DeserializeOptionalEnum(cameraProps, "ProjectionType", projectionType, Utils::CameraProjectionTypeFromString);
			cc.Camera.SetProjectionType(projectionType);

			Utils::DeserializeOptionalCallback(cameraProps, "PerspectiveFOV", [&](float perspectiveFOV) { cc.Camera.SetPerspectiveVerticalFOV(perspectiveFOV); });
			Utils::DeserializeOptionalCallback(cameraProps, "PerspectiveNear", [&](float perspectiveNear) { cc.Camera.SetPerspectiveNearClip(perspectiveNear); });
			Utils::DeserializeOptionalCallback(cameraProps, "PerspectiveFar", [&](float perspectiveFar) { cc.Camera.SetPerspectiveFarClip(perspectiveFar); });

			Utils::DeserializeOptionalCallback(cameraProps, "OrthographicSize", [&](float orthographicSize) { cc.Camera.SetOrthographicSize(orthographicSize); });
			Utils::DeserializeOptionalCallback(cameraProps, "OrthographicNear", [&](float orthographicNear) { cc.Camera.SetOrthographicNearClip(orthographicNear); });
			Utils::DeserializeOptionalCallback(cameraProps, "OrthographicFar", [&](float orthographicFar) { cc.Camera.SetOrthographicFarClip(orthographicFar); });

			Utils::DeserializeOptional<bool>(cameraComponent, "Primary", cc.Primary);
			Utils::DeserializeOptional<bool>(cameraComponent, "FixedAspectRatio", cc.FixedAspectRatio);

			// Settings
			if (auto& settingsProps = cameraComponent["Settings"])
			{
				auto& settings = cc.Camera.GetCameraSettings();

				Utils::DeserializeOptional<float>(settingsProps, "Bloom Threshold", settings.BloomThreshold);
				Utils::DeserializeOptionalAsset<Texture2D>(settingsProps, "Bloom Dirt Texture", settings.BloomDirtTexture);
				Utils::DeserializeOptional<float>(settingsProps, "DOFStrength", settings.DOFStrength);
				Utils::DeserializeOptional<float>(settingsProps, "DOFTarget", settings.DOFTarget);
				Utils::DeserializeOptional<float>(settingsProps, "DOFFocusRange", settings.DOFFocusRange);
				Utils::DeserializeOptional<float>(settingsProps, "DOFFocusFalloff", settings.DOFFocusFalloff);
				Utils::DeserializeOptionalAsset<Texture2D>(settingsProps, "LUT", settings.LUT);
			}
		}

		if (auto captureComponent = entity["CaptureComponent"])
		{
			auto& cc = deserializedEntity.AddComponent<CaptureComponent>();

			Utils::DeserializeOptional<bool>(captureComponent, "Capture", cc.Capture);
			Utils::DeserializeOptional<bool>(captureComponent, "Cumulative", cc.Cumulative);
			Utils::DeserializeOptionalEnum(captureComponent, "Type", cc.Type, SceneRendererContext::RenderVisualizationModeFromString);
			Utils::DeserializeOptionalEnum(captureComponent, "Mask", cc.MaskType, Utils::CaptureMaskTypeFromString);
			Utils::DeserializeOptionalAsset<Texture2D>(captureComponent, "Target", cc.Target);
		}

		if (auto scriptComponent = entity["ScriptComponent"])
		{
			auto& sc = deserializedEntity.AddComponent<ScriptComponent>();
			Utils::DeserializeOptional<std::string>(scriptComponent, "ClassName", sc.ClassName);

			if (auto scriptFields = scriptComponent["ScriptFields"])
			{
				Ref<ScriptClass> entityClass = ScriptEngine::GetEntityClass(sc.ClassName);

				if (entityClass)
				{
					const auto& fields = entityClass->GetFields();
					auto& entityFields = ScriptEngine::GetScriptFieldMap(deserializedEntity);

					for (auto scriptField : scriptFields)
					{
						std::string name;
						ScriptFieldType type;

						if (!Utils::DeserializeOptional<std::string>(scriptField, "Name", name))
							continue;

						if (!Utils::DeserializeOptionalEnum(scriptField, "Type", type, Utils::ScriptFieldTypeFromString))
							continue;

						ScriptFieldInstance& fieldInstance = entityFields[name];

						DY_CORE_ASSERT(fields.find(name) != fields.end());

						if (fields.find(name) == fields.end())
							continue;

						fieldInstance.Field = fields.at(name);

						switch (type)
						{
							READ_SCRIPT_FIELD(Float, float);
							READ_SCRIPT_FIELD(Double, double);
							READ_SCRIPT_FIELD(Bool, bool);
							READ_SCRIPT_FIELD(Char, char);
							READ_SCRIPT_FIELD(Byte, int8_t);
							READ_SCRIPT_FIELD(Short, int16_t);
							READ_SCRIPT_FIELD(Int, int32_t);
							READ_SCRIPT_FIELD(Long, int64_t);
							READ_SCRIPT_FIELD(UShort, uint16_t);
							READ_SCRIPT_FIELD(UInt, uint32_t);
							READ_SCRIPT_FIELD(ULong, uint64_t);
							READ_SCRIPT_FIELD(Vector2, glm::vec2);
							READ_SCRIPT_FIELD(Vector3, glm::vec3);
							READ_SCRIPT_FIELD(Vector4, glm::vec4);
							READ_SCRIPT_FIELD(Entity, UUID);
							READ_SCRIPT_FIELD(Asset, UUID);
							READ_SCRIPT_FIELD(Scene, UUID);
							READ_SCRIPT_FIELD(Texture, UUID);
							READ_SCRIPT_FIELD(VirtualTexture, UUID);
							READ_SCRIPT_FIELD(Mesh, UUID);
							READ_SCRIPT_FIELD(Animation, UUID);
							READ_SCRIPT_FIELD(Material, UUID);
							READ_SCRIPT_FIELD(Audio, UUID);
							READ_SCRIPT_FIELD(VideoPlayer, UUID);
						}
					}
				}
			}
		}

		if (auto spriteRendererComponent = entity["SpriteRendererComponent"])
		{
			auto& src = deserializedEntity.AddComponent<SpriteRendererComponent>();

			Utils::DeserializeOptional<glm::vec4>(spriteRendererComponent, "Color", src.Color);
			Utils::DeserializeOptionalAsset<Texture2D>(spriteRendererComponent, "Texture", src.Texture);
			Utils::DeserializeOptional<float>(spriteRendererComponent, "TilingFactor", src.TilingFactor);
		}

		if (auto circleRendererComponent = entity["CircleRendererComponent"])
		{
			auto& crc = deserializedEntity.AddComponent<CircleRendererComponent>();

			Utils::DeserializeOptional<glm::vec4>(circleRendererComponent, "Color", crc.Color);
			Utils::DeserializeOptional<float>(circleRendererComponent, "Thickness", crc.Thickness);
			Utils::DeserializeOptional<float>(circleRendererComponent, "Fade", crc.Fade);
		}

		if (auto textCompontent = entity["TextComponent"])
		{
			auto& tc = deserializedEntity.AddComponent<TextComponent>();

			Utils::DeserializeOptional<std::string>(textCompontent, "TextString", tc.TextString);
			Utils::DeserializeOptionalEnum(textCompontent, "Alignment", tc.Alignment, Utils::TextAlignmentTypeFromString);
			Utils::DeserializeOptional<glm::vec4>(textCompontent, "Color", tc.Color);
			Utils::DeserializeOptional<float>(textCompontent, "Kerning", tc.Kerning);
			Utils::DeserializeOptional<float>(textCompontent, "LineSpacing", tc.LineSpacing);
			Utils::DeserializeOptional<float>(textCompontent, "MaxWidth", tc.MaxWidth);
			Utils::DeserializeOptionalAsset<Font>(textCompontent, "Font", tc.Font);
		}

		if (auto particleSystemComponent = entity["ParticleSystemComponent"])
		{
			auto& psc = deserializedEntity.AddComponent<ParticleSystemComponent>();

			Ref<ParticleSystem> particleSystem = nullptr;
			Utils::DeserializeOptionalAsset<ParticleSystem>(particleSystemComponent, "Particle System", particleSystem);

			Ref<MaterialAsset> material = nullptr;
			Utils::DeserializeOptionalAsset<MaterialAsset>(particleSystemComponent, "Material", material);

			psc.SetParticleSystem(particleSystem, material);
		}

		if (auto rigidbody2DComponent = entity["Rigidbody2DComponent"])
		{
			auto& rb2d = deserializedEntity.AddComponent<RigidBody2DComponent>();

			Utils::DeserializeOptionalEnum(rigidbody2DComponent, "BodyType", rb2d.Type, Utils::RigidBody2DBodyTypeFromString);
			Utils::DeserializeOptional<bool>(rigidbody2DComponent, "FixedRotation", rb2d.FixedRotation);
		}

		if (auto boxCollider2DComponent = entity["BoxCollider2DComponent"])
		{
			auto& bc2d = deserializedEntity.AddComponent<BoxCollider2DComponent>();

			Utils::DeserializeOptional<glm::vec2>(boxCollider2DComponent, "Offset", bc2d.Offset);
			Utils::DeserializeOptional<glm::vec2>(boxCollider2DComponent, "Size", bc2d.Size);
			Utils::DeserializeOptional<float>(boxCollider2DComponent, "Density", bc2d.Density);
			Utils::DeserializeOptional<float>(boxCollider2DComponent, "Friction", bc2d.Friction);
			Utils::DeserializeOptional<float>(boxCollider2DComponent, "Restitution", bc2d.Restitution);
			Utils::DeserializeOptional<float>(boxCollider2DComponent, "RestitutionThreshold", bc2d.RestitutionThreshold);
		}

		if (auto circleCollider2DComponent = entity["CircleCollider2DComponent"])
		{
			auto& cc2d = deserializedEntity.AddComponent<CircleCollider2DComponent>();

			Utils::DeserializeOptional<glm::vec2>(circleCollider2DComponent, "Offset", cc2d.Offset);
			Utils::DeserializeOptional<float>(circleCollider2DComponent, "Radius", cc2d.Radius);
			Utils::DeserializeOptional<float>(circleCollider2DComponent, "Density", cc2d.Density);
			Utils::DeserializeOptional<float>(circleCollider2DComponent, "Friction", cc2d.Friction);
			Utils::DeserializeOptional<float>(circleCollider2DComponent, "Restitution", cc2d.Restitution);
			Utils::DeserializeOptional<float>(circleCollider2DComponent, "RestitutionThreshold", cc2d.RestitutionThreshold);
		}

		if (auto staticMeshComponent = entity["StaticMeshComponent"])
		{
			auto& smc = deserializedEntity.AddComponent<StaticMeshComponent>();

			Ref<Model> model;
			if (Utils::DeserializeOptionalAsset<Model>(staticMeshComponent, "Mesh", model))
			{
				smc.SetModel(model);

				Ref<AnimationGraph> animationGraph;
				if (Utils::DeserializeOptionalAsset<AnimationGraph>(staticMeshComponent, "Animation Graph", animationGraph))
					smc.SetAnimationGraph(animationGraph);

				if (auto& materials = staticMeshComponent["Materials"])
				{
					uint32_t index = 0;
					for (auto& material : materials)
					{
						if (index >= smc.m_Materials.size())
							break;

						UUID materialHandle = material.as<UUID>();
						smc.m_Materials[index] = materialHandle == 0 ? nullptr : AssetManager::GetAsset<MaterialAsset>(materialHandle);

						index++;
					}
				}
			}
		}

		if (auto directionalLightComponent = entity["DirectionalLightComponent"])
		{
			auto& dlc = deserializedEntity.AddComponent<DirectionalLightComponent>();

			Utils::DeserializeOptional<glm::vec3>(directionalLightComponent, "Color", dlc.Color);
			Utils::DeserializeOptional<float>(directionalLightComponent, "Intensity", dlc.Intensity);
		}

		if (auto pointLightComponent = entity["PointLightComponent"])
		{
			auto& plc = deserializedEntity.AddComponent<PointLightComponent>();

			Utils::DeserializeOptional<glm::vec3>(pointLightComponent, "Color", plc.Color);
			Utils::DeserializeOptional<float>(pointLightComponent, "Intensity", plc.Intensity);
			Utils::DeserializeOptional<float>(pointLightComponent, "Radius", plc.Radius);
			Utils::DeserializeOptional<bool>(pointLightComponent, "Casts Shadows", plc.CastsShadows);
		}

		if (auto skyLightComponent = entity["SkyLightComponent"])
		{
			auto& slc = deserializedEntity.AddComponent<SkyLightComponent>();

			Utils::DeserializeOptionalEnum(skyLightComponent, "Type", slc.Type, Utils::SkyLightComponentTypeFromString);
			Utils::DeserializeOptionalAsset<EnvironmentMap>(skyLightComponent, "Environment Map", slc.EnvironmentMap);
			Utils::DeserializeOptionalAsset<Texture2D>(skyLightComponent, "Flow Map", slc.FlowMap);
			Utils::DeserializeOptional<float>(skyLightComponent, "Intensity", slc.Intensity);
		}

		if (auto decalComponent = entity["DecalComponent"])
		{
			auto& dc = deserializedEntity.AddComponent<DecalComponent>();

			Utils::DeserializeOptional<bool>(decalComponent, "Constrain Angle", dc.ConstrainAngle);
			Utils::DeserializeOptionalAsset<Texture2D>(decalComponent, "Texture", dc.Texture);
		}

		if (auto postProcessingVolumeComponent = entity["PostProcessVolumeComponent"])
		{
			auto& ppvc = deserializedEntity.AddComponent<PostProcessVolumeComponent>();

			Utils::DeserializeOptional<bool>(postProcessingVolumeComponent, "Bounded", ppvc.Bounded);
			Utils::DeserializeOptional<bool>(postProcessingVolumeComponent, "Enabled", ppvc.Enabled);
			Utils::DeserializeOptionalAsset<MaterialAsset>(postProcessingVolumeComponent, "Material", ppvc.Material);
		}

		if (auto volumeComponent = entity["VolumeComponent"])
		{
			auto& vc = deserializedEntity.AddComponent<VolumeComponent>();

			Utils::DeserializeOptionalEnum(volumeComponent, "Blend", vc.Blend, Utils::VolumeBlendTypeFromString);
			Utils::DeserializeOptional<glm::vec3>(volumeComponent, "Color", vc.Color);
			Utils::DeserializeOptional<float>(volumeComponent, "Extinction Scale", vc.ExtinctionScale);
			Utils::DeserializeOptional<float>(volumeComponent, "Scattering Distribution", vc.ScatteringDistribution);
			Utils::DeserializeOptional<float>(volumeComponent, "Scattering Intensity", vc.ScatteringIntensity);
		}

		if (auto audioComponent = entity["AudioComponent"])
		{
			auto& ac = deserializedEntity.AddComponent<AudioComponent>();

			if (Utils::DeserializeOptionalAsset<Audio>(audioComponent, "Sound", ac.AudioSound))
			{
				// Audio Parameters
				Utils::DeserializeOptionalCallback(audioComponent, "StartPosition", [&](uint32_t startPosition) { ac.SetStartPosition(startPosition); });
				Utils::DeserializeOptional<bool>(audioComponent, "StartOnAwake", ac.StartOnAwake);
				Utils::DeserializeOptionalCallback(audioComponent, "3D", [&](bool is3D) { ac.AudioSound->SetIs3D(is3D); });
				Utils::DeserializeOptionalCallback(audioComponent, "Looping", [&](bool looping) { ac.AudioSound->SetLooping(looping); });
				Utils::DeserializeOptionalCallback(audioComponent, "Volume", [&](float volume) { ac.AudioSound->SetVolume(volume); });
				Utils::DeserializeOptionalCallback(audioComponent, "Pan", [&](float pan) { ac.AudioSound->SetPan(pan); });
				Utils::DeserializeOptionalCallback(audioComponent, "Speed", [&](float speed) { ac.AudioSound->SetSpeed(speed); });
				Utils::DeserializeOptionalCallback(audioComponent, "Radius", [&](float radius) { ac.AudioSound->SetRadius(radius); });
				Utils::DeserializeOptionalCallback(audioComponent, "Echo", [&](bool echo) { ac.AudioSound->SetEcho(echo); });
			}
		}

		if (auto splineComponent = entity["SplineComponent"])
		{
			auto& sc = deserializedEntity.AddComponent<SplineComponent>();

			if (auto& pointsNode = splineComponent["Points"])
			{
				std::vector<SplineComponent::SplinePoint> points;
				points.reserve(pointsNode.size());

				for (const auto& pointNode : pointsNode)
				{
					auto& point = points.emplace_back();
					Utils::DeserializeOptional<glm::vec3>(pointNode, "Position", point.Position);
					Utils::DeserializeOptional<glm::vec3>(pointNode, "Tangent", point.Tangent);
					Utils::DeserializeOptionalEnum(pointNode, "Type", point.Type, Utils::SplineTypeFromString);
				}

				sc.SetPoints(points);
			}
		}

		if (auto rigidbodyComponent = entity["RigidbodyComponent"])
		{
			auto& rbc = deserializedEntity.AddComponent<RigidBodyComponent>();
			Utils::DeserializeOptionalEnum(rigidbodyComponent, "BodyType", rbc.Type, Utils::RigidBodyBodyTypeFromString);
			
			Utils::DeserializeOptional<PhysicsLayerID>(rigidbodyComponent, "Layer", rbc.Layer);
			Utils::DeserializeOptional<bool>(rigidbodyComponent, "Sensor", rbc.Sensor);
			Utils::DeserializeOptionalEnum(rigidbodyComponent, "Mode", rbc.Mode, Utils::RigidBodyModeFromString);

			if (rbc.Mode == RigidBodyComponent::MassMode::Mass)
				Utils::DeserializeOptional<float>(rigidbodyComponent, "Mass", rbc.Mass);
			else
				Utils::DeserializeOptional<float>(rigidbodyComponent, "Density", rbc.Density);

			if (auto materialProperties = rigidbodyComponent["Material Properties"])
			{
				Utils::DeserializeOptional<float>(materialProperties, "Friction", rbc.Friction);
				Utils::DeserializeOptional<float>(materialProperties, "Restitution", rbc.Restitution);
			}
		}

		if (auto softBodyComponent = entity["SoftBodyComponent"])
		{
			auto& sbc = deserializedEntity.AddComponent<SoftBodyComponent>();
			Utils::DeserializeOptional(softBodyComponent, "Friction", sbc.Friction);
			Utils::DeserializeOptional(softBodyComponent, "Restitution", sbc.Restitution);
			Utils::DeserializeOptional(softBodyComponent, "Pressure", sbc.Pressure);

			Utils::DeserializeOptional(softBodyComponent, "VertexMass", sbc.VertexMass);
			Utils::DeserializeOptional(softBodyComponent, "VertexRadius", sbc.VertexRadius);
			Utils::DeserializeOptional(softBodyComponent, "UseVertexColorAsWeight", sbc.UseVertexColorAsWeight);
		}

		if (auto boxColliderComponent = entity["BoxColliderComponent"])
		{
			auto& bcc = deserializedEntity.AddComponent<BoxColliderComponent>();
			Utils::DeserializeOptional(boxColliderComponent, "Size", bcc.Size);
		}

		if (auto sphereColliderComponent = entity["SphereColliderComponent"])
		{
			auto& scc = deserializedEntity.AddComponent<SphereColliderComponent>();
			Utils::DeserializeOptional(sphereColliderComponent, "Radius", scc.Radius);
		}

		if (auto capsuleColliderComponent = entity["CapsuleColliderComponent"])
		{
			auto& ccc = deserializedEntity.AddComponent<CapsuleColliderComponent>();
			Utils::DeserializeOptional(capsuleColliderComponent, "Radius", ccc.Radius);
			Utils::DeserializeOptional(capsuleColliderComponent, "HalfHeight", ccc.HalfHeight);
		}

		if (auto meshColliderComponent = entity["MeshColliderComponent"])
		{
			auto& mcc = deserializedEntity.AddComponent<MeshColliderComponent>();
			Utils::DeserializeOptionalEnum(meshColliderComponent, "MeshType", mcc.Type, Utils::MeshColliderMeshTypeFromString);
		}

		if (auto pointConstraintComponent = entity["PointConstraintComponent"])
		{
			auto& pcc = deserializedEntity.AddComponent<PointConstraintComponent>();
			Utils::DeserializeConstraintComponent(pointConstraintComponent, &pcc);
			Utils::DeserializeConstraintSpace(pointConstraintComponent, pcc.Space);

			Utils::DeserializeOptional(pointConstraintComponent, "Local Point", pcc.LocalPoint);
			Utils::DeserializeOptional(pointConstraintComponent, "Target Point", pcc.TargetPoint);
		}

		if (auto coneConstraintComponent = entity["ConeConstraintComponent"])
		{
			auto& ccc = deserializedEntity.AddComponent<ConeConstraintComponent>();
			Utils::DeserializeConstraintComponent(coneConstraintComponent, &ccc);
			Utils::DeserializeConstraintSpace(coneConstraintComponent, ccc.Space);

			Utils::DeserializeReferenceFrame(coneConstraintComponent, ccc, [](const YAML::Node& node, ConeConstraintComponent::ReferenceFrame& frame)
			{
				frame.Offset = node["Offset"].as<glm::vec3>();
				Utils::DeserializeAxis(node["Twist Axis"], frame.TwistAxis);
			});

			Utils::DeserializeOptional(coneConstraintComponent, "Half Cone Angle", ccc.HalfConeAngle);
		}

		if (auto distanceConstraintComponent = entity["DistanceConstraintComponent"])
		{
			auto& dcc = deserializedEntity.AddComponent<DistanceConstraintComponent>();
			Utils::DeserializeConstraintComponent(distanceConstraintComponent, &dcc);

			Utils::DeserializeOptionalEnum(distanceConstraintComponent, "Type", dcc.Type, Utils::DistanceConstraintTypeFromString);
			dcc.UpdateType();

			if (dcc.Type == DistanceConstraintComponent::DistanceType::Fixed)
				Utils::DeserializeOptional(distanceConstraintComponent, "Distance", dcc.Distance);
			else if (dcc.Type == DistanceConstraintComponent::DistanceType::Range)
			{
				Utils::DeserializeOptional(distanceConstraintComponent, "Min Distance", dcc.MinDistance);
				Utils::DeserializeOptional(distanceConstraintComponent, "Max Distance", dcc.MaxDistance);
			}
		}

		if (auto springConstraintComponent = entity["SpringConstraintComponent"])
		{
			auto& scc = deserializedEntity.AddComponent<SpringConstraintComponent>();

			Utils::DeserializeOptionalEnum(springConstraintComponent, "Type", scc.Type, Utils::SpringConstraintTypeFromString);

			if (scc.Type == SpringConstraintComponent::SpringType::FrequencyAndDamping)
				Utils::DeserializeOptional(springConstraintComponent, "Frequency", scc.Frequency);
			else if (scc.Type == SpringConstraintComponent::SpringType::StiffnessAndDamping)
				Utils::DeserializeOptional(springConstraintComponent, "Stiffness", scc.Stiffness);

			Utils::DeserializeOptional(springConstraintComponent, "Damping", scc.Damping);
		}

		if (auto hingeConstraintComponent = entity["HingeConstraintComponent"])
		{
			auto& hcc = deserializedEntity.AddComponent<HingeConstraintComponent>();
			Utils::DeserializeConstraintComponent(hingeConstraintComponent, &hcc);
			Utils::DeserializeConstraintSpace(hingeConstraintComponent, hcc.Space);

			Utils::DeserializeReferenceFrame(hingeConstraintComponent, hcc, [](const YAML::Node& node, HingeConstraintComponent::ReferenceFrame& frame)
			{
				Utils::DeserializeOptional(node, "Point", frame.Point);
				Utils::DeserializeAxis(node["Hinge Axis"], frame.HingeAxis);
				Utils::DeserializeAxis(node["Normal Axis"], frame.NormalAxis);
			});

			Utils::DeserializeOptional(hingeConstraintComponent, "Min Rotation", hcc.MinRotation);
			Utils::DeserializeOptional(hingeConstraintComponent, "Max Rotation", hcc.MaxRotation);
			Utils::DeserializeOptional(hingeConstraintComponent, "Max Friction Torque", hcc.MaxFrictionTorque);
		}

		if (auto fixedConstraintComponent = entity["FixedConstraintComponent"])
		{
			auto& fcc = deserializedEntity.AddComponent<FixedConstraintComponent>();
			Utils::DeserializeConstraintComponent(fixedConstraintComponent, &fcc);
			Utils::DeserializeConstraintSpace(fixedConstraintComponent, fcc.Type);

			Utils::DeserializeReferenceFrame(fixedConstraintComponent, fcc, [](const YAML::Node& node, FixedConstraintComponent::ReferenceFrame& frame)
			{
				Utils::DeserializeOptional(node, "Point", frame.Point);
				Utils::DeserializeAxis(node["Axis X"], frame.AxisX);
				Utils::DeserializeAxis(node["Axis Y"], frame.AxisY);
			});
		}

		if (auto gearConstraintComponent = entity["GearConstraintComponent"])
		{
			auto& gcc = deserializedEntity.AddComponent<GearConstraintComponent>();
			Utils::DeserializeConstraintComponent(gearConstraintComponent, &gcc);
			Utils::DeserializeConstraintSpace(gearConstraintComponent, gcc.Space);

			Utils::DeserializeAxis(gearConstraintComponent["Local Hinge Axis"], gcc.LocalHingeAxis);
			Utils::DeserializeAxis(gearConstraintComponent["Target Hinge Axis"], gcc.TargetHingeAxis);
			Utils::DeserializeOptional(gearConstraintComponent, "Ratio", gcc.Ratio);
		}

		if (auto pulleyConstraintComponent = entity["PulleyConstraintComponent"])
		{
			auto& pcc = deserializedEntity.AddComponent<PulleyConstraintComponent>();
			Utils::DeserializeConstraintComponent(pulleyConstraintComponent, &pcc);
			Utils::DeserializeConstraintSpace(pulleyConstraintComponent, pcc.Space);

			Utils::DeserializeReferenceFrame(pulleyConstraintComponent, pcc, [](const YAML::Node& node, PulleyConstraintComponent::ReferenceFrame& frame)
			{
				Utils::DeserializeOptional(node, "Body Point", frame.BodyPoint);
				Utils::DeserializeOptional(node, "Fixed Point", frame.FixedPoint);
			});

			Utils::DeserializeOptional(pulleyConstraintComponent, "Ratio", pcc.Ratio);
			Utils::DeserializeOptional(pulleyConstraintComponent, "Min Length", pcc.MinLength);
			Utils::DeserializeOptional(pulleyConstraintComponent, "Max Length", pcc.MaxLength);
		}

		if (auto rackAndPinionConstraintComponent = entity["RackAndPinionConstraintComponent"])
		{
			auto& rapcc = deserializedEntity.AddComponent<RackAndPinionConstraintComponent>();
			Utils::DeserializeConstraintComponent(rackAndPinionConstraintComponent, &rapcc);
			Utils::DeserializeConstraintSpace(rackAndPinionConstraintComponent, rapcc.Space);

			Utils::DeserializeOptionalEnum(rackAndPinionConstraintComponent, "Mode", rapcc.Mode, Utils::RackAndPinionConstraintModeFromString);

			Utils::DeserializeAxis(rackAndPinionConstraintComponent["Hinge Axis"], rapcc.HingeAxis);
			Utils::DeserializeAxis(rackAndPinionConstraintComponent["Slider Axis"], rapcc.SliderAxis);

			if (rapcc.Mode == RackAndPinionConstraintComponent::RatioMode::Ratio)
				Utils::DeserializeOptional(rackAndPinionConstraintComponent, "Ratio", rapcc.Ratio);
			else
			{
				Utils::DeserializeOptional(rackAndPinionConstraintComponent, "Rack Teeth Count", rapcc.RackTeethCount);
				Utils::DeserializeOptional(rackAndPinionConstraintComponent, "Pinion Teeth Count", rapcc.PinionTeethCount);
				Utils::DeserializeOptional(rackAndPinionConstraintComponent, "Rack Length", rapcc.RackLength);
			}
		}

		if (auto swingTwistConstraintComponent = entity["SwingTwistConstraintComponent"])
		{
			auto& stcc = deserializedEntity.AddComponent<SwingTwistConstraintComponent>();
			Utils::DeserializeConstraintComponent(swingTwistConstraintComponent, &stcc);
			Utils::DeserializeConstraintSpace(swingTwistConstraintComponent, stcc.Space);

			Utils::DeserializeOptionalEnum(swingTwistConstraintComponent, "Swing Type", stcc.SwingType, Utils::ConstraintSwingTypeFromString);

			Utils::DeserializeReferenceFrame(swingTwistConstraintComponent, stcc, [](const YAML::Node& node, SwingTwistConstraintComponent::ReferenceFrame& frame)
			{
				Utils::DeserializeOptional(node, "Body Point", frame.Position);
				Utils::DeserializeAxis(node["Twist Axis"], frame.TwistAxis);
				Utils::DeserializeAxis(node["Plane Axis"], frame.PlaneAxis);
			});

			Utils::DeserializeOptional(swingTwistConstraintComponent, "Normal Half Cone Angle", stcc.NormalHalfConeAngle);
			Utils::DeserializeOptional(swingTwistConstraintComponent, "Plane Half Cone Angle", stcc.PlaneHalfConeAngle);
			Utils::DeserializeOptional(swingTwistConstraintComponent, "Twist Min Angle", stcc.TwistMinAngle);
			Utils::DeserializeOptional(swingTwistConstraintComponent, "Twist Max Angle", stcc.TwistMaxAngle);
			Utils::DeserializeOptional(swingTwistConstraintComponent, "Max Friction Torque", stcc.MaxFrictionTorque);
		}

		if (auto sliderConstraintComponent = entity["SliderConstraintComponent"])
		{
			auto& scc = deserializedEntity.AddComponent<SliderConstraintComponent>();
			Utils::DeserializeConstraintComponent(sliderConstraintComponent, &scc);
			Utils::DeserializeConstraintSpace(sliderConstraintComponent, scc.Space);

			Utils::DeserializeReferenceFrame(sliderConstraintComponent, scc, [](const YAML::Node& node, SliderConstraintComponent::ReferenceFrame& frame)
			{
				Utils::DeserializeOptional(node, "Point", frame.Point);
				Utils::DeserializeAxis(node["Slider Axis"], frame.SliderAxis);
				Utils::DeserializeAxis(node["Normal Axis"], frame.NormalAxis);
			});

			Utils::DeserializeOptional(sliderConstraintComponent, "Slider Min", scc.SliderMin);
			Utils::DeserializeOptional(sliderConstraintComponent, "Slider Max", scc.SliderMax);
			Utils::DeserializeOptional(sliderConstraintComponent, "Max Friction Force", scc.MaxFrictionForce);
		}

		if (auto sixDOFConstraintComponent = entity["SixDOFConstraintComponent"])
		{
			auto& sdcc = deserializedEntity.AddComponent<SixDOFConstraintComponent>();
			Utils::DeserializeConstraintComponent(sixDOFConstraintComponent, &sdcc);
			Utils::DeserializeConstraintSpace(sixDOFConstraintComponent, sdcc.Space);

			Utils::DeserializeReferenceFrame(sixDOFConstraintComponent, sdcc, [](const YAML::Node& node, SixDOFConstraintComponent::ReferenceFrame& frame)
			{
				Utils::DeserializeOptional(node, "Position", frame.Position);
				Utils::DeserializeAxis(node["Axis X"], frame.AxisX);
				Utils::DeserializeAxis(node["Axis Y"], frame.AxisY);
			});

			Utils::DeserializeOptionalEnum(sixDOFConstraintComponent, "Swing Type", sdcc.SwingType, Utils::ConstraintSwingTypeFromString);

			if (auto axesNode = sixDOFConstraintComponent["Axes"])
			{
				for (auto axisNode : axesNode)
				{
					SixDOFConstraintComponent::Axis axis = SixDOFConstraintComponent::Axis::AxisCount;
					if (!Utils::DeserializeOptionalEnum(axisNode, "Name", axis, Utils::SixDOFConstraintAxisFromString))
						continue;

					if (axis == SixDOFConstraintComponent::Axis::AxisCount)
						continue;

					SixDOFConstraintComponent::AxisStatus status = SixDOFConstraintComponent::AxisStatus::Free;
					Utils::DeserializeOptionalEnum(axisNode, "Status", status, Utils::SixDOFConstraintAxisStatusFromString);
					sdcc.SetAxisStatus(axis, status);

					Utils::DeserializeOptional(axisNode, "Max Friction", sdcc.MaxFriction[axis]);

					if (status == SixDOFConstraintComponent::AxisStatus::Custom)
					{
						Utils::DeserializeOptional(axisNode, "Limit Min", sdcc.LimitMin[axis]);
						Utils::DeserializeOptional(axisNode, "Limit Max", sdcc.LimitMax[axis]);
					}
				}
			}
		}

		if (auto followConstraintComponent = entity["FollowConstraintComponent"])
		{
			auto& fcc = deserializedEntity.AddComponent<FollowConstraintComponent>();
			Utils::DeserializeConstraintComponent(followConstraintComponent, &fcc);

			Utils::DeserializeOptional(followConstraintComponent, "Looping", fcc.Looping);
			Utils::DeserializeAxis(followConstraintComponent["Normal"], fcc.Normal);
			Utils::DeserializeOptional(followConstraintComponent, "Start Fraction", fcc.StartFraction);
			Utils::DeserializeOptional(followConstraintComponent, "Max Friction Force", fcc.MaxFrictionForce);
			Utils::DeserializeOptionalEnum(followConstraintComponent, "Rotation Constraint", fcc.RotationConstraint, Utils::FollowConstraintRotationTypeFromString);
			Utils::DeserializeOptional(followConstraintComponent, "Base Target", fcc.BaseTarget);
		}

		if (auto springArmComponent = entity["SpringArmComponent"])
		{
			auto& sac = deserializedEntity.AddComponent<SpringArmComponent>();

			Utils::DeserializeOptional(springArmComponent, "TargetLength", sac.TargetLength);
			Utils::DeserializeOptional(springArmComponent, "ProbeRadius", sac.ProbeRadius);
			Utils::DeserializeOptional(springArmComponent, "TargetOffset", sac.TargetOffset);
			Utils::DeserializeOptional(springArmComponent, "SocketOffset", sac.SocketOffset);
		}

		if (auto fieldComponent = entity["FieldComponent"])
		{
			auto& fc = deserializedEntity.AddComponent<FieldComponent>();

			Utils::DeserializeOptionalEnum(fieldComponent, "Type", fc.Type, Utils::FieldTypeFromString);
			Utils::DeserializeOptional(fieldComponent, "Layer", fc.Layer);

			if (fc.Type == FieldComponent::FieldType::Directional)
			{
				// Directional
				Utils::DeserializeOptional(fieldComponent, "Force", fc.Force);
			}
			else if (fc.Type == FieldComponent::FieldType::Radial)
			{
				// Radial
				Utils::DeserializeOptional(fieldComponent, "Magnitude", fc.Magnitude);
				Utils::DeserializeOptional(fieldComponent, "Radius", fc.Radius);
				Utils::DeserializeOptional(fieldComponent, "Falloff", fc.Falloff);
			}
			else
			{
				// Buoyancy
				Utils::DeserializeOptional(fieldComponent, "Buoyancy", fc.Buoyancy);
				Utils::DeserializeOptional(fieldComponent, "Linear Drag", fc.LinearDrag);
				Utils::DeserializeOptional(fieldComponent, "Angular Drag", fc.AngularDrag);
				Utils::DeserializeOptional(fieldComponent, "Fluid Velocity", fc.FluidVelocity);
			}
		}

		if (auto landscapeComponent = entity["LandscapeComponent"])
		{
			auto& lc = deserializedEntity.AddComponent<LandscapeComponent>();

			Utils::DeserializeOptional(landscapeComponent, "Resolution", lc.Resolution);
			Utils::DeserializeOptionalAsset(landscapeComponent, "Material", lc.Material);

			lc.Allocate();

			auto heightNodeList = landscapeComponent["Height"];

			uint32_t index = 0;
			for (const auto& heightNode : heightNodeList)
				lc.Data->Set<float>(index++, heightNode.as<float>());

			lc.Build();
		}
	}

	void EntityRegistrySerializer::DeserializeEntity(FileStreamReader& stream, Ref<EntityRegistry> scene)
	{
		// Create Entity with ID and Tag Component
		EntityHandle uuid;
		stream.ReadRaw<EntityHandle>(uuid);

		std::string tag;
		stream.ReadString(tag);

		Entity entity = scene->CreateEntityWithUUID(uuid, tag);

		// Relationship Component
		if (stream.ReadRaw<bool>())
		{
			auto& rc = entity.GetComponent<RelationshipComponent>();
			stream.ReadRaw<EntityHandle>(rc.ParentHandle);

			uint32_t childCount = stream.ReadRaw<uint32_t>();
			rc.Children.reserve(childCount);

			for (uint32_t i = 0; i < childCount; i++)
				rc.Children.push_back(stream.ReadRaw<EntityHandle>());
		}

		// Attachment Component
		if (stream.ReadRaw<bool>())
		{
			auto& ac = entity.AddComponent<AttachmentComponent>();
			stream.ReadString(ac.BoneName);
		}

		// Prefab Component
		if (stream.ReadRaw<bool>())
		{
			auto& pc = entity.AddComponent<PrefabComponent>();
			stream.ReadRaw<AssetHandle>(pc.PrefabID);
		}

		// Folder Component
		if (stream.ReadRaw<bool>())
		{
			entity.RemoveComponent<TransformComponent>();
			auto& fc = entity.AddComponent<FolderComponent>();
			stream.ReadRaw<glm::vec4>(fc.Color);
		}

		// Transform Component
		if (stream.ReadRaw<bool>())
		{
			auto& transform = entity.GetComponent<TransformComponent>().Transform;
			stream.ReadRaw<glm::vec3>(transform.Translation);
			stream.ReadRaw<glm::quat>(transform.Rotation);
			stream.ReadRaw<glm::vec3>(transform.Scale);
		}

		// Camera Component
		if (stream.ReadRaw<bool>())
		{
			auto& cc = entity.AddComponent<CameraComponent>();
			auto& camera = cc.Camera;
			auto& settings = camera.GetCameraSettings();

			// Camera
			camera.SetProjectionType((SceneCamera::ProjectionType)stream.ReadRaw<uint8_t>());
			camera.SetPerspectiveVerticalFOV(stream.ReadRaw<float>());
			camera.SetPerspectiveNearClip(stream.ReadRaw<float>());
			camera.SetPerspectiveFarClip(stream.ReadRaw<float>());
			camera.SetOrthographicSize(stream.ReadRaw<float>());
			camera.SetOrthographicNearClip(stream.ReadRaw<float>());
			camera.SetOrthographicFarClip(stream.ReadRaw<float>());

			stream.ReadRaw<bool>(cc.Primary);
			stream.ReadRaw<bool>(cc.FixedAspectRatio);

			// Settings
			stream.ReadRaw<float>(settings.BloomThreshold);

			if (const AssetHandle bloomDirtHandle = stream.ReadRaw<AssetHandle>())
				settings.BloomDirtTexture = AssetManager::GetAsset<Texture2D>(bloomDirtHandle);

			stream.ReadRaw<float>(settings.DOFStrength);
			stream.ReadRaw<float>(settings.DOFTarget);
			stream.ReadRaw<float>(settings.DOFFocusRange);
			stream.ReadRaw<float>(settings.DOFFocusFalloff);

			if (const AssetHandle lutHandle = stream.ReadRaw<AssetHandle>())
				settings.LUT = AssetManager::GetAsset<Texture2D>(lutHandle);
		}

		// Capture Component
		if (stream.ReadRaw<bool>())
		{
			auto& cc = entity.AddComponent<CaptureComponent>();
			stream.ReadRaw<bool>(cc.Capture);
			stream.ReadRaw<bool>(cc.Cumulative);
			cc.Type = (SceneRendererContext::RendererVisualizationMode)stream.ReadRaw<uint8_t>();
			cc.MaskType = (SceneRendererContext::RenderMaskType)stream.ReadRaw<uint8_t>();
			cc.Target = AssetManager::GetAsset<Texture2D>(stream.ReadRaw<AssetHandle>());
		}

		// Script Component
		if (stream.ReadRaw<bool>())
		{
			auto& sc = entity.AddComponent<ScriptComponent>();
			stream.ReadString(sc.ClassName);

			Ref<ScriptClass> entityClass = ScriptEngine::GetEntityClass(sc.ClassName);

			if (entityClass)
			{
				const auto& fields = entityClass->GetFields();
				auto& entityFields = ScriptEngine::GetScriptFieldMap(entity);

				uint32_t scriptFieldCount = stream.ReadRaw<uint32_t>();
				for (uint32_t scriptFieldIndex = 0; scriptFieldIndex < scriptFieldCount; scriptFieldIndex++)
				{
					std::string name;
					stream.ReadString(name);
					const ScriptFieldType type = (ScriptFieldType)stream.ReadRaw<uint8_t>();

					ScriptFieldInstance& fieldInstance = entityFields[name];

					DY_CORE_VERIFY(fields.find(name) != fields.end());

					if (fields.find(name) == fields.end())
						continue;

					fieldInstance.Field = fields.at(name);

#define READ_SCRIPT_FIELD_RUNTIME(FieldType, Type)	\
						case ScriptFieldType::FieldType:				\
						{												\
							Type data = stream.ReadRaw<Type>();			\
							fieldInstance.SetValue(data);				\
							break;										\
						}


					switch (type)
					{
						READ_SCRIPT_FIELD_RUNTIME(Float, float);
						READ_SCRIPT_FIELD_RUNTIME(Double, double);
						READ_SCRIPT_FIELD_RUNTIME(Bool, bool);
						READ_SCRIPT_FIELD_RUNTIME(Char, char);
						READ_SCRIPT_FIELD_RUNTIME(Byte, int8_t);
						READ_SCRIPT_FIELD_RUNTIME(Short, int16_t);
						READ_SCRIPT_FIELD_RUNTIME(Int, int32_t);
						READ_SCRIPT_FIELD_RUNTIME(Long, int64_t);
						READ_SCRIPT_FIELD_RUNTIME(UShort, uint16_t);
						READ_SCRIPT_FIELD_RUNTIME(UInt, uint32_t);
						READ_SCRIPT_FIELD_RUNTIME(ULong, uint64_t);
						READ_SCRIPT_FIELD_RUNTIME(Vector2, glm::vec2);
						READ_SCRIPT_FIELD_RUNTIME(Vector3, glm::vec3);
						READ_SCRIPT_FIELD_RUNTIME(Vector4, glm::vec4);
						READ_SCRIPT_FIELD_RUNTIME(Entity, UUID);
						READ_SCRIPT_FIELD_RUNTIME(Asset, UUID);
						READ_SCRIPT_FIELD_RUNTIME(Scene, UUID);
						READ_SCRIPT_FIELD_RUNTIME(Texture, UUID);
						READ_SCRIPT_FIELD_RUNTIME(VirtualTexture, UUID);
						READ_SCRIPT_FIELD_RUNTIME(Mesh, UUID);
						READ_SCRIPT_FIELD_RUNTIME(Animation, UUID);
						READ_SCRIPT_FIELD_RUNTIME(Material, UUID);
						READ_SCRIPT_FIELD_RUNTIME(Audio, UUID);
						READ_SCRIPT_FIELD_RUNTIME(VideoPlayer, UUID);
					}
				}
			}
		}

		// Sprite Renderer Component
		if (stream.ReadRaw<bool>())
		{
			auto& src = entity.AddComponent<SpriteRendererComponent>();
			stream.ReadRaw<glm::vec4>(src.Color);
			stream.ReadRaw<float>(src.TilingFactor);

			if (const AssetHandle textureHandle = stream.ReadRaw<AssetHandle>())
				src.Texture = AssetManager::GetAsset<Texture2D>(textureHandle);
		}

		// Circle Renderer Component
		if (stream.ReadRaw<bool>())
		{
			auto& crc = entity.AddComponent<CircleRendererComponent>();
			stream.ReadRaw<glm::vec4>(crc.Color);
			stream.ReadRaw<float>(crc.Thickness);
			stream.ReadRaw<float>(crc.Fade);
		}

		// Text Component
		if (stream.ReadRaw<bool>())
		{
			auto& tc = entity.AddComponent<TextComponent>();
			stream.ReadString(tc.TextString);
			tc.Alignment = (TextAlignment)stream.ReadRaw<uint8_t>();
			stream.ReadRaw<glm::vec4>(tc.Color);
			stream.ReadRaw<float>(tc.Kerning);
			stream.ReadRaw<float>(tc.LineSpacing);
			stream.ReadRaw<float>(tc.MaxWidth);

			if (const AssetHandle fontHandle = stream.ReadRaw<AssetHandle>())
				tc.Font = AssetManager::GetAsset<Font>(fontHandle);
		}

		// Particle System Component
		if (stream.ReadRaw<bool>())
		{
			auto& psc = entity.AddComponent<ParticleSystemComponent>();

			const Ref<ParticleSystem> particleSystem = AssetManager::GetAsset<ParticleSystem>(stream.ReadRaw<AssetHandle>());
			const Ref<MaterialAsset> material = AssetManager::GetAsset<MaterialAsset>(stream.ReadRaw<AssetHandle>());
			psc.SetParticleSystem(particleSystem, material);
		}

		// Rigidbody 2D Component
		if (stream.ReadRaw<bool>())
		{
			auto& rbc = entity.AddComponent<RigidBody2DComponent>();
			rbc.Type = (RigidBody2DComponent::BodyType)stream.ReadRaw<uint8_t>();
			stream.ReadRaw<bool>(rbc.FixedRotation);
		}

		// Box Collider 2D Component
		if (stream.ReadRaw<bool>())
		{
			auto& bcc = entity.AddComponent<BoxCollider2DComponent>();
			stream.ReadRaw<glm::vec2>(bcc.Offset);
			stream.ReadRaw<glm::vec2>(bcc.Size);
			stream.ReadRaw<float>(bcc.Density);
			stream.ReadRaw<float>(bcc.Friction);
			stream.ReadRaw<float>(bcc.Restitution);
			stream.ReadRaw<float>(bcc.RestitutionThreshold);
		}

		// Circle Collider 2D Component
		if (stream.ReadRaw<bool>())
		{
			auto& ccc = entity.AddComponent<CircleCollider2DComponent>();
			stream.ReadRaw<glm::vec2>(ccc.Offset);
			stream.ReadRaw<float>(ccc.Radius);
			stream.ReadRaw<float>(ccc.Density);
			stream.ReadRaw<float>(ccc.Friction);
			stream.ReadRaw<float>(ccc.Restitution);
			stream.ReadRaw<float>(ccc.RestitutionThreshold);
		}

		// Static Mesh Component
		if (stream.ReadRaw<bool>())
		{
			auto& smc = entity.AddComponent<StaticMeshComponent>();
			if (const AssetHandle meshHandle = stream.ReadRaw<AssetHandle>())
			{
				smc.SetModel(AssetManager::GetAsset<Model>(meshHandle));

				if (const AssetHandle animationGraphHandle = stream.ReadRaw<AssetHandle>())
					smc.SetAnimationGraph(AssetManager::GetAsset<AnimationGraph>(animationGraphHandle));

				const uint32_t materialCount = stream.ReadRaw<uint32_t>();
				for (uint32_t materialIndex = 0; materialIndex < materialCount; materialIndex++)
				{
					if (const AssetHandle materialHandle = stream.ReadRaw<AssetHandle>())
						smc.m_Materials[materialIndex] = AssetManager::GetAsset<MaterialAsset>(materialHandle);
				}
			}
		}

		// Directional Light Component
		if (stream.ReadRaw<bool>())
		{
			auto& dlc = entity.AddComponent<DirectionalLightComponent>();
			stream.ReadRaw<glm::vec3>(dlc.Color);
			stream.ReadRaw<float>(dlc.Intensity);
		}

		// Point Light Component
		if (stream.ReadRaw<bool>())
		{
			auto& plc = entity.AddComponent<PointLightComponent>();
			stream.ReadRaw<glm::vec3>(plc.Color);
			stream.ReadRaw<float>(plc.Intensity);
			stream.ReadRaw<float>(plc.Radius);
			stream.ReadRaw<bool>(plc.CastsShadows);
		}

		// Sky Light Component
		if (stream.ReadRaw<bool>())
		{
			auto& slc = entity.AddComponent<SkyLightComponent>();
			slc.Type = (SkyLightComponent::SkyType)stream.ReadRaw<uint8_t>();

			if (slc.Type == SkyLightComponent::SkyType::EnvironmentMap)
			{
				if (const AssetHandle environmentMapHandle = stream.ReadRaw<AssetHandle>())
					slc.EnvironmentMap = AssetManager::GetAsset<EnvironmentMap>(environmentMapHandle);

				if (const AssetHandle flowMapHandle = stream.ReadRaw<AssetHandle>())
					slc.FlowMap = AssetManager::GetAsset<Texture2D>(flowMapHandle);
			}

			stream.ReadRaw<float>(slc.Intensity);
		}

		// Decal Component
		if (stream.ReadRaw<bool>())
		{
			auto& dc = entity.AddComponent<DecalComponent>();
			stream.ReadRaw<bool>(dc.ConstrainAngle);
			if (const AssetHandle textureHandle = stream.ReadRaw<AssetHandle>())
				dc.Texture = AssetManager::GetAsset<Texture2D>(textureHandle);
		}

		// Post Process Volume Component
		if (stream.ReadRaw<bool>())
		{
			auto& ppvc = entity.AddComponent<PostProcessVolumeComponent>();
			stream.ReadRaw<bool>(ppvc.Bounded);
			stream.ReadRaw<bool>(ppvc.Enabled);

			if (const AssetHandle materialHandle = stream.ReadRaw<AssetHandle>())
				ppvc.Material = AssetManager::GetAsset<MaterialAsset>(materialHandle);
		}

		// Volume Component
		if (stream.ReadRaw<bool>())
		{
			auto& vc = entity.AddComponent<VolumeComponent>();
			vc.Blend = (VolumeComponent::BlendType)stream.ReadRaw<uint8_t>();
			stream.ReadRaw<glm::vec3>(vc.Color);
			stream.ReadRaw<float>(vc.ExtinctionScale);
			stream.ReadRaw<float>(vc.ScatteringDistribution);
			stream.ReadRaw<float>(vc.ScatteringIntensity);
		}

		// Audio Component
		if (stream.ReadRaw<bool>())
		{
			auto& ac = entity.AddComponent<AudioComponent>();

			if (const AssetHandle audioHandle = stream.ReadRaw<AssetHandle>())
			{
				ac.AudioSound = AssetManager::GetAsset<Audio>(audioHandle);
				stream.ReadRaw<uint32_t>(ac.StartPosition);
				stream.ReadRaw<bool>(ac.StartOnAwake);
				ac.AudioSound->SetIs3D(stream.ReadRaw<bool>());
				ac.AudioSound->SetLooping(stream.ReadRaw<bool>());
				ac.AudioSound->SetVolume(stream.ReadRaw<float>());
				ac.AudioSound->SetPan(stream.ReadRaw<float>());
				ac.AudioSound->SetSpeed(stream.ReadRaw<float>());
				ac.AudioSound->SetRadius(stream.ReadRaw<float>());
				ac.AudioSound->SetEcho(stream.ReadRaw<bool>());
			}
		}

		// Spline Component
		if (stream.ReadRaw<bool>())
		{
			auto& sc = entity.AddComponent<SplineComponent>();

			const uint32_t pointCount = stream.ReadRaw<uint32_t>();
			std::vector<SplineComponent::SplinePoint> points;
			points.reserve(pointCount);
			for (uint32_t pointIndex = 0; pointIndex < pointCount; pointIndex++)
			{
				auto& point = points.emplace_back();
				stream.ReadRaw<glm::vec3>(point.Position);
				stream.ReadRaw<glm::vec3>(point.Tangent);
				point.Type = (SplineComponent::SplineType)stream.ReadRaw<uint8_t>();
			}

			sc.SetPoints(points);
		}

		// Rigid Body Component
		if (stream.ReadRaw<bool>())
		{
			auto& rbc = entity.AddComponent<RigidBodyComponent>();
			rbc.Type = (RigidBodyComponent::BodyType)stream.ReadRaw<uint8_t>();
			rbc.Layer = stream.ReadRaw<PhysicsLayerID>();
			rbc.Sensor = stream.ReadRaw<bool>();
			rbc.Mode = (RigidBodyComponent::MassMode)stream.ReadRaw<uint8_t>();
			stream.ReadRaw<float>(rbc.Density);
			stream.ReadRaw<float>(rbc.Friction);
			stream.ReadRaw<float>(rbc.Restitution);
		}

		// Soft Body Component
		if (stream.ReadRaw<bool>())
		{
			auto& sbc = entity.AddComponent<SoftBodyComponent>();
			stream.ReadRaw<float>(sbc.Friction);
			stream.ReadRaw<float>(sbc.Restitution);
			stream.ReadRaw<float>(sbc.Pressure);

			stream.ReadRaw<float>(sbc.VertexMass);
			stream.ReadRaw<float>(sbc.VertexRadius);
			stream.ReadRaw<bool>(sbc.UseVertexColorAsWeight);
		}

		// Box Collider Component
		if (stream.ReadRaw<bool>())
		{
			auto& bcc = entity.AddComponent<BoxColliderComponent>();
			stream.ReadRaw<glm::vec3>(bcc.Size);
		}

		// Sphere Collider Component
		if (stream.ReadRaw<bool>())
		{
			auto& scc = entity.AddComponent<SphereColliderComponent>();
			stream.ReadRaw<float>(scc.Radius);
		}

		// Capsule Collider Component
		if (stream.ReadRaw<bool>())
		{
			auto& ccc = entity.AddComponent<CapsuleColliderComponent>();
			stream.ReadRaw<float>(ccc.Radius);
			stream.ReadRaw<float>(ccc.HalfHeight);
		}

		// Mesh Collider Component
		if (stream.ReadRaw<bool>())
		{
			auto& mcc = entity.AddComponent<MeshColliderComponent>();
			mcc.Type = (MeshColliderComponent::MeshType)stream.ReadRaw<uint8_t>();
		}

		// Point Constraint Component
		if (stream.ReadRaw<bool>())
		{
			auto& pcc = entity.AddComponent<PointConstraintComponent>();
			Utils::DeserializeConstraintComponent(stream, &pcc);
			pcc.Space = (ConstraintSpace)stream.ReadRaw<uint8_t>();
			stream.ReadRaw<glm::vec3>(pcc.LocalPoint);
			stream.ReadRaw<glm::vec3>(pcc.TargetPoint);
		}

		// Cone Constraint Component
		if (stream.ReadRaw<bool>())
		{
			auto& ccc = entity.AddComponent<ConeConstraintComponent>();
			Utils::DeserializeConstraintComponent(stream, &ccc);
			ccc.Space = (ConstraintSpace)stream.ReadRaw<uint8_t>();
			stream.ReadRaw<glm::vec3>(ccc.LocalReferenceFrame.Offset);
			Utils::DeserializeAxis(stream, ccc.LocalReferenceFrame.TwistAxis);
			stream.ReadRaw<glm::vec3>(ccc.TargetReferenceFrame.Offset);
			Utils::DeserializeAxis(stream, ccc.TargetReferenceFrame.TwistAxis);
			stream.ReadRaw<float>(ccc.HalfConeAngle);
		}

		// Distance Constraint Component
		if (stream.ReadRaw<bool>())
		{
			auto& dcc = entity.AddComponent<DistanceConstraintComponent>();
			Utils::DeserializeConstraintComponent(stream, &dcc);
			dcc.Type = (DistanceConstraintComponent::DistanceType)stream.ReadRaw<uint8_t>();

			if (dcc.Type == DistanceConstraintComponent::DistanceType::Fixed)
				stream.ReadRaw<float>(dcc.Distance);
			else if (dcc.Type == DistanceConstraintComponent::DistanceType::Range)
			{
				stream.ReadRaw<float>(dcc.MinDistance);
				stream.ReadRaw<float>(dcc.MaxDistance);
			}
		}

		// Spring Constraint Component
		if (stream.ReadRaw<bool>())
		{
			auto& scc = entity.AddComponent<SpringConstraintComponent>();
			scc.Type = (SpringConstraintComponent::SpringType)stream.ReadRaw<uint8_t>();

			if (scc.Type == SpringConstraintComponent::SpringType::FrequencyAndDamping)
				stream.ReadRaw<float>(scc.Frequency);
			else if (scc.Type == SpringConstraintComponent::SpringType::StiffnessAndDamping)
				stream.ReadRaw<float>(scc.Stiffness);

			stream.ReadRaw<float>(scc.Damping);
		}

		// Hinge Constraint Component
		if (stream.ReadRaw<bool>())
		{
			auto& hcc = entity.AddComponent<HingeConstraintComponent>();
			Utils::DeserializeConstraintComponent(stream, &hcc);
			hcc.Space = (ConstraintSpace)stream.ReadRaw<uint8_t>();

			stream.ReadRaw<glm::vec3>(hcc.LocalReferenceFrame.Point);
			Utils::DeserializeAxis(stream, hcc.LocalReferenceFrame.HingeAxis);
			Utils::DeserializeAxis(stream, hcc.LocalReferenceFrame.NormalAxis);
			stream.ReadRaw<glm::vec3>(hcc.TargetReferenceFrame.Point);
			Utils::DeserializeAxis(stream, hcc.TargetReferenceFrame.HingeAxis);
			Utils::DeserializeAxis(stream, hcc.TargetReferenceFrame.NormalAxis);

			stream.ReadRaw<float>(hcc.MinRotation);
			stream.ReadRaw<float>(hcc.MaxRotation);
			stream.ReadRaw<float>(hcc.MaxFrictionTorque);
		}

		// Fixed Constraint Component
		if (stream.ReadRaw<bool>())
		{
			auto& fcc = entity.AddComponent<FixedConstraintComponent>();
			Utils::DeserializeConstraintComponent(stream, &fcc);
			fcc.Type = (ConstraintSpace)stream.ReadRaw<uint8_t>();

			stream.ReadRaw<glm::vec3>(fcc.LocalReferenceFrame.Point);
			Utils::DeserializeAxis(stream, fcc.LocalReferenceFrame.AxisX);
			Utils::DeserializeAxis(stream, fcc.LocalReferenceFrame.AxisY);
			stream.ReadRaw<glm::vec3>(fcc.TargetReferenceFrame.Point);
			Utils::DeserializeAxis(stream, fcc.TargetReferenceFrame.AxisX);
			Utils::DeserializeAxis(stream, fcc.TargetReferenceFrame.AxisY);
		}

		// Gear Constraint Component
		if (stream.ReadRaw<bool>())
		{
			auto& gcc = entity.AddComponent<GearConstraintComponent>();
			Utils::DeserializeConstraintComponent(stream, &gcc);
			gcc.Space = (ConstraintSpace)stream.ReadRaw<uint8_t>();

			Utils::DeserializeAxis(stream, gcc.LocalHingeAxis);
			Utils::DeserializeAxis(stream, gcc.TargetHingeAxis);
			stream.ReadRaw<Fraction>(gcc.Ratio);
		}

		// Pulley Constraint Component
		if (stream.ReadRaw<bool>())
		{
			auto& pcc = entity.AddComponent<PulleyConstraintComponent>();
			Utils::DeserializeConstraintComponent(stream, &pcc);
			pcc.Space = (ConstraintSpace)stream.ReadRaw<uint8_t>();

			stream.ReadRaw<glm::vec3>(pcc.LocalReferenceFrame.BodyPoint);
			stream.ReadRaw<glm::vec3>(pcc.LocalReferenceFrame.FixedPoint);
			stream.ReadRaw<glm::vec3>(pcc.TargetReferenceFrame.BodyPoint);
			stream.ReadRaw<glm::vec3>(pcc.TargetReferenceFrame.FixedPoint);

			stream.ReadRaw<Fraction>(pcc.Ratio);
			stream.ReadRaw<float>(pcc.MinLength);
			stream.ReadRaw<float>(pcc.MaxLength);
		}

		// Rack And Pinion Constraint Component
		if (stream.ReadRaw<bool>())
		{
			auto& rapcc = entity.AddComponent<RackAndPinionConstraintComponent>();
			Utils::DeserializeConstraintComponent(stream, &rapcc);
			rapcc.Space = (ConstraintSpace)stream.ReadRaw<uint8_t>();

			rapcc.Mode = (RackAndPinionConstraintComponent::RatioMode)stream.ReadRaw<uint8_t>();
			Utils::DeserializeAxis(stream, rapcc.HingeAxis);
			Utils::DeserializeAxis(stream, rapcc.SliderAxis);

			if (rapcc.Mode == RackAndPinionConstraintComponent::RatioMode::Ratio)
				stream.ReadRaw<float>(rapcc.Ratio);
			else
			{
				stream.ReadRaw<uint32_t>(rapcc.RackTeethCount);
				stream.ReadRaw<uint32_t>(rapcc.PinionTeethCount);
				stream.ReadRaw<float>(rapcc.RackLength);
			}
		}

		// Swing Twist Constraint Component
		if (stream.ReadRaw<bool>())
		{
			auto& stcc = entity.AddComponent<SwingTwistConstraintComponent>();
			Utils::DeserializeConstraintComponent(stream, &stcc);
			stcc.Space = (ConstraintSpace)stream.ReadRaw<uint8_t>();
			stcc.SwingType = (ConstraintSwingType)stream.ReadRaw<uint8_t>();

			stream.ReadRaw<glm::vec3>(stcc.LocalReferenceFrame.Position);
			Utils::DeserializeAxis(stream, stcc.LocalReferenceFrame.TwistAxis);
			Utils::DeserializeAxis(stream, stcc.LocalReferenceFrame.PlaneAxis);
			stream.ReadRaw<glm::vec3>(stcc.TargetReferenceFrame.Position);
			Utils::DeserializeAxis(stream, stcc.TargetReferenceFrame.TwistAxis);
			Utils::DeserializeAxis(stream, stcc.TargetReferenceFrame.PlaneAxis);

			stream.ReadRaw<float>(stcc.NormalHalfConeAngle);
			stream.ReadRaw<float>(stcc.PlaneHalfConeAngle);
			stream.ReadRaw<float>(stcc.TwistMinAngle);
			stream.ReadRaw<float>(stcc.TwistMaxAngle);
			stream.ReadRaw<float>(stcc.MaxFrictionTorque);
		}

		// Slider Constraint Component
		if (stream.ReadRaw<bool>())
		{
			auto& scc = entity.AddComponent<SliderConstraintComponent>();
			Utils::DeserializeConstraintComponent(stream, &scc);
			scc.Space = (ConstraintSpace)stream.ReadRaw<uint8_t>();

			stream.ReadRaw<glm::vec3>(scc.LocalReferenceFrame.Point);
			Utils::DeserializeAxis(stream, scc.LocalReferenceFrame.SliderAxis);
			Utils::DeserializeAxis(stream, scc.LocalReferenceFrame.NormalAxis);
			stream.ReadRaw<glm::vec3>(scc.TargetReferenceFrame.Point);
			Utils::DeserializeAxis(stream, scc.TargetReferenceFrame.SliderAxis);
			Utils::DeserializeAxis(stream, scc.TargetReferenceFrame.NormalAxis);

			stream.ReadRaw<float>(scc.SliderMin);
			stream.ReadRaw<float>(scc.SliderMax);
			stream.ReadRaw<float>(scc.MaxFrictionForce);
		}

		// Six DOF Constraint Component
		if (stream.ReadRaw<bool>())
		{
			auto& sdcc = entity.AddComponent<SixDOFConstraintComponent>();
			Utils::DeserializeConstraintComponent(stream, &sdcc);
			sdcc.Space = (ConstraintSpace)stream.ReadRaw<uint8_t>();

			stream.ReadRaw<glm::vec3>(sdcc.LocalReferenceFrame.Position);
			Utils::DeserializeAxis(stream, sdcc.LocalReferenceFrame.AxisX);
			Utils::DeserializeAxis(stream, sdcc.LocalReferenceFrame.AxisY);
			stream.ReadRaw<glm::vec3>(sdcc.TargetReferenceFrame.Position);
			Utils::DeserializeAxis(stream, sdcc.TargetReferenceFrame.AxisX);
			Utils::DeserializeAxis(stream, sdcc.TargetReferenceFrame.AxisY);

			sdcc.SwingType = (ConstraintSwingType)stream.ReadRaw<uint8_t>();

			for (uint32_t axisIndex = 0; axisIndex < SixDOFConstraintComponent::Axis::AxisCount; axisIndex++)
			{
				const SixDOFConstraintComponent::Axis axis = (SixDOFConstraintComponent::Axis)axisIndex;
				const SixDOFConstraintComponent::AxisStatus status = (SixDOFConstraintComponent::AxisStatus)stream.ReadRaw<uint8_t>();
				sdcc.SetAxisStatus(axis, status);

				stream.ReadRaw<float>(sdcc.MaxFriction[axisIndex]);

				if (status == SixDOFConstraintComponent::AxisStatus::Custom)
				{
					stream.ReadRaw<float>(sdcc.LimitMin[axisIndex]);
					stream.ReadRaw<float>(sdcc.LimitMax[axisIndex]);
				}
			}
		}

		// Follow Constraint Component
		if (stream.ReadRaw<bool>())
		{
			auto& fcc = entity.AddComponent<FollowConstraintComponent>();
			Utils::DeserializeConstraintComponent(stream, &fcc);

			stream.ReadRaw<bool>(fcc.Looping);
			Utils::DeserializeAxis(stream, fcc.Normal);
			stream.ReadRaw<float>(fcc.StartFraction);
			stream.ReadRaw<float>(fcc.MaxFrictionForce);
			fcc.RotationConstraint = (FollowConstraintComponent::RotationConstraintType)stream.ReadRaw<uint8_t>();
			stream.ReadRaw<EntityHandle>(fcc.BaseTarget);
		}

		// Spring Arm Component
		if (stream.ReadRaw<bool>())
		{
			auto& sac = entity.AddComponent<SpringArmComponent>();
			stream.ReadRaw<float>(sac.TargetLength);
			stream.ReadRaw<float>(sac.ProbeRadius);
			stream.ReadRaw<glm::vec3>(sac.TargetOffset);
			stream.ReadRaw<glm::vec3>(sac.SocketOffset);
		}

		// Field Component
		if (stream.ReadRaw<bool>())
		{
			auto& fc = entity.AddComponent<FieldComponent>();
			fc.Type = (FieldComponent::FieldType)stream.ReadRaw<uint8_t>();
			stream.ReadRaw<PhysicsLayerID>(fc.Layer);

			if (fc.Type == FieldComponent::FieldType::Directional)
			{
				// Directional
				stream.ReadRaw<glm::vec3>(fc.Force);
			}
			else if (fc.Type == FieldComponent::FieldType::Radial)
			{
				// Radial
				stream.ReadRaw<float>(fc.Magnitude);
				stream.ReadRaw<float>(fc.Radius);
				stream.ReadRaw<float>(fc.Falloff);
			}
			else
			{
				// Buoyancy
				stream.ReadRaw<float>(fc.Buoyancy);
				stream.ReadRaw<float>(fc.LinearDrag);
				stream.ReadRaw<float>(fc.AngularDrag);
				stream.ReadRaw<glm::vec3>(fc.FluidVelocity);
			}
		}

		// Landscape Component
		if (stream.ReadRaw<bool>())
		{
			auto& lc = entity.AddComponent<LandscapeComponent>();

			stream.ReadRaw<glm::uvec2>(lc.Resolution);
			lc.Material = AssetManager::GetAsset<MaterialAsset>(stream.ReadRaw<AssetHandle>());
			stream.ReadBuffer(lc.Data);

			lc.Build();
		}
	}

}