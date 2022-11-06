#include "dypch.h"
#include "SceneSerializer.h"

#include "Entity.h"
#include "Components.h"

#include <fstream>

#include <yaml-cpp/yaml.h>

#include "Dymatic/Scene/ScriptEngine.h"

namespace YAML {

	template<>
	struct convert<glm::vec2>
	{
		static Node encode(const glm::vec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};

}
namespace Dymatic {

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

	static std::string RigidBody2DBodyTypeToString(Rigidbody2DComponent::BodyType bodyType)
	{
		switch (bodyType)
		{
			case Rigidbody2DComponent::BodyType::Static:    return "Static";
			case Rigidbody2DComponent::BodyType::Dynamic:   return "Dynamic";
			case Rigidbody2DComponent::BodyType::Kinematic: return "Kinematic";
		}

		DY_CORE_ASSERT(false, "Unknown body type");
		return {};
	}

	static Rigidbody2DComponent::BodyType RigidBody2DBodyTypeFromString(const std::string& bodyTypeString)
	{
		if (bodyTypeString == "Static")    return Rigidbody2DComponent::BodyType::Static;
		if (bodyTypeString == "Dynamic")   return Rigidbody2DComponent::BodyType::Dynamic;
		if (bodyTypeString == "Kinematic") return Rigidbody2DComponent::BodyType::Kinematic;

		DY_CORE_ASSERT(false, "Unknown body type");
		return Rigidbody2DComponent::BodyType::Static;
	}

	static std::string RigidBodyBodyTypeToString(RigidbodyComponent::BodyType bodyType)
	{
		switch (bodyType)
		{
		case RigidbodyComponent::BodyType::Static:    return "Static";
		case RigidbodyComponent::BodyType::Dynamic:   return "Dynamic";
		}

		DY_CORE_ASSERT(false, "Unknown body type");
		return {};
	}

	static RigidbodyComponent::BodyType RigidBodyBodyTypeFromString(const std::string& bodyTypeString)
	{
		if (bodyTypeString == "Static")    return RigidbodyComponent::BodyType::Static;
		if (bodyTypeString == "Dynamic")   return RigidbodyComponent::BodyType::Dynamic;

		DY_CORE_ASSERT(false, "Unknown body type");
		return RigidbodyComponent::BodyType::Static;
	}

	static std::string MeshColliderMeshTypeToString(MeshColliderComponent::MeshType meshType)
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
		if (meshTypeString == "Triangle")    return MeshColliderComponent::MeshType::Triangle;
		if (meshTypeString == "Convex")    return MeshColliderComponent::MeshType::Convex;

		DY_CORE_ASSERT(false, "Unknown mesh type");
		return MeshColliderComponent::MeshType::Triangle;
	}

	SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
		: m_Scene(scene)
	{
	}

	static void SerializeEntity(YAML::Emitter& out, Entity entity)
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

		if (entity.HasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap; // TransformComponent

			auto& tc = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "Translation" << YAML::Value << tc.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << tc.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << tc.Scale;

			out << YAML::EndMap; // TransformComponent
		}

		if (entity.HasComponent<CameraComponent>())
		{
			out << YAML::Key << "CameraComponent";
			out << YAML::BeginMap; // CameraComponent

			auto& cameraComponent = entity.GetComponent<CameraComponent>();
			auto& camera = cameraComponent.Camera;

			out << YAML::Key << "Camera" << YAML::Value;
			out << YAML::BeginMap; // Camera
			out << YAML::Key << "ProjectionType" << YAML::Value << (int)camera.GetProjectionType();
			out << YAML::Key << "PerspectiveFOV" << YAML::Value << camera.GetPerspectiveVerticalFOV();
			out << YAML::Key << "PerspectiveNear" << YAML::Value << camera.GetPerspectiveNearClip();
			out << YAML::Key << "PerspectiveFar" << YAML::Value << camera.GetPerspectiveFarClip();
			out << YAML::Key << "OrthographicSize" << YAML::Value << camera.GetOrthographicSize();
			out << YAML::Key << "OrthographicNear" << YAML::Value << camera.GetOrthographicNearClip();
			out << YAML::Key << "OrthographicFar" << YAML::Value << camera.GetOrthographicFarClip();
			out << YAML::EndMap; // Camera

			out << YAML::Key << "Primary" << YAML::Value << cameraComponent.Primary;
			out << YAML::Key << "FixedAspectRatio" << YAML::Value << cameraComponent.FixedAspectRatio;

			out << YAML::EndMap; // CameraComponent
		}

