 workspace "Dymatic"
	architecture "x64"
	startproject "Sandbox"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}
	

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "Dymatic/vendor/GLFW/include"
IncludeDir["Glad"] = "Dymatic/vendor/Glad/include"
IncludeDir["ImGui"] = "Dymatic/vendor/imgui"
IncludeDir["glm"] = "Dymatic/vendor/glm"
IncludeDir["stb_image"] = "Dymatic/vendor/stb_image"

include "Dymatic/vendor/GLFW"
include "Dymatic/vendor/Glad"
include "Dymatic/vendor/imgui"




project "Dymatic"
	location "Dymatic"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "dypch.h"
	pchsource "Dymatic/src/dypch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/stb_image/**.h",
		"%{prj.name}/vendor/stb_image/**.cpp",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl"
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb_image}"
	}

	links
	{
		"GLFW",
		"Glad",
		"ImGui",
		"opengl32.lib"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"DY_PLATFORM_WINDOWS",
			"DY_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

	filter "configurations:Debug"
		 defines "DY_DEBUG"
		 runtime "Debug"
		 symbols "on"

	filter "configurations:Release"
		 defines "DY_RELEASE"
		 runtime "Release"
		 optimize "on"

	filter "configurations:Dist"
		 defines "DY_DIST"
		 runtime "Release"
		 optimize "on"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"Dymatic/vendor/spdlog/include",
		"Dymatic/src",
		"Dymatic/vendor",
		"%{IncludeDir.glm}"
	}

	links
	{
		"Dymatic"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"DY_PLATFORM_WINDOWS"
		}


	filter "configurations:Debug"
		 defines "DY_DEBUG"
		 runtime "Debug"
		 symbols "on"

	filter "configurations:Release"
		 defines "DY_RELEASE"
		 runtime "Release"
		 optimize "on"

	filter "configurations:Dist"
		 defines "DY_DIST"
		 runtime "Release"
		 optimize "on"
