#include "dypch.h"
#include "Dymatic/Utils/PlatformUtils.h"

#include "Dymatic/Core/Application.h"

#include "Dymatic/Math/Math.h"

#include <commdlg.h>
#include <atlstr.h>
#include <shobjidl_core.h>

#include <OleCtl.h>

#include <oleidl.h>
#include <shellapi.h>

#include "windowsx.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace Dymatic {

#pragma region Platform

	static ITaskbarList3* s_TaskbarList = nullptr;
	static std::vector<Taskbar::ThumbnailButton> s_ThumbnailButtons;
	static const uint32_t MaxThumbnailButtons = 7;

	static HWND GetPlatformWindow()
	{
		return glfwGetWin32Window((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow());
	}

	class DropManager : public IDropTarget
	{
	public:
		ULONG AddRef() { return 1; }
		ULONG Release() { return 0; }

		HRESULT QueryInterface(REFIID riid, void** ppvObject)
		{
			if (riid == IID_IDropTarget)
			{
				*ppvObject = this;
				return S_OK;
			}
			*ppvObject = NULL;
			return E_NOINTERFACE;
		};

		HRESULT DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
		{
			WindowDragEnterEvent e;
			Application::Get().OnEvent(e);

			*pdwEffect &= DROPEFFECT_COPY;
			return S_OK;
		}

		HRESULT DragLeave()
		{
			WindowDragLeaveEvent e;
			Application::Get().OnEvent(e);

			return S_OK;
		}

		HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
		{
			WindowDragOverEvent e;
			Application::Get().OnEvent(e);

			*pdwEffect &= DROPEFFECT_COPY;
			return S_OK;
		}

		HRESULT Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
		{
			// grfKeyState contains flags for control, alt, shift etc

			FORMATETC fmte = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
			STGMEDIUM stgm;

			if (SUCCEEDED(pDataObj->GetData(&fmte, &stgm)))
			{
				std::vector<std::string> paths;

				HDROP hdrop = (HDROP)stgm.hGlobal;
				UINT file_count = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);

				for (UINT i = 0; i < file_count; i++)
				{
					TCHAR szFile[MAX_PATH];
					UINT cch = DragQueryFile(hdrop, i, szFile, MAX_PATH);
					if (cch > 0 && cch < MAX_PATH)
					{
						std::wstring wstr = szFile;
						std::string str(wstr.begin(), wstr.end());

						paths.push_back(str);
					}
				}

				ReleaseStgMedium(&stgm);

				WindowDropEvent e{ paths };
				Application::Get().OnEvent(e);
			}

			*pdwEffect &= DROPEFFECT_COPY;
			return S_OK;
		}
	};
	static DropManager s_DropManager;

	void PlatformUtils::Init()
	{
		OleInitialize(NULL);

		CoInitialize(NULL);
		CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, (void**)&s_TaskbarList);
		s_TaskbarList->HrInit();

		RegisterDragDrop(GetPlatformWindow(), &s_DropManager);
	}

	void PlatformUtils::Shutdown()
	{
		s_TaskbarList->Release();
		CoUninitialize();

		RevokeDragDrop(GetPlatformWindow());

		OleUninitialize();
	}

	void Taskbar::SetLoading(bool loading)
	{
		s_TaskbarList->SetProgressState(GetPlatformWindow(), loading ? TBPF_INDETERMINATE : TBPF_NOPROGRESS);
	}

	void Taskbar::SetProgress(float progress)
	{
		HWND activeWindow = GetPlatformWindow();

		s_TaskbarList->SetProgressState(activeWindow, TBPF_NORMAL);
		s_TaskbarList->SetProgressValue(activeWindow, (ULONGLONG)(progress * 100), 100);
	}

	void Taskbar::FlashIcon()
	{
		FlashWindow(GetPlatformWindow(), false);
	}

	void Taskbar::SetNotificationIcon(Ref<Texture2D> icon, glm::vec3 tint)
	{
		if (icon == nullptr)
			s_TaskbarList->SetOverlayIcon(GetPlatformWindow(), NULL, NULL);
		else
		{
			HINSTANCE hInstance = GetModuleHandle(NULL);
			HICON hIcon;
		
			// Convert Ref<Texture2D> to HICON
			{
				uint32_t width = icon->GetWidth();
				uint32_t height = icon->GetHeight();
				uint32_t size = width * height * 4;

				// Retrieve the Texture2D pixel data
				unsigned char* iconData = new unsigned char[size];
				icon->GetData(iconData, size);

				unsigned char* flippedData = new unsigned char[size];
				for (uint32_t y = 0; y < height; y++)
				{
					uint32_t srcOffset = y * width * 4;
					uint32_t destOffset = (height - y - 1) * width * 4;
					memcpy(flippedData + destOffset, iconData + srcOffset, width * 4);
				}

				// Reverse the channels
				for (uint32_t i = 0; i < size; i += 4)
				{
					unsigned char temp = flippedData[i];
					flippedData[i] = flippedData[i + 2] * tint.z;
					flippedData[i + 1] *= tint.y;
					flippedData[i + 2] = temp * tint.z * tint.x;
				}

				hIcon = CreateIcon(hInstance, width, height, 1, 32, NULL, flippedData);
				delete[] iconData;
				delete[] flippedData;
			}

			// Set the notification overlay icon
			s_TaskbarList->SetOverlayIcon(GetPlatformWindow(), hIcon, NULL);

			DestroyIcon(hIcon);
		}
	}

	void Taskbar::AddThumbnailButton(const ThumbnailButton& button)
	{
		s_ThumbnailButtons.push_back(button);
		UpdateThumbnailButtons();
	}
	
	void Taskbar::RemoveThumbnailButton(uint32_t index)
	{
		if (index < 0 || index >= s_ThumbnailButtons.size())
			return;

		s_ThumbnailButtons.erase(s_ThumbnailButtons.begin() + index);
		UpdateThumbnailButtons();
	}
	
	void Taskbar::SetThumbnailButtons(const std::vector<ThumbnailButton>& buttons)
	{
		s_ThumbnailButtons = buttons;
		UpdateThumbnailButtons();
	}

	void Taskbar::UpdateThumbnailButtons()
	{
		HINSTANCE hInstance = GetModuleHandle(NULL);
		
		THUMBBUTTON thumbnailButtons[MaxThumbnailButtons];
		for (int i = 0; i < MaxThumbnailButtons; i++)
		{
			thumbnailButtons[i].dwMask = THB_ICON | THB_TOOLTIP | THB_FLAGS;
			thumbnailButtons[i].iId = i;
			thumbnailButtons[i].dwFlags = THBF_HIDDEN;
			thumbnailButtons[i].szTip[0] = '\0';
		}

		static bool s_Init = false;
		if (!s_Init && IsWindowVisible(GetPlatformWindow()))
		{
			s_Init = true;
			s_TaskbarList->ThumbBarAddButtons(GetPlatformWindow(), MaxThumbnailButtons, thumbnailButtons);
		}

		for (uint32_t index = 0; index < s_ThumbnailButtons.size() && index < MaxThumbnailButtons; index++)
		{
			auto& button = s_ThumbnailButtons[index];

			// Convert Ref<Texture2D> to HICON
			HICON hIcon;
			{
				uint32_t width = button.Icon->GetWidth();
				uint32_t height = button.Icon->GetHeight();
				uint32_t size = width * height * 4;

				// Retrieve the Texture2D pixel data
				unsigned char* iconData = new unsigned char[size];
				button.Icon->GetData(iconData, size);

				// Create a new buffer to flip the icon vertically
				unsigned char* flippedData = new unsigned char[size];
				for (uint32_t y = 0; y < height; y++)
				{
					uint32_t srcOffset = y * width * 4;
					uint32_t destOffset = (height - y - 1) * width * 4;
					memcpy(flippedData + destOffset, iconData + srcOffset, width * 4);
				}

				// Reverse the channels
				for (uint32_t i = 0; i < size; i += 4)
				{
					unsigned char temp = flippedData[i];
					flippedData[i] = flippedData[i + 2];
					flippedData[i + 2] = temp;
				}

				hIcon = CreateIcon(hInstance, width, height, 1, 32, NULL, flippedData);
				delete[] iconData;
				delete[] flippedData;
			}
			thumbnailButtons[index].hIcon = hIcon;

			uint32_t length = (button.Tooltip.length() > 259) ? 259 : button.Tooltip.length();
			for (uint32_t i = 0; i <= length; i++)
				thumbnailButtons[index].szTip[i] = button.Tooltip[i];
			thumbnailButtons[index].szTip[length] = '\0';

			thumbnailButtons[index].dwFlags = button.Enabled ? THBF_ENABLED : THBF_DISABLED;
		}

		s_TaskbarList->ThumbBarUpdateButtons(GetPlatformWindow(), MaxThumbnailButtons, thumbnailButtons);
	}

	void Taskbar::ThumbnailButtonCallback(uint32_t index)
	{
		if (s_ThumbnailButtons[index].Callback)
			s_ThumbnailButtons[index].Callback(s_ThumbnailButtons[index]);
	}
