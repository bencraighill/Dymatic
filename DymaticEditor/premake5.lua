project "DymaticEditor"
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
		"DymaticEditor.rc"
	}

	includedirs
	{
		"%{wks.location}/DymaticEditor/src",

		"%{wks.location}/Dymatic/vendor/spdlog/include",
		"%{wks.location}/Dymatic/src",
		"%{wks.location}/Dymatic/vendor",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.filewatch}",
		"%{IncludeDir.assimp}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.ImGuiNode}",

		"%{wks.location}/DymaticEditor/vendor/git2/include",
		"%{wks.location}/DymaticEditor/vendor/pybind11/include",
		"%{wks.location}/DymaticEditor/vendor/Python/include"
	}

	links
	{
		"Dymatic",

		"vendor/git2/Lib/git2.lib"
	}

	libdirs
	{
		"vendor/Python/lib"
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
			"{COPYDIR} \"%{LibraryDir.irrKlang_DLL}\" \"%{cfg.targetdir}\"",

			"{COPYDIR} \"%{wks.location}/DymaticEditor/vendor/git2/Bin/git2.dll\" \"%{cfg.targetdir}\"",
			"{COPYDIR} \"%{wks.location}/DymaticEditor/vendor/Python/bin/\" \"%{cfg.targetdir}\""
		}

	filter "configurations:Release"
		defines "DY_RELEASE"
		runtime "Release"
		optimize "on"

		postbuildcommands
		{
			"{COPYDIR} \"%{LibraryDir.PhysX_ReleaseDLL}\" \"%{cfg.targetdir}\"",
			"{COPYDIR} \"%{LibraryDir.assimp_ReleaseDLL}\" \"%{cfg.targetdir}\"",
			"{COPYDIR} \"%{LibraryDir.irrKlang_DLL}\" \"%{cfg.targetdir}\"",

			"{COPYDIR} \"%{wks.location}/DymaticEditor/vendor/git2/Bin/git2.dll\" \"%{cfg.targetdir}\"",
			"{COPYDIR} \"%{wks.location}/DymaticEditor/vendor/Python/bin/\" \"%{cfg.targetdir}\""
		}

	filter "configurations:Dist"
		defines "DY_DIST"
		runtime "Release"
		optimize "on"

		postbuildcommands
		{
			"{COPYDIR} \"%{LibraryDir.PhysX_ReleaseDLL}\" \"%{cfg.targetdir}\"",
			"{COPYDIR} \"%{LibraryDir.assimp_ReleaseDLL}\" \"%{cfg.targetdir}\"",
			"{COPYDIR} \"%{LibraryDir.irrKlang_DLL}\" \"%{cfg.targetdir}\"",

			"{COPYDIR} \"%{wks.location}/DymaticEditor/vendor/git2/Bin/git2.dll\" \"%{cfg.targetdir}\"",
			"{COPYDIR} \"%{wks.location}/DymaticEditor/vendor/Python/bin/\" \"%{cfg.targetdir}\""
		}