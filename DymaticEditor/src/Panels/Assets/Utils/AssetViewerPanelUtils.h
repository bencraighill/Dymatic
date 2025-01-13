#pragma once
#include "Dymatic/Asset/Asset.h"

#include "TextSymbols.h"

#include <filesystem>
#include <spdlog/fmt/fmt.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>

#include "Panels/UI.h"
#include "Dymatic/Core/Application.h"

namespace Dymatic::Utils {

	static std::string GetViewerWindowName(const char* icon, AssetHandle handle, const std::filesystem::path& filepath, const char* viewerType)
	{
		// Allow for window renaming (as the asset name/metadata can be updated at any time)
		return UI::GetVaryingWindowNameWithID(fmt::format("{} {} ({})", icon, filepath.stem().string(), handle), handle);
	}

	static ImVec2 GetMaximizedSize(const ImVec2& actualSize, const ImVec2& maxSize)
	{
		// Calculate the scaling factors
		const float widthScale = maxSize.x / actualSize.x;
		const float heightScale = maxSize.y / actualSize.y;

		// Use the smaller scaling factor to ensure the image fits within the bounds
		const float scale = std::min(widthScale, heightScale);

		// Calculate the new dimensions
		return actualSize * scale;
	}

}