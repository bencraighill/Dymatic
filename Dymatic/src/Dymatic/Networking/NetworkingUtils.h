#pragma once

#include <string>

namespace Dymatic::Utils {

	bool IsValidIPAddress(std::string_view ipAddress);

	// Platform-specific implementations
	std::string ResolveDomainName(std::string_view name);

}