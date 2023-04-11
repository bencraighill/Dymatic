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

		"vendor/CSplash/**.h",
		"vendor/CSplash/**.cpp",
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
		"%{IncludeDir.Box2D}",
		"%{IncludeDir.irrKlang}",
		"%{IncludeDir.SDL}",
		"%{IncludeDir.PhysX}",
		"%{IncludeDir.PhysXShared}",
		"%{IncludeDir.PhysXToolkit}",
		"%{IncludeDir.PhysXFoundation}",
		"%{IncludeDir.msdf_atlas_gen}",
		"%{IncludeDir.msdfgen}",
		"%{IncludeDir.filewatch}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.ImGuiNode}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.CSplash}",
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

			"%{Library.SDL_Debug}",

			"%{Library.ShaderC_Debug}",
			"%{Library.SPIRV_Cross_Debug}",
			"%{Library.SPIRV_Cross_GLSL_Debug}",
			
			"%{Library.PhysX_Debug_LowLevel_static_64}",
			"%{Library.PhysX_Debug_LowLevelAABB_static_64}",
			"%{Library.PhysX_Debug_LowLevelDynamics_static_64}",
			"%{Library.PhysX_Debug_PhysX_64}",
			"%{Library.PhysX_Debug_PhysXCharacterKinematic_static_64}",
			"%{Library.PhysX_Debug_PhysXCommon_64}",
			"%{Library.PhysX_Debug_PhysXCooking_64}",
			"%{Library.PhysX_Debug_PhysXExtensions_static_64}",
			"%{Library.PhysX_Debug_PhysXFoundation_64}",
			"%{Library.PhysX_Debug_PhysXPvdSDK_static_64}",
			"%{Library.PhysX_Debug_PhysXTask_static_64}",
			"%{Library.PhysX_Debug_PhysXVehicle_static_64}",
			"%{Library.PhysX_Debug_SceneQuery_static_64}",
			"%{Library.PhysX_Debug_SimulationController_static_64}"
		}

	filter "configurations:Release"
		defines "DY_RELEASE"
		runtime "Release"
		optimize "on"

		links
		{
			"%{Library.assimp_Release}",

			"%{Library.SDL_Release}",

			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}",

			"%{Library.PhysX_Release_SimulationController_static_64}",
			"%{Library.PhysX_Release_LowLevel_static_64}",
			"%{Library.PhysX_Release_LowLevelAABB_static_64}",
			"%{Library.PhysX_Release_LowLevelDynamics_static_64}",
			"%{Library.PhysX_Release_PhysX_64}",
			"%{Library.PhysX_Release_PhysXCharacterKinematic_static_64}",
			"%{Library.PhysX_Release_PhysXCommon_64}",
			"%{Library.PhysX_Release_PhysXCooking_64}",
			"%{Library.PhysX_Release_PhysXExtensions_static_64}",
			"%{Library.PhysX_Release_PhysXFoundation_64}",
			"%{Library.PhysX_Release_PhysXPvdSDK_static_64}",
			"%{Library.PhysX_Release_PhysXTask_static_64}",
			"%{Library.PhysX_Release_PhysXVehicle_static_64}",
			"%{Library.PhysX_Release_SceneQuery_static_64}"
		}

	filter "configurations:Dist"
		defines "DY_DIST"
		runtime "Release"
		optimize "on"

		links
		{
			"%{Library.assimp_Release}",

			"%{Library.SDL_Debug}",

			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}",

			"%{Library.PhysX_Release_SimulationController_static_64}",
			"%{Library.PhysX_Release_LowLevel_static_64}",
			"%{Library.PhysX_Release_LowLevelAABB_static_64}",
			"%{Library.PhysX_Release_LowLevelDynamics_static_64}",
			"%{Library.PhysX_Release_PhysX_64}",
			"%{Library.PhysX_Release_PhysXCharacterKinematic_static_64}",
			"%{Library.PhysX_Release_PhysXCommon_64}",
			"%{Library.PhysX_Release_PhysXCooking_64}",
			"%{Library.PhysX_Release_PhysXExtensions_static_64}",
			"%{Library.PhysX_Release_PhysXFoundation_64}",
			"%{Library.PhysX_Release_PhysXPvdSDK_static_64}",
			"%{Library.PhysX_Release_PhysXTask_static_64}",
			"%{Library.PhysX_Release_PhysXVehicle_static_64}",
			"%{Library.PhysX_Release_SceneQuery_static_64}"
		}