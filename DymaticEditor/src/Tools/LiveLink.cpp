#include "Tools/LiveLink.h"

#include "Dymatic/Core/Base.h"
#include "Panels/UI.h"
#include "Fonts.h"
#include "TextSymbols.h"

#include "Dymatic/Scene/Scene.h"

#include <imgui_internal.h>

#include <winsock2.h>
#include <windows.h>

#include <thread>
#include <nlohmann/json.hpp>

#define PORT 18080

using json = nlohmann::json;

namespace Dymatic {

	static std::thread s_LiveLinkServerThread;
	static std::atomic<bool> s_ServerRunning;

	static struct sockaddr_in s_ServerAddress;
	static SOCKET s_ServerSocket;

	static LiveLink::MotionData s_CurrentMotionData;
	static bool s_LiveLinkPanelVisible = false;
	static std::string s_DeviceName;
	static uint64_t s_LiveLinkCameraEntityID;

	static bool s_InitLiveLinkPosition;
	static glm::vec3 s_PreviousPosition;

	static bool ExtractMotionData(const json& jsonData, LiveLink::MotionData& motionData)
	{
		try
		{
			// Extract Roll
			if (!jsonData.contains("roll") || !jsonData["roll"].is_number())
				return false;

			motionData.Roll = jsonData["roll"].get<double>();

			// Extract Pitch
			if (!jsonData.contains("pitch") || !jsonData["pitch"].is_number())
				return false;

			motionData.Pitch = jsonData["pitch"].get<double>();

			// Extract Yaw
			if (!jsonData.contains("yaw") || !jsonData["yaw"].is_number())
				return false;

			motionData.Yaw = jsonData["yaw"].get<double>();

			// Extract Position
			if (!jsonData.contains("positionX") || !jsonData["positionX"].is_number() || !jsonData.contains("positionY") || !jsonData["positionY"].is_number() || !jsonData.contains("positionZ") || !jsonData["positionZ"].is_number())
				return false;

			motionData.Position = glm::vec3(jsonData["positionX"].get<double>(), jsonData["positionY"].get<double>(), jsonData["positionZ"].get<double>());

			// Extract velocity
			if (!jsonData.contains("velocityX") || !jsonData["velocityX"].is_number() || !jsonData.contains("velocityY") || !jsonData["velocityY"].is_number() || !jsonData.contains("velocityZ") || !jsonData["velocityZ"].is_number())
				return false;

			motionData.Velocity = glm::vec3(jsonData["velocityX"].get<double>(), jsonData["velocityY"].get<double>(), jsonData["velocityZ"].get<double>());

			// Extract acceleration
			if (!jsonData.contains("accelerationX") || !jsonData["accelerationX"].is_number() || !jsonData.contains("accelerationY") || !jsonData["accelerationY"].is_number() || !jsonData.contains("accelerationZ") || !jsonData["accelerationZ"].is_number())
				return false;

			motionData.Acceleration = glm::vec3(jsonData["accelerationX"].get<double>(), jsonData["accelerationY"].get<double>(), jsonData["accelerationZ"].get<double>());

			return true;
		}
		catch (json::exception& e)
		{
			// Handle failed JSON parsing
			DY_CORE_ERROR("JSON parsing error: {}", e.what());
		}

		return false;
	}

	static void HandleClientRequest(SOCKET clientSocket)
	{
		// Validate and get the data
		char buffer[1024] = { 0 };
		int recievedSize = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (recievedSize == SOCKET_ERROR)
		{
			DY_CORE_ERROR("recv failed with error '{}'", WSAGetLastError());
			return;
		}

		std::string request(buffer, recievedSize);

		// Find JSON data in the POST request
		const size_t jsonStart = request.find("\r\n\r\n");
		if (jsonStart != std::string::npos)
		{
			// Skip the headers
			std::string jsonBody = request.substr(jsonStart + 4);
			if (!jsonBody.empty())
			{
				try
				{
					json jsonData = json::parse(jsonBody);

					if (jsonData.contains("deviceName"))
					{
						// If data contains a 'deviceName' field this is establishing a connection
						if (jsonData["deviceName"].is_string())
						{
							const std::string name = jsonData["deviceName"].get<std::string>();
							s_DeviceName = name;
						}
						else
							DY_CORE_ERROR("Live Link device name specified was not a valid JSON string!");

						// Respond to the client
						std::string dymaticVersionString(DY_VERSION);
						std::string response = "HTTP/1.1 200 OK\r\n" + fmt::format("Content-Length: {}", dymaticVersionString.size()) + "\r\n\r\n" + dymaticVersionString;
						send(clientSocket, response.c_str(), response.size(), 0);
					}
					else
					{
						// Otherwise, extract the motion data from the JSON body and update the current motion data (only if valid)
						LiveLink::MotionData motionData;
						if (ExtractMotionData(jsonData, motionData))
							s_CurrentMotionData = motionData;

						// Respond to the client
						std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
						send(clientSocket, response.c_str(), response.size(), 0);
					}
				}
				catch (json::exception& e)
				{
					// Handle failed JSON parsing
					DY_CORE_ERROR("JSON parsing error: {}", e.what());
				}
			}
		}

		closesocket(clientSocket);
	}

	static void RunLiveLinkServer()
	{
		WSADATA wsaData;
		SOCKET clientSocket;
		struct sockaddr_in clientAddress;
		int addressLength = sizeof(clientAddress);

		// Initialize Winsock
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			DY_CORE_ERROR("WSAStartup failed with error '{}'", WSAGetLastError());
			return;
		}

