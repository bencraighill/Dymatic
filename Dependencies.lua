
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
IncludeDir["Recast"] = "%{wks.location}/Dymatic/vendor/Recast/Recast/include"
IncludeDir["Detour"] = "%{wks.location}/Dymatic/vendor/Recast/Detour/include"
IncludeDir["DetourCrowd"] = "%{wks.location}/Dymatic/vendor/Recast/DetourCrowd/include"
IncludeDir["DetourTileCache"] = "%{wks.location}/Dymatic/vendor/Recast/DetourTileCache/include"
IncludeDir["irrKlang"] = "%{wks.location}/Dymatic/vendor/irrKlang/include"
IncludeDir["OpenAL"] = "%{wks.location}/Dymatic/vendor/OpenAL/include"
IncludeDir["SDL"] = "%{wks.location}/Dymatic/vendor/SDL/include"
IncludeDir["ffmpeg"] = "%{wks.location}/Dymatic/vendor/ffmpeg/include"
IncludeDir["DualSenseWindows"] = "%{wks.location}/Dymatic/vendor/DualSenseWindows/include"
IncludeDir["GameNetworkingSockets"] = "%{wks.location}/Dymatic/vendor/GameNetworkingSockets/include"
IncludeDir["Jolt"] = "%{wks.location}/Dymatic/vendor/JoltPhysics/include"
IncludeDir["FSR2"] = "%{wks.location}/Dymatic/vendor/FSR2/include"
IncludeDir["msdf_atlas_gen"] = "%{wks.location}/Dymatic/vendor/msdf-atlas-gen/msdf-atlas-gen"
IncludeDir["msdfgen"] = "%{wks.location}/Dymatic/vendor/msdf-atlas-gen/msdfgen"
IncludeDir["ImGuiNode"] = "%{wks.location}/Dymatic/vendor/ImGuiNode"

LibraryDir = {}

LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"
LibraryDir["mono"] = "%{wks.location}/Dymatic/vendor/mono/lib/%{cfg.buildcfg}"

LibraryDir["assimp_DebugDLL"] = "%{wks.location}/Dymatic/vendor/assimp/Bin/Debug"
LibraryDir["assimp_ReleaseDLL"] = "%{wks.location}/Dymatic/vendor/assimp/Bin/Release"

LibraryDir["irrKlang_DLL"] = "%{wks.location}/Dymatic/vendor/irrKlang/Bin"

LibraryDir["GameNetworkingSockets_DebugDLL"] = "%{wks.location}/Dymatic/vendor/GameNetworkingSockets/Bin/Debug"
LibraryDir["GameNetworkingSockets_ReleaseDLL"] = "%{wks.location}/Dymatic/vendor/GameNetworkingSockets/Bin/Release"

LibraryDir["OpenAL_Debug"] = "%{wks.location}/Dymatic/vendor/OpenAL/Lib/Debug"
LibraryDir["OpenAL_DebugDLL"] = "%{wks.location}/Dymatic/vendor/OpenAL/Bin/Debug"
LibraryDir["OpenAL_Release"] = "%{wks.location}/Dymatic/vendor/OpenAL/Lib/Release"
LibraryDir["OpenAL_ReleaseDLL"] = "%{wks.location}/Dymatic/vendor/OpenAL/Bin/Release"

LibraryDir["ffmpeg_DLL"] = "%{wks.location}/Dymatic/vendor/ffmpeg/Bin"

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
Library["WLanAPI"] = "wlanapi.lib"
Library["WinMM"] = "Winmm.lib"
Library["WinVersion"] = "Version.lib"
Library["BCrypt"] = "Bcrypt.lib"
Library["SetupAPI"] = "Setupapi.lib"
Library["HID"] = "Hid.lib"
Library["DbgHelp"] = "DbgHelp.lib"

-- Jolt
Library["Jolt_Debug"] = "%{wks.location}/Dymatic/vendor/JoltPhysics/Lib/Debug/Jolt.lib"
Library["Jolt_Release"] = "%{wks.location}/Dymatic/vendor/JoltPhysics/Lib/Release/Jolt.lib"
Library["Jolt_Dist"] = "%{wks.location}/Dymatic/vendor/JoltPhysics/Lib/Dist/Jolt.lib"

-- Assimp
Library["assimp_Debug"] = "%{wks.location}/Dymatic/vendor/assimp/Lib/Debug/assimp.lib"
Library["assimp_Release"] = "%{wks.location}/Dymatic/vendor/assimp/Lib/Release/assimp.lib"

-- Recast/Detour
Library["Recast_Debug"] = "%{wks.location}/Dymatic/vendor/Recast/Lib/Debug/Recast.lib"
Library["Detour_Debug"] = "%{wks.location}/Dymatic/vendor/Recast/Lib/Debug/Detour.lib"
Library["DetourCrowd_Debug"] = "%{wks.location}/Dymatic/vendor/Recast/Lib/Debug/DetourCrowd.lib"
Library["DetourTileCache_Debug"] = "%{wks.location}/Dymatic/vendor/Recast/Lib/Debug/DetourTileCache.lib"

