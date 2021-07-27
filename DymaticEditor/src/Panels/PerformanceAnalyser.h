#pragma once
#include <Dymatic.h>
#include "Dymatic/Core/Base.h"

struct ProfilerPoint
{
	unsigned int id;
	std::string name;
	float ts;
	unsigned int duration;
	unsigned int index = 0;
	glm::vec3 color;

	ProfilerPoint(unsigned int id, std::string name, float ts, unsigned int duration, glm::vec3 color = glm::vec3(1.0f))
		: id(id), name(name), ts(ts), duration(duration), color(color)
	{
	}
};

namespace Dymatic {
	class PerformanceAnalyser
	{
	public:
		PerformanceAnalyser();
		void OnImGuiRender(Timestep ts);
		void ReadJsonFilePerformanceAnalitics(std::string filepath); 
		
		bool& GetPerformanceAnalyserVisible() { return m_PerformanceAnalyserVisible; }
	private:
		unsigned int GetNextProfilerPointId() { m_NextProfilerPointId++; return m_NextProfilerPointId; }
	private:
		bool m_PerformanceAnalyserVisible = false;

		std::vector<float> m_DeltaSteps;
		float m_DeltaMin = 0.0f;
		float m_DeltaMax = 100.0f;

		std::vector<float> m_MemoryUsageSteps;
		float m_MemoryUsageMin = 0.0f;
		float m_MemoryUsageMax = 200.0f;

		unsigned int m_NextProfilerPointId = 0;
		float m_ProfilerStartTime = 0.0f;

		std::vector<ProfilerPoint> m_ProfilerPoints;
	};

}