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

include "Dymatic/vendor/GLFW"
include "Dymatic/vendor/Glad"
include "Dymatic/vendor/imgui"




project "Dymatic"
	location "Dymatic"
	kind "SharedLib"
	language "C++"
	staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "dypch.h"
	pchsource "Dymatic/src/dypch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}"
	}

	links
	{
		"GLFW",
		"Glad",
		"ImGui",
		"opengl32.lib"
	}

	filter "system:windows"
		cppdialect "C++17"
		systemversion "latest"

		defines
		{
			"DY_PLATFORM_WINDOWS",
			"DY_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} \"../bin/" .. outputdir .. "/Sandbox/\"")
		}

	filter "configurations:Debug"
		 defines "DY_DEBUG"
		 runtime "Debug"
		 symbols "On"

	filter "configurations:Release"
		 defines "DY_RELEASE"
		 runtime "Release"
		 optimize "On"

	filter "configurations:Dist"
		 defines "DY_DIST"
		 runtime "Release"
		 optimize "On"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	staticruntime "off"

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
		"%{IncludeDir.glm}"
	}

	links
	{
		"Dymatic"
	}

	filter "system:windows"
		cppdialect "C++17"
		systemversion "latest"

		defines
		{
			"DY_PLATFORM_WINDOWS"
		}


	filter "configurations:Debug"
		 defines "DY_DEBUG"
		 runtime "Debug"
		 symbols "On"

	filter "configurations:Release"
		 defines "DY_RELEASE"
		 runtime "Release"
		 optimize "On"

	filter "configurations:Dist"
		 defines "DY_DIST"
		 runtime "Release"
		 optimize "On"