		// Create a socket
		if ((s_ServerSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		{
			DY_CORE_ERROR("Socket creation failed with error '{}'", WSAGetLastError());
			WSACleanup();
			return;
		}

		// Bind the socket to the port
		s_ServerAddress.sin_family = AF_INET;
		s_ServerAddress.sin_addr.s_addr = INADDR_ANY;
		s_ServerAddress.sin_port = htons(PORT);

		if (bind(s_ServerSocket, (struct sockaddr*)&s_ServerAddress, sizeof(s_ServerAddress)) == SOCKET_ERROR)
		{
			DY_CORE_ERROR("Socket binding failed with error '{}'", WSAGetLastError());
			closesocket(s_ServerSocket);
			WSACleanup();
			return;
		}

		// Listen for incoming connections
		if (listen(s_ServerSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			DY_CORE_ERROR("Server socket listen failed with error '{}'", WSAGetLastError());
			closesocket(s_ServerSocket);
			WSACleanup();
			return;
		}

		DY_CORE_INFO("Live link server is listening on port {}", PORT);

		// Main connection loop
		while (s_ServerRunning)
		{
			clientSocket = accept(s_ServerSocket, (struct sockaddr*)&clientAddress, &addressLength);
			if (clientSocket == INVALID_SOCKET)
			{
				if (s_ServerRunning)
					DY_CORE_ERROR("Client accept failed with error '{}'", WSAGetLastError());

				break;
			}

			HandleClientRequest(clientSocket);
		}

		// Cleanup upon shutdown
		DY_CORE_INFO("Cleaning up Live Link Server");
		WSACleanup();
	}

	void LiveLink::Init()
	{
		DY_CORE_INFO("Initializing Dymatic Live Link server...");
		s_ServerRunning = true;
		s_DeviceName.clear();
		s_LiveLinkServerThread = std::thread(RunLiveLinkServer);

		s_InitLiveLinkPosition = true;
	}

	void LiveLink::Shutdown()
	{
		if (!s_ServerRunning)
			return;

		DY_CORE_INFO("Shutting down Dymatic Live Link server...");

		s_ServerRunning = false;
		closesocket(s_ServerSocket);

		s_LiveLinkServerThread.join();
	}

	const LiveLink::MotionData& LiveLink::GetMotionData()
	{
		return s_CurrentMotionData;
	}

	void LiveLink::OnImGuiRender(Timestep ts, Ref<Scene> activeScene)
	{
		// Live Link Update
		if (s_ServerRunning)
		{
			Entity liveLinkCamera = activeScene->GetEntityByUUID(s_LiveLinkCameraEntityID);
			if (liveLinkCamera && liveLinkCamera.HasComponent<TransformComponent>())
			{
				const auto& motionData = LiveLink::GetMotionData();

				if (s_InitLiveLinkPosition)
				{
					s_PreviousPosition = motionData.Position;
					s_InitLiveLinkPosition = false;
				}

				Transform& transform = liveLinkCamera.GetComponent<TransformComponent>().Transform;
				transform.SetRotationRadians(glm::vec3(motionData.Pitch, motionData.Yaw, motionData.Roll));
				transform.Translation += (motionData.Position - s_PreviousPosition);
				s_PreviousPosition = motionData.Position;
			}
		}

		// Draw Live Link Panel
		if (!s_LiveLinkPanelVisible)
			return;

		auto& io = ImGui::GetIO();
		auto& style = ImGui::GetStyle();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
		UI::CenterAppearingWindow(ImVec2(800.0f, 600.0f));
		ImGui::Begin("Live Link", &s_LiveLinkPanelVisible, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		// Header
		UI::PushFont(FontType::Bold);
		const char* text = "Live Link";
		ImGui::Dummy(ImVec2((ImGui::GetWindowContentRegionWidth() - ImGui::CalcTextSize(text).x) * 0.5f, 0.0f));
		ImGui::SameLine();
		ImGui::TextDisabled(text);
		UI::PopFont();

		ImGui::Dummy(ImVec2(0.0f, 10.0f));
			if (ImGui::CloseButton(ImGui::GetID("##LiveLinkCloseButton"), ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x - style.WindowPadding.x - 10.0f, ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMin().y)))
				s_LiveLinkPanelVisible = false;

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg) * ImVec4(1.25f, 1.25f, 1.25f, 1.0f));
		ImGui::BeginChild("##LiveLinkContentRegion", ImVec2(0.0f, ImGui::GetContentRegionAvail().y - 60.0f), false, ImGuiWindowFlags_AlwaysUseWindowPadding);
		ImGui::PopStyleColor();

		const bool serverRunning = s_ServerRunning;

		ImGui::Text("Live Link Camera Entity");
		ImGui::SameLine();
		UI::DrawEntitySelectionInput("##LiveLinkCameraEntity", s_LiveLinkCameraEntityID);

		// Main Live Link Content
		if (ImGui::Button(serverRunning ? FA_CIRCLE_XMARK " Stop Live Link Server" : FA_LINK " Start Live Link Server", ImVec2(ImGui::GetContentRegionAvailWidth(), 0.0f)))
		{
			if (s_ServerRunning)
				Shutdown();
			else
				Init();
		}

		if (serverRunning)
		{
			if (s_DeviceName.empty())
				ImGui::Text(FA_HOURGLASS "Waiting for Live Link Connection...");
			else
			{
				ImGui::TextDisabled(FA_MOBILE "Connected Device");
				ImGui::SameLine();
				ImGui::Text(s_DeviceName.c_str());
			}
		}

		ImGui::EndChild();

		ImGui::PopStyleVar();
		ImGui::End();
	}

	void LiveLink::ToggleVisibility()
	{
		s_LiveLinkPanelVisible = !s_LiveLinkPanelVisible;
	}

}