#pragma endregion

#pragma region File Dialogue
	std::string FileDialogs::OpenFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		CHAR currentDir[256] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = GetPlatformWindow();
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		if (GetCurrentDirectoryA(256, currentDir))
			ofn.lpstrInitialDir = currentDir;
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&ofn) == TRUE)
			return ofn.lpstrFile;

		return std::string();
	}

	std::string FileDialogs::SaveFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		CHAR currentDir[256] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = GetPlatformWindow();
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		if (GetCurrentDirectoryA(256, currentDir))
			ofn.lpstrInitialDir = currentDir;
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

		// Sets the default extension by extracting it from the filter
		ofn.lpstrDefExt = strchr(filter, '\0') + 1;

		if (GetSaveFileNameA(&ofn) == TRUE)
			return ofn.lpstrFile;

		return std::string();
	}

	std::string FileDialogs::SelectFolder()
	{
		LPWSTR path;
		IFileDialog* pfd;
		if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd))))
		{
			DWORD dwOptions;
			if (SUCCEEDED(pfd->GetOptions(&dwOptions)))
			{
				pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);
			}
			if (SUCCEEDED(pfd->Show(NULL)))
			{
				IShellItem* psi;
				if (SUCCEEDED(pfd->GetResult(&psi)))
				{
					if (!SUCCEEDED(psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &path)))
					{
						return std::string();
					}
					psi->Release();
				}
			}
			else
				return std::string();
			pfd->Release();
		}
		else
			return std::string();

		return CW2A(path);
	}
