#pragma once
#include "Dymatic/Asset/AssetHandle.h"

#include "Dymatic/Scene/Transform.h"

#include <unordered_map>
#include <string>
#include <glm/glm.hpp>

namespace Dymatic {

	enum class AnimationGraphDataType
	{
		Bool,
		Int,
		Float,
		Vector2,
		Vector3,
		Vector4,
		Transform,
		Handle
	};

	struct AnimationGraphData
	{
		AnimationGraphDataType Type;

		union
		{
			bool Bool;
			int Int;
			float Float;
			glm::vec2 Vector2;
			glm::vec3 Vector3;
			glm::vec4 Vector4;
			Transform Transform;
			AssetHandle Handle;
		};

		AnimationGraphData();
		AnimationGraphData(bool value);
		AnimationGraphData(int value);
		AnimationGraphData(float value);
		AnimationGraphData(const glm::vec2& value);
		AnimationGraphData(const glm::vec3& value);
		AnimationGraphData(const glm::vec4& value);
		AnimationGraphData(const Dymatic::Transform& value);
		AnimationGraphData(AssetHandle value);
	};

	struct AnimationParameterMap
	{
		std::unordered_map<std::string, AnimationGraphData> Parameters;
	};

	typedef uint64_t AnimationGraphValueHandle;

	// Storage per Animation Graph instance
	struct AnimationGraphInstanceData
	{
		union InstanceValue
		{
			uint64_t Handle;
			uint32_t UInt;
			float Float;
			bool Bool;
		};

		AnimationGraphInstanceData(const AnimationParameterMap& parameters)
			: Parameters(parameters) {}

		float DeltaTime = 0.0f;
		AnimationParameterMap Parameters;
		std::unordered_map<AnimationGraphValueHandle, InstanceValue> InstanceValues;

		InstanceValue& GetValue(AnimationGraphValueHandle value);
	};

}