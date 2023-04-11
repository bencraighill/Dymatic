local DymaticRootDir = '../../../..'
include (DymaticRootDir .. "/vendor/premake/premake_customization/solution_items.lua")

workspace "{projectName}"
	architecture "x86_64"
	startproject "{projectName}"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "{projectName}"
	kind "SharedLib"
	language "C#"
	dotnetframework "4.7.2"

	targetdir ("Binaries")
	objdir ("Intermediates")

	files 
	{
		"Source/**.cs",
		"Properties/**.cs"
	}

	links
	{
		"Dymatic-ScriptCore"
	}

	filter "configurations:Debug"
		optimize "Off"
		symbols "Default"

	filter "configurations:Release"
		optimize "On"
		symbols "Default"

	filter "configurations:Dist"
		optimize "Full"
		symbols "Off"

group "Dymatic"
	include (DymaticRootDir .. "/Dymatic-ScriptCore")
group ""