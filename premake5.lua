include "./vendor/premake/premake_customization/solution_items.lua"
include "Dependencies.lua"

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
include "DymaticLauncher"