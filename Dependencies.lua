
-- Dymatic Dependencies

VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["stb_image"] = "%{wks.location}/Dymatic/vendor/stb_image"
IncludeDir["yaml_cpp"] = "%{wks.location}/Dymatic/vendor/yaml-cpp/include"
IncludeDir["GLFW"] = "%{wks.location}/Dymatic/vendor/GLFW/include"
IncludeDir["Glad"] = "%{wks.location}/Dymatic/vendor/Glad/include"
IncludeDir["ImGui"] = "%{wks.location}/Dymatic/vendor/ImGui"
IncludeDir["ImGuizmo"] = "%{wks.location}/Dymatic/vendor/ImGuizmo"
IncludeDir["glm"] = "%{wks.location}/Dymatic/vendor/glm"
IncludeDir["entt"] = "%{wks.location}/Dymatic/vendor/entt/include"
IncludeDir["Box2D"] = "%{wks.location}/Dymatic/vendor/Box2D/include"
IncludeDir["filewatch"] = "%{wks.location}/Dymatic/vendor/filewatch"
IncludeDir["mono"] = "%{wks.location}/Dymatic/vendor/mono/include"
IncludeDir["shaderc"] = "%{wks.location}/Dymatic/vendor/shaderc/include"
IncludeDir["SPIRV_Cross"] = "%{wks.location}/Dymatic/vendor/SPIRV-Cross"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
IncludeDir["assimp"] = "%{wks.location}/Dymatic/vendor/assimp/include"
IncludeDir["irrKlang"] = "%{wks.location}/Dymatic/vendor/irrKlang/include"
IncludeDir["SDL"] = "%{wks.location}/Dymatic/vendor/SDL/include"
IncludeDir["PhysX"] = "%{wks.location}/Dymatic/vendor/PhysX/physx/include"
IncludeDir["PhysXShared"] = "%{wks.location}/Dymatic/vendor/PhysX/pxshared/include"
IncludeDir["PhysXToolkit"] = "%{wks.location}/Dymatic/vendor/PhysX/physx/samples/pxtoolkit/include"
IncludeDir["PhysXFoundation"] = "%{wks.location}/Dymatic/vendor/PhysX/physx/source/foundation/include"
IncludeDir["msdf_atlas_gen"] = "%{wks.location}/Dymatic/vendor/msdf-atlas-gen/msdf-atlas-gen"
IncludeDir["msdfgen"] = "%{wks.location}/Dymatic/vendor/msdf-atlas-gen/msdfgen"
IncludeDir["ImGuiNode"] = "%{wks.location}/Dymatic/vendor/ImGuiNode"

LibraryDir = {}

LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"
LibraryDir["mono"] = "%{wks.location}/Dymatic/vendor/mono/lib/%{cfg.buildcfg}"

LibraryDir["PhysX_Debug"] = "%{wks.location}/Dymatic/vendor/PhysX/Lib/debug"
LibraryDir["PhysX_DebugDLL"] = "%{wks.location}/Dymatic/vendor/PhysX/Bin/debug"
LibraryDir["PhysX_Release"] = "%{wks.location}/Dymatic/vendor/PhysX/Lib/release"
LibraryDir["PhysX_ReleaseDLL"] = "%{wks.location}/Dymatic/vendor/PhysX/Bin/release"

LibraryDir["assimp_DebugDLL"] = "%{wks.location}/Dymatic/vendor/assimp/Bin/Debug"
LibraryDir["assimp_ReleaseDLL"] = "%{wks.location}/Dymatic/vendor/assimp/Bin/Release"
LibraryDir["irrKlang_DLL"] = "%{wks.location}/Dymatic/vendor/irrKlang/Bin"

Library = {}

-- Mono
Library["mono"] = "%{LibraryDir.mono}/libmono-static-sgen.lib"

-- Vulkan and SpirV
Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
Library["VulkanUtils"] = "%{LibraryDir.VulkanSDK}/VkLayer_utils.lib"

