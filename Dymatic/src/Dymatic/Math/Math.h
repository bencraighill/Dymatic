#pragma once

#include <glm/glm.hpp>

namespace Dymatic::Math {

	bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
	unsigned int GetRandomInRange(int min, int max);
	float RandomRange(float min, float max);
	float NormalizeAngle(const float value, const float shift, const float size = 360.0f);
	float LerpAngle(float a, float b, float x, float min = -180, float max = 180);
	float Lerp(float a, float b, float x);
	float Absolute(float x);
	float FloatDistance(float x, float y);
	float RoundUp(float numToRound, float multiple = 1.0f);
	bool NearlyEqual(float a, float b, float ErrorTolerence);
	float Distance(glm::vec2 a, glm::vec2 b);
	float InterpEaseIn(float a, float b, float Alpha, float Exp);
	float InterpEaseOut(float a, float b, float Alpha, float Exp);
	float InterpEaseInOut(float a, float b, float Alpha, float Exp);
}