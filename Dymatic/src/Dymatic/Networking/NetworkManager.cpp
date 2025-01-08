#include "dypch.h"
#include "Dymatic/Networking/NetworkManager.h"

#include "Dymatic/Core/BufferStream.h"
#include "Dymatic/Networking/PacketType.h"
#include "Dymatic/Networking/Server.h"
#include "Dymatic/Networking/Client.h"

#include "Dymatic/Scripting/ScriptEngine.h"

namespace Dymatic {

	static Ref<Server> s_Server = nullptr;
	static Ref<Client> s_Client = nullptr;

	static Buffer s_ScratchBuffer(1024);

	void NetworkManager::StartServer(const uint32_t& port)
	{
		s_Server = CreateRef<Server>(port);
		s_Server->Start();
		
		s_Server->SetDataReceivedCallback([](const ClientInfo& clientInfo, const Buffer data)
		{
			BufferStreamReader stream(data);
			PacketType packetType;
			stream.ReadRaw<PacketType>(packetType);
			
			switch (packetType)
			{
			case PacketType::None:
			{
				DY_CORE_WARN("Server recieve null packet type");
				break;
			}
			case PacketType::InternalMessage:
			{
				break;
			}
			case PacketType::ScriptMessage:
			{
				UUID entityID;
				stream.ReadRaw<UUID>(entityID);
				std::string methodName;
				stream.ReadString(methodName);
				Buffer args;
				stream.ReadBuffer(args);

				ScriptEngine::ExecuteEntityMethod(entityID, methodName, args);

				DY_CORE_INFO("Executing script method '{}' on entity {}", methodName, entityID);
				
				break;
			}
			default:
			{
				DY_CORE_WARN("Server recieve unknown packet type");
				break;
			}
			}
		});

		s_Server->SetClientConnectedCallback([](const ClientInfo& clientInfo)
		{
			DY_CORE_INFO("Client connected: {}", clientInfo.ConnectionDesc);
		});
		
		s_Server->SetClientDisconnectedCallback([](const ClientInfo& clientInfo)
		{
			DY_CORE_INFO("Client disconnected: {}", clientInfo.ConnectionDesc);
		});
	}

	void NetworkManager::StopServer()
	{
		s_Server->Stop();
		s_Server = nullptr;
	}

	void NetworkManager::StartClient(const std::string& ip)
	{
		s_Client = CreateRef<Client>();
		
		s_Client->SetServerConnectedCallback([&]()
		{
			DY_CORE_INFO("Connected to server '{}'", ip);
		});
		
		s_Client->SetServerDisconnectedCallback([&]() 
		{
			DY_CORE_WARN("Lost connection to server");
			StopClient();
		});
		
		s_Client->SetDataReceivedCallback([&](const Buffer data) 
		{
			DY_CORE_INFO("Packet received");
			
			BufferStreamReader stream(data);
			
			PacketType packetType;
			stream.ReadRaw<PacketType>(packetType);

			switch (packetType)
			{
			case PacketType::None:
			{
				DY_CORE_WARN("Network manager recieve null packet type");
				break;
			}
			case PacketType::InternalMessage:
			{
				break;
			}
			case PacketType::ScriptMessage:
			{
				UUID entityID;
				stream.ReadRaw<UUID>(entityID);
				std::string methodName;
				stream.ReadString(methodName);
				Buffer args;
				stream.ReadBuffer(args);

				ScriptEngine::ExecuteEntityMethod(entityID, methodName, args);

				DY_CORE_INFO("Executing script method '{}' on entity {}", methodName, entityID);

				break;
			}
			case PacketType::ServerShutdown:
			case PacketType::ClientKick:
			{
				StopClient();
				break;
			}
			default:
			{
				DY_CORE_WARN("Network manager recieve invalid packet type");
				break;
			}
			}
			
		});
		
		s_Client->ConnectToServer(ip);
		
		// Wait for server response
		while (s_Client->GetConnectionStatus() != Client::ConnectionStatus::Connected && s_Client->GetConnectionStatus() != Client::ConnectionStatus::FailedToConnect)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	void NetworkManager::StopClient()
	{
		s_Client->Disconnect();
		s_Client = nullptr;
	}

	void NetworkManager::Send(const Buffer& buffer)
	{
		if (s_Server)
			s_Server->SendBufferToAllClients(buffer);
		else if (s_Client)
			s_Client->SendBuffer(buffer);
		else
			DY_CORE_WARN("Network manager failed to send buffer - no network");
	}

	bool NetworkManager::IsServer()
	{
		return s_Server != nullptr;
	}

}