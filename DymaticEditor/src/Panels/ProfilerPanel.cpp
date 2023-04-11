#include "ProfilerPanel.h"

#include "Dymatic/Debug/Instrumentor.h"

#include "Settings/Preferences.h"
#include "TextSymbols.h"

#include <stdio.h>
#include <stdint.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <windows.h>
#include "psapi.h"

#define _PRISizeT   "I"
#define ImSnprintf  _snprintf

namespace Dymatic {
	
	static std::vector<ProfileResult> s_ProfileResults;
	static auto s_StartTime = FloatingPointMicroseconds{ std::chrono::steady_clock::now().time_since_epoch() };
	
	static void WriteProfileResult(const ProfileResult& result)
	{
		s_ProfileResults.push_back(result);
	}

	ProfilerPanel::ProfilerPanel()
	{
		// Settings
		Open = true;
		ReadOnly = false;
		Cols = 24;
		OptShowOptions = true;
		OptShowDataPreview = true;
		OptShowHexII = false;
		OptShowAscii = true;
		OptGreyOutZeroes = true;
		OptUpperCaseHex = true;
		OptMidColsCount = 8;
		OptAddrDigitsCount = 0;
		OptFooterExtraHeight = 0.0f;
		HighlightColor = IM_COL32(255, 255, 255, 50);

		// State/Internals
		ContentsWidthChanged = false;
		DataPreviewAddr = DataEditingAddr = (size_t)-1;
		DataEditingTakeFocus = false;
		memset(DataInputBuf, 0, sizeof(DataInputBuf));
		memset(AddrInputBuf, 0, sizeof(AddrInputBuf));
		GotoAddr = (size_t)-1;
		HighlightMin = HighlightMax = (size_t)-1;
		PreviewEndianess = 0;
		PreviewDataType = ImGuiDataType_S32;
	}

	void ProfilerPanel::GotoAddrAndHighlight(size_t addr_min, size_t addr_max)
	{
		GotoAddr = addr_min;
		HighlightMin = addr_min;
		HighlightMax = addr_max;
	}

	void ProfilerPanel::CalcSizes(Sizes& s, size_t mem_size, size_t base_display_addr)
	{
		ImGuiStyle& style = ImGui::GetStyle();
		s.AddrDigitsCount = OptAddrDigitsCount;
		if (s.AddrDigitsCount == 0)
			for (size_t n = base_display_addr + mem_size - 1; n > 0; n >>= 4)
				s.AddrDigitsCount++;
		s.LineHeight = ImGui::GetTextLineHeight();
		s.GlyphWidth = ImGui::CalcTextSize("F").x + 1;                  // We assume the font is mono-space
		s.HexCellWidth = (float)(int)(s.GlyphWidth * 2.5f);             // "FF " we include trailing space in the width to easily catch clicks everywhere
		s.SpacingBetweenMidCols = (float)(int)(s.HexCellWidth * 0.25f); // Every OptMidColsCount columns we add a bit of extra spacing
		s.PosHexStart = (s.AddrDigitsCount + 2) * s.GlyphWidth;
		s.PosHexEnd = s.PosHexStart + (s.HexCellWidth * Cols);
		s.PosAsciiStart = s.PosAsciiEnd = s.PosHexEnd;
		if (OptShowAscii)
		{
			s.PosAsciiStart = s.PosHexEnd + s.GlyphWidth * 1;
			if (OptMidColsCount > 0)
				s.PosAsciiStart += (float)((Cols + OptMidColsCount - 1) / OptMidColsCount) * s.SpacingBetweenMidCols;
			s.PosAsciiEnd = s.PosAsciiStart + Cols * s.GlyphWidth;
		}
		s.WindowWidth = s.PosAsciiEnd + style.ScrollbarSize + style.WindowPadding.x * 2 + s.GlyphWidth;
	}

	void ProfilerPanel::OnImGuiRender()
	{
		auto& profilerVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Profiler);

		if (!profilerVisible)
			return;
		