#pragma endregion
	
#pragma region Splash

#define WS_EX_LAYERED 0x00080000
#define LWA_COLORKEY 1
#define LWA_ALPHA    2

	typedef BOOL(WINAPI* lpfnSetLayeredWindowAttributes)
		(HWND hWnd, COLORREF cr, BYTE bAlpha, DWORD dwFlags);
	lpfnSetLayeredWindowAttributes g_pSetLayeredWindowAttributes;

	static const COLORREF s_SplashTransparentColor = RGB(255, 0, 255);
	static HWND s_SplashHandle;
	static std::string s_SplashApplicationName;
	static LPCTSTR s_SplashlpszClassName;
	static HBITMAP s_SplashBitmap;
	static HBITMAP s_SplashBitmapTinted;
	static int s_SplashRounding;
	static DWORD s_SplashWidth;
	static DWORD s_SplashHeight;
	static int s_SplashDominantRed, s_SplashDominantGreen, s_SplashDominantBlue;

	static LPCWSTR StringToLPCWSTR(const std::string& str)
	{
		int length = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
		wchar_t* buffer = new wchar_t[length];
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buffer, length);
		return buffer;
	}

	static void SplashOnPaint(HWND hwnd)
	{
		if (!s_SplashBitmap)
			return;

		//  Paint the background by BitBlting the bitmap
		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(hwnd, &ps);

		RECT   rect;
		::GetClientRect(s_SplashHandle, &rect);

		HDC hMemDC = ::CreateCompatibleDC(hDC);
		HBITMAP hOldBmp = (HBITMAP)::SelectObject(hMemDC, s_SplashBitmap);

		BitBlt(hDC, 0, 0, s_SplashWidth, s_SplashHeight, hMemDC, 0, 0, SRCCOPY);

		::SelectObject(hMemDC, hOldBmp);

		::DeleteDC(hMemDC);

		EndPaint(hwnd, &ps);
	}

	static LRESULT CALLBACK SplashWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
			HANDLE_MSG(hwnd, WM_PAINT, SplashOnPaint);
		}

		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	static void SplashFreeResources()
	{
		if (s_SplashBitmap)
			::DeleteObject(s_SplashBitmap);
		s_SplashBitmap = NULL;

		if (s_SplashBitmapTinted)
			::DeleteObject(s_SplashBitmapTinted);
		s_SplashBitmapTinted = NULL;
	}

	static bool SplashMakeTransparent()
	{
		if (s_SplashHandle && g_pSetLayeredWindowAttributes && s_SplashTransparentColor)
		{
			SetWindowLong(s_SplashHandle, GWL_EXSTYLE, GetWindowLong(s_SplashHandle, GWL_EXSTYLE) | WS_EX_LAYERED);
			g_pSetLayeredWindowAttributes(s_SplashHandle, s_SplashTransparentColor, 0, LWA_COLORKEY);
		}
		return TRUE;
	}

	void Splash::Init(const std::string& name, const std::string& splash, int rounding)
	{
		// Call Shutdown to close any open splash
		Shutdown();

		s_SplashRounding = rounding;
		s_SplashApplicationName = name;

		// Initialize
		{
			s_SplashHandle = NULL;
			s_SplashlpszClassName = TEXT("SPLASH");
			
			HMODULE hUser32 = GetModuleHandle(TEXT("USER32.DLL"));
			g_pSetLayeredWindowAttributes = (lpfnSetLayeredWindowAttributes)
				GetProcAddress(hUser32, "SetLayeredWindowAttributes");
		}

		// Set bitmap
		{
			HBITMAP    hBitmap = NULL;
			hBitmap = (HBITMAP)::LoadImage(0, StringToLPCWSTR(splash), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

			int nRetValue;
			BITMAP  csBitmapSize;

			// Free loaded resource
			SplashFreeResources();

			if (hBitmap)
			{
				s_SplashBitmap = hBitmap;
				
				//  Get bitmap size
				nRetValue = ::GetObject(hBitmap, sizeof(csBitmapSize), &csBitmapSize);
				if (nRetValue == 0)
				{
					SplashFreeResources();
				}
				s_SplashWidth = (DWORD)csBitmapSize.bmWidth;
				s_SplashHeight = (DWORD)csBitmapSize.bmHeight;

				// Calculate the bitmap's average color
				int numPixels = s_SplashWidth * s_SplashHeight;
				int numBytes = numPixels * 4;
				unsigned char* pixelData = new unsigned char[numBytes];
				GetBitmapBits(hBitmap, numBytes, pixelData);
				
				int sumR = 0, sumG = 0, sumB = 0;
				for (int i = 0; i < numBytes; i += 4)
				{
					sumB += pixelData[i];
					sumG += pixelData[i + 1];
					sumR += pixelData[i + 2];
				}
				
				int avgR = (sumR / numPixels);
				int avgG = (sumG / numPixels);
				int avgB = (sumB / numPixels);

				// Adjust the color to be closer to the mid tones
				{
					double brightnessValue = 0.299f * avgR + 0.587f * avgG + 0.114f * avgB;
					if (brightnessValue < 128)
					{
						s_SplashDominantRed = avgR + (128 - brightnessValue) / 2;
						s_SplashDominantGreen = avgG + (128 - brightnessValue) / 2;
						s_SplashDominantBlue = avgB + (128 - brightnessValue) / 2;
					}
					else 
					{
						s_SplashDominantRed = avgR - (brightnessValue - 128) / 2;
						s_SplashDominantGreen = avgG - (brightnessValue - 128) / 2;
						s_SplashDominantBlue = avgB - (brightnessValue - 128) / 2;
					}
				}

				// Create a copy of the bitmap, slightly tinted
				for (int i = 0; i < numBytes; i++)
					pixelData[i] = Math::Lerp(pixelData[i], 0, 0.35f);
				s_SplashBitmapTinted = ::CreateBitmap(s_SplashWidth, s_SplashHeight, 1, 32, pixelData);

				delete[] pixelData;
			}
		}

		SplashMakeTransparent();

		// Create a new splash window
		{
			// Register the window with ExtWndProc as the window procedure
			
			WNDCLASSEX wndclass;
			wndclass.cbSize = sizeof(wndclass);
			wndclass.style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW;
			wndclass.lpfnWndProc = SplashWindowProc;
			wndclass.cbClsExtra = 0;
			wndclass.cbWndExtra = DLGWINDOWEXTRA;
			wndclass.hInstance = ::GetModuleHandle(NULL);
			wndclass.hIcon = NULL;
			wndclass.hCursor = ::LoadCursor(NULL, IDC_WAIT);
			wndclass.hbrBackground = (HBRUSH)::GetStockObject(LTGRAY_BRUSH);
			wndclass.lpszMenuName = NULL;
			wndclass.lpszClassName = s_SplashlpszClassName;
			wndclass.hIconSm = NULL;

			if (!RegisterClassEx(&wndclass))
				DY_CORE_ERROR("Failed to register splash window class");
			
			//  Create the window of the application, passing the this pointer so that ExtWndProc can use that for message forwarding
			DWORD nScrWidth = ::GetSystemMetrics(SM_CXFULLSCREEN);
			DWORD nScrHeight = ::GetSystemMetrics(SM_CYFULLSCREEN);

			int x = (nScrWidth - s_SplashWidth) / 2;
			int y = (nScrHeight - s_SplashHeight) / 2;
			s_SplashHandle = ::CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, s_SplashlpszClassName,
				TEXT("Banner"), WS_POPUP, x, y,
				s_SplashWidth, s_SplashHeight, NULL, NULL, NULL, NULL);

			// Display the window
			if (s_SplashHandle)
			{
				SplashMakeTransparent();
				ShowWindow(s_SplashHandle, SW_SHOW);
				UpdateWindow(s_SplashHandle);
			}

			// Ensure the window is rounded
			RECT rect;
			GetWindowRect(s_SplashHandle, &rect);
			rect = { 0, 0, rect.right - rect.left, rect.bottom - rect.top };
			SetWindowRgn(s_SplashHandle, CreateRoundRectRgn(rect.left, rect.top, rect.right, rect.bottom, s_SplashRounding, s_SplashRounding), false);
		}
	}

	static std::wstring StringToWString(const std::string& s)
	{
		int len;
		int slength = (int)s.length() + 1;
		len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
		wchar_t* buf = new wchar_t[len];
		MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
		std::wstring r(buf);
		delete[] buf;
		return r;
	}

	static void SplashDrawImage(HBITMAP hBitmap, RECT rect)
	{
		HDC hdc = GetDC(s_SplashHandle);
		HBRUSH brush = CreatePatternBrush(hBitmap);
		FillRect(hdc, &rect, brush);
		DeleteObject(brush);
		ReleaseDC(s_SplashHandle, hdc);
	}

	static void SplashDrawText(std::string text, RECT rect, int fontSize, LPCTSTR font, int weight = FW_NORMAL)
	{
		HDC hDC = GetDC(s_SplashHandle);
		SetBkMode(hDC, TRANSPARENT);
		HBITMAP hBmpOld = (HBITMAP)SelectObject(hDC, s_SplashBitmap);
		HFONT NewFont = CreateFont(fontSize, 0, 0, 0, weight, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 0, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, font);
		HBRUSH NewBrush = CreateSolidBrush(RGB(150, 150, 150));
		SetTextColor(hDC, RGB(255, 255, 255));
		SelectObject(hDC, NewFont);
		SelectObject(hDC, NewBrush);
		DrawText(hDC, StringToWString(text).c_str(), text.length(), &rect, DT_LEFT | DT_WORDBREAK);
		DeleteObject(NewBrush);
		DeleteObject(NewFont);
		SelectObject(hDC, hBmpOld);
		ReleaseDC(s_SplashHandle, hDC);
	}

	static void SplashDrawRect(int left, int top, int right, int bottom, COLORREF color, int cornerRadius, bool border, int alpha = 255)
	{
		HDC hDC = GetDC(s_SplashHandle);
		SetBkMode(hDC, TRANSPARENT);
		HBITMAP hBmpOld = (HBITMAP)SelectObject(hDC, s_SplashHandle);

		// Calculate the coordinates of the rectangle
		int rectLeft = left + cornerRadius;
		int rectTop = top;
		int rectRight = right - cornerRadius;
		int rectBottom = bottom;

		// Draw the rounded rectangle
		HBRUSH NewBrush = CreateSolidBrush(color);
		HPEN nullPen = (HPEN)GetStockObject(border ? BLACK_PEN : NULL_PEN);
		SelectObject(hDC, NewBrush);
		SelectObject(hDC, nullPen);

		RoundRect(hDC, left, top, right, bottom, cornerRadius, cornerRadius);
		
		DeleteObject(NewBrush);

		SelectObject(hDC, hBmpOld);
		ReleaseDC(s_SplashHandle, hDC);
	}
	
	void Splash::Update(const std::string& message, int progress)
	{
		if (!s_SplashHandle)
			return;

		const int padding = 25;
		const int fontSize = 14;
		const int fontHeight = fontSize + 2;

		// Draw image background
		SplashDrawImage(s_SplashBitmap, RECT{ 0, 0, LONG(s_SplashWidth), LONG(s_SplashHeight) });

		// Draw tinted image background
		SplashDrawImage(s_SplashBitmapTinted, RECT{ 0, LONG(s_SplashHeight - 20 - fontHeight * 2 - 10), LONG(s_SplashWidth), LONG(s_SplashHeight) });
		
		// Update Text
		{
			
			SplashDrawText(progress >= 0 ? fmt::format("{} ({}%)", message, progress) : message, { LONG(padding), LONG(s_SplashHeight - 20 - fontHeight), LONG(s_SplashWidth), LONG(s_SplashHeight - 20) }, fontSize, TEXT("Lato"));
			SplashDrawText(s_SplashApplicationName, {LONG(padding), LONG(s_SplashHeight - 20 - fontHeight * 2), LONG(s_SplashWidth), LONG(s_SplashHeight - 20 - fontHeight)}, fontSize, TEXT("Tahoma"), FW_BOLD);
			SplashDrawText("© Dymatic Technologies 2023", { LONG(s_SplashWidth - 167), LONG(s_SplashHeight - 20 - fontHeight), LONG(s_SplashWidth), LONG(s_SplashHeight - 20) }, fontSize, TEXT("Lato"), FW_THIN);
			SplashDrawText("Version 23.1.0", { LONG(s_SplashWidth - 90), LONG(fontHeight), LONG(s_SplashWidth), LONG(fontHeight + fontHeight) }, fontSize, TEXT("Lato"), FW_THIN);
		}

		// Draw Progress Bar
		if (progress >= 0)
		{
			const int rounding = 10;
			
			const int width = 7;
			const int minX = padding;
			const int maxX = s_SplashWidth - padding;
			const int minY = s_SplashHeight - 10 - width;
			const int maxY = s_SplashHeight - 10;
			
			const float darkerMultiplier = 0.65f;
			const float lighterMultiplier = 1.15f;

			SplashDrawRect(minX, minY, maxX, maxY, RGB(s_SplashDominantRed * darkerMultiplier, s_SplashDominantGreen * darkerMultiplier, s_SplashDominantBlue * darkerMultiplier), rounding, true, 0);
			SplashDrawRect(minX, minY, minX + ((maxX - minX) * (progress / 100.0f)), maxY, RGB(s_SplashDominantRed * lighterMultiplier, s_SplashDominantGreen * lighterMultiplier, s_SplashDominantBlue * lighterMultiplier), rounding, true);
		}
	}
	
	void Splash::Shutdown()
	{
		if (s_SplashHandle)
		{
			DestroyWindow(s_SplashHandle);
			s_SplashHandle = NULL;
			UnregisterClass(s_SplashlpszClassName, ::GetModuleHandle(NULL));
		}
	}

