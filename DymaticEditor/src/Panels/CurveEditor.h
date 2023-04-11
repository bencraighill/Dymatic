#pragma once
#include <Dymatic.h>
#include "Dymatic/Core/Base.h"

namespace Dymatic {

	enum CurveType
	{
		Curve_None = 0,
		Linear,
		Constant,
		Free,
		Automatic,
		CURVE_TYPE_SIZE
	};

	struct CurvePoint
	{
		int CurveID = -1;

		CurveType curveType = Automatic;

		glm::vec2 Position = glm::vec2(0.0f, 0.0f);
		glm::vec2 HandleL = glm::vec2(0.0f, 0.0f);
		glm::vec2 HandleR = glm::vec2(0.0f, 0.0f);

		CurvePoint() = default;

		CurvePoint(int id, glm::vec2 position, glm::vec2 handleL, glm::vec2 handleR)
			: CurveID(id), Position(position), HandleL(handleL), HandleR(handleR)
		{
		}
	};

	struct Channel
	{
		int ChannelID = -1;

		bool m_Visible = false;
		bool m_Enabled = true;
		bool m_Locked = false;

		std::string m_ChannelName = "Unknown Channel";
		glm::vec3 m_Color = glm::vec3(1.0f, 1.0f, 1.0f);

		std::vector<CurvePoint> m_Points;

		Channel() = default;

		Channel(int id, std::string name, glm::vec3 color, std::vector<CurvePoint> points)
			: ChannelID(id), m_ChannelName(name), m_Color(color), m_Points(points)
		{
		}
	};

	class CurveEditor
	{
	public:
		CurveEditor();
		void OnImGuiRender();
		int GetNextCurveID() { m_NextCurveID++; return m_NextCurveID; }

		void OnEvent(Event& e);
		
	private:
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		bool OnMouseScrolled (MouseScrolledEvent& e);

		std::string ConvertCurveTypeToString(CurveType type);
	private:
		std::vector<Channel> m_Channels;

		std::vector<int> m_SelectedPoints = {5, 6};

		glm::vec2 m_Scale = glm::vec2(1.0f, 1.0f);

		float m_XLocation = 0.0f;
		float m_YLocation = 0.0f;

		float previousMouseX;
		float previousMouseY;

		int m_NextCurveID = 1;

		int m_HeldPointID = -1;
		int m_HeldHandle = -1;

		bool m_Clicked = false;

		bool m_AlwaysShowHandles = false;
		bool m_HandlesRelative = true;
	};

}