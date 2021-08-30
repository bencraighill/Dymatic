#pragma once

#include <Dymatic.h>
#include "Dymatic/Core/Base.h"

namespace Dymatic::Sandbox {

	struct Agent
	{
		glm::vec2 position;
		float angle;
		glm::vec4 color;

			Agent(glm::vec2 position, float angle, glm::vec4 color = glm::vec4(1.0f))
			: position(position), angle(angle), color(color)
			{
			}
	};

	class AgentSimulation
	{
	public:
		AgentSimulation();
		void Update(Timestep ts);
	private:
		int hash(int state)
		{
			state ^= 2747636419;
			state *= 2654435769;
			state ^= state >> 16;
			state *= 2654435769;
			state ^= state >> 16;
			state *= 2654435769;
			return state;
		}

		float sense(Agent agent, float sensorAngleOffset);
	private:

		int m_Seed = 12;

		std::vector<Agent> m_Agents;
		int scaleFactor = 1;
		int m_Width = 100 * scaleFactor;
		int m_Height = 100 * scaleFactor;
		glm::vec4 m_TrailMap[100][100];
		Ref<Texture2D> m_Trail;

		float m_MoveSpeed = 10.0f;
		float m_EvaporateSpeed = 0.025f / scaleFactor;
		float m_DiffusedSpeed = 1.0f;

		float m_SensorOffsetDst = 80.0f;
		float m_SensorSize = 10.0f;
		float m_SensorAngleSpacing = 45.0f;
		float m_TurnSpeed = 5.0f;
	};

	class MandelbrotSet
	{
	public:
		MandelbrotSet();
		void OnImGuiRender();

		void DrawSetBuffer();

		double mapToReal(int x, int imageWidth, double minR, double maxR);
		double mapToImaginary(int y, int imageHeight, double minI, double maxI);

		int findMandelbrot(double cr, double ci, int max_iterations);

		void HSVtoRGB(float H, float S, float V, float* outR, float* outG, float* outB);
	private:
		Ref<Texture2D> m_MandelbrotTexture;


		int m_Width = 1080;
		int m_Height = 1080;

		float m_Zoom = 1.0f;

		float m_XPos = 0.0f;
		float m_YPos = 0.0f;

		float m_MoveSpeed = 0.1f;
		float m_ZoomSpeed = 0.1f;
		int m_Iterations = 50; //256 good

		float minR = -1.5, maxR = 0.0, minI = -1.0, maxI = 1.0;
		bool m_ZoomUniform = true;
	};

	struct SandParticle
	{
		int id = -1;
		float lifetime = 0.0f;
		glm::vec2 velocity = glm::vec2(0.0f);
		glm::vec4 color = glm::vec4(1.0f);
		bool updated = false;

		SandParticle();
		SandParticle(int id)
			: id(id)
		{
		}
	};

	class SandSimulation
	{
	public:
		SandSimulation();
		void OnImGuiRender();
	private:
		int GetNextID() { m_NextID++; return m_NextID; }
	private:

		Ref<Texture2D> m_SimulationTexture;
		int m_Width = 512;
		int m_Height = 512;

		int m_NextID = 0;

		SandParticle m_SandParticles[512][512] = {SandParticle(m_NextID)};
	};

	//Cloth Simulation

	struct Point
	{
		unsigned int id;
		glm::vec2 position, prevPosition;
		bool locked;
		Point(unsigned int id, glm::vec2 position, bool locked)
			: id(id), position(position), prevPosition(position), locked(locked)
		{}
	};

	struct Stick
	{
		unsigned int id;
		Point* pointA;
		Point* pointB;
		float length;
		Stick(unsigned int id, Point* a, Point* b, float length)
			: id(id), pointA(a), pointB(b), length(length)
		{}
	};

	class RopeSimulation
	{
	public:
		RopeSimulation();
		void OnImGuiRender(Timestep ts);
		void Simulate(Timestep ts);
	private:
		inline unsigned int GetNextId() { m_NextId++; return m_NextId; }
	private:
		std::vector<Point> m_Points;
		std::vector<Stick> m_Sticks;
		int m_InterationNumber = 5;
		bool m_Simulating = false;
		unsigned int m_NextId = 0;
		Point* joinPoint = nullptr;
	};

}
