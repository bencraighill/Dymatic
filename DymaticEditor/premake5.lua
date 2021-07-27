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
		"src/**.cpp"
	}

	includedirs
	{
		"%{wks.location}/Dymatic/vendor/spdlog/include",
		"%{wks.location}/Dymatic/src",
		"%{wks.location}/Dymatic/vendor",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.ImGuiNode}"
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

	filter "configurations:Release"
		defines "DY_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "DY_DIST"
		runtime "Release"
		optimize "on"