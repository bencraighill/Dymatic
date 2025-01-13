#pragma once

#include "Dymatic/Renderer/AABB.h"

#include <glm/glm.hpp>
#include <array>

namespace Dymatic {

	class Frustum {
	public:
		Frustum() = default;

		Frustum(const glm::mat4& viewProjection)
			: m_ViewProjection(viewProjection)
		{
			ExtractPlanes(m_ViewProjection);
		}

		bool IsBoxVisible(const AABB& aabb) const
		{
			for (int i = 0; i < 6; i++)
			{
				glm::vec3 negative = glm::mix(aabb.Min, aabb.Max, glm::greaterThan(glm::vec3(m_Planes[i]), glm::vec3(0.0)));
				float a = glm::dot(glm::vec4(negative, 1.0), m_Planes[i]);

				if (a < 0.0)
					return false;
			}

			return true;
		}

		float GetBoxScreenCoverage(const AABB& aabb) const
		{
			glm::vec4 corners[8];
			
			// Get the corners of the AABB
			corners[0] = glm::vec4(aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f);
			corners[1] = glm::vec4(aabb.Min.x, aabb.Min.y, aabb.Max.z, 1.0f);
			corners[2] = glm::vec4(aabb.Min.x, aabb.Max.y, aabb.Min.z, 1.0f);
			corners[3] = glm::vec4(aabb.Min.x, aabb.Max.y, aabb.Max.z, 1.0f);
			corners[4] = glm::vec4(aabb.Max.x, aabb.Min.y, aabb.Min.z, 1.0f);
			corners[5] = glm::vec4(aabb.Max.x, aabb.Min.y, aabb.Max.z, 1.0f);
			corners[6] = glm::vec4(aabb.Max.x, aabb.Max.y, aabb.Min.z, 1.0f);
			corners[7] = glm::vec4(aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f);

			// Transform the corners to clip space
			for (auto& corner : corners)
			{
				corner = m_ViewProjection * corner;
				corner /= corner.w;
			}

			// Get the screen space AABB
			glm::vec2 minPt = glm::vec2(corners[0]);
			glm::vec2 maxPt = glm::vec2(corners[0]);
			
			for (const auto& corner : corners)
			{
				minPt = glm::min(minPt, glm::vec2(corner));
				maxPt = glm::max(maxPt, glm::vec2(corner));
			}

			// Compute the screen space AABB area
			glm::vec2 size = maxPt - minPt;

			return glm::clamp(size.x * size.y, 0.0f, 1.0f);
		}

	private:
		glm::mat4 m_ViewProjection;
		std::array<glm::vec4, 6> m_Planes;

		void ExtractPlanes(const glm::mat4& matrix)
		{
			for (int i = 0; i < 3; i++)
			{
				for (int j = 0; j < 2; j++)
				{
					m_Planes[i * 2 + j].x = matrix[0][3] + (j == 0 ? matrix[0][i] : -matrix[0][i]);
					m_Planes[i * 2 + j].y = matrix[1][3] + (j == 0 ? matrix[1][i] : -matrix[1][i]);
					m_Planes[i * 2 + j].z = matrix[2][3] + (j == 0 ? matrix[2][i] : -matrix[2][i]);
					m_Planes[i * 2 + j].w = matrix[3][3] + (j == 0 ? matrix[3][i] : -matrix[3][i]);
					m_Planes[i * 2 + j] *= glm::length(glm::vec3(m_Planes[i * 2 + j]));
				}
			}
		}

		float DistanceToPlane(const glm::vec4& plane, const glm::vec3& point) const
		{
			return glm::dot(glm::vec3(plane), point) + plane.w;
		}
	};

}