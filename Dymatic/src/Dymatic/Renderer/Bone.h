#pragma once

#include <vector>
#include <assimp/scene.h>
#include <list>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Dymatic {

	struct KeyPosition
	{
		glm::vec3 position;
		float timeStamp;
	};

	struct KeyRotation
	{
		glm::quat orientation;
		float timeStamp;
	};

	struct KeyScale
	{
		glm::vec3 scale;
		float timeStamp;
	};

	class Bone
	{
	public:
		static Ref<Bone> Create(const std::string& name, int ID, const aiNodeAnim* channel) { return CreateRef<Bone>(name, ID, channel); }

		Bone(const std::string& name, int ID, const aiNodeAnim* channel);

		void Update(float animationTime);

		const glm::mat4& GetLocalTransform() { return m_LocalTransform; }
		const std::string& GetBoneName() const { return m_Name; }
		int GetBoneID() { return m_ID; }

		uint32_t GetPositionIndex(float animationTime);
		uint32_t GetRotationIndex(float animationTime);
		uint32_t GetScaleIndex(float animationTime);

	private:
		float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);
		glm::mat4 InterpolatePosition(float animationTime);
		glm::mat4 InterpolateRotation(float animationTime);
		glm::mat4 InterpolateScaling(float animationTime);

	private:
		std::vector<KeyPosition> m_Positions;
		std::vector<KeyRotation> m_Rotations;
		std::vector<KeyScale> m_Scales;
		uint32_t m_NumPositions;
		uint32_t m_NumRotations;
		uint32_t m_NumScalings;

		glm::mat4 m_LocalTransform;
		std::string m_Name;
		uint32_t m_ID;
	};
}
