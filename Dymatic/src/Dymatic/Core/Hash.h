#pragma once

namespace std {
	template<>
	struct hash<glm::vec2>
	{
		std::size_t operator()(const glm::vec2& v) const noexcept
		{
			std::hash<float> hasher;
			std::size_t h1 = hasher(v.x);
			std::size_t h2 = hasher(v.y);

			// Combine the hash values
			return h1 ^ (h2 << 1);
		}
	};
	
	template<>
	struct hash<glm::vec3>
	{
		std::size_t operator()(const glm::vec3& v) const noexcept
		{
			std::hash<float> hasher;
			std::size_t h1 = hasher(v.x);
			std::size_t h2 = hasher(v.y);
			std::size_t h3 = hasher(v.z);

			// Combine the hash values
			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};
}