Library["Recast_Release"] = "%{wks.location}/Dymatic/vendor/Recast/Lib/Release/Recast.lib"
Library["Detour_Release"] = "%{wks.location}/Dymatic/vendor/Recast/Lib/Release/Detour.lib"
Library["DetourCrowd_Release"] = "%{wks.location}/Dymatic/vendor/Recast/Lib/Release/DetourCrowd.lib"
Library["DetourTileCache_Release"] = "%{wks.location}/Dymatic/vendor/Recast/Lib/Release/DetourTileCache.lib"

-- IrrKlang
Library["irrKlang"] = "%{wks.location}/Dymatic/vendor/irrKlang/Lib/irrKlang.lib"

-- OpenAL
Library["OpenAL_Debug_common"] = "%{LibraryDir.OpenAL_Debug}/common.lib"
Library["OpenAL_Debug_ex_common"] = "%{LibraryDir.OpenAL_Debug}/ex-common.lib"
Library["OpenAL_Debug_OpenAL32"] = "%{LibraryDir.OpenAL_Debug}/OpenAL32.lib"

Library["OpenAL_Release_common"] = "%{LibraryDir.OpenAL_Release}/common.lib"
Library["OpenAL_Release_ex_common"] = "%{LibraryDir.OpenAL_Release}/ex-common.lib"
Library["OpenAL_Release_OpenAL32"] = "%{LibraryDir.OpenAL_Release}/OpenAL32.lib"

-- SDL
Library["SDL_Debug"] = "%{wks.location}/Dymatic/vendor/SDL/Lib/Debug/SDL2-staticd.lib"
Library["SDL_Release"] = "%{wks.location}/Dymatic/vendor/SDL/Lib/Release/SDL2-static.lib"

-- ffmpeg
Library["ffmpeg_avcodec"] = "%{wks.location}/Dymatic/vendor/ffmpeg/Lib/avcodec.lib"
Library["ffmpeg_avdevice"] = "%{wks.location}/Dymatic/vendor/ffmpeg/Lib/avdevice.lib"
Library["ffmpeg_avfilter"] = "%{wks.location}/Dymatic/vendor/ffmpeg/Lib/avfilter.lib"
Library["ffmpeg_avformat"] = "%{wks.location}/Dymatic/vendor/ffmpeg/Lib/avformat.lib"
Library["ffmpeg_avutil"] = "%{wks.location}/Dymatic/vendor/ffmpeg/Lib/avutil.lib"
Library["ffmpeg_postproc"] = "%{wks.location}/Dymatic/vendor/ffmpeg/Lib/postproc.lib"
Library["ffmpeg_swresample"] = "%{wks.location}/Dymatic/vendor/ffmpeg/Lib/swresample.lib"
Library["ffmpeg_swscale"] = "%{wks.location}/Dymatic/vendor/ffmpeg/Lib/swscale.lib"

-- FSR2
Library["FSR2_api_Debug"] = "%{wks.location}/Dymatic/vendor/FSR2/lib/debug/ffx_fsr2_api_x64d.lib"
Library["FSR2_api_gl_Debug"] = "%{wks.location}/Dymatic/vendor/FSR2/lib/debug/ffx_fsr2_api_gl_x64d.lib"
Library["FSR2_api_Release"] = "%{wks.location}/Dymatic/vendor/FSR2/lib/release/ffx_fsr2_api_x64.lib"
Library["FSR2_api_gl_Release"] = "%{wks.location}/Dymatic/vendor/FSR2/lib/release/ffx_fsr2_api_gl_x64.lib"

-- DualSenseWindows
Library["DualSenseWindows_Debug"] = "%{wks.location}/Dymatic/vendor/DualSenseWindows/Lib/Debug/ds5w_x64.lib"
Library["DualSenseWindows_Release"] = "%{wks.location}/Dymatic/vendor/DualSenseWindows/Lib/Release/ds5w_x64.lib"

-- GameNetworkingSockets
Library["GameNetworkingSockets_Debug"] = "%{wks.location}/Dymatic/vendor/GameNetworkingSockets/Lib/Debug/GameNetworkingSockets.lib"
Library["GameNetworkingSockets_s_Debug"] = "%{wks.location}/Dymatic/vendor/GameNetworkingSockets/Lib/Debug/GameNetworkingSockets_s.lib"
Library["GameNetworkingSockets_Release"] = "%{wks.location}/Dymatic/vendor/GameNetworkingSockets/Lib/Release/GameNetworkingSockets.lib"
Library["GameNetworkingSockets_s_Release"] = "%{wks.location}/Dymatic/vendor/GameNetworkingSockets/Lib/Release/GameNetworkingSockets_s.lib"