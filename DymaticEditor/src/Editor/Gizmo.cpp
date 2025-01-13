#include "Editor/Gizmo.h"

#include "Dymatic/Core/Base.h"

#include <imgui/imgui.h>
#include <ImGuizmo/ImGuizmo.h>

namespace Dymatic {

	ImGuizmo::OPERATION Utils::GetImGuizmoOperation(const GizmoOperation operation)
	{
		ImGuizmo::OPERATION result = (ImGuizmo::OPERATION)0;

		if ((bool)(operation & GizmoOperation::Translate))	result = result | ImGuizmo::OPERATION::TRANSLATE;
		if ((bool)(operation & GizmoOperation::Rotate))		result = result | ImGuizmo::OPERATION::ROTATE;
		if ((bool)(operation & GizmoOperation::Scale))		result = result | ImGuizmo::OPERATION::SCALE;

		return result;
	}

	ImGuizmo::MODE Utils::GetImGuizmoMode(const GizmoMode mode)
	{
		switch (mode)
		{
		case GizmoMode::Local:	return ImGuizmo::MODE::LOCAL;
		case GizmoMode::World:	return ImGuizmo::MODE::WORLD;
		}

		DY_CORE_ASSERT(false);
		return ImGuizmo::MODE::LOCAL;
	}

	GizmoOperation operator|(GizmoOperation lhs, GizmoOperation rhs)
	{
		using T = std::underlying_type_t<GizmoOperation>;
		return static_cast<GizmoOperation>(static_cast<T>(lhs) | static_cast<T>(rhs));
	}

	GizmoOperation operator&(GizmoOperation lhs, GizmoOperation rhs)
	{
		using T = std::underlying_type_t<GizmoOperation>;
		return static_cast<GizmoOperation>(static_cast<T>(lhs) & static_cast<T>(rhs));
	}

	GizmoOperation& operator|=(GizmoOperation& lhs, GizmoOperation rhs)
	{
		lhs = lhs | rhs;
		return lhs;
	}

	GizmoOperation& operator&=(GizmoOperation& lhs, GizmoOperation rhs)
	{
		lhs = lhs & rhs;
		return lhs;
	}

	GizmoOperation operator^(GizmoOperation lhs, GizmoOperation rhs)
	{
		using T = std::underlying_type_t<GizmoOperation>;
		return static_cast<GizmoOperation>(static_cast<T>(lhs) ^ static_cast<T>(rhs));
	}

	GizmoOperation& operator^=(GizmoOperation& lhs, GizmoOperation rhs)
	{
		lhs = lhs ^ rhs;
		return lhs;
	}

}