		if (entity.HasComponent<SpriteRendererComponent>())
		{
			out << YAML::Key << "SpriteRendererComponent";
			out << YAML::BeginMap; // SpriteRendererComponent

			auto& spriteRendererComponent = entity.GetComponent<SpriteRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << spriteRendererComponent.Color;
			if (spriteRendererComponent.Texture)
				out << YAML::Key << "TexturePath" << YAML::Value << spriteRendererComponent.Texture->GetPath();

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

		if (entity.HasComponent<ParticleSystemComponent>())
		{
			out << YAML::Key << "ParticleSystemComponent";
			out << YAML::BeginMap; // ParticleSystemComponent

			auto& particleSystemComponent = entity.GetComponent<ParticleSystemComponent>();
			out << YAML::Key << "Offset" << YAML::Value << particleSystemComponent.Offset;
			out << YAML::Key << "Velocity" << YAML::Value << particleSystemComponent.Velocity;
			out << YAML::Key << "VelocityVariation" << YAML::Value << particleSystemComponent.VelocityVariation;
			out << YAML::Key << "Gravity" << YAML::Value << particleSystemComponent.Gravity;
			out << YAML::Key << "ColorMethod" << YAML::Value << particleSystemComponent.ColorMethod;
			out << YAML::Key << "ColorBegin" << YAML::Value << particleSystemComponent.ColorBegin;
			out << YAML::Key << "ColorEnd" << YAML::Value << particleSystemComponent.ColorEnd;
			out << YAML::Key << "ColorConstant" << YAML::Value << particleSystemComponent.ColorConstant;
			out << YAML::Key << "ColorPoints" << YAML::Value << YAML::BeginSeq; particleSystemComponent.ColorEnd;
			for (auto colorPoint : particleSystemComponent.ColorPoints)
			{
				out << YAML::BeginMap; // Color Point
				out << YAML::Key << "Point" << YAML::Value << colorPoint.point;
				out << YAML::Key << "Color" << YAML::Value << colorPoint.color;
				out << YAML::EndMap; // Color Point
			}
			out << YAML::EndSeq;

			out << YAML::Key << "SizeBegin" << YAML::Value << particleSystemComponent.SizeBegin;
			out << YAML::Key << "SizeEnd" << YAML::Value << particleSystemComponent.SizeEnd;
			out << YAML::Key << "SizeVariation" << YAML::Value << particleSystemComponent.SizeVariation;
			out << YAML::Key << "LifeTime" << YAML::Value << particleSystemComponent.LifeTime;
			out << YAML::Key << "EmissionNumber" << YAML::Value << particleSystemComponent.EmissionNumber;
			out << YAML::Key << "Active" << YAML::Value << particleSystemComponent.Active;
			out << YAML::Key << "FaceCamera" << YAML::Value << particleSystemComponent.FaceCamera;

			out << YAML::EndMap; // ParticleSystemComponent
		}

		if (entity.HasComponent<Rigidbody2DComponent>())
		{
			out << YAML::Key << "Rigidbody2DComponent";
			out << YAML::BeginMap; // Rigidbody2DComponent

			auto& rb2dComponent = entity.GetComponent<Rigidbody2DComponent>();
			out << YAML::Key << "BodyType" << YAML::Value << RigidBody2DBodyTypeToString(rb2dComponent.Type);
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

		if (entity.HasComponent<NativeScriptComponent>())
		{
			out << YAML::Key << "NativeScriptComponent";
			out << YAML::BeginMap; // NativeScriptComponent
		
			auto& nsc = entity.GetComponent<NativeScriptComponent>();
			out << YAML::Key << "Name" << YAML::Value << nsc.ScriptName;
			out << YAML::Key << "Parameters";
			out << YAML::BeginSeq;
			
			std::vector<ScriptEngine::ParamData> data;
			ScriptEngine::GetScriptParamData(nsc.ScriptName, data);
			for (size_t i = 0; i < nsc.Params.size() && i < data.size(); i++)
			{
				out << YAML::Value;
				switch (data[i].type)
				{
				case ScriptEngine::ParamData::Boolean:
					out << nsc.Params[i].Boolean;
					break;
				case ScriptEngine::ParamData::Float:
					out << nsc.Params[i].Float;
					break;
				case ScriptEngine::ParamData::Integer:
					out << nsc.Params[i].Integer;
					break;
				}
			}
			out << YAML::EndSeq;
		
			out << YAML::EndMap; // NativeScriptComponent
		}

		if (entity.HasComponent<StaticMeshComponent>())
		{
			out << YAML::Key << "StaticMeshComponent";
			out << YAML::BeginMap; // StaticMeshComponent

			auto& staticMeshComponent = entity.GetComponent<StaticMeshComponent>();
			if (staticMeshComponent.m_Model)
				out << YAML::Key << "MeshPath" << YAML::Value << staticMeshComponent.m_Model->GetPath();

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
			out << YAML::Key << "Type" << YAML::Value << skyLightComponent.Type;
			if (skyLightComponent.Type == 0)
			{
				if (!skyLightComponent.Filepath.empty())
					out << YAML::Key << "SkyLightPath" << YAML::Value << skyLightComponent.Filepath;
			}
			out << YAML::Key << "Intensity" << YAML::Value << skyLightComponent.Intensity;

			out << YAML::EndMap; // SkyLightComponent
		}

		if (entity.HasComponent<AudioComponent>())
		{
			out << YAML::Key << "AudioComponent";
			out << YAML::BeginMap; // AudioComponent

			auto& audioComponent = entity.GetComponent<AudioComponent>();
			if (audioComponent.m_Sound)
				out << YAML::Key << "SoundPath" << YAML::Value << audioComponent.m_Sound->GetPath();

			out << YAML::EndMap; // AudioComponent
		}

		if (entity.HasComponent<RigidbodyComponent>())
		{
			out << YAML::Key << "RigidbodyComponent";
			out << YAML::BeginMap; // RigidbodyComponent

			auto& rigidbodyComponent = entity.GetComponent<RigidbodyComponent>();
			out << YAML::Key << "BodyType" << YAML::Value << RigidBodyBodyTypeToString(rigidbodyComponent.Type);
			out << YAML::Key << "Density" << YAML::Value << rigidbodyComponent.Density;

			out << YAML::EndMap; // RigidbodyComponent
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
			out << YAML::Key << "MeshType" << YAML::Value << MeshColliderMeshTypeToString(meshColliderComponent.Type);

			out << YAML::EndMap; // MeshColliderComponent
		}

		out << YAML::EndMap; // Entity
	}

	void SceneSerializer::Serialize(const std::string& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << "Untitled";
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		m_Scene->m_Registry.each([&](auto entityID)
		{
			Entity entity = { entityID, m_Scene.get() };
			if (!entity || entity.HasComponent<SceneComponent>())
				return;

			SerializeEntity(out, entity);
		});
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	void SceneSerializer::SerializeRuntime(const std::string& filepath)
	{
		// Not implemented
		DY_CORE_ASSERT(false);
	}

	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		YAML::Node data;
		try
		{
			data = YAML::LoadFile(filepath);
		}
		catch (YAML::ParserException e)
		{
			DY_CORE_ERROR("Failed to load .dymatic file '{0}'\n     {1}", filepath, e.what());
			return false;
		}

		if (!data["Scene"])
			return false;

		std::string sceneName = data["Scene"].as<std::string>();
		DY_CORE_TRACE("Deserializing scene '{0}'", sceneName);

		auto entities = data["Entities"];
		if (entities)
		{
			for (auto entity : entities)
			{
				uint64_t uuid = entity["Entity"].as<uint64_t>();

				std::string name;
				auto tagComponent = entity["TagComponent"];
				if (tagComponent)
					name = tagComponent["Tag"].as<std::string>();

				DY_CORE_TRACE("Deserialized entity with ID = {0}, name = {1}", uuid, name);

				Entity deserializedEntity = m_Scene->CreateEntityWithUUID(uuid, name);

				auto transformComponent = entity["TransformComponent"];
				if (transformComponent)
				{
					// Entities always have transforms
					auto& tc = deserializedEntity.GetComponent<TransformComponent>();
					tc.Translation = transformComponent["Translation"].as<glm::vec3>();
					tc.Rotation = transformComponent["Rotation"].as<glm::vec3>();
					tc.Scale = transformComponent["Scale"].as<glm::vec3>();
				}

				auto cameraComponent = entity["CameraComponent"];
				if (cameraComponent)
				{
					auto& cc = deserializedEntity.AddComponent<CameraComponent>();

					auto& cameraProps = cameraComponent["Camera"];
					cc.Camera.SetProjectionType((SceneCamera::ProjectionType)cameraProps["ProjectionType"].as<int>());

					cc.Camera.SetPerspectiveVerticalFOV(cameraProps["PerspectiveFOV"].as<float>());
					cc.Camera.SetPerspectiveNearClip(cameraProps["PerspectiveNear"].as<float>());
					cc.Camera.SetPerspectiveFarClip(cameraProps["PerspectiveFar"].as<float>());

					cc.Camera.SetOrthographicSize(cameraProps["OrthographicSize"].as<float>());
					cc.Camera.SetOrthographicNearClip(cameraProps["OrthographicNear"].as<float>());
					cc.Camera.SetOrthographicFarClip(cameraProps["OrthographicFar"].as<float>());

					cc.Primary = cameraComponent["Primary"].as<bool>();
					cc.FixedAspectRatio = cameraComponent["FixedAspectRatio"].as<bool>();
				}

				auto spriteRendererComponent = entity["SpriteRendererComponent"];
				if (spriteRendererComponent)
				{
					auto& src = deserializedEntity.AddComponent<SpriteRendererComponent>();
					src.Color = spriteRendererComponent["Color"].as<glm::vec4>();

					if (spriteRendererComponent["TexturePath"])
						src.Texture = Texture2D::Create(spriteRendererComponent["TexturePath"].as<std::string>());

					if (spriteRendererComponent["TilingFactor"])
						src.TilingFactor = spriteRendererComponent["TilingFactor"].as<float>();
				}

				auto circleRendererComponent = entity["CircleRendererComponent"];
				if (circleRendererComponent)
				{
					auto& crc = deserializedEntity.AddComponent<CircleRendererComponent>();
					crc.Color = circleRendererComponent["Color"].as<glm::vec4>();
					crc.Thickness = circleRendererComponent["Thickness"].as<float>();
					crc.Fade = circleRendererComponent["Fade"].as<float>();
				}

				auto particleSystemComponent = entity["ParticleSystemComponent"];
				if (particleSystemComponent)
				{
					auto& ps = deserializedEntity.AddComponent<ParticleSystemComponent>();

					ps.Offset = particleSystemComponent["Offset"].as<glm::vec3>();
					ps.Velocity = particleSystemComponent["Velocity"].as<glm::vec3>();
					ps.VelocityVariation = particleSystemComponent["VelocityVariation"].as<glm::vec3>();
					ps.Gravity = particleSystemComponent["Gravity"].as<glm::vec3>();
					ps.ColorMethod = particleSystemComponent["ColorMethod"].as<int>();
					ps.ColorBegin = particleSystemComponent["ColorBegin"].as<glm::vec4>();
					ps.ColorEnd = particleSystemComponent["ColorEnd"].as<glm::vec4>();
					ps.ColorConstant = particleSystemComponent["ColorConstant"].as<glm::vec4>();

					auto colorPoints = particleSystemComponent["ColorPoints"];
					for (auto colorPoint : colorPoints)
						ps.ColorPoints.push_back({ ps.GetNextColorPointId(), colorPoint["Point"].as<float>(), colorPoint["Color"].as<glm::vec4>() });

					ps.SizeBegin = particleSystemComponent["SizeBegin"].as<float>();
					ps.SizeEnd = particleSystemComponent["SizeEnd"].as<float>();
					ps.SizeVariation = particleSystemComponent["SizeVariation"].as<float>();
					ps.LifeTime = particleSystemComponent["LifeTime"].as<float>();
					ps.EmissionNumber = particleSystemComponent["EmissionNumber"].as<int>();
					ps.Active = particleSystemComponent["Active"].as<bool>();
					ps.FaceCamera = particleSystemComponent["FaceCamera"].as<bool>();
				}

				auto rigidbody2DComponent = entity["Rigidbody2DComponent"];
				if (rigidbody2DComponent)
				{
					auto& rb2d = deserializedEntity.AddComponent<Rigidbody2DComponent>();
					rb2d.Type = RigidBody2DBodyTypeFromString(rigidbody2DComponent["BodyType"].as<std::string>());
					rb2d.FixedRotation = rigidbody2DComponent["FixedRotation"].as<bool>();
				}

				auto boxCollider2DComponent = entity["BoxCollider2DComponent"];
				if (boxCollider2DComponent)
				{
					auto& bc2d = deserializedEntity.AddComponent<BoxCollider2DComponent>();
					bc2d.Offset = boxCollider2DComponent["Offset"].as<glm::vec2>();
					bc2d.Size = boxCollider2DComponent["Size"].as<glm::vec2>();
					bc2d.Density = boxCollider2DComponent["Density"].as<float>();
					bc2d.Friction = boxCollider2DComponent["Friction"].as<float>();
					bc2d.Restitution = boxCollider2DComponent["Restitution"].as<float>();
					bc2d.RestitutionThreshold = boxCollider2DComponent["RestitutionThreshold"].as<float>();
				}

				auto circleCollider2DComponent = entity["CircleCollider2DComponent"];
				if (circleCollider2DComponent)
				{
					auto& cc2d = deserializedEntity.AddComponent<CircleCollider2DComponent>();
					cc2d.Offset = circleCollider2DComponent["Offset"].as<glm::vec2>();
					cc2d.Radius = circleCollider2DComponent["Radius"].as<float>();
					cc2d.Density = circleCollider2DComponent["Density"].as<float>();
					cc2d.Friction = circleCollider2DComponent["Friction"].as<float>();
					cc2d.Restitution = circleCollider2DComponent["Restitution"].as<float>();
					cc2d.RestitutionThreshold = circleCollider2DComponent["RestitutionThreshold"].as<float>();
				}

				auto nativeScriptComponent = entity["NativeScriptComponent"];
				if (nativeScriptComponent)
				{
					auto& nsc = deserializedEntity.AddComponent<NativeScriptComponent>();
					nsc.ScriptName = nativeScriptComponent["Name"].as<std::string>();


					std::vector<ScriptEngine::ParamData> data;
					ScriptEngine::GetScriptParamData(nsc.ScriptName, data);
					ScriptEngine::SetupScript(nsc);
					auto& params = nativeScriptComponent["Parameters"];
					for (size_t i = 0; i < nsc.Params.size() && i < data.size(); i++)
					{
						switch (data[i].type)
						{
						case ScriptEngine::ParamData::Boolean:
							nsc.Params[i].Boolean = params[i].as<bool>();
							break;
						case ScriptEngine::ParamData::Float:
							nsc.Params[i].Float = params[i].as<float>();
							break;
						case ScriptEngine::ParamData::Integer:
							nsc.Params[i].Integer = params[i].as<int>();
							break;
						}
					}
				}

				auto staticMeshComponent = entity["StaticMeshComponent"];
				if (staticMeshComponent)
				{
					auto& smc = deserializedEntity.AddComponent<StaticMeshComponent>();
					auto& meshPath = staticMeshComponent["MeshPath"];
					if (meshPath)
						smc.LoadModel(meshPath.as<std::string>());
				}

				auto directionalLightComponent = entity["DirectionalLightComponent"];
				if (directionalLightComponent)
				{
					auto& dlc = deserializedEntity.AddComponent<DirectionalLightComponent>();
					dlc.Color = directionalLightComponent["Color"].as<glm::vec3>();
					dlc.Intensity = directionalLightComponent["Intensity"].as<float>();
				}

				auto pointLightComponent = entity["PointLightComponent"];
				if (pointLightComponent)
				{
					auto& plc = deserializedEntity.AddComponent<PointLightComponent>();

					if (auto& color = pointLightComponent["Color"])
						plc.Color = color.as<glm::vec3>();

					if (auto& intensity = pointLightComponent["Intensity"])
						plc.Intensity = intensity.as<float>();

					if (auto& radius = pointLightComponent["Radius"])
						plc.Radius = radius.as<float>();

					if (auto& castsShadows = pointLightComponent["Casts Shadows"])
						plc.CastsShadows = castsShadows.as<bool>();
				}

				auto skyLightComponent = entity["SkyLightComponent"];
				if (skyLightComponent)
				{
					auto& slc = deserializedEntity.AddComponent<SkyLightComponent>();

					if (auto& skyLightType = skyLightComponent["Type"])
						slc.Type = skyLightType.as<int>();

					auto& skyLightPath = skyLightComponent["SkyLightPath"];
					if (skyLightPath)
						slc.Load(skyLightPath.as<std::string>());

					slc.Intensity = skyLightComponent["Intensity"].as<float>();
				}

				auto audioComponent = entity["AudioComponent"];
				if (audioComponent)
				{
					auto& ac = deserializedEntity.AddComponent<AudioComponent>();
					if (audioComponent["SoundPath"])
						ac.LoadSound(audioComponent["SoundPath"].as<std::string>());
				}

				auto rigidbodyComponent = entity["RigidbodyComponent"];
				if (rigidbodyComponent)
				{
					auto& rbc = deserializedEntity.AddComponent<RigidbodyComponent>();
					rbc.Type = RigidBodyBodyTypeFromString(rigidbodyComponent["BodyType"].as<std::string>());
					rbc.Density = rigidbodyComponent["Density"].as<float>();
				}

				auto boxColliderComponent = entity["BoxColliderComponent"];
				if (boxColliderComponent)
				{
					auto& bcc = deserializedEntity.AddComponent<BoxColliderComponent>();
					bcc.Size = boxColliderComponent["Size"].as<glm::vec3>();
				}

				auto sphereColliderComponent = entity["SphereColliderComponent"];
				if (sphereColliderComponent)
				{
					auto& scc = deserializedEntity.AddComponent<SphereColliderComponent>();
					scc.Radius = sphereColliderComponent["Radius"].as<float>();
				}

				auto capsuleColliderComponent = entity["CapsuleColliderComponent"];
				if (capsuleColliderComponent)
				{
					auto& ccc = deserializedEntity.AddComponent<CapsuleColliderComponent>();
					ccc.Radius = capsuleColliderComponent["Radius"].as<float>();
					ccc.HalfHeight = capsuleColliderComponent["HalfHeight"].as<float>();
				}

				auto meshColliderComponent = entity["MeshColliderComponent"];
				if (meshColliderComponent)
				{
					auto& mcc = deserializedEntity.AddComponent<MeshColliderComponent>();
					mcc.Type = MeshColliderMeshTypeFromString(meshColliderComponent["MeshType"].as<std::string>());
				}

			}
		}

		return true;
	}

	bool SceneSerializer::DeserializeRuntime(const std::string& filepath)
	{
		// Not implemented
		DY_CORE_ASSERT(false);
		return false;
	}

}
