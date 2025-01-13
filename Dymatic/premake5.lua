project "Dymatic"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "dypch.h"
	pchsource "src/dypch.cpp"

	files
	{
		"src/**.h",
		"src/**.cpp",
		"vendor/stb_image/**.h",
		"vendor/stb_image/**.cpp",
		"vendor/glm/glm/**.hpp",
		"vendor/glm/glm/**.inl",

		"vendor/ImGuiNode/**.h",
		"vendor/ImGuiNode/**.cpp",

		"vendor/ImGuizmo/ImGuizmo.h",
		"vendor/ImGuizmo/ImGuizmo.cpp"
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE"
	}

	includedirs
	{
		"src",
		"vendor/spdlog/include",
		"%{IncludeDir.assimp}",
		"%{IncludeDir.Recast}",
		"%{IncludeDir.Detour}",
		"%{IncludeDir.DetourCrowd}",
		"%{IncludeDir.DetourTileCache}",
		"%{IncludeDir.Box2D}",
		"%{IncludeDir.irrKlang}",
		"%{IncludeDir.OpenAL}",
		"%{IncludeDir.SDL}",
		"%{IncludeDir.ffmpeg}",
		"%{IncludeDir.FSR2}",
		"%{IncludeDir.DualSenseWindows}",
		"%{IncludeDir.GameNetworkingSockets}",
		"%{IncludeDir.Jolt}",
		"%{IncludeDir.msdf_atlas_gen}",
		"%{IncludeDir.msdfgen}",
		"%{IncludeDir.filewatch}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.ImGuiNode}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.mono}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.VulkanSDK}"
	}

	links
	{
		"Box2D",
		"GLFW",
		"Glad",
		"ImGui",
		"%{Library.irrKlang}",
		"yaml-cpp",
		"msdf-atlas-gen",
		"msdfgen",
		"opengl32.lib",
		
		"%{Library.mono}",
	}

	filter "files:vendor/ImGuizmo/**.cpp"
	flags { "NoPCH" }

	filter "system:windows"
		systemversion "latest"

		defines
		{
		}

		links
		{
			"%{Library.WinSock}",
			"%{Library.WLanAPI}",
			"%{Library.WinMM}",
			"%{Library.WinVersion}",
			"%{Library.BCrypt}",
			"%{Library.SetupAPI}",
			"%{Library.HID}",
			"%{Library.DbgHelp}",
		}

	filter "configurations:Debug"
		defines "DY_DEBUG"
		runtime "Debug"
		symbols "on"

		links
		{
			"%{Library.assimp_Debug}",

			"%{Library.ShaderC_Debug}",
			"%{Library.SPIRV_Cross_Debug}",
			"%{Library.SPIRV_Cross_GLSL_Debug}",

			"%{Library.Jolt_Debug}",
			
			"%{Library.OpenAL_Debug_common}",
			"%{Library.OpenAL_Debug_ex_common}",
			"%{Library.OpenAL_Debug_OpenAL32}",

			"%{Library.SDL_Debug}",

			"%{Library.Recast_Debug}",
			"%{Library.Detour_Debug}",
			"%{Library.DetourCrowd_Debug}",
			"%{Library.DetourTileCache_Debug}",

			"%{Library.ffmpeg_avcodec}",
			"%{Library.ffmpeg_avdevice}",
			"%{Library.ffmpeg_avfilter}",
			"%{Library.ffmpeg_avformat}",
			"%{Library.ffmpeg_avutil}",
			"%{Library.ffmpeg_postproc}",
			"%{Library.ffmpeg_swresample}",
			"%{Library.ffmpeg_swscale}",

			"%{Library.FSR2_api_Debug}",
			"%{Library.FSR2_api_gl_Debug}",

			"%{Library.DualSenseWindows_Debug}",

			"%{Library.GameNetworkingSockets_Debug}",
			"%{Library.GameNetworkingSockets_s_Debug}"
		}

		defines
		{
			"JPH_FLOATING_POINT_EXCEPTIONS_ENABLED",
			"JPH_DEBUG_RENDERER",
			"JPH_PROFILE_ENABLED",
			"JPH_OBJECT_STREAM",
			"JPH_USE_AVX2",
			"JPH_USE_AVX",
			"JPH_USE_SSE4_1",
			"JPH_USE_SSE4_2",
			"JPH_USE_LZCNT",
			"JPH_USE_TZCNT",
			"JPH_USE_F16C",
			"JPH_USE_FMADD"
		}

		buildoptions
		{
			"/bigobj"
		}

	filter "configurations:Release"
		defines "DY_RELEASE"
		runtime "Release"
		optimize "on"

		links
		{
			"%{Library.assimp_Release}",

			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}",

			"%{Library.Jolt_Release}",

			"%{Library.OpenAL_Release_common}",
			"%{Library.OpenAL_Release_ex_common}",
			"%{Library.OpenAL_Release_OpenAL32}",

			"%{Library.SDL_Release}",

			"%{Library.Recast_Release}",
			"%{Library.Detour_Release}",
			"%{Library.DetourCrowd_Release}",
			"%{Library.DetourTileCache_Release}",

			"%{Library.ffmpeg_avcodec}",
			"%{Library.ffmpeg_avdevice}",
			"%{Library.ffmpeg_avfilter}",
			"%{Library.ffmpeg_avformat}",
			"%{Library.ffmpeg_avutil}",
			"%{Library.ffmpeg_postproc}",
			"%{Library.ffmpeg_swresample}",
			"%{Library.ffmpeg_swscale}",

			"%{Library.FSR2_api_Release}",
			"%{Library.FSR2_api_gl_Release}",

			"%{Library.DualSenseWindows_Release}",

			"%{Library.GameNetworkingSockets_Release}",
			"%{Library.GameNetworkingSockets_s_Release}"
		}

		defines
		{
			"JPH_NO_DEBUG",
			"JPH_FLOATING_POINT_EXCEPTIONS_ENABLED",
			"JPH_DEBUG_RENDERER",
			"JPH_PROFILE_ENABLED",
			"JPH_OBJECT_STREAM",
			"JPH_USE_AVX2",
			"JPH_USE_AVX",
			"JPH_USE_SSE4_1",
			"JPH_USE_SSE4_2",
			"JPH_USE_LZCNT",
			"JPH_USE_TZCNT",
			"JPH_USE_F16C",
			"JPH_USE_FMADD"
		}

	filter "configurations:Dist"
		defines "DY_DIST"
		runtime "Release"
		optimize "on"

		links
		{
			"%{Library.assimp_Release}",

			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}",

			"%{Library.Jolt_Dist}",

			"%{Library.OpenAL_Release_common}",
			"%{Library.OpenAL_Release_ex_common}",
			"%{Library.OpenAL_Release_OpenAL32}",

			"%{Library.SDL_Debug}",

			"%{Library.Recast_Release}",
			"%{Library.Detour_Release}",
			"%{Library.DetourCrowd_Release}",
			"%{Library.DetourTileCache_Release}",

			"%{Library.ffmpeg_avcodec}",
			"%{Library.ffmpeg_avdevice}",
			"%{Library.ffmpeg_avfilter}",
			"%{Library.ffmpeg_avformat}",
			"%{Library.ffmpeg_avutil}",
			"%{Library.ffmpeg_postproc}",
			"%{Library.ffmpeg_swresample}",
			"%{Library.ffmpeg_swscale}",

			"%{Library.FSR2_api_Release}",
			"%{Library.FSR2_api_gl_Release}",

			"%{Library.DualSenseWindows_Release}",

			"%{Library.GameNetworkingSockets_Release}",
			"%{Library.GameNetworkingSockets_s_Release}"
		}

		defines
		{
			"JPH_NO_DEBUG",
			"JPH_OBJECT_STREAM",
			"JPH_USE_AVX2",
			"JPH_USE_AVX",
			"JPH_USE_SSE4_1",
			"JPH_USE_SSE4_2",
			"JPH_USE_LZCNT",
			"JPH_USE_TZCNT",
			"JPH_USE_F16C",
			"JPH_USE_FMADD"
		}