#include "PerformanceAnalyser.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_internal.h"
#include "imgui/imgui.h" 

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
		return b->LastFrontMostStampCount - a->LastFrontMostStampCount;
	}

	void PerformanceAnalyser::OnImGuiRender(Timestep ts)
	{
		if (m_PerformanceAnalyserVisible)
		{
			ImGui::Begin("Performance Analyser", &m_PerformanceAnalyserVisible);
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
			if (ImGui::Button("Assert Program")) { DY_CORE_ASSERT(false, "Manual Assertion - No Error"); }

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
						ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
						drawList->AddText((max + min) / 2.0f - ImGui::CalcTextSize("HEAVY LOAD") / 2.0f, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Text)), "HEAVY LOAD");
						ImGui::PopFont();
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
							sprintf(overlay, "Process Memeory %f", average);
							ImGui::PlotLines("##ProcessMemoryGraph", values, IM_ARRAYSIZE(values), values_offset, overlay, m_MemoryUsageMin, m_MemoryUsageMax, ImVec2(ImGui::GetContentRegionAvail().x, 150.0f));
						}
						ImGui::DragFloat("Memory Display Min", &m_MemoryUsageMin, 0.001f);
						ImGui::DragFloat("Memory Display Max", &m_MemoryUsageMax, 0.001f);
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
					ImGuiContext& g = *GImGui;
					ImGuiIO& io = g.IO;
					ImGuiMetricsConfig* cfg = &g.DebugMetricsConfig;
					if (cfg->ShowStackTool)
						ImGui::ShowStackToolWindow(&cfg->ShowStackTool);

					ImGui::BeginChild("##ImGui Debug");

					// Basic info
					ImGui::Text("Dear ImGui %s", ImGui::GetVersion());
					ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
					ImGui::Text("%d vertices, %d indices (%d triangles)", io.MetricsRenderVertices, io.MetricsRenderIndices, io.MetricsRenderIndices / 3);
					ImGui::Text("%d visible windows, %d active allocations", io.MetricsRenderWindows, io.MetricsActiveAllocations);
					//SameLine(); if (SmallButton("GC")) { g.GcCompactAll = true; }

					ImGui::Separator();

					// Debugging enums
					enum { WRT_OuterRect, WRT_OuterRectClipped, WRT_InnerRect, WRT_InnerClipRect, WRT_WorkRect, WRT_Content, WRT_ContentIdeal, WRT_ContentRegionRect, WRT_Count }; // Windows Rect Type
					const char* wrt_rects_names[WRT_Count] = { "OuterRect", "OuterRectClipped", "InnerRect", "InnerClipRect", "WorkRect", "Content", "ContentIdeal", "ContentRegionRect" };
					enum { TRT_OuterRect, TRT_InnerRect, TRT_WorkRect, TRT_HostClipRect, TRT_InnerClipRect, TRT_BackgroundClipRect, TRT_ColumnsRect, TRT_ColumnsWorkRect, TRT_ColumnsClipRect, TRT_ColumnsContentHeadersUsed, TRT_ColumnsContentHeadersIdeal, TRT_ColumnsContentFrozen, TRT_ColumnsContentUnfrozen, TRT_Count }; // Tables Rect Type
					const char* trt_rects_names[TRT_Count] = { "OuterRect", "InnerRect", "WorkRect", "HostClipRect", "InnerClipRect", "BackgroundClipRect", "ColumnsRect", "ColumnsWorkRect", "ColumnsClipRect", "ColumnsContentHeadersUsed", "ColumnsContentHeadersIdeal", "ColumnsContentFrozen", "ColumnsContentUnfrozen" };
					if (cfg->ShowWindowsRectsType < 0)
						cfg->ShowWindowsRectsType = WRT_WorkRect;
					if (cfg->ShowTablesRectsType < 0)
						cfg->ShowTablesRectsType = TRT_WorkRect;

					struct Funcs
					{
						static ImRect GetTableRect(ImGuiTable* table, int rect_type, int n)
						{
							if (rect_type == TRT_OuterRect) { return table->OuterRect; }
							else if (rect_type == TRT_InnerRect) { return table->InnerRect; }
							else if (rect_type == TRT_WorkRect) { return table->WorkRect; }
							else if (rect_type == TRT_HostClipRect) { return table->HostClipRect; }
							else if (rect_type == TRT_InnerClipRect) { return table->InnerClipRect; }
							else if (rect_type == TRT_BackgroundClipRect) { return table->BgClipRect; }
							else if (rect_type == TRT_ColumnsRect) { ImGuiTableColumn* c = &table->Columns[n]; return ImRect(c->MinX, table->InnerClipRect.Min.y, c->MaxX, table->InnerClipRect.Min.y + table->LastOuterHeight); }
							else if (rect_type == TRT_ColumnsWorkRect) { ImGuiTableColumn* c = &table->Columns[n]; return ImRect(c->WorkMinX, table->WorkRect.Min.y, c->WorkMaxX, table->WorkRect.Max.y); }
							else if (rect_type == TRT_ColumnsClipRect) { ImGuiTableColumn* c = &table->Columns[n]; return c->ClipRect; }
							else if (rect_type == TRT_ColumnsContentHeadersUsed) { ImGuiTableColumn* c = &table->Columns[n]; return ImRect(c->WorkMinX, table->InnerClipRect.Min.y, c->ContentMaxXHeadersUsed, table->InnerClipRect.Min.y + table->LastFirstRowHeight); } // Note: y1/y2 not always accurate
							else if (rect_type == TRT_ColumnsContentHeadersIdeal) { ImGuiTableColumn* c = &table->Columns[n]; return ImRect(c->WorkMinX, table->InnerClipRect.Min.y, c->ContentMaxXHeadersIdeal, table->InnerClipRect.Min.y + table->LastFirstRowHeight); }
							else if (rect_type == TRT_ColumnsContentFrozen) { ImGuiTableColumn* c = &table->Columns[n]; return ImRect(c->WorkMinX, table->InnerClipRect.Min.y, c->ContentMaxXFrozen, table->InnerClipRect.Min.y + table->LastFirstRowHeight); }
							else if (rect_type == TRT_ColumnsContentUnfrozen) { ImGuiTableColumn* c = &table->Columns[n]; return ImRect(c->WorkMinX, table->InnerClipRect.Min.y + table->LastFirstRowHeight, c->ContentMaxXUnfrozen, table->InnerClipRect.Max.y); }
							IM_ASSERT(0);
							return ImRect();
						}

						static ImRect GetWindowRect(ImGuiWindow* window, int rect_type)
						{
							if (rect_type == WRT_OuterRect) { return window->Rect(); }
							else if (rect_type == WRT_OuterRectClipped) { return window->OuterRectClipped; }
							else if (rect_type == WRT_InnerRect) { return window->InnerRect; }
							else if (rect_type == WRT_InnerClipRect) { return window->InnerClipRect; }
							else if (rect_type == WRT_WorkRect) { return window->WorkRect; }
							else if (rect_type == WRT_Content) { ImVec2 min = window->InnerRect.Min - window->Scroll + window->WindowPadding; return ImRect(min, min + window->ContentSize); }
							else if (rect_type == WRT_ContentIdeal) { ImVec2 min = window->InnerRect.Min - window->Scroll + window->WindowPadding; return ImRect(min, min + window->ContentSizeIdeal); }
							else if (rect_type == WRT_ContentRegionRect) { return window->ContentRegionRect; }
							IM_ASSERT(0);
							return ImRect();
						}
					};

					// Tools
					if (ImGui::TreeNode("Tools"))
					{
						// Stack Tool is your best friend!
						ImGui::Checkbox("Show stack tool", &cfg->ShowStackTool);
						ImGui::SameLine();
						MetricsHelpMarker("You can also call ImGui::ShowStackToolWindow() from your code.");

						ImGui::Checkbox("Show windows begin order", &cfg->ShowWindowsBeginOrder);
						ImGui::Checkbox("Show windows rectangles", &cfg->ShowWindowsRects);
						ImGui::SameLine();
						ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12);
						cfg->ShowWindowsRects |= ImGui::Combo("##show_windows_rect_type", &cfg->ShowWindowsRectsType, wrt_rects_names, WRT_Count, WRT_Count);
						if (cfg->ShowWindowsRects && g.NavWindow != NULL)
						{
							ImGui::BulletText("'%s':", g.NavWindow->Name);
							ImGui::Indent();
							for (int rect_n = 0; rect_n < WRT_Count; rect_n++)
							{
								ImRect r = Funcs::GetWindowRect(g.NavWindow, rect_n);
								ImGui::Text("(%6.1f,%6.1f) (%6.1f,%6.1f) Size (%6.1f,%6.1f) %s", r.Min.x, r.Min.y, r.Max.x, r.Max.y, r.GetWidth(), r.GetHeight(), wrt_rects_names[rect_n]);
							}
							ImGui::Unindent();
						}

						ImGui::Checkbox("Show tables rectangles", &cfg->ShowTablesRects);
						ImGui::SameLine();
						ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12);
						cfg->ShowTablesRects |= ImGui::Combo("##show_table_rects_type", &cfg->ShowTablesRectsType, trt_rects_names, TRT_Count, TRT_Count);
						if (cfg->ShowTablesRects && g.NavWindow != NULL)
						{
							for (int table_n = 0; table_n < g.Tables.GetMapSize(); table_n++)
							{
								ImGuiTable* table = g.Tables.TryGetMapData(table_n);
								if (table == NULL || table->LastFrameActive < g.FrameCount - 1 || (table->OuterWindow != g.NavWindow && table->InnerWindow != g.NavWindow))
									continue;

								ImGui::BulletText("Table 0x%08X (%d columns, in '%s')", table->ID, table->ColumnsCount, table->OuterWindow->Name);
								if (ImGui::IsItemHovered())
									ImGui::GetForegroundDrawList()->AddRect(table->OuterRect.Min - ImVec2(1, 1), table->OuterRect.Max + ImVec2(1, 1), IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
								ImGui::Indent();
								char buf[128];
								for (int rect_n = 0; rect_n < TRT_Count; rect_n++)
								{
									if (rect_n >= TRT_ColumnsRect)
									{
										if (rect_n != TRT_ColumnsRect && rect_n != TRT_ColumnsClipRect)
											continue;
										for (int column_n = 0; column_n < table->ColumnsCount; column_n++)
										{
											ImRect r = Funcs::GetTableRect(table, rect_n, column_n);
											ImFormatString(buf, IM_ARRAYSIZE(buf), "(%6.1f,%6.1f) (%6.1f,%6.1f) Size (%6.1f,%6.1f) Col %d %s", r.Min.x, r.Min.y, r.Max.x, r.Max.y, r.GetWidth(), r.GetHeight(), column_n, trt_rects_names[rect_n]);
											ImGui::Selectable(buf);
											if (ImGui::IsItemHovered())
												ImGui::GetForegroundDrawList()->AddRect(r.Min - ImVec2(1, 1), r.Max + ImVec2(1, 1), IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
										}
									}
									else
									{
										ImRect r = Funcs::GetTableRect(table, rect_n, -1);
										ImFormatString(buf, IM_ARRAYSIZE(buf), "(%6.1f,%6.1f) (%6.1f,%6.1f) Size (%6.1f,%6.1f) %s", r.Min.x, r.Min.y, r.Max.x, r.Max.y, r.GetWidth(), r.GetHeight(), trt_rects_names[rect_n]);
										ImGui::Selectable(buf);
										if (ImGui::IsItemHovered())
											ImGui::GetForegroundDrawList()->AddRect(r.Min - ImVec2(1, 1), r.Max + ImVec2(1, 1), IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
									}
								}
								ImGui::Unindent();
							}
						}

						// The Item Picker tool is super useful to visually select an item and break into the call-stack of where it was submitted.
						if (ImGui::Button("Item Picker.."))
							ImGui::DebugStartItemPicker();
						ImGui::SameLine();
						MetricsHelpMarker("Will call the IM_DEBUG_BREAK() macro to break in debugger.\nWarning: If you don't have a debugger attached, this will probably crash.");

						ImGui::TreePop();
					}

					// Windows
					if (ImGui::TreeNode("Windows", "Windows (%d)", g.Windows.Size))
					{
						//SetNextItemOpen(true, ImGuiCond_Once);
						ImGui::DebugNodeWindowsList(&g.Windows, "By display order");
						ImGui::DebugNodeWindowsList(&g.WindowsFocusOrder, "By focus order (root windows)");
						if (ImGui::TreeNode("By submission order (begin stack)"))
						{
							// Here we display windows in their submitted order/hierarchy, however note that the Begin stack doesn't constitute a Parent<>Child relationship!
							ImVector<ImGuiWindow*>& temp_buffer = g.WindowsTempSortBuffer;
							temp_buffer.resize(0);
							for (int i = 0; i < g.Windows.Size; i++)
								if (g.Windows[i]->LastFrameActive + 1 >= g.FrameCount)
									temp_buffer.push_back(g.Windows[i]);
							struct Func { static int IMGUI_CDECL WindowComparerByBeginOrder(const void* lhs, const void* rhs) { return ((int)(*(const ImGuiWindow* const*)lhs)->BeginOrderWithinContext - (*(const ImGuiWindow* const*)rhs)->BeginOrderWithinContext); } };
							ImQsort(temp_buffer.Data, (size_t)temp_buffer.Size, sizeof(ImGuiWindow*), Func::WindowComparerByBeginOrder);
							ImGui::DebugNodeWindowsListByBeginStackParent(temp_buffer.Data, temp_buffer.Size, NULL);
							ImGui::TreePop();
						}

						ImGui::TreePop();
					}

					// DrawLists
					int drawlist_count = 0;
					for (int viewport_i = 0; viewport_i < g.Viewports.Size; viewport_i++)
						drawlist_count += g.Viewports[viewport_i]->DrawDataBuilder.GetDrawListCount();
					if (ImGui::TreeNode("DrawLists", "DrawLists (%d)", drawlist_count))
					{
						ImGui::Checkbox("Show ImDrawCmd mesh when hovering", &cfg->ShowDrawCmdMesh);
						ImGui::Checkbox("Show ImDrawCmd bounding boxes when hovering", &cfg->ShowDrawCmdBoundingBoxes);
						for (int viewport_i = 0; viewport_i < g.Viewports.Size; viewport_i++)
						{
							ImGuiViewportP* viewport = g.Viewports[viewport_i];
							bool viewport_has_drawlist = false;
							for (int layer_i = 0; layer_i < IM_ARRAYSIZE(viewport->DrawDataBuilder.Layers); layer_i++)
								for (int draw_list_i = 0; draw_list_i < viewport->DrawDataBuilder.Layers[layer_i].Size; draw_list_i++)
								{
									if (!viewport_has_drawlist)
										ImGui::Text("Active DrawLists in Viewport #%d, ID: 0x%08X", viewport->Idx, viewport->ID);
									viewport_has_drawlist = true;
									ImGui::DebugNodeDrawList(NULL, viewport, viewport->DrawDataBuilder.Layers[layer_i][draw_list_i], "DrawList");
								}
						}
						ImGui::TreePop();
					}

					// Viewports
					if (ImGui::TreeNode("Viewports", "Viewports (%d)", g.Viewports.Size))
					{
						ImGui::Indent(g.FontSize + (g.Style.FramePadding.x * 2.0f));

						// -- Render Viewport Thumbnails -- //
						ImGuiContext& g = *GImGui;
						ImGuiWindow* window = g.CurrentWindow;

						// We don't display full monitor bounds (we could, but it often looks awkward), instead we display just enough to cover all of our viewports.
						float SCALE = 1.0f / 8.0f;
						ImRect bb_full(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
						for (int n = 0; n < g.Viewports.Size; n++)
							bb_full.Add(g.Viewports[n]->GetMainRect());
						ImVec2 p = window->DC.CursorPos;
						ImVec2 off = p - bb_full.Min * SCALE;
						for (int n = 0; n < g.Viewports.Size; n++)
						{
							ImGuiViewportP* viewport = g.Viewports[n];
							ImRect viewport_draw_bb(off + (viewport->Pos) * SCALE, off + (viewport->Pos + viewport->Size) * SCALE);
							ImGui::DebugRenderViewportThumbnail(window->DrawList, viewport, viewport_draw_bb);
						}
						ImGui::Dummy(bb_full.GetSize() * SCALE);
						// --------------------------------- //

						ImGui::Unindent(g.FontSize + (g.Style.FramePadding.x * 2.0f));

						bool open = ImGui::TreeNode("Monitors", "Monitors (%d)", g.PlatformIO.Monitors.Size);
						ImGui::SameLine();
						MetricsHelpMarker("Dear ImGui uses monitor data:\n- to query DPI settings on a per monitor basis\n- to position popup/tooltips so they don't straddle monitors.");
						if (open)
						{
							for (int i = 0; i < g.PlatformIO.Monitors.Size; i++)
							{
								const ImGuiPlatformMonitor& mon = g.PlatformIO.Monitors[i];
								ImGui::BulletText("Monitor #%d: DPI %.0f%%\n MainMin (%.0f,%.0f), MainMax (%.0f,%.0f), MainSize (%.0f,%.0f)\n WorkMin (%.0f,%.0f), WorkMax (%.0f,%.0f), WorkSize (%.0f,%.0f)",
									i, mon.DpiScale * 100.0f,
									mon.MainPos.x, mon.MainPos.y, mon.MainPos.x + mon.MainSize.x, mon.MainPos.y + mon.MainSize.y, mon.MainSize.x, mon.MainSize.y,
									mon.WorkPos.x, mon.WorkPos.y, mon.WorkPos.x + mon.WorkSize.x, mon.WorkPos.y + mon.WorkSize.y, mon.WorkSize.x, mon.WorkSize.y);
							}
							ImGui::TreePop();
						}

						if (ImGui::TreeNode("Inferred order (front-to-back)"))
						{
							static ImVector<ImGuiViewportP*> viewports;
							viewports.resize(g.Viewports.Size);
							memcpy(viewports.Data, g.Viewports.Data, g.Viewports.size_in_bytes());
							if (viewports.Size > 1)
							{
								ImQsort(viewports.Data, viewports.Size, sizeof(ImGuiViewport*), ViewportComparerByFrontMostStampCount);
							}
							for (int i = 0; i < viewports.Size; i++)
								ImGui::BulletText("Viewport #%d, ID: 0x%08X, FrontMostStampCount = %08d, Window: \"%s\"", viewports[i]->Idx, viewports[i]->ID, viewports[i]->LastFrontMostStampCount, viewports[i]->Window ? viewports[i]->Window->Name : "N/A");
							ImGui::TreePop();
						}

						for (int i = 0; i < g.Viewports.Size; i++)
							ImGui::DebugNodeViewport(g.Viewports[i]);
						ImGui::TreePop();
					}

					// Details for Popups
					if (ImGui::TreeNode("Popups", "Popups (%d)", g.OpenPopupStack.Size))
					{
						for (int i = 0; i < g.OpenPopupStack.Size; i++)
						{
							ImGuiWindow* window = g.OpenPopupStack[i].Window;
							ImGui::BulletText("PopupID: %08x, Window: '%s'%s%s", g.OpenPopupStack[i].PopupId, window ? window->Name : "NULL", window && (window->Flags & ImGuiWindowFlags_ChildWindow) ? " ChildWindow" : "", window && (window->Flags & ImGuiWindowFlags_ChildMenu) ? " ChildMenu" : "");
						}
						ImGui::TreePop();
					}

					// Details for TabBars
					if (ImGui::TreeNode("TabBars", "Tab Bars (%d)", g.TabBars.GetAliveCount()))
					{
						for (int n = 0; n < g.TabBars.GetMapSize(); n++)
							if (ImGuiTabBar* tab_bar = g.TabBars.TryGetMapData(n))
							{
								ImGui::PushID(tab_bar);
								ImGui::DebugNodeTabBar(tab_bar, "TabBar");
								ImGui::PopID();
							}
						ImGui::TreePop();
					}

					// Details for Tables
					if (ImGui::TreeNode("Tables", "Tables (%d)", g.Tables.GetAliveCount()))
					{
						for (int n = 0; n < g.Tables.GetMapSize(); n++)
							if (ImGuiTable* table = g.Tables.TryGetMapData(n))
								ImGui::DebugNodeTable(table);
						ImGui::TreePop();
					}

					// Details for Fonts
#ifndef IMGUI_DISABLE_DEMO_WINDOWS
					ImFontAtlas* atlas = g.IO.Fonts;
					if (ImGui::TreeNode("Fonts", "Fonts (%d)", atlas->Fonts.Size))
					{
						ImGui::ShowFontAtlas(atlas);
						ImGui::TreePop();
					}
#endif

					// Details for Docking
#ifdef IMGUI_HAS_DOCK
					if (ImGui::TreeNode("Docking"))
					{
						static bool root_nodes_only = true;
						ImGuiDockContext* dc = &g.DockContext;
						ImGui::Checkbox("List root nodes", &root_nodes_only);
						ImGui::Checkbox("Ctrl shows window dock info", &cfg->ShowDockingNodes);
						if (ImGui::SmallButton("Clear nodes")) { ImGui::DockContextClearNodes(&g, 0, true); }
						ImGui::SameLine();
						if (ImGui::SmallButton("Rebuild all")) { dc->WantFullRebuild = true; }
						for (int n = 0; n < dc->Nodes.Data.Size; n++)
							if (ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p)
								if (!root_nodes_only || node->IsRootNode())
									ImGui::DebugNodeDockNode(node, "Node");
						ImGui::TreePop();
					}
#endif // #ifdef IMGUI_HAS_DOCK

					// Settings
					if (ImGui::TreeNode("Settings"))
					{
						if (ImGui::SmallButton("Clear"))
							ImGui::ClearIniSettings();
						ImGui::SameLine();
						if (ImGui::SmallButton("Save to memory"))
							ImGui::SaveIniSettingsToMemory();
						ImGui::SameLine();
						if (ImGui::SmallButton("Save to disk"))
							ImGui::SaveIniSettingsToDisk(g.IO.IniFilename);
						ImGui::SameLine();
						if (g.IO.IniFilename)
							ImGui::Text("\"%s\"", g.IO.IniFilename);
						else
							ImGui::TextUnformatted("<NULL>");
						ImGui::Text("SettingsDirtyTimer %.2f", g.SettingsDirtyTimer);
						if (ImGui::TreeNode("SettingsHandlers", "Settings handlers: (%d)", g.SettingsHandlers.Size))
						{
							for (int n = 0; n < g.SettingsHandlers.Size; n++)
								ImGui::BulletText("%s", g.SettingsHandlers[n].TypeName);
							ImGui::TreePop();
						}
						if (ImGui::TreeNode("SettingsWindows", "Settings packed data: Windows: %d bytes", g.SettingsWindows.size()))
						{
							for (ImGuiWindowSettings* settings = g.SettingsWindows.begin(); settings != NULL; settings = g.SettingsWindows.next_chunk(settings))
								ImGui::DebugNodeWindowSettings(settings);
							ImGui::TreePop();
						}

						if (ImGui::TreeNode("SettingsTables", "Settings packed data: Tables: %d bytes", g.SettingsTables.size()))
						{
							for (ImGuiTableSettings* settings = g.SettingsTables.begin(); settings != NULL; settings = g.SettingsTables.next_chunk(settings))
								ImGui::DebugNodeTableSettings(settings);
							ImGui::TreePop();
						}

#ifdef IMGUI_HAS_DOCK
						if (ImGui::TreeNode("SettingsDocking", "Settings packed data: Docking"))
						{
							ImGuiDockContext* dc = &g.DockContext;
							ImGui::Text("In SettingsWindows:");
							for (ImGuiWindowSettings* settings = g.SettingsWindows.begin(); settings != NULL; settings = g.SettingsWindows.next_chunk(settings))
								if (settings->DockId != 0)
									ImGui::BulletText("Window '%s' -> DockId %08X", settings->GetName(), settings->DockId);
							//ImGui::Text("In SettingsNodes:");
							//for (int n = 0; n < dc->NodesSettings.Size; n++)
							//{
							//	ImGuiDockNodeSettings* settings = &dc->NodesSettings[n];
							//	const char* selected_tab_name = NULL;
							//	if (settings->SelectedTabId)
							//	{
							//		if (ImGuiWindow* window = ImGui::FindWindowByID(settings->SelectedTabId))
							//			selected_tab_name = window->Name;
							//		else if (ImGuiWindowSettings* window_settings = ImGui::FindWindowSettings(settings->SelectedTabId))
							//			selected_tab_name = window_settings->GetName();
							//	}
							//	ImGui::BulletText("Node %08X, Parent %08X, SelectedTab %08X ('%s')", settings->ID, settings->ParentNodeId, settings->SelectedTabId, selected_tab_name ? selected_tab_name : settings->SelectedTabId ? "N/A" : "");
							//}
							ImGui::TreePop();
						}
#endif // #ifdef IMGUI_HAS_DOCK

						if (ImGui::TreeNode("SettingsIniData", "Settings unpacked data (.ini): %d bytes", g.SettingsIniData.size()))
						{
							ImGui::InputTextMultiline("##Ini", (char*)(void*)g.SettingsIniData.c_str(), g.SettingsIniData.Buf.Size, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 20), ImGuiInputTextFlags_ReadOnly);
							ImGui::TreePop();
						}
						ImGui::TreePop();
					}

					// Misc Details
					if (ImGui::TreeNode("Internal state"))
					{
						const char* input_source_names[] = { "None", "Mouse", "Keyboard", "Gamepad", "Nav", "Clipboard" }; IM_ASSERT(IM_ARRAYSIZE(input_source_names) == ImGuiInputSource_COUNT);

						ImGui::Text("WINDOWING");
						ImGui::Indent();
						ImGui::Text("HoveredWindow: '%s'", g.HoveredWindow ? g.HoveredWindow->Name : "NULL");
						ImGui::Text("HoveredWindow->Root: '%s'", g.HoveredWindow ? g.HoveredWindow->RootWindowDockTree->Name : "NULL");
						ImGui::Text("HoveredWindowUnderMovingWindow: '%s'", g.HoveredWindowUnderMovingWindow ? g.HoveredWindowUnderMovingWindow->Name : "NULL");
						ImGui::Text("HoveredDockNode: 0x%08X", g.HoveredDockNode ? g.HoveredDockNode->ID : 0);
						ImGui::Text("MovingWindow: '%s'", g.MovingWindow ? g.MovingWindow->Name : "NULL");
						ImGui::Text("MouseViewport: 0x%08X (UserHovered 0x%08X, LastHovered 0x%08X)", g.MouseViewport->ID, g.IO.MouseHoveredViewport, g.MouseLastHoveredViewport ? g.MouseLastHoveredViewport->ID : 0);
						ImGui::Unindent();

						ImGui::Text("ITEMS");
						ImGui::Indent();
						ImGui::Text("ActiveId: 0x%08X/0x%08X (%.2f sec), AllowOverlap: %d, Source: %s", g.ActiveId, g.ActiveIdPreviousFrame, g.ActiveIdTimer, g.ActiveIdAllowOverlap, input_source_names[g.ActiveIdSource]);
						ImGui::Text("ActiveIdWindow: '%s'", g.ActiveIdWindow ? g.ActiveIdWindow->Name : "NULL");
						ImGui::Text("ActiveIdUsing: Wheel: %d, NavDirMask: %X, NavInputMask: %X, KeyInputMask: %llX", g.ActiveIdUsingMouseWheel, g.ActiveIdUsingNavDirMask, g.ActiveIdUsingNavInputMask, g.ActiveIdUsingKeyInputMask);
						ImGui::Text("HoveredId: 0x%08X (%.2f sec), AllowOverlap: %d", g.HoveredIdPreviousFrame, g.HoveredIdTimer, g.HoveredIdAllowOverlap); // Not displaying g.HoveredId as it is update mid-frame
						ImGui::Text("DragDrop: %d, SourceId = 0x%08X, Payload \"%s\" (%d bytes)", g.DragDropActive, g.DragDropPayload.SourceId, g.DragDropPayload.DataType, g.DragDropPayload.DataSize);
						ImGui::Unindent();

						ImGui::Text("NAV,FOCUS");
						ImGui::Indent();
						ImGui::Text("NavWindow: '%s'", g.NavWindow ? g.NavWindow->Name : "NULL");
						ImGui::Text("NavId: 0x%08X, NavLayer: %d", g.NavId, g.NavLayer);
						ImGui::Text("NavInputSource: %s", input_source_names[g.NavInputSource]);
						ImGui::Text("NavActive: %d, NavVisible: %d", g.IO.NavActive, g.IO.NavVisible);
						ImGui::Text("NavActivateId/DownId/PressedId/InputId: %08X/%08X/%08X/%08X", g.NavActivateId, g.NavActivateDownId, g.NavActivatePressedId, g.NavActivateInputId);
						ImGui::Text("NavActivateFlags: %04X", g.NavActivateFlags);
						ImGui::Text("NavDisableHighlight: %d, NavDisableMouseHover: %d", g.NavDisableHighlight, g.NavDisableMouseHover);
						ImGui::Text("NavFocusScopeId = 0x%08X", g.NavFocusScopeId);
						ImGui::Text("NavWindowingTarget: '%s'", g.NavWindowingTarget ? g.NavWindowingTarget->Name : "NULL");
						ImGui::Unindent();

						ImGui::TreePop();
					}

					// Overlay: Display windows Rectangles and Begin Order
					if (cfg->ShowWindowsRects || cfg->ShowWindowsBeginOrder)
					{
						for (int n = 0; n < g.Windows.Size; n++)
						{
							ImGuiWindow* window = g.Windows[n];
							if (!window->WasActive)
								continue;
							ImDrawList* draw_list = ImGui::GetForegroundDrawList(window);
							if (cfg->ShowWindowsRects)
							{
								ImRect r = Funcs::GetWindowRect(window, cfg->ShowWindowsRectsType);
								draw_list->AddRect(r.Min, r.Max, IM_COL32(255, 0, 128, 255));
							}
							if (cfg->ShowWindowsBeginOrder && !(window->Flags & ImGuiWindowFlags_ChildWindow))
							{
								char buf[32];
								ImFormatString(buf, IM_ARRAYSIZE(buf), "%d", window->BeginOrderWithinContext);
								float font_size = ImGui::GetFontSize();
								draw_list->AddRectFilled(window->Pos, window->Pos + ImVec2(font_size, font_size), IM_COL32(200, 100, 100, 255));
								draw_list->AddText(window->Pos, IM_COL32(255, 255, 255, 255), buf);
							}
						}
					}

					// Overlay: Display Tables Rectangles
					if (cfg->ShowTablesRects)
					{
						for (int table_n = 0; table_n < g.Tables.GetMapSize(); table_n++)
						{
							ImGuiTable* table = g.Tables.TryGetMapData(table_n);
							if (table == NULL || table->LastFrameActive < g.FrameCount - 1)
								continue;
							ImDrawList* draw_list = ImGui::GetForegroundDrawList(table->OuterWindow);
							if (cfg->ShowTablesRectsType >= TRT_ColumnsRect)
							{
								for (int column_n = 0; column_n < table->ColumnsCount; column_n++)
								{
									ImRect r = Funcs::GetTableRect(table, cfg->ShowTablesRectsType, column_n);
									ImU32 col = (table->HoveredColumnBody == column_n) ? IM_COL32(255, 255, 128, 255) : IM_COL32(255, 0, 128, 255);
									float thickness = (table->HoveredColumnBody == column_n) ? 3.0f : 1.0f;
									draw_list->AddRect(r.Min, r.Max, col, 0.0f, 0, thickness);
								}
							}
							else
							{
								ImRect r = Funcs::GetTableRect(table, cfg->ShowTablesRectsType, -1);
								draw_list->AddRect(r.Min, r.Max, IM_COL32(255, 0, 128, 255));
							}
						}
					}

#ifdef IMGUI_HAS_DOCK
					// Overlay: Display Docking info
					if (cfg->ShowDockingNodes && g.IO.KeyCtrl && g.HoveredDockNode)
					{
						char buf[64] = "";
						char* p = buf;
						ImGuiDockNode* node = g.HoveredDockNode;
						ImDrawList* overlay_draw_list = node->HostWindow ? ImGui::GetForegroundDrawList(node->HostWindow) : ImGui::GetForegroundDrawList(ImGui::GetMainViewport());
						p += ImFormatString(p, buf + IM_ARRAYSIZE(buf) - p, "DockId: %X%s\n", node->ID, node->IsCentralNode() ? " *CentralNode*" : "");
						p += ImFormatString(p, buf + IM_ARRAYSIZE(buf) - p, "WindowClass: %08X\n", node->WindowClass.ClassId);
						p += ImFormatString(p, buf + IM_ARRAYSIZE(buf) - p, "Size: (%.0f, %.0f)\n", node->Size.x, node->Size.y);
						p += ImFormatString(p, buf + IM_ARRAYSIZE(buf) - p, "SizeRef: (%.0f, %.0f)\n", node->SizeRef.x, node->SizeRef.y);
						int depth = ImGui::DockNodeGetDepth(node);
						overlay_draw_list->AddRect(node->Pos + ImVec2(3, 3) * (float)depth, node->Pos + node->Size - ImVec2(3, 3) * (float)depth, IM_COL32(200, 100, 100, 255));
						ImVec2 pos = node->Pos + ImVec2(3, 3) * (float)depth;
						overlay_draw_list->AddRectFilled(pos - ImVec2(1, 1), pos + ImGui::CalcTextSize(buf) + ImVec2(1, 1), IM_COL32(200, 100, 100, 255));
						overlay_draw_list->AddText(NULL, 0.0f, pos, IM_COL32(255, 255, 255, 255), buf);
					}
#endif // #ifdef IMGUI_HAS_DOCK

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