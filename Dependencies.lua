
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
IncludeDir["shaderc"] = "%{wks.location}/Dymatic/vendor/shaderc/include"
IncludeDir["SPIRV_Cross"] = "%{wks.location}/Dymatic/vendor/SPIRV-Cross"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
IncludeDir["assimp"] = "%{wks.location}/Dymatic/vendor/assimp/include"
IncludeDir["irrKlang"] = "%{wks.location}/Dymatic/vendor/irrKlang/include"
IncludeDir["PhysX"] = "%{wks.location}/Dymatic/vendor/PhysX/physx/include"
IncludeDir["PhysXShared"] = "%{wks.location}/Dymatic/vendor/PhysX/pxshared/include"

IncludeDir["ImGuiNode"] = "%{wks.location}/Dymatic/vendor/ImGuiNode"
IncludeDir["CSplash"] = "%{wks.location}/Dymatic/vendor/CSplash"


LibraryDir = {}

LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"
LibraryDir["VulkanSDK_Debug"] = "%{wks.location}/Dymatic/vendor/VulkanSDK/Lib"
LibraryDir["VulkanSDK_DebugDLL"] = "%{wks.location}/Dymatic/vendor/VulkanSDK/Bin"

LibraryDir["PhysX_Debug"] = "%{wks.location}/Dymatic/vendor/VulkanSDK/Lib/debug"
LibraryDir["PhysX_DebugDLL"] = "%{wks.location}/Dymatic/vendor/VulkanSDK/Bin/debug"
LibraryDir["PhysX_Release"] = "%{wks.location}/Dymatic/vendor/VulkanSDK/Lib/release"
LibraryDir["PhysX_ReleaseDLL"] = "%{wks.location}/Dymatic/vendor/VulkanSDK/Bin/release"

LibraryDir["irrKlang_DLL"] = "%{wks.location}/Dymatic/vendor/irrKlang/Bin"

Library = {}
Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
Library["VulkanUtils"] = "%{LibraryDir.VulkanSDK}/VkLayer_utils.lib"

Library["ShaderC_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/shaderc_sharedd.lib"
Library["SPIRV_Cross_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/spirv-cross-glsld.lib"
Library["SPIRV_Tools_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/SPIRV-Toolsd.lib"

Library["ShaderC_Release"] = "%{LibraryDir.VulkanSDK}/shaderc_shared.lib"
Library["SPIRV_Cross_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsl.lib"

Library["assimp"] = "%{wks.location}/Dymatic/vendor/assimp/Lib/assimp.lib"
Library["irrKlang"] = "%{wks.location}/Dymatic/vendor/irrKlang/Lib/irrKlang.lib"