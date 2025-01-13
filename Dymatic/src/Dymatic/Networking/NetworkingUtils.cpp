#include "dypch.h"
#include "Dymatic/Networking/NetworkingUtils.h"

#include <steam/isteamnetworkingutils.h>

namespace Dymatic::Utils {

	bool IsValidIPAddress(std::string_view ipAddress)
	{
		std::string ipAddressStr(ipAddress.data(), ipAddress.size());

		SteamNetworkingIPAddr address;
		return address.ParseString(ipAddressStr.c_str());
	}

}