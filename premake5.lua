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
	include "Dymatic/vendor/Box2D"
	include "Dymatic/vendor/GLFW"
	include "Dymatic/vendor/Glad"
	include "Dymatic/vendor/imgui"
	include "Dymatic/vendor/yaml-cpp"
	
	group "Dependencies/msdf"
		include "Dymatic/vendor/msdf-atlas-gen"
	group ""
group ""

group "Core"
	include "Dymatic"
	include "DymaticScriptCore"
group ""

group "Tools"
	include "CrashManager"
	include "DymaticEditor"
	include "DymaticRuntime"
group ""