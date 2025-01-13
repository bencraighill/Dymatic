#pragma once

#include "Panels/UI.h"
#include <imgui_internal.h>

#include "Dymatic/Asset/Asset.h"

#include "Dymatic/Editor/Compiler.h"
#include "Panels/Nodes/Utilities/Search.h"

namespace Dymatic::UI {

	// TODO: Move to styling
	class GraphConstants
	{
	public:
		static constexpr ImColor BadLinkColor = ImColor(255, 128, 128);
		static constexpr ImColor BadLinkLabelColor = ImColor(45, 32, 32, 180);
		static constexpr ImColor GoodLinkColor = ImColor(110, 180, 255);
		static constexpr ImColor GoodLinkLabelColor = ImColor(32, 32, 45, 180);

		static constexpr ImColor ResultColor = ImColor(166, 143, 119);
		static constexpr ImColor InputNodeColor = ImColor(170, 75, 72);
		static constexpr ImColor PlayerNodeColor = ImColor(80, 122, 72);
		static constexpr ImColor AssetNodeColor = ImColor(75, 150, 200);
		static constexpr ImColor FunctionNodeColor = ImColor(143, 190, 137);
		static constexpr ImColor TransformNodeColor = ImColor(121, 122, 65);
		static constexpr ImColor ParameterNodeColor = ImColor(143, 190, 137);
		static constexpr ImColor SpaceConversionColor = ImColor(200, 200, 200);
		static constexpr ImColor StateNodeColor = ImColor(12, 12, 15);
		static constexpr ImColor ConstantFloatColor = ImColor(78, 113, 43);
		static constexpr ImColor ConstantVectorColor = ImColor(182, 151, 46);

		static constexpr ImColor BasicPinColor = ImColor(255, 255, 255);
		static constexpr ImColor AssetPinColor = ImColor(51, 150, 215);
		static constexpr ImColor BoolPinColor = ImColor(220, 48, 48);
		static constexpr ImColor IntPinColor = ImColor(68, 201, 156);
		static constexpr ImColor FloatPinColor = ImColor(147, 226, 74);
		static constexpr ImColor VectorPinColor = ImColor(242, 192, 33);
		static constexpr ImColor TransformPinColor = ImColor(245, 111, 0);

		static constexpr ImColor RedChannelPinColor = ImColor(255, 0, 0);
		static constexpr ImColor GreenChannelPinColor = ImColor(0, 255, 0);
		static constexpr ImColor BlueChannelPinColor = ImColor(0, 0, 255);
		static constexpr ImColor AlphaChannelPinColor = ImColor(125, 125, 125);

		static constexpr int PinIconSize = 24;
		static constexpr float CommentPadding = 40.0f;
	};

	enum class ValueInputType
	{
		Bool,

		Int,
		Float,
		Vector2,
		Vector3,
		Vector4,
		Transform,
		
		Handle,

		String,
		StringMultiline,
	};
	
	ImRect GetItemRect();
	ImRect ExpandedRect(const ImRect& rect, float x, float y);

	bool BeginDockedGraphWindow(const char* label, AssetHandle handle, const ImGuiWindowFlags windowFlags = 0);
	void DrawGraphOverlay(const char* overlayText);

	void DrawLabel(const char* label, ImColor color);
	void DrawErrorMessage(const float minX, const float maxX, const float maxY);
	void DrawNodeErrorMessage(uint64_t id);
	void DrawNodeShadow(uint64_t id);
	void DrawNodeComment(uint64_t id, bool& enabled, bool& pinned, std::string& comment, const glm::vec3& color);
	void DrawCommentNode(const uint64_t id, std::string& comment, glm::vec2& size, const glm::vec3& color);
	void DrawNodeDefaultValueInput(const ValueInputType type, void* data, const AssetType assetType = AssetType::None, const bool spring = false);

	uint64_t DrawSearchWindow(const char* graphName, const NodeSearchTree& tree, std::string& searchBuffer, bool& contextSensitive, std::function<void()> onUpdate);
	void DrawCompilerResultWindow(AssetHandle handle, const CompilerResult& result);
}