#include "dypch.h"
#include "Math.h"
#include <Math.h>

#include <iostream>
#include <cstdlib> 
#include <ctime>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

namespace Dymatic::Math {

	bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale)
	{
		// From glm::decompose in matrix_decompose.inl

		using namespace glm;
		using T = float;

		mat4 LocalMatrix(transform);

		// Normalize the matrix.
		if (epsilonEqual(LocalMatrix[3][3], static_cast<float>(0), epsilon<T>()))
			return false;

		// First, isolate perspective.  This is the messiest.
		if (
			epsilonNotEqual(LocalMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(LocalMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(LocalMatrix[2][3], static_cast<T>(0), epsilon<T>()))
		{
			// Clear the perspective partition
			LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
			LocalMatrix[3][3] = static_cast<T>(1);
		}

		// Next take care of translation (easy).
		translation = vec3(LocalMatrix[3]);
		LocalMatrix[3] = vec4(0, 0, 0, LocalMatrix[3].w);

		vec3 Row[3], Pdum3;

		// Now get scale and shear.
		for (length_t i = 0; i < 3; ++i)
			for (length_t j = 0; j < 3; ++j)
				Row[i][j] = LocalMatrix[i][j];

		// Compute X scale factor and normalize first row.
		scale.x = length(Row[0]);
		Row[0] = detail::scale(Row[0], static_cast<T>(1));
		scale.y = length(Row[1]);
		Row[1] = detail::scale(Row[1], static_cast<T>(1));
		scale.z = length(Row[2]);
		Row[2] = detail::scale(Row[2], static_cast<T>(1));

		// At this point, the matrix (in rows[]) is orthonormal.
		// Check for a coordinate system flip.  If the determinant
		// is -1, then negate the matrix and the scaling factors.
#if 0
		Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
		if (dot(Row[0], Pdum3) < 0)
		{
			for (length_t i = 0; i < 3; i++)
			{
				scale[i] *= static_cast<T>(-1);
				Row[i] *= static_cast<T>(-1);
			}
		}
#endif

		rotation.y = asin(-Row[0][2]);
		if (cos(rotation.y) != 0) {
			rotation.x = atan2(Row[1][2], Row[2][2]);
			rotation.z = atan2(Row[0][1], Row[0][0]);
		}
		else {
			rotation.x = atan2(-Row[2][0], Row[1][1]);
			rotation.z = 0;
		}


		return true;
	}

	unsigned int GetRandomInRange(int min, int max)
	{
		return ((rand() * std::time(nullptr)) % max + min);
	}

	// Normalizes any number to an arbitrary range 
	// by assuming the range wraps around when going below min or above max 
	float NormalizeAngle(const float value, const float shift, const float size)
	{
		return value + ceil((-value + shift) / size) * size;
	}

	float LerpAngle(float a, float b, float x)
	{
		float angleDifference = b - a;
		while (angleDifference > 180) angleDifference -= 360;
		while (angleDifference < -180) angleDifference += 360;
		return NormalizeAngle(a + angleDifference * x, -180);
	}

	float Lerp(float a, float b, float x)
	{
		return a + (b - a) * x;
	}

	float Absolute(float x)
	{
		return (x < 0 ? (x * -1) : (x));
	}

	float FloatDistance(float x, float y)
	{
		return Absolute(Absolute(x) - Absolute(y));
	}

	float RoundUp(float numToRound, float multiple)
	{
		if (multiple == 0)
			return numToRound;
		int a = 6 % 4;
		float remainder = std::fmod(abs(numToRound), multiple);
		if (remainder == 0)
			return numToRound;

		if (numToRound < 0)
			return -(abs(numToRound) - remainder);
		else
			return numToRound + multiple - remainder;
	}

	bool NearlyEqual(float a, float b, float ErrorTolerence)
	{
		return (Absolute(a - b) <= ErrorTolerence);
	}

}