#pragma endregion

#pragma region Process
	void Process::CreateApplicationProcess(const std::filesystem::path& path, std::vector<std::string> args)
	{
		// Additional information
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;

		// Set the size of the structures
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		// Setup Arguments (First arg is executable path)
		std::string argString = path.string();
		for (auto& arg : args)
			argString = argString + " " + arg;

		LPSTR commandLine = strdup(argString.c_str());

		// Start the program up
		CreateProcessA(NULL,// The path
			commandLine,	// Command line
			NULL,			// Process handle not inheritable
			NULL,			// Thread handle not inheritable
			FALSE,			// Set handle inheritance to FALSE
			0,				// No creation flags
			NULL,			// Use parent's environment block
			NULL,			// Use parent's starting directory 
			&si,			// Pointer to STARTUPINFO structure
			&pi				// Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
		);

		// Close process and thread handles. 
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
#pragma endregion

#pragma region System
	std::string System::Execute(const std::string& command)
	{
		std::array<char, 256> buffer;
		std::string result;
		std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
		if (!pipe)
			throw std::runtime_error("popen() failed!");
		while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
			result += buffer.data();
		return result;
	}
#pragma endregion

#pragma region Time
	float Time::GetTime()
	{
		return glfwGetTime();
	}
#pragma endregion

}