		if (ImGui::Begin(CHARACTER_ICON_MEMORY " Profiler", &profilerVisible, ImGuiWindowFlags_NoScrollbar))
		{
			ImGui::BeginTabBar("##ProfilerTabs");
			if (ImGui::BeginTabItem("Memory"))
			{
				DrawMemoryEditor();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Timers"))
			{
				DrawProfileTimers();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Usage"))
			{
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		ImGui::End();
	}

	static std::unordered_map<std::string, ImVec4> s_TimerColors;
	
	void ProfilerPanel::DrawProfileTimers()
	{	
		auto drawList = ImGui::GetWindowDrawList();
		auto& io = ImGui::GetIO();
		auto& style = ImGui::GetStyle();

		static bool s_Dragging;

		const float height = 30.0f;
		const float totalWidth = ImGui::GetWindowSize().x;
		const bool isHovered = ImGui::IsWindowHovered();

		if (isHovered && io.MouseWheel != 0.0f)
		{
			auto offset = (m_TimerViewEnd - m_TimerViewStart) * (double)io.MouseWheel * 0.1;
			m_TimerViewStart += offset;
			m_TimerViewEnd -= offset;
		}

		if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			s_Dragging = true;

		if (s_Dragging && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
			s_Dragging = false;
		
		if (s_Dragging)
		{
			auto offset = (m_TimerViewEnd - m_TimerViewStart) * (double)io.MouseDelta.x / totalWidth;
			m_TimerViewStart -= offset;
			m_TimerViewEnd -= offset;
		}
		
		if (ImGui::Button(m_TimerRecording ? "Stop" : "Start"))
		{
			m_TimerRecording = !m_TimerRecording;
			Instrumentor::Get().SetWriteFunction(m_TimerRecording ? &WriteProfileResult : nullptr);

			if (m_TimerRecording)
			{
				auto now = (FloatingPointMicroseconds{ std::chrono::steady_clock::now().time_since_epoch() } - s_StartTime).count();
				m_TimerViewStart = now;
				m_TimerViewEnd = now + (10.0 * 1e+6);
			}
		}

		if (!m_TimerRecording && !s_ProfileResults.empty())
		{
			ImGui::SameLine();
			if (ImGui::Button("Clear"))
			{
				s_ProfileResults.clear();
				s_TimerColors.clear();
			}
		}
		
		ImGui::Separator();

		{
			const float width = ImGui::GetContentRegionAvailWidth() * 0.5f;

			ImGui::SetNextItemWidth(width);
			ImGui::InputDouble("View Start", &m_TimerViewStart, 0.0, 0.0, "%.3f");

			ImGui::SameLine();

			ImGui::SetNextItemWidth(width);
			ImGui::InputDouble("View End", &m_TimerViewEnd, 0.0, 0.0, "%.3f");
		}
		
		ImGui::Separator();

		if (!s_ProfileResults.empty())
		{
			for (int i = s_ProfileResults.size() - 1; i >= 0; i--)
			{
				auto& result = s_ProfileResults[i];
				
				if ((result.Start - s_StartTime + result.ElapsedTime).count() < m_TimerViewStart || (result.Start - s_StartTime).count() > m_TimerViewEnd)
					continue;

				const float width = totalWidth * (result.ElapsedTime / (m_TimerViewEnd - m_TimerViewStart)).count();
				
				ImVec2 start = ImGui::GetCursorScreenPos() + ImVec2(((result.Start - s_StartTime).count() - m_TimerViewStart) / (m_TimerViewEnd - m_TimerViewStart) * totalWidth, height * (result.StackLevel - 1));
				ImVec2 end = start + ImVec2(width, height);
				
				ImVec4 color;
				if (s_TimerColors.find(result.Name) == s_TimerColors.end())
				{
					color = { (float)rand() / (float)RAND_MAX, (float)rand() / (float)RAND_MAX, (float)rand() / (float)RAND_MAX, 1.0f };
					s_TimerColors[result.Name] = color;
				}
				else
					color = s_TimerColors[result.Name];
				
				const ImVec2 size = ImGui::CalcTextSize(result.Name.c_str());
				drawList->AddRectFilled(start, end, ImGui::ColorConvertFloat4ToU32(color));
				if (width > size.x)
					drawList->AddText((start + end - size) * 0.5f, IM_COL32_WHITE, result.Name.c_str());

				const ImVec2 mousePos = ImGui::GetMousePos();
				if (mousePos.x > start.x && mousePos.x < end.x && mousePos.y > start.y && mousePos.y < end.y)
				{
					drawList->AddRect(start, end, ImGui::ColorConvertFloat4ToU32(color * ImVec4(0.9f, 0.9f, 0.9f, 1.0f)), 0.0f, 0, 2.0f);
					ImGui::BeginTooltip();
					ImGui::TextDisabled(result.Name.c_str());
					ImGui::Text("Elapsed Time: %.3f" CHARACTER_SYMBOL_MU "s", result.ElapsedTime.count());
					ImGui::Text("Start Time: %.3f" CHARACTER_SYMBOL_MU "s", result.Start.count());
					ImGui::Text("Stack Level: %d", result.StackLevel);
					ImGui::Text("Thread ID: %d", result.ThreadID);
					ImGui::EndTooltip();
				}
					
			}
		}

		// Draw loading bar if timing is in process
		if (m_TimerRecording)
		{
			static const ImVec4 ProgressBarColorA = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
			static const ImVec4 ProgressBarColorB = ImVec4(0.97f, 0.97f, 0.97f, 1.0f);

			const ImVec2 windowPos = ImGui::GetWindowPos();
			const ImVec2 windowSize = ImGui::GetWindowSize();
			const ImU32 colorB = ImGui::ColorConvertFloat4ToU32(ProgressBarColorB);

			const char* text = "Dymatic function timing in progress...";
			const ImVec2 textSize = ImGui::CalcTextSize(text);
			
			// Draw loading bars
			for (uint32_t i = 0; i < 2; i++)
			{
				const float posY[] = 
				{
					windowPos.y + windowSize.y - style.WindowPadding.y - (10.0f * 1.0f),
					windowPos.y + windowSize.y - style.WindowPadding.y - (10.0f * 2.0f) - textSize.y - (style.FramePadding.y * 2.0f)
				};
				
				drawList->AddRectFilled(ImVec2(windowPos.x, posY[i]), ImVec2(windowPos.x + windowSize.x, posY[i] + 10.0f), ImGui::ColorConvertFloat4ToU32(ProgressBarColorA));
				unsigned int total = 0;
				float space = 0.0f;
				while (space < windowSize.x)
				{
					float value = fmod(10.0f * ImGui::GetTime(), 10.0f);
					ImVec2 points[4] = { ImVec2(windowPos.x + std::min(total * 10.0f + value, windowSize.x), posY[i]), ImVec2(windowPos.x + std::min(total * 10.0f + 5.0f + value, windowSize.x), posY[i]), ImVec2(windowPos.x + std::min(total * 10.0f + value, windowSize.x), posY[i] + 10.0f), ImVec2(windowPos.x + std::min(total * 10.0f - 5.0f + value, windowSize.x), posY[i] + 10.0f) };
					drawList->AddConvexPolyFilled(points, 4, colorB);

					space += 10.0f;
					total++;
				}
			}

			// Main loading text
			ImGui::PushFont(io.Fonts->Fonts[0]);
			drawList->AddText(ImVec2(windowPos.x + (windowSize.x - textSize.x) * 0.5f, windowPos.y + windowSize.y - style.WindowPadding.y - 10.0f - textSize.y - style.FramePadding.y), ImGui::GetColorU32(ImGuiCol_Text), text);
			ImGui::PopFont();
		}
	}

	static uintptr_t GetProcessBaseAddress()
	{
		DWORD_PTR   baseAddress = 0;
		HANDLE      processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
		HMODULE* moduleArray;
		LPBYTE      moduleArrayBytes;
		DWORD       bytesRequired;

		if (processHandle)
		{
			if (EnumProcessModules(processHandle, NULL, 0, &bytesRequired))
			{
				if (bytesRequired)
				{
					moduleArrayBytes = (LPBYTE)LocalAlloc(LPTR, bytesRequired);

					if (moduleArrayBytes)
					{
						unsigned int moduleCount;

						moduleCount = bytesRequired / sizeof(HMODULE);
						moduleArray = (HMODULE*)moduleArrayBytes;

						if (EnumProcessModules(processHandle, moduleArray, bytesRequired, &bytesRequired))
						{
							baseAddress = (DWORD_PTR)moduleArray[0];
						}

						LocalFree(moduleArrayBytes);
					}
				}
			}

			CloseHandle(processHandle);
		}

		return baseAddress;
	}

	static size_t GetProcessMemorySize()
	{
		PROCESS_MEMORY_COUNTERS_EX pmc;
		GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
		return pmc.PrivateUsage;
	}

	static void GetProcessMemory(void* buffer, uintptr_t address, size_t size)
	{
		ReadProcessMemory(GetCurrentProcess(), (LPCVOID)address, buffer, size, NULL);
	}

	static void SetProcessMemory(void* buffer, uintptr_t address, size_t size)
	{
		WriteProcessMemory(GetCurrentProcess(), (LPVOID)address, buffer, size, NULL);
	}

	// Utilities for Data Preview
	static const char* DataTypeGetDesc(ImGuiDataType data_type)
	{
		const char* descs[] = { "Int8", "Uint8", "Int16", "Uint16", "Int32", "Uint32", "Int64", "Uint64", "Float", "Double" };
		IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
		return descs[data_type];
	}

	static size_t DataTypeGetSize(ImGuiDataType data_type)
	{
		const size_t sizes[] = { 1, 1, 2, 2, 4, 4, 8, 8, sizeof(float), sizeof(double) };
		IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
		return sizes[data_type];
	}

	static const char* DataFormatGetDesc(ProfilerPanel::DataFormat data_format)
	{
		const char* descs[] = { "Bin", "Dec", "Hex" };
		IM_ASSERT(data_format >= 0 && data_format < ProfilerPanel::DataFormat_COUNT);
		return descs[data_format];
	}

	static bool IsBigEndian()
	{
		uint16_t x = 1;
		char c[2];
		memcpy(c, &x, 2);
		return c[0] != 0;
	}

	static void* EndianessCopyBigEndian(void* _dst, void* _src, size_t s, int is_little_endian)
	{
		if (is_little_endian)
		{
			uint8_t* dst = (uint8_t*)_dst;
			uint8_t* src = (uint8_t*)_src + s - 1;
			for (int i = 0, n = (int)s; i < n; ++i)
				memcpy(dst++, src--, 1);
			return _dst;
		}
		else
		{
			return memcpy(_dst, _src, s);
		}
	}

	static void* EndianessCopyLittleEndian(void* _dst, void* _src, size_t s, int is_little_endian)
	{
		if (is_little_endian)
		{
			return memcpy(_dst, _src, s);
		}
		else
		{
			uint8_t* dst = (uint8_t*)_dst;
			uint8_t* src = (uint8_t*)_src + s - 1;
			for (int i = 0, n = (int)s; i < n; ++i)
				memcpy(dst++, src--, 1);
			return _dst;
		}
	}

	void ProfilerPanel::DrawMemoryEditor()
	{
		uintptr_t base_display_addr = GetProcessBaseAddress();
		size_t mem_size = GetProcessMemorySize();

		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
			ImGui::OpenPopup("context");
		
		if (Cols < 1)
			Cols = 1;

		Sizes s;
		CalcSizes(s, mem_size, base_display_addr);
		ImGuiStyle& style = ImGui::GetStyle();

		// We begin into our scrolling region with the 'ImGuiWindowFlags_NoMove' in order to prevent click from moving the window.
		// This is used as a facility since our main click detection code doesn't assign an ActiveId so the click would normally be caught as a window-move.
		const float height_separator = style.ItemSpacing.y;
		float footer_height = OptFooterExtraHeight;
		if (OptShowOptions)
			footer_height += height_separator + ImGui::GetFrameHeightWithSpacing() * 1;
		if (OptShowDataPreview)
			footer_height += height_separator + ImGui::GetFrameHeightWithSpacing() * 1 + ImGui::GetTextLineHeightWithSpacing() * 3;
		ImGui::BeginChild("##scrolling", ImVec2(0, -footer_height), false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

		// We are not really using the clipper API correctly here, because we rely on visible_start_addr/visible_end_addr for our scrolling function.
		const int line_total_count = (int)((mem_size + Cols - 1) / Cols);
		ImGuiListClipper clipper;
		clipper.Begin(line_total_count, s.LineHeight);
		clipper.Step();
		const size_t visible_start_addr = clipper.DisplayStart * Cols;
		const size_t visible_end_addr = clipper.DisplayEnd * Cols;

		// Load the actual data from memory
		ImU8* mem_data = new ImU8[visible_end_addr - visible_start_addr];
		GetProcessMemory(mem_data, base_display_addr + visible_start_addr, visible_end_addr - visible_start_addr);

		bool data_next = false;

		if (ReadOnly || DataEditingAddr >= mem_size)
			DataEditingAddr = (size_t)-1;
		if (DataPreviewAddr >= mem_size)
			DataPreviewAddr = (size_t)-1;

		size_t preview_data_type_size = OptShowDataPreview ? DataTypeGetSize(PreviewDataType) : 0;

		size_t data_editing_addr_backup = DataEditingAddr;
		size_t data_editing_addr_next = (size_t)-1;
		if (DataEditingAddr != (size_t)-1)
		{
			// Move cursor but only apply on next frame so scrolling with be synchronized (because currently we can't change the scrolling while the window is being rendered)
			if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)) && DataEditingAddr >= (size_t)Cols) { data_editing_addr_next = DataEditingAddr - Cols; DataEditingTakeFocus = true; }
			else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)) && DataEditingAddr < mem_size - Cols) { data_editing_addr_next = DataEditingAddr + Cols; DataEditingTakeFocus = true; }
			else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)) && DataEditingAddr > 0) { data_editing_addr_next = DataEditingAddr - 1; DataEditingTakeFocus = true; }
			else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)) && DataEditingAddr < mem_size - 1) { data_editing_addr_next = DataEditingAddr + 1; DataEditingTakeFocus = true; }
		}
		if (data_editing_addr_next != (size_t)-1 && (data_editing_addr_next / Cols) != (data_editing_addr_backup / Cols))
		{
			// Track cursor movements
			const int scroll_offset = ((int)(data_editing_addr_next / Cols) - (int)(data_editing_addr_backup / Cols));
			const bool scroll_desired = (scroll_offset < 0 && data_editing_addr_next < visible_start_addr + Cols * 2) || (scroll_offset > 0 && data_editing_addr_next > visible_end_addr - Cols * 2);
			if (scroll_desired)
				ImGui::SetScrollY(ImGui::GetScrollY() + scroll_offset * s.LineHeight);
		}

		// Draw vertical separator
		ImVec2 window_pos = ImGui::GetWindowPos();
		if (OptShowAscii)
			draw_list->AddLine(ImVec2(window_pos.x + s.PosAsciiStart - s.GlyphWidth, window_pos.y), ImVec2(window_pos.x + s.PosAsciiStart - s.GlyphWidth, window_pos.y + 9999), ImGui::GetColorU32(ImGuiCol_Border));

		const ImU32 color_text = ImGui::GetColorU32(ImGuiCol_Text);
		const ImU32 color_disabled = OptGreyOutZeroes ? ImGui::GetColorU32(ImGuiCol_TextDisabled) : color_text;

		const char* format_address = OptUpperCaseHex ? "%0*" _PRISizeT "X: " : "%0*" _PRISizeT "x: ";
		const char* format_data = OptUpperCaseHex ? "%0*" _PRISizeT "X" : "%0*" _PRISizeT "x";
		const char* format_byte = OptUpperCaseHex ? "%02X" : "%02x";
		const char* format_byte_space = OptUpperCaseHex ? "%02X " : "%02x ";

		for (int line_i = clipper.DisplayStart; line_i < clipper.DisplayEnd; line_i++) // display only visible lines
		{
			size_t addr = (size_t)(line_i * Cols);
			ImGui::Text(format_address, s.AddrDigitsCount, base_display_addr + addr);

			// Draw Hexadecimal
			for (int n = 0; n < Cols && addr < mem_size; n++, addr++)
			{
				float byte_pos_x = s.PosHexStart + s.HexCellWidth * n;
				if (OptMidColsCount > 0)
					byte_pos_x += (float)(n / OptMidColsCount) * s.SpacingBetweenMidCols;
				ImGui::SameLine(byte_pos_x);

				// Draw highlight
				bool is_highlight_from_user_range = (addr >= HighlightMin && addr < HighlightMax);
				bool is_highlight_from_preview = (addr >= DataPreviewAddr && addr < DataPreviewAddr + preview_data_type_size);
				if (is_highlight_from_user_range || is_highlight_from_preview)
				{
					ImVec2 pos = ImGui::GetCursorScreenPos();
					float highlight_width = s.GlyphWidth * 2;
					bool is_next_byte_highlighted = (addr + 1 < mem_size) && ((HighlightMax != (size_t)-1 && addr + 1 < HighlightMax));
					if (is_next_byte_highlighted || (n + 1 == Cols))
					{
						highlight_width = s.HexCellWidth;
						if (OptMidColsCount > 0 && n > 0 && (n + 1) < Cols && ((n + 1) % OptMidColsCount) == 0)
							highlight_width += s.SpacingBetweenMidCols;
					}
					draw_list->AddRectFilled(pos, ImVec2(pos.x + highlight_width, pos.y + s.LineHeight), HighlightColor);
				}

				if (DataEditingAddr == addr)
				{
					// Display text input on current byte
					bool data_write = false;
					ImGui::PushID((void*)addr);
					if (DataEditingTakeFocus)
					{
						ImGui::SetKeyboardFocusHere();
						ImGui::CaptureKeyboardFromApp(true);
						sprintf(AddrInputBuf, format_data, s.AddrDigitsCount, base_display_addr + addr);
						sprintf(DataInputBuf, format_byte, mem_data[addr - visible_start_addr]);
					}
					struct UserData
					{
						// FIXME: We should have a way to retrieve the text edit cursor position more easily in the API, this is rather tedious. This is such a ugly mess we may be better off not using InputText() at all here.
						static int Callback(ImGuiInputTextCallbackData* data)
						{
							UserData* user_data = (UserData*)data->UserData;
							if (!data->HasSelection())
								user_data->CursorPos = data->CursorPos;
							if (data->SelectionStart == 0 && data->SelectionEnd == data->BufTextLen)
							{
								// When not editing a byte, always rewrite its content (this is a bit tricky, since InputText technically "owns" the master copy of the buffer we edit it in there)
								data->DeleteChars(0, data->BufTextLen);
								data->InsertChars(0, user_data->CurrentBufOverwrite);
								data->SelectionStart = 0;
								data->SelectionEnd = 2;
								data->CursorPos = 0;
							}
							return 0;
						}
						char   CurrentBufOverwrite[3];  // Input
						int    CursorPos;               // Output
					};
					UserData user_data;
					user_data.CursorPos = -1;
					sprintf(user_data.CurrentBufOverwrite, format_byte, mem_data[addr - visible_start_addr]);
					ImGuiInputTextFlags flags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_CallbackAlways;
#if IMGUI_VERSION_NUM >= 18104
					flags |= ImGuiInputTextFlags_AlwaysOverwrite;
#else
					flags |= ImGuiInputTextFlags_AlwaysInsertMode;
#endif
					ImGui::SetNextItemWidth(s.GlyphWidth * 2);
					if (ImGui::InputText("##data", DataInputBuf, IM_ARRAYSIZE(DataInputBuf), flags, UserData::Callback, &user_data))
						data_write = data_next = true;
					else if (!DataEditingTakeFocus && !ImGui::IsItemActive())
						DataEditingAddr = data_editing_addr_next = (size_t)-1;
					DataEditingTakeFocus = false;
					if (user_data.CursorPos >= 2)
						data_write = data_next = true;
					if (data_editing_addr_next != (size_t)-1)
						data_write = data_next = false;
					unsigned int data_input_value = 0;
					if (data_write && sscanf(DataInputBuf, "%X", &data_input_value) == 1)
					{
						//mem_data[addr - visible_start_addr] = (ImU8)data_input_value;
						SetProcessMemory(&data_input_value, base_display_addr + addr, 1);
					}
					ImGui::PopID();
				}
				else
				{
					// NB: The trailing space is not visible but ensure there's no gap that the mouse cannot click on.
					ImU8 b = mem_data[addr - visible_start_addr];

					if (OptShowHexII)
					{
						if ((b >= 32 && b < 128))
							ImGui::Text(".%c ", b);
						else if (b == 0xFF && OptGreyOutZeroes)
							ImGui::TextDisabled("## ");
						else if (b == 0x00)
							ImGui::Text("   ");
						else
							ImGui::Text(format_byte_space, b);
					}
					else
					{
						if (b == 0 && OptGreyOutZeroes)
							ImGui::TextDisabled("00 ");
						else
							ImGui::Text(format_byte_space, b);
					}
					if (!ReadOnly && ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
					{
						DataEditingTakeFocus = true;
						data_editing_addr_next = addr;
					}
				}
			}

			if (OptShowAscii)
			{
				// Draw ASCII values
				ImGui::SameLine(s.PosAsciiStart);
				ImVec2 pos = ImGui::GetCursorScreenPos();
				addr = line_i * Cols;
				ImGui::PushID(line_i);
				if (ImGui::InvisibleButton("ascii", ImVec2(s.PosAsciiEnd - s.PosAsciiStart, s.LineHeight)))
				{
					DataEditingAddr = DataPreviewAddr = addr + (size_t)((ImGui::GetIO().MousePos.x - pos.x) / s.GlyphWidth);
					DataEditingTakeFocus = true;
				}
				ImGui::PopID();
				for (int n = 0; n < Cols && addr < mem_size; n++, addr++)
				{
					if (addr == DataEditingAddr)
					{
						draw_list->AddRectFilled(pos, ImVec2(pos.x + s.GlyphWidth, pos.y + s.LineHeight), ImGui::GetColorU32(ImGuiCol_FrameBg));
						draw_list->AddRectFilled(pos, ImVec2(pos.x + s.GlyphWidth, pos.y + s.LineHeight), ImGui::GetColorU32(ImGuiCol_TextSelectedBg));
					}
					unsigned char c = mem_data[addr - visible_start_addr];
					char display_c = (c < 32 || c >= 128) ? '.' : c;
					draw_list->AddText(pos, (display_c == c) ? color_text : color_disabled, &display_c, &display_c + 1);
					pos.x += s.GlyphWidth;
				}
			}
		}
		IM_ASSERT(clipper.Step() == false);
		clipper.End();
		ImGui::PopStyleVar(2);
		ImGui::EndChild();

		// Notify the main window of our ideal child content size (FIXME: we are missing an API to get the contents size from the child)
		ImGui::SetCursorPosX(s.WindowWidth);

		if (data_next && DataEditingAddr < mem_size)
		{
			DataEditingAddr = DataPreviewAddr = DataEditingAddr + 1;
			DataEditingTakeFocus = true;
		}
		else if (data_editing_addr_next != (size_t)-1)
		{
			DataEditingAddr = DataPreviewAddr = data_editing_addr_next;
		}

		const bool lock_show_data_preview = OptShowDataPreview;
		if (OptShowOptions)
		{
			ImGui::Separator();
			DrawOptionsLine(s, mem_data, mem_size, base_display_addr);
		}

		if (lock_show_data_preview)
		{
			ImGui::Separator();
			DrawPreviewLine(s, mem_data, mem_size, base_display_addr, visible_start_addr);
		}

		delete[] mem_data;
	}

	void ProfilerPanel::DrawOptionsLine(const Sizes& s, void* mem_data, size_t mem_size, size_t base_display_addr)
	{
		IM_UNUSED(mem_data);
		ImGuiStyle& style = ImGui::GetStyle();
		const char* format_range = OptUpperCaseHex ? "Range %0*" _PRISizeT "X..%0*" _PRISizeT "X" : "Range %0*" _PRISizeT "x..%0*" _PRISizeT "x";

		// Options menu
		if (ImGui::Button("Options"))
			ImGui::OpenPopup("context");
		if (ImGui::BeginPopup("context"))
		{
			ImGui::SetNextItemWidth(s.GlyphWidth * 7 + style.FramePadding.x * 2.0f);
			if (ImGui::DragInt("##cols", &Cols, 0.2f, 4, 64, "%d cols")) { ContentsWidthChanged = true; if (Cols < 1) Cols = 1; }
			ImGui::Checkbox("Show Data Preview", &OptShowDataPreview);
			ImGui::Checkbox("Show HexII", &OptShowHexII);
			if (ImGui::Checkbox("Show Ascii", &OptShowAscii)) { ContentsWidthChanged = true; }
			ImGui::Checkbox("Grey out zeroes", &OptGreyOutZeroes);
			ImGui::Checkbox("Uppercase Hex", &OptUpperCaseHex);

			ImGui::EndPopup();
		}

		ImGui::SameLine();
		ImGui::Text(format_range, s.AddrDigitsCount, base_display_addr, s.AddrDigitsCount, base_display_addr + mem_size - 1);
		ImGui::SameLine();
		ImGui::SetNextItemWidth((s.AddrDigitsCount + 1) * s.GlyphWidth + style.FramePadding.x * 2.0f);
		if (ImGui::InputText("##addr", AddrInputBuf, IM_ARRAYSIZE(AddrInputBuf), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue))
		{
			size_t goto_addr;
			if (sscanf(AddrInputBuf, "%" _PRISizeT "X", &goto_addr) == 1)
			{
				GotoAddr = goto_addr - base_display_addr;
				HighlightMin = HighlightMax = (size_t)-1;
			}
		}

		if (GotoAddr != (size_t)-1)
		{
			if (GotoAddr < mem_size)
			{
				ImGui::BeginChild("##scrolling");
				ImGui::SetScrollFromPosY(ImGui::GetCursorStartPos().y + (GotoAddr / Cols) * ImGui::GetTextLineHeight());
				ImGui::EndChild();
				DataEditingAddr = DataPreviewAddr = GotoAddr;
				DataEditingTakeFocus = true;
			}
			GotoAddr = (size_t)-1;
		}
	}
	
	void ProfilerPanel::DrawPreviewLine(const Sizes& s, void* mem_data_void, size_t mem_size, size_t base_display_addr, size_t visible_start_addr)
	{
		IM_UNUSED(base_display_addr);
		ImU8* mem_data = (ImU8*)mem_data_void;
		ImGuiStyle& style = ImGui::GetStyle();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Preview as:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth((s.GlyphWidth * 10.0f) + style.FramePadding.x * 2.0f + style.ItemInnerSpacing.x);
		if (ImGui::BeginCombo("##combo_type", DataTypeGetDesc(PreviewDataType), ImGuiComboFlags_HeightLargest))
		{
			for (int n = 0; n < ImGuiDataType_COUNT; n++)
				if (ImGui::Selectable(DataTypeGetDesc((ImGuiDataType)n), PreviewDataType == n))
					PreviewDataType = (ImGuiDataType)n;
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth((s.GlyphWidth * 6.0f) + style.FramePadding.x * 2.0f + style.ItemInnerSpacing.x);
		ImGui::Combo("##combo_endianess", &PreviewEndianess, "LE\0BE\0\0");

		char buf[128] = "";
		float x = s.GlyphWidth * 6.0f;
		bool has_value = DataPreviewAddr != (size_t)-1;
		if (has_value)
			DrawPreviewData(DataPreviewAddr - visible_start_addr, mem_data, mem_size, PreviewDataType, DataFormat_Dec, buf, (size_t)IM_ARRAYSIZE(buf));
		ImGui::Text("Dec"); ImGui::SameLine(x); ImGui::TextUnformatted(has_value ? buf : "N/A");
		if (has_value)
			DrawPreviewData(DataPreviewAddr - visible_start_addr, mem_data, mem_size, PreviewDataType, DataFormat_Hex, buf, (size_t)IM_ARRAYSIZE(buf));
		ImGui::Text("Hex"); ImGui::SameLine(x); ImGui::TextUnformatted(has_value ? buf : "N/A");
		if (has_value)
			DrawPreviewData(DataPreviewAddr - visible_start_addr, mem_data, mem_size, PreviewDataType, DataFormat_Bin, buf, (size_t)IM_ARRAYSIZE(buf));
		buf[IM_ARRAYSIZE(buf) - 1] = 0;
		ImGui::Text("Bin"); ImGui::SameLine(x); ImGui::TextUnformatted(has_value ? buf : "N/A");
	}

	void* ProfilerPanel::EndianessCopy(void* dst, void* src, size_t size) const
	{
		static void* (*fp)(void*, void*, size_t, int) = NULL;
		if (fp == NULL)
			fp = IsBigEndian() ? EndianessCopyBigEndian : EndianessCopyLittleEndian;
		return fp(dst, src, size, PreviewEndianess);
	}

	static const char* FormatBinary(const uint8_t* buf, int width)
	{
		IM_ASSERT(width <= 64);
		size_t out_n = 0;
		static char out_buf[64 + 8 + 1];
		int n = width / 8;
		for (int j = n - 1; j >= 0; --j)
		{
			for (int i = 0; i < 8; ++i)
				out_buf[out_n++] = (buf[j] & (1 << (7 - i))) ? '1' : '0';
			out_buf[out_n++] = ' ';
		}
		IM_ASSERT(out_n < IM_ARRAYSIZE(out_buf));
		out_buf[out_n] = 0;
		return out_buf;
	}

	void ProfilerPanel::DrawPreviewData(size_t addr, const unsigned char* mem_data, size_t mem_size, int data_type, DataFormat data_format, char* out_buf, size_t out_buf_size) const
	{
		uint8_t buf[8];
		size_t elem_size = DataTypeGetSize(data_type);
		size_t size = addr + elem_size > mem_size ? mem_size - addr : elem_size;
		memcpy(buf, mem_data + addr, size);

		if (data_format == DataFormat_Bin)
		{
			uint8_t binbuf[8];
			EndianessCopy(binbuf, buf, size);
			ImSnprintf(out_buf, out_buf_size, "%s", FormatBinary(binbuf, (int)size * 8));
			return;
		}

		out_buf[0] = 0;
		switch (data_type)
		{
		case ImGuiDataType_S8:
		{
			int8_t int8 = 0;
			EndianessCopy(&int8, buf, size);
			if (data_format == DataFormat_Dec) { ImSnprintf(out_buf, out_buf_size, "%hhd", int8); return; }
			if (data_format == DataFormat_Hex) { ImSnprintf(out_buf, out_buf_size, "0x%02x", int8 & 0xFF); return; }
			break;
		}
		case ImGuiDataType_U8:
		{
			uint8_t uint8 = 0;
			EndianessCopy(&uint8, buf, size);
			if (data_format == DataFormat_Dec) { ImSnprintf(out_buf, out_buf_size, "%hhu", uint8); return; }
			if (data_format == DataFormat_Hex) { ImSnprintf(out_buf, out_buf_size, "0x%02x", uint8 & 0XFF); return; }
			break;
		}
		case ImGuiDataType_S16:
		{
			int16_t int16 = 0;
			EndianessCopy(&int16, buf, size);
			if (data_format == DataFormat_Dec) { ImSnprintf(out_buf, out_buf_size, "%hd", int16); return; }
			if (data_format == DataFormat_Hex) { ImSnprintf(out_buf, out_buf_size, "0x%04x", int16 & 0xFFFF); return; }
			break;
		}
		case ImGuiDataType_U16:
		{
			uint16_t uint16 = 0;
			EndianessCopy(&uint16, buf, size);
			if (data_format == DataFormat_Dec) { ImSnprintf(out_buf, out_buf_size, "%hu", uint16); return; }
			if (data_format == DataFormat_Hex) { ImSnprintf(out_buf, out_buf_size, "0x%04x", uint16 & 0xFFFF); return; }
			break;
		}
		case ImGuiDataType_S32:
		{
			int32_t int32 = 0;
			EndianessCopy(&int32, buf, size);
			if (data_format == DataFormat_Dec) { ImSnprintf(out_buf, out_buf_size, "%d", int32); return; }
			if (data_format == DataFormat_Hex) { ImSnprintf(out_buf, out_buf_size, "0x%08x", int32); return; }
			break;
		}
		case ImGuiDataType_U32:
		{
			uint32_t uint32 = 0;
			EndianessCopy(&uint32, buf, size);
			if (data_format == DataFormat_Dec) { ImSnprintf(out_buf, out_buf_size, "%u", uint32); return; }
			if (data_format == DataFormat_Hex) { ImSnprintf(out_buf, out_buf_size, "0x%08x", uint32); return; }
			break;
		}
		case ImGuiDataType_S64:
		{
			int64_t int64 = 0;
			EndianessCopy(&int64, buf, size);
			if (data_format == DataFormat_Dec) { ImSnprintf(out_buf, out_buf_size, "%lld", (long long)int64); return; }
			if (data_format == DataFormat_Hex) { ImSnprintf(out_buf, out_buf_size, "0x%016llx", (long long)int64); return; }
			break;
		}
		case ImGuiDataType_U64:
		{
			uint64_t uint64 = 0;
			EndianessCopy(&uint64, buf, size);
			if (data_format == DataFormat_Dec) { ImSnprintf(out_buf, out_buf_size, "%llu", (long long)uint64); return; }
			if (data_format == DataFormat_Hex) { ImSnprintf(out_buf, out_buf_size, "0x%016llx", (long long)uint64); return; }
			break;
		}
		case ImGuiDataType_Float:
		{
			float float32 = 0.0f;
			EndianessCopy(&float32, buf, size);
			if (data_format == DataFormat_Dec) { ImSnprintf(out_buf, out_buf_size, "%f", float32); return; }
			if (data_format == DataFormat_Hex) { ImSnprintf(out_buf, out_buf_size, "%a", float32); return; }
			break;
		}
		case ImGuiDataType_Double:
		{
			double float64 = 0.0;
			EndianessCopy(&float64, buf, size);
			if (data_format == DataFormat_Dec) { ImSnprintf(out_buf, out_buf_size, "%f", float64); return; }
			if (data_format == DataFormat_Hex) { ImSnprintf(out_buf, out_buf_size, "%a", float64); return; }
			break;
		}
		case ImGuiDataType_COUNT:
			break;
		} // Switch
		IM_ASSERT(0); // Shouldn't reach
	}
	
	ProfilerPanel::Sizes::Sizes()
	{
		{ memset(this, 0, sizeof(*this)); }
	}

}