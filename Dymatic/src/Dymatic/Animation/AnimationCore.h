#pragma once

#include <glm/glm.hpp>

namespace Dymatic {

	struct FABRIKChainLink
	{
	public:
		glm::vec3 Position;
		float Length;
		int32_t BoneIndex;
	};

	void SolveTwoBoneIK(glm::mat4& rootTransform, glm::mat4& jointTransform, glm::mat4& endTransform, const glm::vec3& jointTarget, const glm::vec3& effector, bool allowStretching, float startStretchRatio, float maxStretchScale);
	void SolveTwoBoneIK(glm::mat4& rootTransform, glm::mat4& jointTransform, glm::mat4& endTransform, const glm::vec3& jointTarget, const glm::vec3& effector, float upperLimbLength, float lowerLimbLength, bool allowStretching, float startStretchRatio, float maxStretchScale);
	void SolveTwoBoneIK(const glm::vec3& rootPos, const glm::vec3& jointPos, const glm::vec3& endPos, const glm::vec3& jointTarget, const glm::vec3& effector, glm::vec3& outJointPos, glm::vec3& outEndPos, bool allowStretching, float startStretchRatio, float maxStretchScale);
	void SolveTwoBoneIK(const glm::vec3& rootPos, const glm::vec3& jointPos, const glm::vec3& endPos, const glm::vec3& jointTarget, const glm::vec3& effector, glm::vec3& outJointPos, glm::vec3& outEndPos, float upperLimbLength, float lowerLimbLength, bool allowStretching, float startStretchRatio, float maxStretchScale);

	bool SolveFabrik(std::vector<FABRIKChainLink>& chain, const glm::vec3& targetPosition, float maximumReach, float precision, int32_t maxIterations);

}