Library["ShaderC_Debug"] = "%{LibraryDir.VulkanSDK}/shaderc_sharedd.lib"
Library["SPIRV_Cross_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsld.lib"
Library["SPIRV_Tools_Debug"] = "%{LibraryDir.VulkanSDK}/SPIRV-Toolsd.lib"

Library["ShaderC_Release"] = "%{LibraryDir.VulkanSDK}/shaderc_shared.lib"
Library["SPIRV_Cross_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsl.lib"

-- Windows
Library["WinSock"] = "Ws2_32.lib"
Library["WinMM"] = "Winmm.lib"
Library["WinVersion"] = "Version.lib"
Library["BCrypt"] = "Bcrypt.lib"
Library["SetupAPI"] = "Setupapi.lib"
Library["HID"] = "Hid.lib"
Library["DbgHelp"] = "DbgHelp.lib"

-- PhysX
Library["PhysX_Debug_LowLevel_static_64"] = "%{LibraryDir.PhysX_Debug}/LowLevel_static_64.lib"
Library["PhysX_Debug_LowLevelAABB_static_64"] = "%{LibraryDir.PhysX_Debug}/LowLevelAABB_static_64.lib"
Library["PhysX_Debug_LowLevelDynamics_static_64"] = "%{LibraryDir.PhysX_Debug}/LowLevelDynamics_static_64.lib"
Library["PhysX_Debug_PhysX_64"] = "%{LibraryDir.PhysX_Debug}/PhysX_64.lib"
Library["PhysX_Debug_PhysXCharacterKinematic_static_64"] = "%{LibraryDir.PhysX_Debug}/PhysXCharacterKinematic_static_64.lib"
Library["PhysX_Debug_PhysXCommon_64"] = "%{LibraryDir.PhysX_Debug}/PhysXCommon_64.lib"
Library["PhysX_Debug_PhysXCooking_64"] = "%{LibraryDir.PhysX_Debug}/PhysXCooking_64.lib"
Library["PhysX_Debug_PhysXExtensions_static_64"] = "%{LibraryDir.PhysX_Debug}/PhysXExtensions_static_64.lib"
Library["PhysX_Debug_PhysXFoundation_64"] = "%{LibraryDir.PhysX_Debug}/PhysXFoundation_64.lib"
Library["PhysX_Debug_PhysXPvdSDK_static_64"] = "%{LibraryDir.PhysX_Debug}/PhysXPvdSDK_static_64.lib"
Library["PhysX_Debug_PhysXTask_static_64"] = "%{LibraryDir.PhysX_Debug}/PhysXTask_static_64.lib"
Library["PhysX_Debug_PhysXVehicle_static_64"] = "%{LibraryDir.PhysX_Debug}/PhysXVehicle_static_64.lib"
Library["PhysX_Debug_SceneQuery_static_64"] = "%{LibraryDir.PhysX_Debug}/SceneQuery_static_64.lib"
Library["PhysX_Debug_SimulationController_static_64"] = "%{LibraryDir.PhysX_Debug}/SimulationController_static_64.lib"

Library ["PhysX_Release_LowLevel_static_64"] = "%{LibraryDir.PhysX_Release}/LowLevel_static_64.lib"
Library ["PhysX_Release_LowLevelAABB_static_64"] = "%{LibraryDir.PhysX_Release}/LowLevelAABB_static_64.lib"
Library ["PhysX_Release_LowLevelDynamics_static_64"] = "%{LibraryDir.PhysX_Release}/LowLevelDynamics_static_64.lib"
Library ["PhysX_Release_PhysX_64"] = "%{LibraryDir.PhysX_Release}/PhysX_64.lib"
Library ["PhysX_Release_PhysXCharacterKinematic_static_64"] = "%{LibraryDir.PhysX_Release}/PhysXCharacterKinematic_static_64.lib"
Library ["PhysX_Release_PhysXCommon_64"] = "%{LibraryDir.PhysX_Release}/PhysXCommon_64.lib"
Library ["PhysX_Release_PhysXCooking_64"] = "%{LibraryDir.PhysX_Release}/PhysXCooking_64.lib"
Library ["PhysX_Release_PhysXExtensions_static_64"] = "%{LibraryDir.PhysX_Release}/PhysXExtensions_static_64.lib"
Library ["PhysX_Release_PhysXFoundation_64"] = "%{LibraryDir.PhysX_Release}/PhysXFoundation_64.lib"
Library ["PhysX_Release_PhysXPvdSDK_static_64"] = "%{LibraryDir.PhysX_Release}/PhysXPvdSDK_static_64.lib"
Library ["PhysX_Release_PhysXTask_static_64"] = "%{LibraryDir.PhysX_Release}/PhysXTask_static_64.lib"
Library ["PhysX_Release_PhysXVehicle_static_64"] = "%{LibraryDir.PhysX_Release}/PhysXVehicle_static_64.lib"
Library ["PhysX_Release_SceneQuery_static_64"] = "%{LibraryDir.PhysX_Release}/SceneQuery_static_64.lib"
Library ["PhysX_Release_SimulationController_static_64"] = "%{LibraryDir.PhysX_Release}/SimulationController_static_64.lib"

-- Assimp
Library["assimp_Debug"] = "%{wks.location}/Dymatic/vendor/assimp/Lib/Debug/assimp.lib"
Library["assimp_Release"] = "%{wks.location}/Dymatic/vendor/assimp/Lib/Release/assimp.lib"

-- IrrKlang
Library["irrKlang"] = "%{wks.location}/Dymatic/vendor/irrKlang/Lib/irrKlang.lib"

-- SDL
Library["SDL_Debug"] = "%{wks.location}/Dymatic/vendor/SDL/Lib/Debug/SDL2-staticd.lib"
Library["SDL_Release"] = "%{wks.location}/Dymatic/vendor/SDL/Lib/Release/SDL2-static.lib"