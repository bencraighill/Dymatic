include "./vendor/premake/premake_customization/solution_items.lua"

workspace "Dymatic"
	architecture "x86_64"
	startproject "DymaticEditor"
	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}


	solution_items
	{
		".editorconfig"
	}

	flags
	{
		"MultiProcessorCompile"
	}
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"



IncludeDir = {}
IncludeDir["GLFW"] = "%{wks.location}/Dymatic/vendor/GLFW/include"
IncludeDir["Glad"] = "%{wks.location}/Dymatic/vendor/Glad/include"
IncludeDir["ImGui"] = "%{wks.location}/Dymatic/vendor/imgui"
IncludeDir["ImGuiNode"] = "%{wks.location}/Dymatic/vendor/ImGuiNode"
IncludeDir["glm"] = "%{wks.location}/Dymatic/vendor/glm"
IncludeDir["stb_image"] = "%{wks.location}/Dymatic/vendor/stb_image"
IncludeDir["entt"] = "%{wks.location}/Dymatic/vendor/entt/include"
IncludeDir["CSplash"] = "%{wks.location}/Dymatic/vendor/CSplash"
IncludeDir["WinToast"] = "%{wks.location}/Dymatic/vendor/WinToast"
IncludeDir["yaml_cpp"] = "%{wks.location}/Dymatic/vendor/yaml-cpp/include"
IncludeDir["ImGuizmo"] = "%{wks.location}/Dymatic/vendor/ImGuizmo"

group "Dependencies"
	include "vendor/premake"
	include "Dymatic/vendor/GLFW"
	include "Dymatic/vendor/Glad"
	include "Dymatic/vendor/imgui"
	include "Dymatic/vendor/yaml-cpp"
group ""

include "Dymatic"
include "Sandbox"
include "DymaticEditor"