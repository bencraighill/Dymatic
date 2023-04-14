project "DymaticRuntime"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp",
		"DymaticRuntime.rc"
	}

	includedirs
	{
		"%{wks.location}/DymaticRuntime/src",

		"%{wks.location}/Dymatic/vendor/spdlog/include",
		"%{wks.location}/Dymatic/src",
		"%{wks.location}/Dymatic/vendor",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.filewatch}",
		"%{IncludeDir.assimp}",
		"%{IncludeDir.yaml_cpp}",
	}

	links
	{
		"Dymatic"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines "DY_DEBUG"
		runtime "Debug"
		symbols "on"
		
		postbuildcommands
		{
			"{COPYDIR} \"%{LibraryDir.VulkanSDK_DebugDLL}\" \"%{cfg.targetdir}\"",
			"{COPYDIR} \"%{LibraryDir.PhysX_DebugDLL}\" \"%{cfg.targetdir}\"",
			"{COPYDIR} \"%{LibraryDir.assimp_DebugDLL}\" \"%{cfg.targetdir}\"",
			"{COPYDIR} \"%{LibraryDir.irrKlang_DLL}\" \"%{cfg.targetdir}\""
		}

	filter "configurations:Release"
		defines "DY_RELEASE"
		runtime "Release"
		optimize "on"

		postbuildcommands
		{
			"{COPYDIR} \"%{LibraryDir.PhysX_ReleaseDLL}\" \"%{cfg.targetdir}\"",
			"{COPYDIR} \"%{LibraryDir.assimp_ReleaseDLL}\" \"%{cfg.targetdir}\"",
			"{COPYDIR} \"%{LibraryDir.irrKlang_DLL}\" \"%{cfg.targetdir}\""
		}

	filter "configurations:Dist"
		defines "DY_DIST"
		runtime "Release"
		optimize "on"

		postbuildcommands
		{
			"{COPYDIR} \"%{LibraryDir.PhysX_ReleaseDLL}\" \"%{cfg.targetdir}\"",
			"{COPYDIR} \"%{LibraryDir.assimp_ReleaseDLL}\" \"%{cfg.targetdir}\"",
			"{COPYDIR} \"%{LibraryDir.irrKlang_DLL}\" \"%{cfg.targetdir}\""
		}