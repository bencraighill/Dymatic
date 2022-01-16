#include "PerformanceAnalyser.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h" 
#include "imgui/imgui_internal.h"

#include "Dymatic/Math/StringUtils.h"
#include "Dymatic/Math/Math.h"

#include "psapi.h"

namespace Dymatic {

	PerformanceAnalyser::PerformanceAnalyser()
	{
	}

	void PerformanceAnalyser::OnImGuiRender(Timestep ts)
	{
		if (m_PerformanceAnalyserVisible)
		{
			ImGui::Begin("Performance Analyser", &m_PerformanceAnalyserVisible);
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
//			{
//				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
//				bool open = (ImGui::TreeNodeEx("ImGui Debug", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding));
//				ImGui::PopStyleVar();
//				if (open)
//				{
//					//ImGui::BeginChild("##Metrics");
//
//					// Debugging enums
//					enum { WRT_OuterRect, WRT_OuterRectClipped, WRT_InnerRect, WRT_InnerClipRect, WRT_WorkRect, WRT_Content, WRT_ContentRegionRect, WRT_Count }; // Windows Rect Type
//					const char* wrt_rects_names[WRT_Count] = { "OuterRect", "OuterRectClipped", "InnerRect", "InnerClipRect", "WorkRect", "Content", "ContentRegionRect" };
//					enum { TRT_OuterRect, TRT_WorkRect, TRT_HostClipRect, TRT_InnerClipRect, TRT_BackgroundClipRect, TRT_ColumnsRect, TRT_ColumnsClipRect, TRT_ColumnsContentHeadersUsed, TRT_ColumnsContentHeadersIdeal, TRT_ColumnsContentRowsFrozen, TRT_ColumnsContentRowsUnfrozen, TRT_Count }; // Tables Rect Type
//					const char* trt_rects_names[TRT_Count] = { "OuterRect", "WorkRect", "HostClipRect", "InnerClipRect", "BackgroundClipRect", "ColumnsRect", "ColumnsClipRect", "ColumnsContentHeadersUsed", "ColumnsContentHeadersIdeal", "ColumnsContentRowsFrozen", "ColumnsContentRowsUnfrozen" };
//
//					// State
//					static bool show_windows_rects = false;
//					static int  show_windows_rect_type = WRT_WorkRect;
//					static bool show_windows_begin_order = false;
//					static bool show_tables_rects = false;
//					static int  show_tables_rect_type = TRT_WorkRect;
//					static bool show_drawcmd_mesh = true;
//					static bool show_drawcmd_aabb = true;
//					static bool show_docking_nodes = false;
//
//					// Basic info
//					ImGuiContext& g = *GImGui;
//					ImGuiIO& io = ImGui::GetIO();
//					ImGui::Text("Dear ImGui %s", ImGui::GetVersion());
//					ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
//					ImGui::Text("%d vertices, %d indices (%d triangles)", io.MetricsRenderVertices, io.MetricsRenderIndices, io.MetricsRenderIndices / 3);
//					ImGui::Text("%d active windows (%d visible)", io.MetricsActiveWindows, io.MetricsRenderWindows);
//					ImGui::Text("%d active allocations", io.MetricsActiveAllocations);
//					ImGui::Separator();
//
//					// Helper functions to display common structures:
//					// - NodeDrawList()
//					// - NodeColumns()
//					// - NodeWindow()
//					// - NodeWindows()
//					// - NodeViewport()
//					// - NodeDockNode()
//					// - NodeTabBar()
//					// - NodeStorage()
//					struct Funcs
//					{
//						static ImRect GetWindowRect(ImGuiWindow* window, int rect_type)
//						{
//							if (rect_type == WRT_OuterRect) { return window->Rect(); }
//							else if (rect_type == WRT_OuterRectClipped) { return window->OuterRectClipped; }
//							else if (rect_type == WRT_InnerRect) { return window->InnerRect; }
//							else if (rect_type == WRT_InnerClipRect) { return window->InnerClipRect; }
//							else if (rect_type == WRT_WorkRect) { return window->WorkRect; }
//							else if (rect_type == WRT_Content) { ImVec2 min = window->InnerRect.Min - window->Scroll + window->WindowPadding; return ImRect(min, min + window->ContentSize); }
//							else if (rect_type == WRT_ContentRegionRect) { return window->ContentRegionRect; }
//							IM_ASSERT(0);
//							return ImRect();
//						}
//
//						static void NodeDrawCmdShowMeshAndBoundingBox(ImDrawList* fg_draw_list, const ImDrawList* draw_list, const ImDrawCmd* draw_cmd, int elem_offset, bool show_mesh, bool show_aabb)
//						{
//							IM_ASSERT(show_mesh || show_aabb);
//							ImDrawIdx* idx_buffer = (draw_list->IdxBuffer.Size > 0) ? draw_list->IdxBuffer.Data : NULL;
//
//							// Draw wire-frame version of all triangles
//							ImRect clip_rect = draw_cmd->ClipRect;
//							ImRect vtxs_rect(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
//							ImDrawListFlags backup_flags = fg_draw_list->Flags;
//							fg_draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines; // Disable AA on triangle outlines is more readable for very large and thin triangles.
//							for (unsigned int base_idx = elem_offset; base_idx < (elem_offset + draw_cmd->ElemCount); base_idx += 3)
//							{
//								ImVec2 triangle[3];
//								for (int n = 0; n < 3; n++)
//								{
//									ImVec2 p = draw_list->VtxBuffer[idx_buffer ? idx_buffer[base_idx + n] : (base_idx + n)].pos;
//									triangle[n] = p;
//									vtxs_rect.Add(p);
//								}
//								if (show_mesh)
//									fg_draw_list->AddPolyline(triangle, 3, IM_COL32(255, 255, 0, 255), true, 1.0f); // In yellow: mesh triangles
//							}
//							// Draw bounding boxes
//							if (show_aabb)
//							{
//								fg_draw_list->AddRect(ImFloor(clip_rect.Min), ImFloor(clip_rect.Max), IM_COL32(255, 0, 255, 255)); // In pink: clipping rectangle submitted to GPU
//								fg_draw_list->AddRect(ImFloor(vtxs_rect.Min), ImFloor(vtxs_rect.Max), IM_COL32(0, 255, 255, 255)); // In cyan: bounding box of triangles
//							}
//							fg_draw_list->Flags = backup_flags;
//						}
//
//						// Note that both 'window' and 'viewport' may be NULL here. Viewport is generally null of destroyed popups which previously owned a viewport.
//						static void NodeDrawList(ImGuiWindow* window, ImGuiViewportP* viewport, ImDrawList* draw_list, const char* label)
//						{
//							bool node_open = ImGui::TreeNode(draw_list, "%s: '%s' %d vtx, %d indices, %d cmds", label, draw_list->_OwnerName ? draw_list->_OwnerName : "", draw_list->VtxBuffer.Size, draw_list->IdxBuffer.Size, draw_list->CmdBuffer.Size);
//							if (draw_list == ImGui::GetWindowDrawList())
//							{
//								ImGui::SameLine();
//								ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "CURRENTLY APPENDING"); // Can't display stats for active draw list! (we don't have the data double-buffered)
//								if (node_open) ImGui::TreePop();
//								return;
//							}
//
//							ImDrawList* fg_draw_list = viewport ? ImGui::GetForegroundDrawList(viewport) : NULL; // Render additional visuals into the top-most draw list
//							if (window && fg_draw_list && ImGui::IsItemHovered())
//								fg_draw_list->AddRect(window->Pos, window->Pos + window->Size, IM_COL32(255, 255, 0, 255));
//							if (!node_open)
//								return;
//
//							if (window && !window->WasActive)
//								ImGui::TextDisabled("Warning: owning Window is inactive. This DrawList is not being rendered!");
//
//							unsigned int elem_offset = 0;
//							for (const ImDrawCmd* pcmd = draw_list->CmdBuffer.begin(); pcmd < draw_list->CmdBuffer.end(); elem_offset += pcmd->ElemCount, pcmd++)
//							{
//								if (pcmd->UserCallback == NULL && pcmd->ElemCount == 0)
//									continue;
//								if (pcmd->UserCallback)
//								{
//									ImGui::BulletText("Callback %p, user_data %p", pcmd->UserCallback, pcmd->UserCallbackData);
//									continue;
//								}
//
//								ImDrawIdx* idx_buffer = (draw_list->IdxBuffer.Size > 0) ? draw_list->IdxBuffer.Data : NULL;
//								char buf[300];
//								ImFormatString(buf, IM_ARRAYSIZE(buf), "DrawCmd:%5d triangles, Tex 0x%p, ClipRect (%4.0f,%4.0f)-(%4.0f,%4.0f)",
//									pcmd->ElemCount / 3, (void*)(intptr_t)pcmd->TextureId,
//									pcmd->ClipRect.x, pcmd->ClipRect.y, pcmd->ClipRect.z, pcmd->ClipRect.w);
//								bool pcmd_node_open = ImGui::TreeNode((void*)(pcmd - draw_list->CmdBuffer.begin()), "%s", buf);
//								if (ImGui::IsItemHovered() && (show_drawcmd_mesh || show_drawcmd_aabb) && fg_draw_list)
//									NodeDrawCmdShowMeshAndBoundingBox(fg_draw_list, draw_list, pcmd, elem_offset, show_drawcmd_mesh, show_drawcmd_aabb);
//								if (!pcmd_node_open)
//									continue;
//
//								// Calculate approximate coverage area (touched pixel count)
//								// This will be in pixels squared as long there's no post-scaling happening to the renderer output.
//								float total_area = 0.0f;
//								for (unsigned int base_idx = elem_offset; base_idx < (elem_offset + pcmd->ElemCount); base_idx += 3)
//								{
//									ImVec2 triangle[3];
//									for (int n = 0; n < 3; n++)
//										triangle[n] = draw_list->VtxBuffer[idx_buffer ? idx_buffer[base_idx + n] : (base_idx + n)].pos;
//									total_area += ImTriangleArea(triangle[0], triangle[1], triangle[2]);
//								}
//
//								// Display vertex information summary. Hover to get all triangles drawn in wire-frame
//								ImFormatString(buf, IM_ARRAYSIZE(buf), "Mesh: ElemCount: %d, VtxOffset: +%d, IdxOffset: +%d, Area: ~%0.f px", pcmd->ElemCount, pcmd->VtxOffset, pcmd->IdxOffset, total_area);
//								ImGui::Selectable(buf);
//								if (ImGui::IsItemHovered() && fg_draw_list)
//									NodeDrawCmdShowMeshAndBoundingBox(fg_draw_list, draw_list, pcmd, elem_offset, true, false);
//
//								// Display individual triangles/vertices. Hover on to get the corresponding triangle highlighted.
//								ImGuiListClipper clipper(pcmd->ElemCount / 3); // Manually coarse clip our print out of individual vertices to save CPU, only items that may be visible.
//								while (clipper.Step())
//									for (int prim = clipper.DisplayStart, idx_i = elem_offset + clipper.DisplayStart * 3; prim < clipper.DisplayEnd; prim++)
//									{
//										char* buf_p = buf, * buf_end = buf + IM_ARRAYSIZE(buf);
//										ImVec2 triangle[3];
//										for (int n = 0; n < 3; n++, idx_i++)
//										{
//											ImDrawVert& v = draw_list->VtxBuffer[idx_buffer ? idx_buffer[idx_i] : idx_i];
//											triangle[n] = v.pos;
//											buf_p += ImFormatString(buf_p, buf_end - buf_p, "%s %04d: pos (%8.2f,%8.2f), uv (%.6f,%.6f), col %08X\n",
//												(n == 0) ? "Vert:" : "     ", idx_i, v.pos.x, v.pos.y, v.uv.x, v.uv.y, v.col);
//										}
//
//										ImGui::Selectable(buf, false);
//										if (fg_draw_list && ImGui::IsItemHovered())
//										{
//											ImDrawListFlags backup_flags = fg_draw_list->Flags;
//											fg_draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines; // Disable AA on triangle outlines is more readable for very large and thin triangles.
//											fg_draw_list->AddPolyline(triangle, 3, IM_COL32(255, 255, 0, 255), true, 1.0f);
//											fg_draw_list->Flags = backup_flags;
//										}
//									}
//								ImGui::TreePop();
//							}
//							ImGui::TreePop();
//						}
//
//						static void NodeColumns(const ImGuiColumns* columns)
//						{
//							if (!ImGui::TreeNode((void*)(uintptr_t)columns->ID, "Columns Id: 0x%08X, Count: %d, Flags: 0x%04X", columns->ID, columns->Count, columns->Flags))
//								return;
//							ImGui::BulletText("Width: %.1f (MinX: %.1f, MaxX: %.1f)", columns->OffMaxX - columns->OffMinX, columns->OffMinX, columns->OffMaxX);
//							for (int column_n = 0; column_n < columns->Columns.Size; column_n++)
//								ImGui::BulletText("Column %02d: OffsetNorm %.3f (= %.1f px)", column_n, columns->Columns[column_n].OffsetNorm, ImGui::GetColumnOffsetFromNorm(columns, columns->Columns[column_n].OffsetNorm));
//							ImGui::TreePop();
//						}
//
//						static void NodeWindows(ImVector<ImGuiWindow*>& windows, const char* label)
//						{
//							if (!ImGui::TreeNode(label, "%s (%d)", label, windows.Size))
//								return;
//							ImGui::Text("(In front-to-back order:)");
//							for (int i = windows.Size - 1; i >= 0; i--) // Iterate front to back
//							{
//								ImGui::PushID(windows[i]);
//								Funcs::NodeWindow(windows[i], "Window");
//								ImGui::PopID();
//							}
//							ImGui::TreePop();
//						}
//
//						static void NodeWindow(ImGuiWindow* window, const char* label)
//						{
//							if (window == NULL)
//							{
//								ImGui::BulletText("%s: NULL", label);
//								return;
//							}
//
//							ImGuiContext& g = *GImGui;
//							const bool is_active = window->WasActive;
//							ImGuiTreeNodeFlags tree_node_flags = (window == g.NavWindow) ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None;
//							if (!is_active) { ImGui::PushStyleColor(ImGuiCol_Text, (ImGuiCol_TextDisabled)); }
//							const bool open = ImGui::TreeNodeEx(label, tree_node_flags, "%s '%s'%s", label, window->Name, is_active ? "" : " *Inactive*");
//							if (!is_active) { ImGui::PopStyleColor(); }
//							if (ImGui::IsItemHovered() && is_active)
//								ImGui::GetForegroundDrawList(window)->AddRect(window->Pos, window->Pos + window->Size, IM_COL32(255, 255, 0, 255));
//							if (!open)
//								return;
//
//							if (window->MemoryCompacted)
//								ImGui::TextDisabled("Note: some memory buffers have been compacted/freed.");
//
//							ImGuiWindowFlags flags = window->Flags;
//							NodeDrawList(window, window->Viewport, window->DrawList, "DrawList");
//							ImGui::BulletText("Pos: (%.1f,%.1f), Size: (%.1f,%.1f), ContentSize (%.1f,%.1f)", window->Pos.x, window->Pos.y, window->Size.x, window->Size.y, window->ContentSize.x, window->ContentSize.y);
//							ImGui::BulletText("Flags: 0x%08X (%s%s%s%s%s%s%s%s%s..)", flags,
//								(flags & ImGuiWindowFlags_ChildWindow) ? "Child " : "", (flags & ImGuiWindowFlags_Tooltip) ? "Tooltip " : "", (flags & ImGuiWindowFlags_Popup) ? "Popup " : "",
//								(flags & ImGuiWindowFlags_Modal) ? "Modal " : "", (flags & ImGuiWindowFlags_ChildMenu) ? "ChildMenu " : "", (flags & ImGuiWindowFlags_NoSavedSettings) ? "NoSavedSettings " : "",
//								(flags & ImGuiWindowFlags_NoMouseInputs) ? "NoMouseInputs" : "", (flags & ImGuiWindowFlags_NoNavInputs) ? "NoNavInputs" : "", (flags & ImGuiWindowFlags_AlwaysAutoResize) ? "AlwaysAutoResize" : "");
//							ImGui::BulletText("WindowClassId: 0x%08X", window->WindowClass.ClassId);
//							ImGui::BulletText("Scroll: (%.2f/%.2f,%.2f/%.2f) Scrollbar:%s%s", window->Scroll.x, window->ScrollMax.x, window->Scroll.y, window->ScrollMax.y, window->ScrollbarX ? "X" : "", window->ScrollbarY ? "Y" : "");
//							ImGui::BulletText("Active: %d/%d, WriteAccessed: %d, BeginOrderWithinContext: %d", window->Active, window->WasActive, window->WriteAccessed, (window->Active || window->WasActive) ? window->BeginOrderWithinContext : -1);
//							ImGui::BulletText("Appearing: %d, Hidden: %d (CanSkip %d Cannot %d), SkipItems: %d", window->Appearing, window->Hidden, window->HiddenFramesCanSkipItems, window->HiddenFramesCannotSkipItems, window->SkipItems);
//							ImGui::BulletText("NavLastIds: 0x%08X,0x%08X, NavLayerActiveMask: %X", window->NavLastIds[0], window->NavLastIds[1], window->DC.NavLayerActiveMask);
//							ImGui::BulletText("NavLastChildNavWindow: %s", window->NavLastChildNavWindow ? window->NavLastChildNavWindow->Name : "NULL");
//							if (!window->NavRectRel[0].IsInverted())
//								ImGui::BulletText("NavRectRel[0]: (%.1f,%.1f)(%.1f,%.1f)", window->NavRectRel[0].Min.x, window->NavRectRel[0].Min.y, window->NavRectRel[0].Max.x, window->NavRectRel[0].Max.y);
//							else
//								ImGui::BulletText("NavRectRel[0]: <None>");
//							ImGui::BulletText("Viewport: %d%s, ViewportId: 0x%08X, ViewportPos: (%.1f,%.1f)", window->Viewport ? window->Viewport->Idx : -1, window->ViewportOwned ? " (Owned)" : "", window->ViewportId, window->ViewportPos.x, window->ViewportPos.y);
//							ImGui::BulletText("ViewportMonitor: %d", window->Viewport ? window->Viewport->PlatformMonitor : -1);
//							ImGui::BulletText("DockId: 0x%04X, DockOrder: %d, Act: %d, Vis: %d", window->DockId, window->DockOrder, window->DockIsActive, window->DockTabIsVisible);
//							if (window->DockNode || window->DockNodeAsHost)
//								NodeDockNode(window->DockNodeAsHost ? window->DockNodeAsHost : window->DockNode, window->DockNodeAsHost ? "DockNodeAsHost" : "DockNode");
//							if (window->RootWindow != window) NodeWindow(window->RootWindow, "RootWindow");
//							if (window->RootWindowDockStop != window->RootWindow) NodeWindow(window->RootWindowDockStop, "RootWindowDockStop");
//							if (window->ParentWindow != NULL) NodeWindow(window->ParentWindow, "ParentWindow");
//							if (window->DC.ChildWindows.Size > 0) NodeWindows(window->DC.ChildWindows, "ChildWindows");
//							if (window->ColumnsStorage.Size > 0 && ImGui::TreeNode("Columns", "Columns sets (%d)", window->ColumnsStorage.Size))
//							{
//								for (int n = 0; n < window->ColumnsStorage.Size; n++)
//									NodeColumns(&window->ColumnsStorage[n]);
//								ImGui::TreePop();
//							}
//							NodeStorage(&window->StateStorage, "Storage");
//							ImGui::TreePop();
//						}
//
//						static void NodeWindowSettings(ImGuiWindowSettings* settings)
//						{
//							ImGui::Text("0x%08X \"%s\" Pos (%d,%d) Size (%d,%d) Collapsed=%d",
//								settings->ID, settings->GetName(), settings->Pos.x, settings->Pos.y, settings->Size.x, settings->Size.y, settings->Collapsed);
//						}
//
//						static void NodeViewport(ImGuiViewportP* viewport)
//						{
//							ImGui::SetNextItemOpen(true, ImGuiCond_Once);
//							if (ImGui::TreeNode((void*)(intptr_t)viewport->ID, "Viewport #%d, ID: 0x%08X, Parent: 0x%08X, Window: \"%s\"", viewport->Idx, viewport->ID, viewport->ParentViewportId, viewport->Window ? viewport->Window->Name : "N/A"))
//							{
//								ImGuiWindowFlags flags = viewport->Flags;
//								ImGui::BulletText("Main Pos: (%.0f,%.0f), Size: (%.0f,%.0f)\nWorkArea Offset Left: %.0f Top: %.0f, Right: %.0f, Bottom: %.0f\nMonitor: %d, DpiScale: %.0f%%",
//									viewport->Pos.x, viewport->Pos.y, viewport->Size.x, viewport->Size.y,
//									viewport->WorkOffsetMin.x, viewport->WorkOffsetMin.y, viewport->WorkOffsetMax.x, viewport->WorkOffsetMax.y,
//									viewport->PlatformMonitor, viewport->DpiScale * 100.0f);
//								if (viewport->Idx > 0) { ImGui::SameLine(); if (ImGui::SmallButton("Reset Pos")) { viewport->Pos = ImVec2(200, 200); if (viewport->Window) viewport->Window->Pos = ImVec2(200, 200); } }
//								ImGui::BulletText("Flags: 0x%04X =%s%s%s%s%s%s%s", viewport->Flags,
//									(flags & ImGuiViewportFlags_CanHostOtherWindows) ? " CanHostOtherWindows" : "", (flags & ImGuiViewportFlags_NoDecoration) ? " NoDecoration" : "",
//									(flags & ImGuiViewportFlags_NoFocusOnAppearing) ? " NoFocusOnAppearing" : "", (flags & ImGuiViewportFlags_NoInputs) ? " NoInputs" : "",
//									(flags & ImGuiViewportFlags_NoRendererClear) ? " NoRendererClear" : "", (flags & ImGuiViewportFlags_Minimized) ? " Minimized" : "",
//									(flags & ImGuiViewportFlags_NoAutoMerge) ? " NoAutoMerge" : "");
//								for (int layer_i = 0; layer_i < IM_ARRAYSIZE(viewport->DrawDataBuilder.Layers); layer_i++)
//									for (int draw_list_i = 0; draw_list_i < viewport->DrawDataBuilder.Layers[layer_i].Size; draw_list_i++)
//										Funcs::NodeDrawList(NULL, viewport, viewport->DrawDataBuilder.Layers[layer_i][draw_list_i], "DrawList");
//								ImGui::TreePop();
//							}
//						}
//
//						static void NodeDockNode(ImGuiDockNode* node, const char* label)
//						{
//							ImGuiContext& g = *GImGui;
//							const bool is_alive = (g.FrameCount - node->LastFrameAlive < 2);    // Submitted with ImGuiDockNodeFlags_KeepAliveOnly
//							const bool is_active = (g.FrameCount - node->LastFrameActive < 2);  // Submitted
//							if (!is_alive) { ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled)); }
//							bool open;
//							if (node->Windows.Size > 0)
//								open = ImGui::TreeNode((void*)(intptr_t)node->ID, "%s 0x%04X%s: %d windows (vis: '%s')", label, node->ID, node->IsVisible ? "" : " (hidden)", node->Windows.Size, node->VisibleWindow ? node->VisibleWindow->Name : "NULL");
//							else
//								open = ImGui::TreeNode((void*)(intptr_t)node->ID, "%s 0x%04X%s: %s split (vis: '%s')", label, node->ID, node->IsVisible ? "" : " (hidden)", (node->SplitAxis == ImGuiAxis_X) ? "horizontal" : (node->SplitAxis == ImGuiAxis_Y) ? "vertical" : "n/a", node->VisibleWindow ? node->VisibleWindow->Name : "NULL");
//							if (!is_alive) { ImGui::PopStyleColor(); }
//							if (is_active && ImGui::IsItemHovered())
//								ImGui::GetForegroundDrawList(node->HostWindow ? node->HostWindow : node->VisibleWindow)->AddRect(node->Pos, node->Pos + node->Size, IM_COL32(255, 255, 0, 255));
//							if (open)
//							{
//								IM_ASSERT(node->ChildNodes[0] == NULL || node->ChildNodes[0]->ParentNode == node);
//								IM_ASSERT(node->ChildNodes[1] == NULL || node->ChildNodes[1]->ParentNode == node);
//								ImGui::BulletText("Pos (%.0f,%.0f), Size (%.0f, %.0f) Ref (%.0f, %.0f)",
//									node->Pos.x, node->Pos.y, node->Size.x, node->Size.y, node->SizeRef.x, node->SizeRef.y);
//								NodeWindow(node->HostWindow, "HostWindow");
//								NodeWindow(node->VisibleWindow, "VisibleWindow");
//								ImGui::BulletText("SelectedTabID: 0x%08X, LastFocusedNodeID: 0x%08X", node->SelectedTabId, node->LastFocusedNodeId);
//								ImGui::BulletText("Misc:%s%s%s%s%s",
//									node->IsDockSpace() ? " IsDockSpace" : "",
//									node->IsCentralNode() ? " IsCentralNode" : "",
//									is_alive ? " IsAlive" : "", is_active ? " IsActive" : "",
//									node->WantLockSizeOnce ? " WantLockSizeOnce" : "");
//								if (ImGui::TreeNode("flags", "LocalFlags: 0x%04X SharedFlags: 0x%04X", node->LocalFlags, node->SharedFlags))
//								{
//									ImGui::CheckboxFlags("LocalFlags: NoDocking", (ImU32*)&node->LocalFlags, ImGuiDockNodeFlags_NoDocking);
//									ImGui::CheckboxFlags("LocalFlags: NoSplit", (ImU32*)&node->LocalFlags, ImGuiDockNodeFlags_NoSplit);
//									ImGui::CheckboxFlags("LocalFlags: NoResize", (ImU32*)&node->LocalFlags, ImGuiDockNodeFlags_NoResize);
//									ImGui::CheckboxFlags("LocalFlags: NoResizeX", (ImU32*)&node->LocalFlags, ImGuiDockNodeFlags_NoResizeX);
//									ImGui::CheckboxFlags("LocalFlags: NoResizeY", (ImU32*)&node->LocalFlags, ImGuiDockNodeFlags_NoResizeY);
//									ImGui::CheckboxFlags("LocalFlags: NoTabBar", (ImU32*)&node->LocalFlags, ImGuiDockNodeFlags_NoTabBar);
//									ImGui::CheckboxFlags("LocalFlags: HiddenTabBar", (ImU32*)&node->LocalFlags, ImGuiDockNodeFlags_HiddenTabBar);
//									ImGui::CheckboxFlags("LocalFlags: NoWindowMenuButton", (ImU32*)&node->LocalFlags, ImGuiDockNodeFlags_NoWindowMenuButton);
//									ImGui::CheckboxFlags("LocalFlags: NoCloseButton", (ImU32*)&node->LocalFlags, ImGuiDockNodeFlags_NoCloseButton);
//									ImGui::TreePop();
//								}
//								if (node->ParentNode)
//									NodeDockNode(node->ParentNode, "ParentNode");
//								if (node->ChildNodes[0])
//									NodeDockNode(node->ChildNodes[0], "Child[0]");
//								if (node->ChildNodes[1])
//									NodeDockNode(node->ChildNodes[1], "Child[1]");
//								if (node->TabBar)
//									NodeTabBar(node->TabBar);
//								ImGui::TreePop();
//							}
//						}
//
//						static void NodeTabBar(ImGuiTabBar* tab_bar)
//						{
//							// Standalone tab bars (not associated to docking/windows functionality) currently hold no discernible strings.
//							char buf[256];
//							char* p = buf;
//							const char* buf_end = buf + IM_ARRAYSIZE(buf);
//							const bool is_active = (tab_bar->PrevFrameVisible >= ImGui::GetFrameCount() - 2);
//							p += ImFormatString(p, buf_end - p, "Tab Bar 0x%08X (%d tabs)%s", tab_bar->ID, tab_bar->Tabs.Size, is_active ? "" : " *Inactive*");
//							if (tab_bar->Flags & ImGuiTabBarFlags_DockNode)
//							{
//								p += ImFormatString(p, buf_end - p, "  { ");
//								for (int tab_n = 0; tab_n < ImMin(tab_bar->Tabs.Size, 3); tab_n++)
//									p += ImFormatString(p, buf_end - p, "%s'%s'", tab_n > 0 ? ", " : "", tab_bar->Tabs[tab_n].Window->Name);
//								p += ImFormatString(p, buf_end - p, (tab_bar->Tabs.Size > 3) ? " ... }" : " } ");
//							}
//							if (!is_active) { ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled)); }
//							bool open = ImGui::TreeNode(tab_bar, "%s", buf);
//							if (!is_active) { ImGui::PopStyleColor(); }
//							if (open)
//							{
//								for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
//								{
//									const ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
//									ImGui::PushID(tab);
//									if (ImGui::SmallButton("<")) { ImGui::TabBarQueueChangeTabOrder(tab_bar, tab, -1); } ImGui::SameLine(0, 2);
//									if (ImGui::SmallButton(">")) { ImGui::TabBarQueueChangeTabOrder(tab_bar, tab, +1); } ImGui::SameLine();
//									ImGui::Text("%02d%c Tab 0x%08X '%s'", tab_n, (tab->ID == tab_bar->SelectedTabId) ? '*' : ' ', tab->ID, (tab->Window || tab->NameOffset != -1) ? tab_bar->GetTabName(tab) : "");
//									ImGui::PopID();
//								}
//								ImGui::TreePop();
//							}
//						}
//
//						static void NodeStorage(ImGuiStorage* storage, const char* label)
//						{
//							if (!ImGui::TreeNode(label, "%s: %d entries, %d bytes", label, storage->Data.Size, storage->Data.size_in_bytes()))
//								return;
//							for (int n = 0; n < storage->Data.Size; n++)
//							{
//								const ImGuiStorage::ImGuiStoragePair& p = storage->Data[n];
//								ImGui::BulletText("Key 0x%08X Value { i: %d }", p.key, p.val_i); // Important: we currently don't store a type, real value may not be integer.
//							}
//							ImGui::TreePop();
//						}
//					};
//
//					// Tools
//					if (ImGui::TreeNode("Tools"))
//					{
//						// The Item Picker tool is super useful to visually select an item and break into the call-stack of where it was submitted.
//						if (ImGui::Button("Item Picker.."))
//							ImGui::DebugStartItemPicker();
//						ImGui::SameLine();
//
//						ImGui::TextDisabled("(?)");
//						if (ImGui::IsItemHovered())
//						{
//							ImGui::BeginTooltip();
//							ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
//							ImGui::TextUnformatted("Will call the IM_DEBUG_BREAK() macro to break in debugger.\nWarning: If you don't have a debugger attached, this will probably crash.");
//							ImGui::PopTextWrapPos();
//							ImGui::EndTooltip();
//						}
//
//						ImGui::Checkbox("Show windows begin order", &show_windows_begin_order);
//						ImGui::Checkbox("Show windows rectangles", &show_windows_rects);
//						ImGui::SameLine();
//						ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12);
//						show_windows_rects |= ImGui::Combo("##show_windows_rect_type", &show_windows_rect_type, wrt_rects_names, WRT_Count, WRT_Count);
//						if (show_windows_rects && g.NavWindow)
//						{
//							ImGui::BulletText("'%s':", g.NavWindow->Name);
//							ImGui::Indent();
//							for (int rect_n = 0; rect_n < WRT_Count; rect_n++)
//							{
//								ImRect r = Funcs::GetWindowRect(g.NavWindow, rect_n);
//								ImGui::Text("(%6.1f,%6.1f) (%6.1f,%6.1f) Size (%6.1f,%6.1f) %s", r.Min.x, r.Min.y, r.Max.x, r.Max.y, r.GetWidth(), r.GetHeight(), wrt_rects_names[rect_n]);
//							}
//							ImGui::Unindent();
//						}
//						ImGui::Checkbox("Show mesh when hovering ImDrawCmd", &show_drawcmd_mesh);
//						ImGui::Checkbox("Show bounding boxes when hovering ImDrawCmd", &show_drawcmd_aabb);
//						ImGui::TreePop();
//					}
//
//					// Contents
//					Funcs::NodeWindows(g.Windows, "Windows");
//					//Funcs::NodeWindows(g.WindowsFocusOrder, "WindowsFocusOrder");
//					if (ImGui::TreeNode("Viewport", "Viewports (%d)", g.Viewports.Size))
//					{
//						ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
//						ImGui::ShowViewportThumbnails();
//						ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
//						bool open = ImGui::TreeNode("Monitors", "Monitors (%d)", g.PlatformIO.Monitors.Size);
//						ImGui::SameLine();
//
//						ImGui::TextDisabled("(?)");
//						if (ImGui::IsItemHovered())
//						{
//							ImGui::BeginTooltip();
//							ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
//							ImGui::TextUnformatted("Dear ImGui uses monitor data:\n- to query DPI settings on a per monitor basis\n- to position popup/tooltips so they don't straddle monitors.");
//							ImGui::PopTextWrapPos();
//							ImGui::EndTooltip();
//						}
//
//						if (open)
//						{
//							for (int i = 0; i < g.PlatformIO.Monitors.Size; i++)
//							{
//								const ImGuiPlatformMonitor& mon = g.PlatformIO.Monitors[i];
//								ImGui::BulletText("Monitor #%d: DPI %.0f%%\n MainMin (%.0f,%.0f), MainMax (%.0f,%.0f), MainSize (%.0f,%.0f)\n WorkMin (%.0f,%.0f), WorkMax (%.0f,%.0f), WorkSize (%.0f,%.0f)",
//									i, mon.DpiScale * 100.0f,
//									mon.MainPos.x, mon.MainPos.y, mon.MainPos.x + mon.MainSize.x, mon.MainPos.y + mon.MainSize.y, mon.MainSize.x, mon.MainSize.y,
//									mon.WorkPos.x, mon.WorkPos.y, mon.WorkPos.x + mon.WorkSize.x, mon.WorkPos.y + mon.WorkSize.y, mon.WorkSize.x, mon.WorkSize.y);
//							}
//							ImGui::TreePop();
//						}
//						for (int i = 0; i < g.Viewports.Size; i++)
//							Funcs::NodeViewport(g.Viewports[i]);
//						ImGui::TreePop();
//					}
//
//					// Details for Popups
//					if (ImGui::TreeNode("Popups", "Popups (%d)", g.OpenPopupStack.Size))
//					{
//						for (int i = 0; i < g.OpenPopupStack.Size; i++)
//						{
//							ImGuiWindow* window = g.OpenPopupStack[i].Window;
//							ImGui::BulletText("PopupID: %08x, Window: '%s'%s%s", g.OpenPopupStack[i].PopupId, window ? window->Name : "NULL", window && (window->Flags & ImGuiWindowFlags_ChildWindow) ? " ChildWindow" : "", window && (window->Flags & ImGuiWindowFlags_ChildMenu) ? " ChildMenu" : "");
//						}
//						ImGui::TreePop();
//					}
//
//					// Details for TabBars
//					if (ImGui::TreeNode("TabBars", "Tab Bars (%d)", g.TabBars.GetSize()))
//					{
//						for (int n = 0; n < g.TabBars.GetSize(); n++)
//							Funcs::NodeTabBar(g.TabBars.GetByIndex(n));
//						ImGui::TreePop();
//					}
//
//					// Details for Tables
//					IM_UNUSED(trt_rects_names);
//					IM_UNUSED(show_tables_rects);
//					IM_UNUSED(show_tables_rect_type);
//#ifdef IMGUI_HAS_TABLE
//					if (ImGui::TreeNode("Tables", "Tables (%d)", g.Tables.GetSize()))
//					{
//						for (int n = 0; n < g.Tables.GetSize(); n++)
//							Funcs::NodeTable(g.Tables.GetByIndex(n));
//						ImGui::TreePop();
//					}
//#endif // #ifdef IMGUI_HAS_TABLE
//
//					// Details for Docking
//#ifdef IMGUI_HAS_DOCK
//					if (ImGui::TreeNode("Dock nodes"))
//					{
//						static bool root_nodes_only = true;
//						ImGuiDockContext* dc = &g.DockContext;
//						ImGui::Checkbox("List root nodes", &root_nodes_only);
//						ImGui::Checkbox("Ctrl shows window dock info", &show_docking_nodes);
//						if (ImGui::SmallButton("Clear nodes")) { ImGui::DockContextClearNodes(&g, 0, true); }
//						ImGui::SameLine();
//						if (ImGui::SmallButton("Rebuild all")) { dc->WantFullRebuild = true; }
//						for (int n = 0; n < dc->Nodes.Data.Size; n++)
//							if (ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p)
//								if (!root_nodes_only || node->IsRootNode())
//									Funcs::NodeDockNode(node, "Node");
//						ImGui::TreePop();
//					}
//#endif // #ifdef IMGUI_HAS_DOCK
//
//					// Settings
//					if (ImGui::TreeNode("Settings"))
//					{
//						if (ImGui::SmallButton("Clear"))
//							ImGui::ClearIniSettings();
//						ImGui::SameLine();
//						if (ImGui::SmallButton("Save to memory"))
//							ImGui::SaveIniSettingsToMemory();
//						ImGui::SameLine();
//						if (ImGui::SmallButton("Save to disk"))
//							ImGui::SaveIniSettingsToDisk(g.IO.IniFilename);
//						ImGui::SameLine();
//						if (g.IO.IniFilename)
//							ImGui::Text("\"%s\"", g.IO.IniFilename);
//						else
//							ImGui::TextUnformatted("<NULL>");
//						ImGui::Text("SettingsDirtyTimer %.2f", g.SettingsDirtyTimer);
//						if (ImGui::TreeNode("SettingsHandlers", "Settings handlers: (%d)", g.SettingsHandlers.Size))
//						{
//							for (int n = 0; n < g.SettingsHandlers.Size; n++)
//								ImGui::BulletText("%s", g.SettingsHandlers[n].TypeName);
//							ImGui::TreePop();
//						}
//						if (ImGui::TreeNode("SettingsWindows", "Settings packed data: Windows: %d bytes", g.SettingsWindows.size()))
//						{
//							for (ImGuiWindowSettings* settings = g.SettingsWindows.begin(); settings != NULL; settings = g.SettingsWindows.next_chunk(settings))
//								Funcs::NodeWindowSettings(settings);
//							ImGui::TreePop();
//						}
//
//#ifdef IMGUI_HAS_TABLE
//						if (ImGui::TreeNode("SettingsTables", "Settings packed data: Tables: %d bytes", g.SettingsTables.size()))
//						{
//							for (ImGuiTableSettings* settings = g.SettingsTables.begin(); settings != NULL; settings = g.SettingsTables.next_chunk(settings))
//								Funcs::NodeTableSettings(settings);
//							ImGui::TreePop();
//						}
//#endif // #ifdef IMGUI_HAS_TABLE
//
//						if (ImGui::TreeNode("SettingsIniData", "Settings unpacked data (.ini): %d bytes", g.SettingsIniData.size()))
//						{
//							ImGui::InputTextMultiline("##Ini", (char*)(void*)g.SettingsIniData.c_str(), g.SettingsIniData.Buf.Size, ImVec2(-FLT_MIN, 0.0f), ImGuiInputTextFlags_ReadOnly);
//							ImGui::TreePop();
//						}
//						ImGui::TreePop();
//					}
//
//					// Misc Details
//					if (ImGui::TreeNode("Internal state"))
//					{
//						const char* input_source_names[] = { "None", "Mouse", "Nav", "NavKeyboard", "NavGamepad" }; IM_ASSERT(IM_ARRAYSIZE(input_source_names) == ImGuiInputSource_COUNT);
//
//						ImGui::Text("WINDOWING");
//						ImGui::Indent();
//						ImGui::Text("HoveredWindow: '%s'", g.HoveredWindow ? g.HoveredWindow->Name : "NULL");
//						ImGui::Text("HoveredRootWindow: '%s'", g.HoveredRootWindow ? g.HoveredRootWindow->Name : "NULL");
//						ImGui::Text("HoveredWindowUnderMovingWindow: '%s'", g.HoveredWindowUnderMovingWindow ? g.HoveredWindowUnderMovingWindow->Name : "NULL");
//						ImGui::Text("HoveredDockNode: 0x%08X", g.HoveredDockNode ? g.HoveredDockNode->ID : 0);
//						ImGui::Text("MovingWindow: '%s'", g.MovingWindow ? g.MovingWindow->Name : "NULL");
//						ImGui::Text("MouseViewport: 0x%08X (UserHovered 0x%08X, LastHovered 0x%08X)", g.MouseViewport->ID, g.IO.MouseHoveredViewport, g.MouseLastHoveredViewport ? g.MouseLastHoveredViewport->ID : 0);
//						ImGui::Unindent();
//
//						ImGui::Text("ITEMS");
//						ImGui::Indent();
//						ImGui::Text("ActiveId: 0x%08X/0x%08X (%.2f sec), AllowOverlap: %d, Source: %s", g.ActiveId, g.ActiveIdPreviousFrame, g.ActiveIdTimer, g.ActiveIdAllowOverlap, input_source_names[g.ActiveIdSource]);
//						ImGui::Text("ActiveIdWindow: '%s'", g.ActiveIdWindow ? g.ActiveIdWindow->Name : "NULL");
//						ImGui::Text("HoveredId: 0x%08X/0x%08X (%.2f sec), AllowOverlap: %d", g.HoveredId, g.HoveredIdPreviousFrame, g.HoveredIdTimer, g.HoveredIdAllowOverlap); // Data is "in-flight" so depending on when the Metrics window is called we may see current frame information or not
//						ImGui::Text("DragDrop: %d, SourceId = 0x%08X, Payload \"%s\" (%d bytes)", g.DragDropActive, g.DragDropPayload.SourceId, g.DragDropPayload.DataType, g.DragDropPayload.DataSize);
//						ImGui::Unindent();
//
//						ImGui::Text("NAV,FOCUS");
//						ImGui::Indent();
//						ImGui::Text("NavWindow: '%s'", g.NavWindow ? g.NavWindow->Name : "NULL");
//						ImGui::Text("NavId: 0x%08X, NavLayer: %d", g.NavId, g.NavLayer);
//						ImGui::Text("NavInputSource: %s", input_source_names[g.NavInputSource]);
//						ImGui::Text("NavActive: %d, NavVisible: %d", g.IO.NavActive, g.IO.NavVisible);
//						ImGui::Text("NavActivateId: 0x%08X, NavInputId: 0x%08X", g.NavActivateId, g.NavInputId);
//						ImGui::Text("NavDisableHighlight: %d, NavDisableMouseHover: %d", g.NavDisableHighlight, g.NavDisableMouseHover);
//						ImGui::Text("NavFocusScopeId = 0x%08X", g.NavFocusScopeId);
//						ImGui::Text("NavWindowingTarget: '%s'", g.NavWindowingTarget ? g.NavWindowingTarget->Name : "NULL");
//						ImGui::Unindent();
//
//						ImGui::TreePop();
//					}
//
//					// Overlay: Display windows Rectangles and Begin Order
//					if (show_windows_rects || show_windows_begin_order)
//					{
//						for (int n = 0; n < g.Windows.Size; n++)
//						{
//							ImGuiWindow* window = g.Windows[n];
//							if (!window->WasActive)
//								continue;
//							ImDrawList* draw_list = ImGui::GetForegroundDrawList(window);
//							if (show_windows_rects)
//							{
//								ImRect r = Funcs::GetWindowRect(window, show_windows_rect_type);
//								draw_list->AddRect(r.Min, r.Max, IM_COL32(255, 0, 128, 255));
//							}
//							if (show_windows_begin_order && !(window->Flags & ImGuiWindowFlags_ChildWindow))
//							{
//								char buf[32];
//								ImFormatString(buf, IM_ARRAYSIZE(buf), "%d", window->BeginOrderWithinContext);
//								float font_size = ImGui::GetFontSize();
//								draw_list->AddRectFilled(window->Pos, window->Pos + ImVec2(font_size, font_size), IM_COL32(200, 100, 100, 255));
//								draw_list->AddText(window->Pos, IM_COL32(255, 255, 255, 255), buf);
//							}
//						}
//					}
//
//#ifdef IMGUI_HAS_TABLE
//					// Overlay: Display Tables Rectangles
//					if (show_tables_rects)
//					{
//						for (int table_n = 0; table_n < g.Tables.GetSize(); table_n++)
//						{
//							ImGuiTable* table = g.Tables.GetByIndex(table_n);
//						}
//					}
//#endif // #ifdef IMGUI_HAS_TABLE
//
//#ifdef IMGUI_HAS_DOCK
//					// Overlay: Display Docking info
//					if (show_docking_nodes && g.IO.KeyCtrl && g.HoveredDockNode)
//					{
//						char buf[64] = "";
//						char* p = buf;
//						ImGuiDockNode* node = g.HoveredDockNode;
//						ImDrawList* overlay_draw_list = node->HostWindow ? ImGui::GetForegroundDrawList(node->HostWindow) : ImGui::GetForegroundDrawList((ImGuiViewportP*)ImGui::GetMainViewport());
//						p += ImFormatString(p, buf + IM_ARRAYSIZE(buf) - p, "DockId: %X%s\n", node->ID, node->IsCentralNode() ? " *CentralNode*" : "");
//						p += ImFormatString(p, buf + IM_ARRAYSIZE(buf) - p, "WindowClass: %08X\n", node->WindowClass.ClassId);
//						p += ImFormatString(p, buf + IM_ARRAYSIZE(buf) - p, "Size: (%.0f, %.0f)\n", node->Size.x, node->Size.y);
//						p += ImFormatString(p, buf + IM_ARRAYSIZE(buf) - p, "SizeRef: (%.0f, %.0f)\n", node->SizeRef.x, node->SizeRef.y);
//						int depth = ImGui::DockNodeGetDepth(node);
//						overlay_draw_list->AddRect(node->Pos + ImVec2(3, 3) * (float)depth, node->Pos + node->Size - ImVec2(3, 3) * (float)depth, IM_COL32(200, 100, 100, 255));
//						ImVec2 pos = node->Pos + ImVec2(3, 3) * (float)depth;
//						overlay_draw_list->AddRectFilled(pos - ImVec2(1, 1), pos + ImGui::CalcTextSize(buf) + ImVec2(1, 1), IM_COL32(200, 100, 100, 255));
//						overlay_draw_list->AddText(NULL, 0.0f, pos, IM_COL32(255, 255, 255, 255), buf);
//					}
//#endif // #ifdef IMGUI_HAS_DOCK
//					//ImGui::EndChild();
//					ImGui::TreePop();
//				}
//
//				ImGui::End();
//			}
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