# pragma once
# include <imgui/imgui.h>

namespace ax {
namespace Drawing {

enum class IconType: ImU32 { Flow, Circle, Square, Grid, RoundSquare, Diamond, Capsule, Braces };

void DrawIcon(ImDrawList* drawList, const ImVec2& a, const ImVec2& b, IconType type, bool filled, ImU32 color, ImU32 innerColor);

} // namespace Drawing
} // namespace ax