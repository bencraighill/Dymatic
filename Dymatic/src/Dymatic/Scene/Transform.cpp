#include "dypch.h"
#include "Transform.h"

#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace Dymatic {

	namespace Utils {
	
		static void RemoveMatrixScaling(glm::mat4& matrix)
		{
			glm::vec3 translation, currentScale, skew;
			glm::quat rotation;
			glm::vec4 perspective;

			glm::decompose(matrix, currentScale, rotation, translation, skew, perspective);

			matrix = glm::scale(matrix, 1.0f / currentScale);
		}
	
	}

	Transform::Transform(const glm::vec3& translation)
		: Translation(translation) {}

	Transform::Transform(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale)
		: Translation(translation), Rotation(rotation), Scale(scale) {}

	glm::vec3 Transform::GetRotationRadians() const
	{
		return glm::eulerAngles(Rotation);
	}

	void Transform::SetRotationRadians(const glm::vec3& rotation)
	{
		Rotation = glm::quat(rotation);
	}

	glm::vec3 Transform::GetRotationDegrees() const
	{
		return glm::degrees(GetRotationRadians());
	}

	void Transform::SetRotationDegrees(const glm::vec3& rotation)
	{
		SetRotationRadians(glm::radians(rotation));
	}

	Transform Transform::Inverse() const
	{
		constexpr float threashold = 1.e-8f;

		if (!glm::any(glm::greaterThan(glm::abs(Scale), glm::vec3(threashold))))
			return Transform();

		DY_CORE_ASSERT(IsRotationNormalized());
		DY_CORE_ASSERT(glm::any(glm::greaterThan(glm::abs(Scale), glm::vec3(threashold))));

		const glm::vec3 inverseScale = glm::vec3(1.0f) / Scale;
		const glm::quat inverseRotation = glm::conjugate(Rotation);

		const glm::vec3 scaledTranslation = inverseScale * Translation;
		const glm::vec3 inverseTranslation = -(inverseRotation * scaledTranslation);

		return Transform(inverseTranslation, inverseRotation, inverseScale);
	}

	bool Transform::IsRotationNormalized() const
	{
		constexpr float threshQuatNormalized = 0.01f;
		float testValue = glm::abs(1.0f - glm::dot(Rotation, Rotation));
		return testValue <= threshQuatNormalized;
	}

	glm::mat4 Transform::GetMatrix() const
	{
		glm::mat4 rotation = glm::toMat4(Rotation);

		return glm::translate(glm::mat4(1.0f), Translation)
			* rotation
			* glm::scale(glm::mat4(1.0f), Scale);
	}

	glm::mat4 Transform::GetMatrixNoScale() const
	{
		return glm::translate(glm::mat4(1.0f), Translation) * glm::toMat4(Rotation);
	}

	bool Transform::operator==(const Transform& other) const
	{
		return Translation == other.Translation && Rotation == other.Rotation && Scale == other.Scale;
	}

	bool Transform::operator!=(const Transform& other) const
	{
		return !(*this == other);
	}

	Transform Transform::operator*(const Transform& other) const
	{
		Transform result;
		Multiply(result, *this, other);
		return result;
	}

	void Transform::operator*=(const Transform& other)
	{
		Multiply(*this, *this, other);
	}

	Transform Transform::ConstructTransformFromMatrix(const glm::mat4& matrix)
	{
		Transform result;

		glm::vec3 skew;
		glm::vec4 perspective;

		glm::decompose(matrix, result.Scale, result.Rotation, result.Translation, skew, perspective);
		return result;
	}

	void Transform::Multiply(Transform& result, const Transform& a, const Transform& b)
	{
		if (glm::any(glm::lessThan(a.Scale, glm::vec3(0.0f))) || glm::any(glm::lessThan(b.Scale, glm::vec3(0.0f))))
			MultiplyUsingMatrixWithScale(result, a, b);
		else
		{
			result.Rotation = b.Rotation * a.Rotation;

			const glm::vec3 scaledTranslationA = a.Translation * b.Scale;
			const glm::vec3 rotatedTranslation = glm::rotate(b.Rotation, scaledTranslationA);
			result.Translation = rotatedTranslation + b.Translation;

			result.Scale = a.Scale * b.Scale;
		}
	}

	void Transform::MultiplyUsingMatrixWithScale(Transform& result, const Transform& a, const Transform& b)
	{
		ConstructTransformFromMatrixWithDesiredScale(a.GetMatrix(), b.GetMatrix(), a.Scale * b.Scale, result);
	}

	void Transform::ConstructTransformFromMatrixWithDesiredScale(const glm::mat4& a, const glm::mat4& b, const glm::vec3& desiredScale, Transform& result)
	{
		glm::mat4 m = a * b;
		Utils::RemoveMatrixScaling(m);

		const glm::vec3 signedScale = glm::sign(desiredScale);

		const glm::vec3 axisX = signedScale.x * glm::vec3(m[0]);
		const glm::vec3 axisY = signedScale.y * glm::vec3(m[1]);
		const glm::vec3 axisZ = signedScale.z * glm::vec3(m[2]);

		// Set back the adjusted axes into the matrix
		m[0] = glm::vec4(axisX, 0.0f);
		m[1] = glm::vec4(axisY, 0.0f);
		m[2] = glm::vec4(axisZ, 0.0f);

		result.Rotation = glm::normalize(glm::quat_cast(m));
		result.Scale = desiredScale;
		result.Translation = glm::vec3(m[3]);
	}

}