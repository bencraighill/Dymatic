#include "dypch.h"
#include "Dymatic/Animation/AnimationGraphData.h"

namespace Dymatic {

	AnimationGraphData::AnimationGraphData()
	{
		memset(this, 0, sizeof(AnimationGraphData));
	}

	AnimationGraphData::AnimationGraphData(bool value)
		: Type(AnimationGraphDataType::Bool), Bool(value) {}

	AnimationGraphData::AnimationGraphData(int value)
		: Type(AnimationGraphDataType::Int), Int(value) {}

	AnimationGraphData::AnimationGraphData(float value)
		: Type(AnimationGraphDataType::Float), Float(value) {}

	AnimationGraphData::AnimationGraphData(const glm::vec2& value)
		: Type(AnimationGraphDataType::Vector2), Vector2(value) {}

	AnimationGraphData::AnimationGraphData(const glm::vec3& value)
		: Type(AnimationGraphDataType::Vector3), Vector3(value) {}

	AnimationGraphData::AnimationGraphData(const glm::vec4& value)
		: Type(AnimationGraphDataType::Vector4), Vector4(value) {}

	AnimationGraphData::AnimationGraphData(const Dymatic::Transform& value)
		: Type(AnimationGraphDataType::Transform), Transform(value) {}

	AnimationGraphData::AnimationGraphData(AssetHandle value)
		: Type(AnimationGraphDataType::Handle), Handle(value) {}

	AnimationGraphInstanceData::InstanceValue& AnimationGraphInstanceData::GetValue(AnimationGraphValueHandle value)
	{
		return InstanceValues[value];
	}

}