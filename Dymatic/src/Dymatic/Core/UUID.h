#pragma once

#include <stdint.h>
#include <xhash>

namespace Dymatic {

	class UUID
	{
	public:
		UUID();
		UUID(uint64_t uuid);
		UUID(const UUID& other);

		operator uint64_t () { return m_UUID; }
		operator const uint64_t() const { return m_UUID; }
	private:
		uint64_t m_UUID;
	};

}

namespace std {

	template <>
	struct hash<Dymatic::UUID>
	{
		std::size_t operator()(const Dymatic::UUID& uuid) const
		{
			return hash<uint64_t>()((uint64_t)uuid);
		}
	};

}