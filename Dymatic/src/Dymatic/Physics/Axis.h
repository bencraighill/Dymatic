#pragma once

#include <glm/glm.hpp>

namespace Dymatic {

	static constexpr float c_AxisMarker = 3.402823466e+18F;
	static constexpr glm::vec3 c_AxisX = glm::vec3(c_AxisMarker, 0.0f, 0.0f);
	static constexpr glm::vec3 c_AxisY = glm::vec3(0.0f, c_AxisMarker, 0.0f);
	static constexpr glm::vec3 c_AxisZ = glm::vec3(0.0f, 0.0f, c_AxisMarker);

	static constexpr uint32_t c_AxisCount = 3;

	enum class Axis
	{
		X,
		Y,
		Z,
	};

}