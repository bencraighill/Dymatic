#pragma once

#include <glm/glm.hpp>

namespace Dymatic {

	class AABB {
	public:
		AABB()
			: Min(glm::vec3(std::numeric_limits<float>::max())),
			Max(glm::vec3(std::numeric_limits<float>::lowest()))
		{
		}

		AABB(const glm::vec3& min, const glm::vec3& max)
			: Min(min), Max(max)
		{
		}
		
		bool Overlaps(const AABB& other) const
		{
			return (Min.x <= other.Max.x && Max.x >= other.Min.x) &&
				(Min.y <= other.Max.y && Max.y >= other.Min.y) &&
				(Min.z <= other.Max.z && Max.z >= other.Min.z);
		}

		bool Contains(const glm::vec3& point) const
		{
			return (point.x >= Min.x && point.x <= Max.x) &&
				(point.y >= Min.y && point.y <= Max.y) &&
				(point.z >= Min.z && point.z <= Max.z);
		}

		void Extend(const glm::vec3& point)
		{
			Min = glm::min(Min, point);
			Max = glm::max(Max, point);
		}

		void Extend(const AABB& other)
		{
			Min = glm::min(Min, other.Min);
			Max = glm::max(Max, other.Max);
		}

		glm::vec3 GetCenter() const
		{
			return 0.5f * (Min + Max);
		}

		glm::vec3 GetSize() const
		{
			return Max - Min;
		}

		glm::vec3 GetHalfSize() const
		{
			return 0.5f * (Max - Min);
		}

		float GetRadius() const
		{
			return glm::length(GetHalfSize());
		}

		float GetHalfArea() const
		{
			const glm::vec3 size = GetSize();
			return (size.x + size.y) * size.z + size.x * size.y;
		}

		AABB Transform(glm::mat4 transform) const
		{
			AABB aabb;

			glm::vec3 corners[8] = {
				glm::vec3(Min.x, Min.y, Min.z),
				glm::vec3(Min.x, Min.y, Max.z),
				glm::vec3(Min.x, Max.y, Min.z),
				glm::vec3(Min.x, Max.y, Max.z),
				glm::vec3(Max.x, Min.y, Min.z),
				glm::vec3(Max.x, Min.y, Max.z),
				glm::vec3(Max.x, Max.y, Min.z),
				glm::vec3(Max.x, Max.y, Max.z)
			};

			for (const auto& corner : corners)
			{
				const glm::vec3 transformed = transform * glm::vec4(corner, 1.0f);
				aabb.Min = glm::min(transformed, aabb.Min);
				aabb.Max = glm::max(transformed, aabb.Max);
			}

			return aabb;
		}

		AABB Translate(const glm::vec3& translation)
		{
			return AABB(Min + translation, Max + translation);
		}

		// Intersection
		bool Intersects(const glm::vec3& center, float radius) const
		{
			float closestX = std::max(Min.x, std::min(center.x, Max.x));
			float closestY = std::max(Min.y, std::min(center.y, Max.y));
			float closestZ = std::max(Min.z, std::min(center.z, Max.z));

			// Calculate the distance squared between the sphere's center and the closest point
			const glm::vec3 closestPoint = { closestX, closestY, closestZ };
			const float distance = glm::distance(center, closestPoint);

			return (distance * distance) <= (radius * radius);
		}

		bool operator==(const AABB& other) const
		{
			return Min == other.Min && Max == other.Max;
		}

		bool operator!=(const AABB& other) const
		{
			return !(*this == other);
		}

	public:
		glm::vec3 Min;
		glm::vec3 Max;
	};

}