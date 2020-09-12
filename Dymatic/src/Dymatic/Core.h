#pragma once

#ifdef DY_PLATFORM_WINDOWS
	#ifdef DY_BUILD_DLL
		#define DYMATIC_API __declspec(dllexport)
	#else
		#define DYMATIC_API __declspec(dllimport)
	#endif
#else
	#error Dymatic only supports Windows Opperating Systems!
#endif
