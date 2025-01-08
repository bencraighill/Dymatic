#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#define DY_USING_TRANSFORM

namespace Dymatic {

	struct Transform
	{
	public:
		glm::vec3 Translation = glm::vec3(0.0f);
		glm::quat Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 Scale = glm::vec3(1.0f);

	public:
		Transform() = default;
		Transform(const glm::vec3& translation);
		Transform(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale);

		// Rotation
		glm::vec3 GetRotationRadians() const;
		void SetRotationRadians(const glm::vec3& rotation);
		glm::vec3 GetRotationDegrees() const;
		void SetRotationDegrees(const glm::vec3& rotation);

		Transform Inverse() const;
		bool IsRotationNormalized() const;

		// Matrix Conversions
		glm::mat4 GetMatrix() const;
		glm::mat4 GetMatrixNoScale() const;

		// Operations
		bool operator==(const Transform& other) const;
		bool operator!=(const Transform& other) const;

		Transform operator*(const Transform& other) const;
		void operator*=(const Transform& other);

		static Transform ConstructTransformFromMatrix(const glm::mat4& matrix);

		static void Multiply(Transform& result, const Transform& a, const Transform& b);
		static void MultiplyUsingMatrixWithScale(Transform& result, const Transform& a, const Transform& b);
		static void ConstructTransformFromMatrixWithDesiredScale(const glm::mat4& a, const glm::mat4& b, const glm::vec3& desiredScale, Transform& result);
	};

}