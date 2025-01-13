#pragma once

#include <cstdint>

namespace ImGuizmo {

	enum OPERATION;
	enum MODE;

}

namespace Dymatic {

	enum class GizmoOperation : uint8_t
	{
		None = 0,
		Translate = (1u << 0),
		Rotate = (1u << 1),
		Scale = (1u << 2),
		Universal = Translate | Rotate | Scale
	};

	enum class GizmoMode
	{
		Local,
		World
	};

	enum class GizmoPivotPoint
	{
		MedianPoint = 0, IndividualOrigins = 1, ActiveElement = 2
	};

	namespace Utils {
	
		ImGuizmo::OPERATION GetImGuizmoOperation(const GizmoOperation operation);
		ImGuizmo::MODE GetImGuizmoMode(const GizmoMode mode);

	}

	GizmoOperation operator|(GizmoOperation lhs, GizmoOperation rhs);
	GizmoOperation operator&(GizmoOperation lhs, GizmoOperation rhs);
	GizmoOperation& operator|=(GizmoOperation& lhs, GizmoOperation rhs);
	GizmoOperation& operator&=(GizmoOperation& lhs, GizmoOperation rhs);
	GizmoOperation operator^(GizmoOperation lhs, GizmoOperation rhs);
	GizmoOperation& operator^=(GizmoOperation& lhs, GizmoOperation rhs);

}