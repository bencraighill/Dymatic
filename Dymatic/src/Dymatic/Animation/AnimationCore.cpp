#include "dypch.h"
#include "Dymatic/Animation/AnimationCore.h"

// Animation Helper Functions
// Note: This file is based on the Two Bone IK implementation from Unreal Engine 5.3 (Epic Games)

#include <glm/gtx/norm.hpp>

namespace Dymatic {

	namespace Utils {

		static void FindBestAxisVectors(const glm::vec3& vector, glm::vec3& axis1, glm::vec3& axis2)
		{
			const float nx = glm::abs(vector.x);
			const float ny = glm::abs(vector.y);
			const float nz = glm::abs(vector.z);

			// Find the best axis vectors
			axis1 = (nz > nx && nz > ny) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f);

			// Project axis1 onto input vector and subtract to get the perpendicular vector
			// Then normalize axis1 and find axis2 as the cross product of axis1 and the input vector
			glm::vec3 projection = glm::dot(axis1, vector) * vector;
			axis1 = glm::normalize(axis1 - projection);
			axis2 = glm::normalize(glm::cross(axis1, vector));
		}

		static glm::vec3 GetTransformLocation(const glm::mat4& transform)
		{
			return glm::vec3(transform[3]);
		}
	}

	void SolveTwoBoneIK(glm::mat4& rootTransform, glm::mat4& jointTransform, glm::mat4& endTransform, const glm::vec3& jointTarget, const glm::vec3& effector, bool allowStretching, float startStretchRatio, float maxStretchScale)
	{
		float lowerLimbLength = glm::length(Utils::GetTransformLocation(endTransform) - Utils::GetTransformLocation(jointTransform));
		float upperLimbLength = glm::length(Utils::GetTransformLocation(jointTransform) - Utils::GetTransformLocation(rootTransform));

		SolveTwoBoneIK(rootTransform, jointTransform, endTransform, jointTarget, effector, upperLimbLength, lowerLimbLength, allowStretching, startStretchRatio, maxStretchScale);
	}

	void SolveTwoBoneIK(glm::mat4& rootTransform, glm::mat4& jointTransform, glm::mat4& endTransform, const glm::vec3& jointTarget, const glm::vec3& effector, float upperLimbLength, float lowerLimbLength, bool allowStretching, float startStretchRatio, float maxStretchScale)
	{
		glm::vec3 outJointPos, outEndPos;

		glm::vec3 rootPos = Utils::GetTransformLocation(rootTransform);
		glm::vec3 jointPos = Utils::GetTransformLocation(jointTransform);
		glm::vec3 endPos = Utils::GetTransformLocation(endTransform);

		SolveTwoBoneIK(rootPos, jointPos, endPos, jointTarget, effector, outJointPos, outEndPos, upperLimbLength, lowerLimbLength, allowStretching, startStretchRatio, maxStretchScale);
		
		// Update transform for upper bone
		{
			// Get difference in direction for old and new joint orientations
			const glm::vec3 oldDir = glm::normalize(jointPos - rootPos);
			const glm::vec3 newDir = glm::normalize(outJointPos - rootPos);
		
			// Find delta rotation that takes us from old to new direction
			const glm::quat deltaRotation = glm::rotation(oldDir, newDir);

			// Extract the existing rotation
			glm::mat4 rotationMat = glm::mat4_cast(glm::quat_cast(rootTransform));

			// Compute new rotation matrix with delta rotation and combine new rotation with existing scaling and set new translation
			// WARNING: This computation assumes no scaling is applied!
			// TODO: Fix this!
			rootTransform = glm::translate(glm::mat4(1.0f), rootPos)
				* glm::toMat4(deltaRotation * glm::quat_cast(rotationMat));
		}
		
		// Update transformation for the middle bone
		{
			glm::vec3 oldDir = glm::normalize(endPos - jointPos);
			glm::vec3 newDir = glm::normalize(outEndPos - outJointPos);
		
			glm::quat deltaRotation = glm::rotation(oldDir, newDir);

			// Extract the existing rotation
			glm::mat4 rotationMat = glm::mat4_cast(glm::quat_cast(jointTransform));
		
			// Compute new rotation matrix with delta rotation and combine new rotation with existing scaling and set new translation
			// WARNING: This computation assumes no scaling is applied!
			// TODO: Fix this!
			jointTransform = glm::translate(glm::mat4(1.0f), outJointPos)
				* glm::toMat4(deltaRotation * glm::quat_cast(rotationMat));
		}
		
		// Update the transform for the end bone.
		// Note: We keep the input rotation and correct just the location for the end bone
		endTransform[3] = glm::vec4(outEndPos, 1.0f);
	}

	void SolveTwoBoneIK(const glm::vec3& rootPos, const glm::vec3& jointPos, const glm::vec3& endPos, const glm::vec3& jointTarget, const glm::vec3& effector, glm::vec3& outJointPos, glm::vec3& outEndPos, bool allowStretching, float startStretchRatio, float maxStretchScale)
	{
		const float lowerLimbLength = glm::length(endPos - jointPos);
		const float upperLimbLength = glm::length(jointPos - rootPos);

		SolveTwoBoneIK(rootPos, jointPos, endPos, jointTarget, effector, outJointPos, outEndPos, upperLimbLength, lowerLimbLength, allowStretching, startStretchRatio, maxStretchScale);
	}

	void SolveTwoBoneIK(const glm::vec3& rootPos, const glm::vec3& jointPos, const glm::vec3& endPos, const glm::vec3& jointTarget, const glm::vec3& effector, glm::vec3& outJointPos, glm::vec3& outEndPos, float upperLimbLength, float lowerLimbLength, bool allowStretching, float startStretchRatio, float maxStretchScale)
	{
		// Note: The source for this uses double-precision. Dymatic just uses floats

		// Reach goal
		glm::vec3 desiredPos = effector;
		glm::vec3 desiredDelta = desiredPos - rootPos;
		float desiredLength = glm::length(desiredDelta);

		// Find lengths of upper/lower limb in the skeleton
		// Use actual sizes instead of reference skeleton, so we take into account translation and scaling from other bone controllers
		float maxLimbLength = lowerLimbLength + upperLimbLength;

		// Check to handle case where desiredPos is the same as rootPos.
		const float lengthThreashold = 1.0e-4;
		glm::vec3 desiredDir;
		if (desiredLength < lengthThreashold)
		{
			desiredLength = lengthThreashold;
			desiredDir = glm::vec3(1.0f, 0.0f, 0.0f);
		}
		else
		{
			desiredDir = glm::normalize(desiredDelta);
		}

		// Get joint target (used for defining plane that joint should be in)
		glm::vec3 jointTargetDelta = jointTarget - rootPos;
		const float jointTargetLengthSquared = glm::length2(jointTargetDelta);

		// Same check as above to cover case when jointTarget position is the same as rootPos
		glm::vec3 jointPlaneNormal, jointBendDir;
		if (jointTargetLengthSquared < (lengthThreashold * lengthThreashold))
		{
			jointBendDir = glm::vec3(0.0f, 1.0f, 0.0);
			jointPlaneNormal = glm::vec3(0.0f, 0.0f, 1.0f);
		}
		else
		{
			jointPlaneNormal = glm::cross(desiredDir, jointTargetDelta);

			// If we are trying to point the limb in the same direction that we are supposed to displace the joint in,
			// we have to just pick 2 random vectors perpendicular to desiredDirection and each other.
			if (glm::length2(jointPlaneNormal) < (lengthThreashold * lengthThreashold))
			{
				Utils::FindBestAxisVectors(desiredDir, jointPlaneNormal, jointBendDir);
			}
			else
			{
				jointPlaneNormal = glm::normalize(jointPlaneNormal);

				// Find the final member of the reference frame by removing any component of jointTargetDelta along desiredDir.
				// This should never leave a zero vector, because we've checked desiredDir and jointTargetDelta are not parallel.
				jointBendDir = jointTargetDelta - (glm::dot(jointTargetDelta, desiredDir) * desiredDir);
				jointBendDir = glm::normalize(jointBendDir);
			}
		}

		if (allowStretching)
		{
			const float scaleRange = maxStretchScale - startStretchRatio;
			if (scaleRange > lengthThreashold && maxLimbLength > lengthThreashold)
			{
				const float reachRatio = desiredLength / maxLimbLength;
				const float scalingFactor = (maxStretchScale - 1.0) * glm::clamp((reachRatio - startStretchRatio) / scaleRange, 0.0f, 1.0f);
				if (scalingFactor > lengthThreashold)
				{
					lowerLimbLength *= (1.0 + scalingFactor);
					upperLimbLength *= (1.0 + scalingFactor);
					maxLimbLength *= (1.0 + scalingFactor);
				}
			}
		}

		outEndPos = desiredPos;
		outJointPos = jointPos;

		// If we are trying to reach a goal beyond the length of the limb, clamp it to something solvable and extend limb fully.
		if (desiredLength >= maxLimbLength)
		{
			outEndPos = rootPos + (maxLimbLength * desiredDir);
			outJointPos = rootPos + (upperLimbLength * desiredDir);
		}
		else
		{
			// So we have a triangle we know the side lengths of. We can work out the angle between DesiredDir and the direction of the upper limb using the sine rule
			const float twoAB = 2.0 * upperLimbLength * desiredLength;
			const float cosAngle = (twoAB != 0.0) ? ((upperLimbLength * upperLimbLength) + (desiredLength * desiredLength) - (lowerLimbLength * lowerLimbLength)) / twoAB : 0.0;

			// If cosAngle is less than 0, the upper arm actually points the opposite way to DesiredDir, so we handle that.
			const bool reverseUpperBone = (cosAngle < 0.0);

			// Angle between upper limb and DesiredDir
			const float angle = std::acos(glm::clamp(cosAngle, -1.0f, 1.0f));

			// Now we calculate the distance of the joint from the root -> effector line.
			const float jointLineDist = upperLimbLength * std::sin(angle);

			// And the final side of that triangle - distance along DesiredDir of perpendicular.
			const float projJointDistSqr = (upperLimbLength * upperLimbLength) - (jointLineDist * jointLineDist);

			// Handle potential floating point inaccuracies
			float projJointDist = (projJointDistSqr > 0.0) ? std::sqrt(projJointDistSqr) : 0.0;
			if (reverseUpperBone)
				projJointDist *= -1.0f;

			// Calculate where to place the joint
			outJointPos = rootPos + (projJointDist * desiredDir) + (jointLineDist * jointBendDir);
		}
	}

	bool SolveFabrik(std::vector<FABRIKChainLink>& chain, const glm::vec3& targetPosition, float maximumReach, float precision, int32_t maxIterations)
	{
		// Implementation from Unreal Engine 5.3, based on algorithm from http://andreasaristidou.com/publications/FABRIK.pdf

		bool boneLocationUpdated = false;
		const float rootToTargetDistanceSquared = glm::distance2(chain[0].Position, targetPosition);
		const int32_t numChainLinks = chain.size();

		// FABRIK Algorithm

		// (Optimization) If the effector is further away than the distance from root to tip just move all bones in a line from root to effector location
		if (rootToTargetDistanceSquared > (maximumReach * maximumReach))
		{
			for (int32_t linkIndex = 1; linkIndex < numChainLinks; linkIndex++)
			{
				const FABRIKChainLink& parentLink = chain[linkIndex - 1];
				FABRIKChainLink& currentLink = chain[linkIndex];
				currentLink.Position = parentLink.Position + glm::normalize(targetPosition - parentLink.Position) * currentLink.Length;
			}

			boneLocationUpdated = true;
		}
		else
		{
			// Otherwise the effector is within reach and we should calculate bone translations to position the tip at the effector location
			const int32_t tipBoneLinkIndex = numChainLinks - 1;

			// Check distance between tip location and effector location
			float slop = glm::distance(chain[tipBoneLinkIndex].Position, targetPosition);
			if (slop > precision)
			{
				// Set tip bone at end effector location
				chain[tipBoneLinkIndex].Position = targetPosition;

				int32_t iterationCount = 0;
				while ((slop > precision) && (iterationCount++ < maxIterations))
				{
					// 'Forward Reaching' stage - adjust bones from end effector
					for (int32_t linkIndex = tipBoneLinkIndex - 1; linkIndex > 0; linkIndex--)
					{
						FABRIKChainLink& currentLink = chain[linkIndex];
						const FABRIKChainLink& childLink  = chain[linkIndex + 1];
						currentLink.Position = childLink.Position + glm::normalize(currentLink.Position - childLink.Position) * childLink.Length;
					}

					// 'Backward Reaching' stage - adjust bones from root
					for (int32_t linkIndex = 1; linkIndex < tipBoneLinkIndex; linkIndex++)
					{
						const FABRIKChainLink& parentLink = chain[linkIndex - 1];
						FABRIKChainLink& currentLink = chain[linkIndex];
						currentLink.Position = parentLink.Position + glm::normalize(currentLink.Position - parentLink.Position) * currentLink.Length;
					}

					// Re-check distance between tip location and effector location
					// Since we're keeping tip on top of effector location, check with its parent bone.
					slop = glm::abs(chain[tipBoneLinkIndex].Length - glm::distance(chain[tipBoneLinkIndex - 1].Position, targetPosition));
				}

				// Place the tip bone based on how close we got to the target
				{
					const FABRIKChainLink& parentLink = chain[tipBoneLinkIndex - 1];
					FABRIKChainLink& currentLink = chain[tipBoneLinkIndex];
					currentLink.Position = parentLink.Position + glm::normalize(currentLink.Position - parentLink.Position) * currentLink.Length;
				}

				boneLocationUpdated = true;
			}
		}

		return boneLocationUpdated;
	}

}