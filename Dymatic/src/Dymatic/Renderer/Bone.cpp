#include "dypch.h"
#include "Dymatic/Renderer/Bone.h"

#include "Dymatic/Renderer/Utils/AssimpGLMHelpers.h"

namespace Dymatic {

	Bone::Bone(const std::string& name, int ID, const aiNodeAnim* channel)
		: m_Name(name), m_ID(ID)
	{
		for (uint32_t positionIndex = 0; positionIndex < channel->mNumPositionKeys; ++positionIndex)
		{
			aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
			float timeStamp = channel->mPositionKeys[positionIndex].mTime;
			KeyPosition data;
			data.position = AssimpGLMHelpers::GetGLMVec(aiPosition);
			data.timeStamp = timeStamp;
			m_Positions.push_back(data);
		}
		
		for (uint32_t rotationIndex = 0; rotationIndex < channel->mNumRotationKeys; ++rotationIndex)
		{
			aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
			float timeStamp = channel->mRotationKeys[rotationIndex].mTime;
			KeyRotation data;
			data.orientation = AssimpGLMHelpers::GetGLMQuat(aiOrientation);
			data.timeStamp = timeStamp;
			m_Rotations.push_back(data);
		}
		
		for (uint32_t keyIndex = 0; keyIndex < channel->mNumScalingKeys; ++keyIndex)
		{
			aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
			float timeStamp = channel->mScalingKeys[keyIndex].mTime;
			KeyScale data;
			data.scale = AssimpGLMHelpers::GetGLMVec(scale);
			data.timeStamp = timeStamp;
			m_Scales.push_back(data);
		}
	}

	Bone::Bone(const std::string& name, int ID, const std::vector<KeyPosition>& positions, const std::vector<KeyRotation>& rotations, const std::vector<KeyScale>& scales)
		: m_Name(name), m_ID(ID), m_Positions(positions), m_Rotations(rotations), m_Scales(scales)
	{}

	glm::mat4 Bone::GetLocalTransform(const float animationTime)
	{
		glm::mat4 translation = InterpolatePosition(animationTime);
		glm::mat4 rotation = InterpolateRotation(animationTime);
		glm::mat4 scale = InterpolateScaling(animationTime);
		return translation * rotation * scale;
	}

	uint32_t Bone::GetPositionIndex(float animationTime)
	{
		for (uint32_t index = 0; index < m_Positions.size() - 1; ++index)
		{
			if (animationTime < m_Positions[index + 1].timeStamp)
				return index;
		}
		
		DY_CORE_ASSERT(false);
	}

	uint32_t Bone::GetRotationIndex(float animationTime)
	{
		for (uint32_t index = 0; index < m_Rotations.size() - 1; ++index)
		{
			if (animationTime < m_Rotations[index + 1].timeStamp)
				return index;
		}
		
		DY_CORE_ASSERT(false);
	}

	uint32_t Bone::GetScaleIndex(float animationTime)
	{
		for (uint32_t index = 0; index < m_Scales.size() - 1; ++index)
		{
			if (animationTime < m_Scales[index + 1].timeStamp)
				return index;
		}
		
		DY_CORE_ASSERT(false);
	}

	float Bone::GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
	{
		float scaleFactor = 0.0f;
		float midWayLength = animationTime - lastTimeStamp;
		float framesDiff = nextTimeStamp - lastTimeStamp;
		scaleFactor = midWayLength / framesDiff;
		return scaleFactor;
	}

	glm::mat4 Bone::InterpolatePosition(float animationTime)
	{
		if (m_Positions.size() == 1)
			return glm::translate(glm::mat4(1.0f), m_Positions[0].position);

		uint32_t p0Index = GetPositionIndex(animationTime);
		uint32_t p1Index = p0Index + 1;
		float scaleFactor = GetScaleFactor(m_Positions[p0Index].timeStamp, m_Positions[p1Index].timeStamp, animationTime);
		glm::vec3 finalPosition = glm::mix(m_Positions[p0Index].position, m_Positions[p1Index].position, scaleFactor);
		return glm::translate(glm::mat4(1.0f), finalPosition);
	}

	glm::mat4 Bone::InterpolateRotation(float animationTime)
	{
		if (m_Rotations.size() == 1)
		{
			auto rotation = glm::normalize(m_Rotations[0].orientation);
			return glm::toMat4(rotation);
		}

		uint32_t p0Index = GetRotationIndex(animationTime);
		uint32_t p1Index = p0Index + 1;
		float scaleFactor = GetScaleFactor(m_Rotations[p0Index].timeStamp, m_Rotations[p1Index].timeStamp, animationTime);
		glm::quat finalRotation = glm::slerp(m_Rotations[p0Index].orientation, m_Rotations[p1Index].orientation, scaleFactor);
		finalRotation = glm::normalize(finalRotation);
		return glm::toMat4(finalRotation);
	}

	glm::mat4 Bone::InterpolateScaling(float animationTime)
	{
		if (m_Scales.size() == 1)
			return glm::scale(glm::mat4(1.0f), m_Scales[0].scale);

		uint32_t p0Index = GetScaleIndex(animationTime);
		uint32_t p1Index = p0Index + 1;
		float scaleFactor = GetScaleFactor(m_Scales[p0Index].timeStamp, m_Scales[p1Index].timeStamp, animationTime);
		glm::vec3 finalScale = glm::mix(m_Scales[p0Index].scale, m_Scales[p1Index].scale, scaleFactor);
		return glm::scale(glm::mat4(1.0f), finalScale);
	}

}