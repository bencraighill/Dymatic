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
		static Ref<Bone> Create(const std::string& name, int ID, const std::vector<KeyPosition>& positions, const std::vector<KeyRotation>& rotations, const std::vector<KeyScale>& scales) { return CreateRef<Bone>(name, ID, positions, rotations, scales); }

	public:
		Bone(const std::string& name, int ID, const aiNodeAnim* channel);
		Bone(const std::string& name, int ID, const std::vector<KeyPosition>& positions, const std::vector<KeyRotation>& rotations, const std::vector<KeyScale>& scales);

		glm::mat4 GetLocalTransform(const float animationTime);

		inline const std::string& GetBoneName() const { return m_Name; }
		inline int GetBoneID() { return m_ID; }


		uint32_t GetPositionIndex(float animationTime);
		uint32_t GetRotationIndex(float animationTime);
		uint32_t GetScaleIndex(float animationTime);

		// Runtime Serialization only
		inline const std::vector<KeyPosition>& GetPositions() const { return m_Positions; }
		inline const std::vector<KeyRotation>& GetRotations() const { return m_Rotations; }
		inline const std::vector<KeyScale>& GetScales() const { return m_Scales; }

	private:
		float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);
		glm::mat4 InterpolatePosition(float animationTime);
		glm::mat4 InterpolateRotation(float animationTime);
		glm::mat4 InterpolateScaling(float animationTime);

	private:
		std::vector<KeyPosition> m_Positions;
		std::vector<KeyRotation> m_Rotations;
		std::vector<KeyScale> m_Scales;
		
		std::string m_Name;
		int m_ID;
	};
}
