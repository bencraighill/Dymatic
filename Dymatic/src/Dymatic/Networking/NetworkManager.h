#pragma once

#include "Dymatic/Core/Buffer.h"
#include "Dymatic/Networking/PacketType.h"

#include <string>

namespace Dymatic {

	class NetworkManager
	{
	public:
		static void Init();
		static void Shutdown();

		static void StartServer(const uint32_t& port);
		static void StopServer();

		static void StartClient(const std::string& ip);
		static void StopClient();

		static void Send(const Buffer& buffer);

		static bool IsServer();
	};
	
}