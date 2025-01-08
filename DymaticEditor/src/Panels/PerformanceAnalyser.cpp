#include "PerformanceAnalyser.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_internal.h"
#include "imgui/imgui.h" 

#include "Fonts.h"
#include "TextSymbols.h"
#include "Dymatic/Math/Math.h"

// Used for Memory Monitor
#include "psapi.h"

// Used for Core Monitor
#define _WIN32_DCOM
#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>
# pragma comment(lib, "wbemuuid.lib")

HRESULT hres;
IWbemServices* pSvc = NULL;
IWbemLocator* pLoc = NULL;
IEnumWbemClassObject* pEnumerator = NULL;
IWbemClassObject* pclsObj;

namespace Dymatic {

	PerformanceAnalyser::PerformanceAnalyser()
	{
		SYSTEM_INFO systemInfo;

		GetSystemInfo(&systemInfo);
		CPUCoreCount = systemInfo.dwNumberOfProcessors;

		// Initialize Core Monitor
		core_average = new int[CPUCoreCount];
		cores = new float*[CPUCoreCount + 1];
		for (size_t i = 0; i < CPUCoreCount + 1; i++)
		{
			cores[i] = new float[60];
			for (size_t k = 0; k < 60; k++)
				cores[i][k] = 0.0f;
		}

		// Initialize COM. ------------------------------------------
		hres = CoInitializeEx(0, COINIT_MULTITHREADED);
		if (FAILED(hres)) {
			std::cout << "Failed to initialize COM library. Error code = 0x" << std::hex << hres << std::endl;
		}

		// Set general COM security levels --------------------------
		// Note: If you are using Windows 2000, you need to specify -
		// the default authentication credentials for a user by using
		// a SOLE_AUTHENTICATION_LIST structure in the pAuthList ----
		// parameter of CoInitializeSecurity ------------------------

		hres = CoInitializeSecurity(
			NULL,
			-1,                          // COM authentication
			NULL,                        // Authentication services
			NULL,                        // Reserved
			RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
			RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
			NULL,                        // Authentication info
			EOAC_NONE,                   // Additional capabilities
			NULL                         // Reserved
		);


		if (FAILED(hres)) {
			std::cout << "Failed to initialize security. Error code = 0x"
				<< std::hex << hres << std::endl;
			CoUninitialize();
		}

		// Obtain the initial locator to WMI -------------------------

		hres = CoCreateInstance(
			CLSID_WbemLocator,
			0,
			CLSCTX_INPROC_SERVER,
			IID_IWbemLocator, (LPVOID*)&pLoc);

		if (FAILED(hres)) {
			std::cout << "Failed to create IWbemLocator object."
				<< " Err code = 0x"
				<< std::hex << hres << std::endl;
			CoUninitialize();
		}

		// Connect to WMI through the IWbemLocator::ConnectServer method

		// Connect to the root\cimv2 namespace with
		// the current user and obtain pointer pSvc
		// to make IWbemServices calls.
		hres = pLoc->ConnectServer(
			_bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
			NULL,                    // User name. NULL = current user
			NULL,                    // User password. NULL = current
			0,                       // Locale. NULL indicates current
			NULL,                    // Security flags.
			0,                       // Authority (e.g. Kerberos)
			0,                       // Context object
			&pSvc                    // pointer to IWbemServices proxy
		);

		if (FAILED(hres)) {
			std::cout << "Could not connect. Error code = 0x" << std::hex << hres << std::endl;
			pLoc->Release();
			CoUninitialize();
		}

		std::cout << "Connected to ROOT\\CIMV2 WMI namespace" << std::endl;

		// Set security levels on the proxy -------------------------

		hres = CoSetProxyBlanket(
			pSvc,                        // Indicates the proxy to set
			RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
			RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
			NULL,                        // Server principal name
			RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
			RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
			NULL,                        // client identity
			EOAC_NONE                    // proxy capabilities
		);

		if (FAILED(hres)) {
			std::cout << "Could not set proxy blanket. Error code = 0x"
				<< std::hex << hres << std::endl;
			pSvc->Release();
			pLoc->Release();
			CoUninitialize();
		}
	}

	PerformanceAnalyser::~PerformanceAnalyser()
	{
		// Cleanup Core Monitor //
		for (size_t i = 0; i < CPUCoreCount; i++)
			delete[] cores[i];
		delete[] cores;

		delete[] core_average;

		//if (pSvc) pSvc->Release();
		//if (pLoc) pLoc->Release();
		if (pEnumerator) pEnumerator->Release();
		if (pclsObj) pclsObj->Release();
		//CoUninitialize();
	}

	static void MetricsHelpMarker(const char* desc)
	{
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(desc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}

	static int IMGUI_CDECL ViewportComparerByFrontMostStampCount(const void* lhs, const void* rhs)
	{
		const ImGuiViewportP* a = *(const ImGuiViewportP* const*)lhs;
		const ImGuiViewportP* b = *(const ImGuiViewportP* const*)rhs;
		return b->LastFocusedStampCount - a->LastFocusedStampCount;
	}

	void PerformanceAnalyser::OnImGuiRender(Timestep ts)
	{
		if (m_PerformanceAnalyserVisible)
		{
			ImGui::Begin(FA_CHART_BAR " Performance Analyzer", &m_PerformanceAnalyserVisible);
			auto drawList = ImGui::GetWindowDrawList();
			{
				{

					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
					bool open = (ImGui::TreeNodeEx("Delta Time", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding));
					ImGui::PopStyleVar();
					if (open)
					{
						static float offset = 0.0f;
						offset += ts.GetSeconds();
						m_DeltaSteps.push_back(ts.GetMilliseconds());
						static float values[90] = {};
						static int values_offset = 0;
						static double refresh_time = 0.0;
						while (refresh_time < ImGui::GetTime()) // Create data at fixed 60 Hz rate for the demo
						{
							static float phase = 0.0f;
							values[values_offset] = m_DeltaSteps[std::clamp(m_DeltaSteps.size() - 90.0f + values_offset, 0.0f, m_DeltaSteps.size() - 1.0f)];
							values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
							phase += 0.10f * values_offset;
							refresh_time += 1.0f / 60.0f;
						}

						{
							float average = 0.0f;
							for (int n = 0; n < IM_ARRAYSIZE(values); n++)
								average += values[n];
							average /= (float)IM_ARRAYSIZE(values);
							char overlay[32];
							sprintf(overlay, "Delta Time %f", average);
							ImGui::PlotLines("##DeltaTimeGraph", values, IM_ARRAYSIZE(values), values_offset, overlay, m_DeltaMin, m_DeltaMax, ImVec2(ImGui::GetContentRegionAvail().x, 150.0f));
						}
						ImGui::DragFloat("Delta Display Min", &m_DeltaMin, 0.001f);
						ImGui::DragFloat("Delta Display Max", &m_DeltaMax, 0.001f);

						ImGui::TreePop();
					}
				}
			}

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			bool open = (ImGui::TreeNodeEx("CPU Core Performance", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding));
			ImGui::PopStyleVar();
			if (open)
			{
				auto rounding = ImGui::GetStyle().FrameRounding;
				auto lineHeight = ImGui::GetTextLineHeight();

				if (refresh_time < ImGui::GetTime())
				{
					UpdateCPUCoreLoadInfo();
					refresh_time += 1.0f;
				}

				for (size_t i = 0; i < CPUCoreCount; i++)
				{
					ImGui::PushID(i);
					char overlay[32];
					sprintf(overlay, "CPU %i: %i%%", i, core_average[i]);
					ImGui::PlotLines("##CPUCoreGraph", cores[i], 60, 5, overlay, 0.0f, 100.0f, ImVec2(250.0f, 100.0f));

					const auto& min = ImGui::GetItemRectMin();
					const auto& max = ImGui::GetItemRectMax();
					drawList->AddRectFilled(ImVec2(min.x, max.y - ((max.y - min.y) * (core_average[i] / 100.0f))), max, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TextSelectedBg)), rounding);

					if (core_average[i] < 5)
					{
						drawList->AddRectFilled(ImVec2(min.x + 50.0f, min.y + ((max.y - min.y) * 0.5f) - lineHeight), ImVec2(max.x - 50.0f, min.y + ((max.y - min.y) * 0.5f) + lineHeight), IM_COL32(25, 25, 30, 200), rounding);
						drawList->AddText((max + min) / 2.0f - ImGui::CalcTextSize("CORE IDLE") / 2.0f, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Text)), "CORE IDLE");
					}

					if (core_average[i] > 95)
					{
						drawList->AddRectFilled(ImVec2(min.x + 50.0f, min.y + ((max.y - min.y) * 0.5f) - lineHeight), ImVec2(max.x - 50.0f, min.y + ((max.y - min.y) * 0.5f) + lineHeight), IM_COL32(255, 15, 30, 200), rounding);
						UI::PushFont(FontType::Bold);
						drawList->AddText((max + min) / 2.0f - ImGui::CalcTextSize("HEAVY LOAD") / 2.0f, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Text)), "HEAVY LOAD");
						UI::PopFont();
					}

					if (i < CPUCoreCount - 1 && ImGui::GetItemRectMax().x + 250.0f < ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x)
						ImGui::SameLine();

					ImGui::PopID();
				}

				ImGui::Text("Total Utilization: %i%%", core_average[CPUCoreCount]);
				ImGui::Text("Core Count: %i", CPUCoreCount);

				ImGui::TreePop();
			}



			//ImGui::BeginChild("##ProfilerWindow", ImVec2(ImGui::GetContentRegionAvail().x, 250.0f));
			//
			//for (int i = 0; i < m_ProfilerPoints.size(); i++) 
			//{
			//	auto min = ImGui::GetWindowPos() + ImVec2(m_ProfilerPoints[i].ts / 2000.0f, (m_ProfilerPoints[i].index * 20.0f) + 0.0f);
			//	auto max = ImGui::GetWindowPos() + ImVec2(m_ProfilerPoints[i].ts / 2000.0f + m_ProfilerPoints[i].duration / 2000.0f, (m_ProfilerPoints[i].index * 20.0f) + 20.0f);
			//	ImGui::GetWindowDrawList()->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32(ImVec4(m_ProfilerPoints[i].color.x, m_ProfilerPoints[i].color.y, m_ProfilerPoints[i].color.z, 1.0f)));
			//	
			//	const ImGuiID id = ImGui::GetCurrentWindow()->GetID(("##ProfilerPoint" + std::to_string(m_ProfilerPoints[i].id)).c_str());
			//	const ImRect bb(min, max);
			//	bool hovered, held;
			//	bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
			//
			//	if (hovered)
			//	{
			//		ImGui::BeginTooltip();
			//		ImGui::Text("Name: %s", m_ProfilerPoints[i].name.c_str());
			//		ImGui::Text("Timestamp: %f", m_ProfilerPoints[i].ts);
			//		ImGui::Text("Duration: %f", m_ProfilerPoints[i].duration);
			//		ImGui::EndTooltip();
			//	}
			//
			//	auto difference = max.x - min.x;
			//	if (difference > 5.0f)
			//	{
			//		std::string displayText = m_ProfilerPoints[i].name;
			//		bool modified = false;
			//		while (ImGui::CalcTextSize(displayText.c_str()).x > difference - (modified ? ImGui::CalcTextSize("...").x : 0.0f) - 2.5f && displayText != "")
			//		{
			//			displayText = displayText.substr(0, displayText.size() - 1);
			//			modified = true;
			//		}
			//
			//		if (modified && ImGui::CalcTextSize("...").x < difference && displayText != "")
			//		{
			//			displayText += "...";
			//		}
			//
			//		auto textPos = (min + ((max - min) / 2)) - (ImGui::CalcTextSize(displayText.c_str()) / 2);
			//		if (true/*textPos.x < min.x*/) { textPos = ImVec2(min.x + 2.5f, textPos.y); }
			//		ImGui::GetWindowDrawList()->AddText(textPos, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Text)), displayText.c_str());
			//	}
			//}
			//ImGui::EndChild();
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				bool open = (ImGui::TreeNodeEx("Process Memory", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding));
				ImGui::PopStyleVar();
				if (open)
				{
					MEMORYSTATUSEX memInfo;
					memInfo.dwLength = sizeof(MEMORYSTATUSEX);
					GlobalMemoryStatusEx(&memInfo);
					DWORDLONG totalVirtualMem = memInfo.ullTotalPageFile;


					PROCESS_MEMORY_COUNTERS_EX pmc;
					GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
					SIZE_T virtualMemUsedByMe = pmc.PrivateUsage;

					{
						static float offset = 0.0f;
						offset += ts.GetSeconds();
						m_MemoryUsageSteps.push_back((float)virtualMemUsedByMe / 1024 / 1000);
						static float values[90] = {};
						static int values_offset = 0;
						static double refresh_time = 0.0;
						while (refresh_time < ImGui::GetTime()) // Create data at fixed 60 Hz rate for the demo
						{
							static float phase = 0.0f;
							values[values_offset] = m_MemoryUsageSteps[std::clamp(m_MemoryUsageSteps.size() - 90.0f + values_offset, 0.0f, m_MemoryUsageSteps.size() - 1.0f)];
							values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
							phase += 0.10f * values_offset;
							refresh_time += 1.0f / 60.0f;
						}

						{
							float average = 0.0f;
							for (int n = 0; n < IM_ARRAYSIZE(values); n++)
								average += values[n];
							average /= (float)IM_ARRAYSIZE(values);
							char overlay[32];
							sprintf(overlay, "Process Memory %f", average);

							// Find the minimum
							float min = 0;
							for (int n = 0; n < IM_ARRAYSIZE(values); n++)
								min = std::fmin(min, (uint32_t)values[n]);

							// Find the maximum
							float max = 0;
							for (int n = 0; n < IM_ARRAYSIZE(values); n++)
								max = std::fmax(max, (uint32_t)values[n]);

							min *= 0.9f;
							max *= 1.1f;

							ImGui::PlotLines("##ProcessMemoryGraph", values, IM_ARRAYSIZE(values), values_offset, overlay, min, max, ImVec2(ImGui::GetContentRegionAvail().x, 150.0f));
						}
					}

					ImGui::Text("Process Memory: %s MB", std::to_string((float)virtualMemUsedByMe / 1024 / 1000).c_str());
					ImGui::Text("Total Memory: %s MB", std::to_string(totalVirtualMem).c_str());

					ImGui::TreePop();
				}
			}


			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				bool open = (ImGui::TreeNodeEx("ImGui Debug", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding));
				ImGui::PopStyleVar();
				if (open)
				{
					ImGui::BeginChild("##ImGui Debug");
					ImGui::ShowMetricsWindow();
					ImGui::EndChild();

					ImGui::TreePop();
				}
			}

			ImGui::End();
		}
	}

	bool PerformanceAnalyser::UpdateCPUCoreLoadInfo()
	{
		DY_PROFILE_FUNCTION();

		int i = 0;
		{
			hres = pSvc->ExecQuery(
				bstr_t("WQL"),
				bstr_t("SELECT * FROM Win32_PerfFormattedData_PerfOS_Processor"),

				WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
				NULL,
				&pEnumerator);

			if (FAILED(hres)) {
				std::cout << "Query for operating system name failed."
					<< " Error code = 0x"
					<< std::hex << hres << std::endl;
				pSvc->Release();
				pLoc->Release();
				CoUninitialize();
				return 0;               // Program has failed.
			}

			ULONG uReturn = 0;

			while (pEnumerator) {
				HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
					&pclsObj, &uReturn);

				if (0 == uReturn) {
					break;
				}

				VARIANT vtProp;

				// Get the value of the Name property
				//hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
				hr = pclsObj->Get(L"PercentProcessorTime", 0, &vtProp, 0, 0);
				//std::wcout << " CPU Usage of CPU " << i << " : " << vtProp.bstrVal << std::endl;

				//memmove(cores[i], cores[i] - 1, sizeof(cores[i]));

				for (size_t k = 0; k < 59; k++)
					cores[i][k] = cores[i][k + 1];

				cores[i][59] = (float)_wtof(vtProp.bstrVal);

				// Compute average
				int total = 0;
				for (size_t k = 0; k < 60; k++)
					total += cores[i][k];
				core_average[i] = total / 60;

				VariantClear(&vtProp);

				//IMPORTANT!!
				pclsObj->Release();

				i++;
			}
		}
	}

	void PerformanceAnalyser::ReadJsonFilePerformanceAnalitics(std::string filepath)
	{
		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary); // ifstream closes itself due to RAII
		if (in)
		{
			in.seekg(0, std::ios::end);
			size_t size = in.tellg();
			if (size != -1)
			{
				result.resize(size);
				in.seekg(0, std::ios::beg);
				in.read(&result[0], size);
			}
			else
			{
				DY_CORE_ERROR("Could not read from file '{0}'", filepath);
				return;
			}
		}

		if (result != "")
		{
			float offset = 0.0f;
			while (result.find("{\"cat\"") != std::string::npos)
			{
				result = result.substr(result.find("{\"cat\""));
				std::string name = result.substr(result.find("\"name\":\"") + 8, result.find("\",\"ph") - 8 - result.find("\"name\":\""));
				float ts = std::stof(result.substr(result.find("\"ts\":") + 5, result.find("},") - 5 - result.find("\"ts\":")));
				auto a = result.substr(result.find("\"dur\":") + 6, result.find(",\"name") - 6 - result.find("\"dur\":"));
				int duration = std::stoi(a);
				m_ProfilerPoints.push_back(ProfilerPoint(GetNextProfilerPointId(), name, ts, duration, glm::vec3(Math::GetRandomInRange(0, 100) / 100.0f, Math::GetRandomInRange(0, 100) / 100.0f, Math::GetRandomInRange(0, 100) / 100.0f)));
				result = result.substr(result.find("}"));
			}

			if (!m_ProfilerPoints.empty())
			{
				float lowest = m_ProfilerPoints[0].ts;
				for (int i = 0; i < m_ProfilerPoints.size(); i++)
				{
					if (m_ProfilerPoints[i].ts < lowest) 
					{
						lowest = m_ProfilerPoints[i].ts;
					}
				}
				m_ProfilerStartTime = lowest;

				for (int i = 0; i < m_ProfilerPoints.size(); i++)
				{
					m_ProfilerPoints[i].ts -= m_ProfilerStartTime;
				}

				// Order Data Indexes
				bool changeMade = true;
				while (changeMade)
				{
					changeMade = false;
					for (int i = 0; i < m_ProfilerPoints.size(); i++)
					{
						for (int y = 0; y < m_ProfilerPoints.size(); y++)
						{
							auto i_id_offset = m_ProfilerPoints[i].id / 100000.0f;
							auto y_id_offset = m_ProfilerPoints[y].id / 100000.0f;
							if (m_ProfilerPoints[i].ts + i_id_offset > m_ProfilerPoints[y].ts + y_id_offset && m_ProfilerPoints[i].ts + i_id_offset < m_ProfilerPoints[y].ts + y_id_offset + m_ProfilerPoints[y].duration && m_ProfilerPoints[i].index == m_ProfilerPoints[y].index)
							{
								m_ProfilerPoints[i].index++;
								changeMade = true;
							}
						}
					}
				}
			}
		}
	}
}