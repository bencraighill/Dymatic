#include "SandboxArea.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "Dymatic/Math/Math.h"

#include "Dymatic/Renderer/ShaderStorageBuffer.h"

namespace Dymatic::Sandbox {

	AgentSimulation::AgentSimulation()
	{
		m_Trail = Texture2D::Create(m_Width, m_Height);

		for (int x = 0; x < m_Width; x++)
		{
			for (int y = 0; y < m_Height; y++)
			{
				m_TrailMap[x][y] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			}
		}

		int angleChange = 1;
		for (int i = 0; i < (360 / angleChange) * 1 + 1; i++)
		{
			glm::vec4 color = glm::vec4(1.0f);
			int random = Math::GetRandomInRange(0, 6);
			random = std::fmod(random + i, 5);
			if (random == 0) { color = glm::vec4(0.7f, 0.4f, 1.0f, 1.0f); }
			else if (random == 1) { color = glm::vec4(0.0f, 1.0f, 0.72f, 1.0f); }
			else if (random == 2) { color = glm::vec4(0.2f, 0.1f, 1.0f, 1.0f); }
			else if (random == 4) { color = glm::vec4(0.5f, 0, 1.0f, 1.0f); }
			else if (random == 5) { color = glm::vec4(1.0f, 0.77f, 0.4f, 1.0f); }
			m_Agents.push_back(Agent(glm::vec2(m_Width / 2, m_Height / 2), (i * angleChange) + 180, color));

		}

		//glm::vec2 centre = glm::vec2(m_Width / 2, m_Height / 2);
		//int checkDst = 25;
		//for (int i = 0; i < 10; i++)
		//{
		//	for (int x = 0; x < m_Width; x++)
		//	{
		//		for (int y = 0; y < m_Height; y++)
		//		{
		//			int distance = std::sqrt(std::pow((x - centre.x), 2) + std::pow(y - centre.y, 2));
		//			if (distance == checkDst)
		//			{
		//				glm::vec4 color = glm::vec4(1.0f);
		//				int random = Math::GetRandomInRange(0, 6);
		//				random = std::fmod(random + i, 5);
		//				if (random == 0) { color = glm::vec4(0.7f, 0.4f, 1.0f, 1.0f); }
		//				else if (random == 1) { color = glm::vec4(0.0f, 1.0f, 0.72f, 1.0f); }
		//				else if (random == 2) { color = glm::vec4(0.2f, 0.1f, 1.0f, 1.0f); }
		//				else if (random == 4) { color = glm::vec4(0.5f, 0, 1.0f, 1.0f); }
		//				else if (random == 5) { color = glm::vec4(1.0f, 0.77f, 0.4f, 1.0f); }
		//				float direction = std::atan2(y - centre.y, x - centre.x);
		//				m_Agents.push_back(Agent(glm::vec2(x, y), direction, color));
		//			}
		//		}
		//	}
		//	checkDst += 5;
		//}

		//for (int i = 0; i < 500; i++)
		//{
		//	m_Agents.push_back(Agent(glm::vec2(Math::GetRandomInRange(1, 255), Math::GetRandomInRange(1, 255)), (float)Math::GetRandomInRange(0, 360)));
		//}
	}

	void AgentSimulation::Update(Timestep ts)
	{
		for (int i = 0; i < m_Agents.size(); i++)
		{
			auto& agent = m_Agents[i];
			int random = hash(agent.position.y * m_Trail->GetWidth() + agent.position.x + hash(i));

			float weightForward = sense(agent, 0);
			float weightLeft = sense(agent, m_SensorAngleSpacing);
			float weightRight = sense(agent, -m_SensorAngleSpacing);

			float randomSteerStrength = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

			if (weightForward > weightLeft && weightForward > weightRight)
			{
				m_Agents[i].angle += 0;
			}
			else if (weightForward < weightLeft && weightForward < weightRight)
			{
				m_Agents[i].angle += (randomSteerStrength - 0.5f) * 2 * m_TurnSpeed * ts.GetSeconds();
			}
			else if (weightRight > weightLeft)
			{
				m_Agents[i].angle -= randomSteerStrength * m_TurnSpeed * ts.GetSeconds();
			}
			else if (weightLeft > weightRight)
			{
				m_Agents[i].angle += randomSteerStrength * m_TurnSpeed * ts.GetSeconds();
			}

			glm::vec2 direction = glm::vec2(std::cos(agent.angle), std::sin(agent.angle));
			glm::vec2 newPos = agent.position + direction * m_MoveSpeed * ts.GetSeconds();

			if (newPos.x < 0 || newPos.x >= m_Width || newPos.y < 0 || newPos.y >= m_Height)
			{
				newPos.x = std::min(m_Trail->GetWidth() - 0.01f, std::max(0.0f, newPos.x));
				newPos.y = std::min(m_Trail->GetHeight() - 0.01f, std::max(0.0f, newPos.y));
				float random = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
				double pi = 2 * acos(0.0);
				m_Agents[i].angle = random * 2 * pi;
			}

			m_Agents[i].position = newPos;
			m_TrailMap[(int)newPos.x][(int)newPos.y] = agent.color;
		}

		for (int x = 0; x < m_Width; x++)
		{
			for (int y = 0; y < m_Height; y++)
			{
				glm::vec4 originalValue = m_TrailMap[x][y];

				glm::vec4 sum = glm::vec4(0.0f);
				for (int offsetX = -1; offsetX <= 1; offsetX++) {
					for (int offsetY = -1; offsetY <= 1; offsetY++)
					{
						int sampleX = x + offsetX;
						int sampleY = y + offsetY;

						if (sampleX >= 0 && sampleX < m_Width && sampleY >= 0 && sampleY < m_Height)
						{
							sum += m_TrailMap[sampleX][sampleY];
						}
					}
				}

				glm::vec4 blurResult = sum / 9.0f;

				glm::vec4 diffusedValue = glm::lerp(originalValue, blurResult, m_DiffusedSpeed * ts.GetSeconds());

				glm::vec4 evaporatedValue;
				evaporatedValue.r = std::max(0.0f, diffusedValue.r - m_EvaporateSpeed * ts.GetSeconds());
				evaporatedValue.g = std::max(0.0f, diffusedValue.g - m_EvaporateSpeed * ts.GetSeconds());
				evaporatedValue.b = std::max(0.0f, diffusedValue.b - m_EvaporateSpeed * ts.GetSeconds());
				evaporatedValue.a = std::max(0.0f, diffusedValue.a - m_EvaporateSpeed * ts.GetSeconds());

				m_TrailMap[x][y] = evaporatedValue;
			}
		}

		//int dataSize = m_Trail->GetWidth() * m_Trail->GetHeight() * 4;
		//char* data = new char[dataSize];
		//for (int i = 0; i < m_TrailMap.size(); i++)
		//{
		//	data[i * 4 + 0] = m_TrailMap[i].r * 255.0f;
		//	data[i * 4 + 1] = m_TrailMap[i].g * 255.0f;
		//	data[i * 4 + 2] = m_TrailMap[i].b * 255.0f;
		//	data[i * 4 + 3] = m_TrailMap[i].a * 255.0f;
		//}
		//m_Trail->SetData(data, dataSize);
		//delete[] data;

		ImGui::Begin("Agent Simulation");

		//int dataSize = m_Trail->GetWidth() * m_Trail->GetHeight() * 4;
		//char* data = new char[dataSize];
		//for (int i = 0; i < dataSize; i += 4)
		//{
		//	data[i + 0] = 0.0f;
		//	data[i + 1] = 0.0f;
		//	data[i + 2] = 0.0f;
		//	data[i + 3] = 255.0f;
		//}

		ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImVec2(m_Width, m_Height) * 1.5f, ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 1.0f)));
		for (int x = 0; x < m_Width; x++)
		{
			for (int y = 0; y < m_Height; y++)
			{
				if (m_TrailMap[x][y].r != 0.0f)
				{
					ImGui::GetWindowDrawList()->AddLine(ImGui::GetWindowPos() + ImVec2(x, y) * 1.5f, (ImGui::GetWindowPos() + ImVec2((x + 1), (y + 1)) * 1.5f), ImGui::ColorConvertFloat4ToU32(ImVec4(m_TrailMap[x][y].r, m_TrailMap[x][y].g, m_TrailMap[x][y].b, m_TrailMap[x][y].a)), 1.0f);
					//ImGui::GetWindowDrawList()->AddCircleFilled(ImGui::GetWindowPos() + ImVec2(x, y), 1.0f, ImGui::ColorConvertFloat4ToU32(ImVec4(m_TrailMap[x][y].r, m_TrailMap[x][y].g, m_TrailMap[x][y].b, m_TrailMap[x][y].a)));
					//data[x * 4 + y * 4 * 256 + 0] = m_TrailMap[x][y].r;
					//data[x * 4 + y * 4 * 256 + 1] = m_TrailMap[x][y].g;
					//data[x * 4 + y * 4 * 256 + 2] = m_TrailMap[x][y].b;
					//data[x * 4 + y * 4 * 256 + 3] = 255.0f;
				}
			}
		}

		//m_Trail->SetData(data, dataSize);
		//delete[] data;

		//ImGui::GetWindowDrawList()->AddImage(reinterpret_cast<void*>(m_Trail->GetRendererID()), ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImVec2(m_Trail->GetWidth(), m_Trail->GetHeight()), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
		ImGui::End();
	}

	float AgentSimulation::sense(Agent agent, float sensorAngleOffset)
	{
		float sensorAngle = agent.angle + sensorAngleOffset;
		glm::vec2 sensorDir = glm::vec2(std::cos(sensorAngle), std::sin(sensorAngle));
		glm::int2 sensorCentre = agent.position + sensorDir + m_SensorOffsetDst;
		float sum = 0;

		for (int offsetX = -m_SensorSize; offsetX <= m_SensorSize; offsetX++)
		{
			for (int offsetY = -m_SensorSize; offsetY <= m_SensorSize; offsetY++)
			{
				glm::int2 pos = sensorCentre + glm::int2(offsetX, offsetY);

				if (pos.x >= 0 && pos.x < m_Width && pos.y >= 0 && pos.y < m_Height)
				{
					sum += m_TrailMap[pos.x][pos.y].x;
				}
			}
		}

		return sum;
	}


	MandelbrotSet::MandelbrotSet()
	{
		m_MandelbrotTexture = Texture2D::Create(m_Width, m_Height);


		m_Iterations = 253;
		m_MoveSpeed = 0.0f;
		m_ZoomSpeed = 40.0f;
		m_Zoom = 502.3f;
		m_XPos = -0.80f;
		m_YPos = 0.18f;
		DrawSetBuffer();

	}

	void MandelbrotSet::OnImGuiRender()
	{
		if (Input::IsKeyPressed(Key::Equal))
		{
			m_Zoom += m_ZoomSpeed;
			DrawSetBuffer();
		}
		if (Input::IsKeyPressed(Key::Minus))
		{
			m_Zoom -= m_ZoomSpeed;
			DrawSetBuffer();
		}

		if (Input::IsKeyPressed(Key::Right))
		{
			m_XPos += m_MoveSpeed;
			DrawSetBuffer();
		}
		if (Input::IsKeyPressed(Key::Left))
		{
			m_XPos -= m_MoveSpeed;
			DrawSetBuffer();
		}
		if (Input::IsKeyPressed(Key::Up))
		{
			m_YPos += m_MoveSpeed;
			DrawSetBuffer();
		}
		if (Input::IsKeyPressed(Key::Down))
		{
			m_YPos -= m_MoveSpeed;
			DrawSetBuffer();
		}

		//static float prev_X = Input::GetMouseX();
		//static float prev_Y = Input::GetMouseY();
		//if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
		//{
		//	prev_X = Input::GetMouseX();
		//	prev_Y = Input::GetMouseY();
		//
		//	m_XPos += (Input::GetMouseX() - prev_X);
		//	m_YPos += (Input::GetMouseY() - prev_Y);
		//
		//	if (Input::GetMouseX() - prev_X != 0.0f)
		//	{
		//		DrawSetBuffer();
		//	}
		//}


		ImGui::Begin("Mandelbrot Set");
		ImGui::GetWindowDrawList()->AddImage(reinterpret_cast<void*>(m_MandelbrotTexture->GetRendererID()), ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImVec2(m_Width * 1.0f, m_Height * 1.0f), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
		static bool prev_down = false;
		static float click_posX = 0.0f;
		static float click_posY = 0.0f;
		if (Input::IsMouseButtonPressed(Mouse::ButtonRight))
		{
			if (!prev_down)
			{
				click_posX = Input::GetMouseX();
				click_posY = Input::GetMouseY();
			}
			ImGui::GetWindowDrawList()->AddRect(ImVec2(click_posX, click_posY), ImVec2(Input::GetMouseX(), Input::GetMouseY()), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.0f, 0.0f, 1.0f)), 0.0f, NULL, 2.0f);
			prev_down = true;
		}
		else
		{
			if (prev_down)
			{
				float current_X_pcent = (Input::GetMouseX() - ImGui::GetWindowPos().x) / m_Width;
				float click_X_pcent = (click_posX - ImGui::GetWindowPos().x) / m_Width;
				float current_Y_pcent = 1.0f - (((Input::GetMouseY() - ImGui::GetWindowPos().y)) / m_Height);
				float click_Y_pcent = 1.0f - ((click_posY - ImGui::GetWindowPos().y) / m_Height);

				minR = minR + ((maxR - minR) * (m_ZoomUniform ? (click_X_pcent > click_Y_pcent ? click_X_pcent : click_Y_pcent) : click_X_pcent));
				maxR = minR + ((maxR - minR) * (m_ZoomUniform ? (current_X_pcent > current_Y_pcent ? current_X_pcent : current_Y_pcent) : current_X_pcent));
				minI = minI + ((maxI - minI) * (m_ZoomUniform ? (click_X_pcent > click_Y_pcent ? click_X_pcent : click_Y_pcent) : click_Y_pcent));
				maxI = minI + ((maxI - minI) * (m_ZoomUniform ? (current_X_pcent > current_Y_pcent ? current_X_pcent : current_Y_pcent) : current_Y_pcent));

				//minR = minR + ((maxR - minR) * (click_X_pcent));
				//maxR = minR + ((maxR - minR) * (current_X_pcent));
				//minI = minI + ((maxI - minI) * (click_Y_pcent));
				//maxI = minI + ((maxI - minI) * (current_Y_pcent));

				DrawSetBuffer();

				//Execute Draw Loop
			}
			prev_down = false;
		}

		ImGui::End();

		ImGui::Begin("Mandelbrot Toolbar");
		ImGui::DragFloat("Zoom", &m_Zoom, 0.1f, 0.0f, 0.0f, "%.4f");
		ImGui::DragFloat("X Position", &m_XPos, 0.1f, 0.0f, 0.0f, "%.4f");
		ImGui::DragFloat("Y Position", &m_YPos, 0.1f, 0.0f, 0.0f, "%.4f");
		ImGui::Separator();
		ImGui::DragFloat("Move Speed", &m_MoveSpeed, 0.1f, 0.0f, 0.0f, "%.4f");
		ImGui::DragFloat("Zoom Speed", &m_ZoomSpeed, 0.1f, 0.0f, 0.0f, "%.4f");
		ImGui::Separator();
		ImGui::InputInt("Iterations", &m_Iterations);
		ImGui::Separator();
		ImGui::DragFloat("minR", &minR, 0.1f, 0.0f, 0.0f, "%.4f");
		ImGui::DragFloat("maxR", &maxR, 0.1f, 0.0f, 0.0f, "%.4f");
		ImGui::DragFloat("minI", &minI, 0.1f, 0.0f, 0.0f, "%.4f");
		ImGui::DragFloat("maxI", &maxI, 0.1f, 0.0f, 0.0f, "%.4f");
		ImGui::Separator();
		if (ImGui::Button("Reload Set")) { DrawSetBuffer(); }
		ImGui::ToggleButton("Uniform Zoom Slider", &m_ZoomUniform);
		ImGui::End();
	}

	double MandelbrotSet::mapToReal(int x, int imageWidth, double minR, double maxR)
	{
		double range = maxR - minR;
		return x * (range / imageWidth) + minR;
	}

	double MandelbrotSet::mapToImaginary(int y, int imageHeight, double minI, double maxI)
	{
		double range = maxI - minI;
		return y * (range / imageHeight) + minI;
	}

	int MandelbrotSet::findMandelbrot(double cr, double ci, int max_iterations)
	{
		int i = 0;
		double zr = 0.0, zi = 0.0;
		while (i < max_iterations && zr * zr + zi * zi < 4.0)
		{
			double temp = zr * zr - zi * zi + cr;
			zi = 2.0 * zr * zi + ci;
			zr = temp;
			i++;
		}

		return i;
	}

	void MandelbrotSet::DrawSetBuffer()
	{
		//int maxN = 256;
		//double minR = -1.5, maxR = 0.0, minI = -1.0, maxI = 1.0;
		//double minR = -1.5, maxR = 0.0, minI = -1.0, maxI = 1.0;
		static int zoom_factor = 2.0f;
		double minR = -1.5 / m_Zoom + m_XPos, maxR = 0.0 / m_Zoom + m_XPos, minI = -1.0 / m_Zoom + m_YPos, maxI = 1.0 / m_Zoom + m_YPos;
		//double minR = -1.5, maxR = 0.0, minI = -1.0, maxI = 1.0;

		int dataSize = m_Width * m_Height * 4;
		char* data = new char[dataSize];


		for (int y = 0; y < m_Height; y++)
		{
			for (int x = 0; x < m_Width; x++)
			{
				double cr = mapToReal(x, m_Width, minR, maxR);
				double ci = mapToImaginary(y, m_Height, minI, maxI);

				int n = findMandelbrot(cr, ci, m_Iterations);


				//float hue = (255.0f * x / m_Iterations);
				//float saturation = 255.0f;
				//float value = n < m_Iterations ? 255.0f : 0.0f;
				//HSVtoRGB(hue, saturation, value, &r, &g, &b);
				//float r;
				//float g;
				//float b;

				int N = 255; // colors per element
				int N3 = N * N * N;
				// map n on the 0..1 interval (real numbers)
				double t = (double)n / (double)m_Iterations;
				// expand n on the 0 .. 256^3 interval (integers)
				n = (int)(t * (double)N3);

				int b = n / (N * N);
				int nn = n - b * N * N;
				int r = nn / N;
				int g = nn - r * N;



				//int r = (n % 100);
				//int g = (n % 123);
				//int b = (n % 255);

				int pixel = x + (y * m_Width);
				data[(pixel * 4) + 0] = r;
				data[(pixel * 4) + 1] = g;
				data[(pixel * 4) + 2] = b;
				data[(pixel * 4) + 3] = 255;
			}
		}

		m_MandelbrotTexture->SetData(data, dataSize);

		delete[] data;
	}

	void MandelbrotSet::HSVtoRGB(float H, float S, float V, float* outR, float* outG, float* outB) {
		if (H > 360.0f || H < 0.0f || S>100.0f || S < 0.0f || V>100.0f || V < 0.0f) {
			DY_ASSERT("The givem HSV values are not in valid range");
			return;
		}
		float s = S / 100.0f;
		float v = V / 100.0f;
		float C = s * v;
		float X = C * (1 - abs(fmod(H / 60.0, 2) - 1));
		float m = v - C;
		float r, g, b;
		if (H >= 0.0f && H < 60.0f) {
			r = C, g = X, b = 0.0f;
		}
		else if (H >= 60.0f && H < 120.0f) {
			r = X, g = C, b = 0.0f;
		}
		else if (H >= 120.0f && H < 180.0f) {
			r = 0.0f, g = C, b = X;
		}
		else if (H >= 180.0f && H < 240.0f) {
			r = 0.0f, g = X, b = C;
		}
		else if (H >= 240.0f && H < 300.0f) {
			r = X, g = 0.0f, b = C;
		}
		else {
			r = C, g = 0.0f, b = X;
		}
		*outR = (r + m) * 255.0f;
		*outG = (g + m) * 255.0f;
		*outB = (b + m) * 255.0f;
	}

	SandSimulation::SandSimulation()
	{
		//m_SimulationTexture = Texture2D::Create(m_Width, m_Height);
		//for (int x = 0; x < m_Width; x++)
		//{
		//	for (int y = 0; y < m_Height; y++)
		//	{
		//		m_SandParticles[x][y] = NULL;
		//	}
		//}
		//m_SandParticles[50][50] = SandParticle(GetNextID());
	}

	void SandSimulation::OnImGuiRender()
	{
		//int dataSize = m_Width * m_Height * 4;
		//char* data = new char[dataSize];
		//for (int x = 0; x < m_Width; x++)
		//{
		//	for (int y = 0; y < m_Height; y++)
		//	{
		//		if (m_SandParticles[x][y].id != -1)
		//		{
		//			DY_CORE_INFO(m_SandParticles[x][y].id);
		//		}
		//	}
		//}
		//m_SimulationTexture->SetData(data, dataSize);
		//delete[] data;
		//
		//ImGui::Begin("Sand Simulation");
		//ImGui::GetWindowDrawList()->AddImage(reinterpret_cast<void*>(m_SimulationTexture->GetRendererID()), ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImVec2(m_Width, m_Height), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
		//ImGui::End();
	}

	SandParticle::SandParticle()
	{

	}

	// Rope Simulation

	RopeSimulation::RopeSimulation()
	{
		m_Points.reserve(255);
		m_Sticks.reserve(255);
	}

	void RopeSimulation::OnImGuiRender(Timestep ts)
	{
		if (m_Simulating)
			Simulate(ts);

		ImGui::Begin("Rope Simulation");
		ImGui::ToggleButton("Simulate", &m_Simulating);
		ImGui::SameLine();
		if (ImGui::Button("Reset")) { m_Points.clear(); m_Sticks.clear(); joinPoint = nullptr; }
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100.0f);
		ImGui::InputFloat("Cutting Tolerance", &m_CuttingTollerance, 0.1f, 0.2f, "%.4f");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100.0f);
		ImGui::InputFloat("Breaking Point", &m_BreakingPoint, 0.1f, 0.2f, "%.1f");
		ImGui::SameLine();
		ImGui::Checkbox("Floor", &m_Floor);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100.0f);
		if (ImGui::BeginCombo("##RopeSimulationPreset", "Select Preset"))
		{
			if (ImGui::Selectable("Grid")) { GenerateGrid(); }
			if (ImGui::Selectable("Tree")) { GenerateTree(); }
			ImGui::EndCombo();
		}

		m_FloorPos = ImGui::GetWindowPos().y + ImGui::GetContentRegionAvail().y;
		m_WindowWidth = ImGui::GetWindowSize().x;

		auto drawList = ImGui::GetWindowDrawList();
		auto shift = (Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift));
		auto ctrl = (Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl));

		// Point computation and interaction occurs

		for (auto& point : m_Points)
		{
			ImGui::PushID(point.id);

			auto centre = ImGui::GetWindowPos() + ImVec2(point.position.x, point.position.y);
			static float radius = 5.0f;

			const ImGuiID id = ImGui::GetID("##PointButton");
			const ImRect bb = ImRect(ImVec2(centre.x - radius, centre.y - radius), ImVec2(centre.x + radius, centre.y + radius));
			bool held, hovered;
			bool clicked = ImGui::ButtonBehavior(bb, id, &hovered, &held);

			if (clicked && shift)
			{
				if (joinPoint == nullptr)
					joinPoint = &point;
				else
				{
					float distance = Math::Distance(joinPoint->position, point.position);
					m_Sticks.push_back({ GetNextId(), joinPoint, &point, distance });
					joinPoint = nullptr;
				}
			}

			if (clicked && !shift)
				point.locked = !point.locked;

			ImGui::PopID();
		}

		// Stick draw call and computation happens

		for (int i = 0; i < m_Sticks.size(); i++)
		{
			auto& stick = m_Sticks[i];
			drawList->AddLine(ImGui::GetWindowPos() + ImVec2(stick.pointA->position.x, stick.pointA->position.y), ImGui::GetWindowPos() + ImVec2(stick.pointB->position.x, stick.pointB->position.y), ImGui::ColorConvertFloat4ToU32(ImVec4(0.8f, 0.8f, 0.8f, 1.0f)));

			auto cPos = glm::vec2(ImGui::GetMousePos().x - ImGui::GetWindowPos().x, ImGui::GetMousePos().y - ImGui::GetWindowPos().y);
			if (Input::IsMouseButtonPressed(Mouse::ButtonLeft) && !shift && !ctrl)
				if (Math::NearlyEqual(Math::Distance(stick.pointA->position, cPos) + Math::Distance(stick.pointB->position, cPos), Math::Distance(stick.pointA->position, stick.pointB->position), m_CuttingTollerance))
					m_Sticks.erase(m_Sticks.begin() + i);
		}

		// Point Draw Call happens later

		for (auto const& point : m_Points)
		{
			auto centre = ImGui::GetWindowPos() + ImVec2(point.position.x, point.position.y);
			float radius = 5.0f;

			drawList->AddCircleFilled(centre, radius, ImGui::ColorConvertFloat4ToU32(point.locked ? ImVec4(0.8f, 0.1f, 0.2f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));
		}

		if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
		{
			joinPoint = nullptr;
			if (ctrl)
			{
				auto pos = ImGui::GetMousePos() - ImGui::GetWindowPos();
				m_Points.push_back({ GetNextId(), glm::vec2(pos.x, pos.y), false });
			}
		}

		if (Input::IsMouseButtonPressed(Mouse::ButtonMiddle))
		{
			auto pos = ImGui::GetMousePos() - ImGui::GetWindowPos();
			m_Points.push_back({ GetNextId(), glm::vec2(pos.x, pos.y), false });
			if (m_PreviousDrawPoint != nullptr)
				m_Sticks.push_back({ GetNextId(), &m_Points.back(), m_PreviousDrawPoint, Math::Distance(m_Points.back().position, m_PreviousDrawPoint->position) });
			m_PreviousDrawPoint = &m_Points.back();
		}
		else
			m_PreviousDrawPoint = nullptr;

		ImGui::End();
	}

	void RopeSimulation::Simulate(Timestep ts)
	{
		for (auto& p : m_Points)
		{
			if (!p.locked)
			{
				glm::vec2 positionBeforeUpdate = p.position;
				p.position += p.position - p.prevPosition;
				p.position += glm::vec2(0.0f, 9.8f) * ts.GetSeconds();
				if (m_Floor)
					p.position.y = std::min(p.position.y, m_FloorPos);
				p.prevPosition = positionBeforeUpdate;
			}
		}

		for (int i = 0; i < m_InterationNumber; i++)
			for (auto& stick : m_Sticks)
			{
				glm::vec2 stickCentre = (stick.pointA->position + stick.pointB->position) / 2.0f;
				glm::vec2 stickDir = glm::normalize(stick.pointA->position - stick.pointB->position);
				if (!stick.pointA->locked)
					stick.pointA->position = stickCentre + stickDir * stick.length / 2.0f;
				if (!stick.pointB->locked)
					stick.pointB->position = stickCentre - stickDir * stick.length / 2.0f;

			}

		for (int i = 0; i < m_Sticks.size(); i++)
		{
			if (Math::Distance(m_Sticks[i].pointA->position, m_Sticks[i].pointB->position) > m_BreakingPoint)
				m_Sticks.erase(m_Sticks.begin() + i);
		}
	}

	void RopeSimulation::GenerateGrid()
	{
		float xOffset = 50.0f;
		float yOffset = 50.0f;
		int gridSize = 50/*30*/;
		float gridGap = 10.0f/*15.0f*/;

		for (int y = 0; y < gridSize; y++)
			for (int x = 0; x < gridSize; x++)
				m_Points.push_back({ GetNextId(), glm::vec2(xOffset + x * gridGap + Math::GetRandomInRange(0, 100) / 25.0f, yOffset + y * gridGap + Math::GetRandomInRange(0, 100) / 25.0f), false });

		for (int y = 0; y < gridSize - 1; y++)
		{
			for (int x = 0; x < gridSize; x++)
			{
				m_Sticks.push_back({ GetNextId(), &m_Points[x + (y * gridSize)], &m_Points[x + ((y + 1) * gridSize)], gridGap });
			}
		}

		for (int x = 0; x < gridSize - 1; x++)
		{
			for (int y = 0; y < gridSize; y++)
			{
				m_Sticks.push_back({ GetNextId(), &m_Points[x + (y * gridSize)], &m_Points[x + 1 + (y * gridSize)], gridGap });
			}
		}
	}

	void RopeSimulation::GenerateTree()
	{
		m_Points.push_back({ GetNextId(), glm::vec2(m_WindowWidth / 2.0f, 50), true });
		TreeLoop(&m_Points.back());
	}
	void RopeSimulation::TreeLoop(Point* point)
	{
		m_TreeSeed++;
		if ((int)std::fmod(hash(m_TreeSeed), 8.0f) != 3 && m_Points.size() < m_Points.capacity() && m_Sticks.size() < m_Sticks.capacity())
		{
			auto size = (int)std::fmod(hash(m_TreeSeed), 4.0f) + 1;
			for (int i = 0; i < size; i++)
			{
				m_Points.push_back({ GetNextId(), point->position + glm::vec2((int)Math::GetRandomInRange(0, 100) - 50, Math::GetRandomInRange(1, 100)), false });
				m_Sticks.push_back({ GetNextId(), point, &m_Points.back(), Math::Distance(point->position, m_Points.back().position) });
				TreeLoop(&m_Points.back());
			}
		}
	}

	namespace GPUSIM {

		Ref<Shader> g_ComputeUpdate;
		Ref<Shader> g_ComputeProcess;
		Ref<Shader> g_ComputeColor;

		Ref<Texture2D> g_BaseTexture;
		Ref<Texture2D> g_ProcessedTexture;
		Ref<Texture2D> g_ColorTexture;

#define PI 3.1415926535897932384626433832795

		static const int WIDTH = 2560; 
		static const int HEIGHT = 1080;
		static const int AGENT_COUNT = 819200;
		static const int NUM_SPECIES = 4;
		static int itterations_per_frame = 1;

		struct Agent
		{
			glm::vec2 position;
			float angle;
			float BUFF_DISCARD;
			glm::ivec4 speciesMask;
		};

		struct SpeciesSettings
		{
			glm::vec4 color;
		};

		// Agent Bindings
		struct Bindings
		{
			Agent agents[AGENT_COUNT];
			SpeciesSettings speciesSettings[NUM_SPECIES];
		};
		Bindings BindingsBuffer;
		Ref<ShaderStorageBuffer> BindingsSSBO;

		// Frame Bindings
		struct FrameInfo
		{
			float u_DeltaTime = 0.0f;
			float u_MoveSpeed = 20.0f;
			float u_EvaporateSpeed = 0.2f;
			float u_DiffuseSpeed = 3.0f;
			float u_TrailWeight = 60.0f;
			int u_SensorOffsetDst = 35;
			int u_SensorSize = 1;
			float u_SensorAngleSpacing = 30.0f;
			float u_TurnSpeed = 2.0f;
			float BLK[3];
			// add blur range here
		};
		FrameInfo FrameInfoBuffer;
		Ref<UniformBuffer> FrameInfoUniformBuffer;

		GPUSimulation::GPUSimulation()
		{
			g_ComputeUpdate = Shader::Create("assets/shaders//GPUSimulation/GPUSimulationUpdate.glsl");
			g_ComputeProcess = Shader::Create("assets/shaders//GPUSimulation/GPUSimulationProcess.glsl");
			g_ComputeColor = Shader::Create("assets/shaders//GPUSimulation/GPUSimulationColor.glsl");
			g_BaseTexture = Texture2D::Create(WIDTH, HEIGHT);
			g_ProcessedTexture = Texture2D::Create(WIDTH, HEIGHT);
			g_ColorTexture = Texture2D::Create(WIDTH, HEIGHT);
			BindingsSSBO = ShaderStorageBuffer::Create(sizeof(Bindings), 9, ShaderStorageBufferUsage::DYNAMIC_READ);
			FrameInfoUniformBuffer = UniformBuffer::Create(sizeof(FrameInfo), 10);

			glm::vec2 pos;
			float radius = glm::min(WIDTH * 0.45f, HEIGHT * 0.45f);
			glm::vec2 middle = glm::vec2(WIDTH * 0.5f, HEIGHT * 0.5f);

			for (size_t i = 0; i < AGENT_COUNT; i++)
			{
				while (true)
				{
					pos = middle + glm::vec2(Math::RandomRange(-radius, radius), Math::RandomRange(-radius, radius));
					const float distance = std::sqrt(std::pow((middle.x - pos.x), 2) + std::pow((middle.y - pos.y), 2));
					if (distance < radius)
					{
						break;
					}
				}

				BindingsBuffer.agents[i].position = pos;
				BindingsBuffer.agents[i].angle = atan2(pos.y - middle.y, pos.x - middle.x) * 180.0f / PI;
				int num = std::floor(Math::RandomRange(1, 4.9999999999));
				switch (num)
				{
				case 1: BindingsBuffer.agents[i].speciesMask = glm::ivec4(1, 0, 0, 0); break;
				case 2: BindingsBuffer.agents[i].speciesMask = glm::ivec4(0, 1, 0, 0); break;
				case 3: BindingsBuffer.agents[i].speciesMask = glm::ivec4(0, 0, 1, 0); break;
				case 4: BindingsBuffer.agents[i].speciesMask = glm::ivec4(0, 0, 0, 1); break;
				}
			}

			BindingsBuffer.speciesSettings[0].color = glm::vec4(0.1f, 0.6f, 0.8f, 1.0f);
			BindingsBuffer.speciesSettings[1].color = glm::vec4(0.1f, 0.2f, 0.5f, 1.0f);
			BindingsBuffer.speciesSettings[2].color = glm::vec4(0.2f, 0.7f, 0.55f, 1.0f);
			BindingsBuffer.speciesSettings[3].color = glm::vec4(0.3f, 0.5f, 0.7f, 1.0f);

			BindingsSSBO->SetData(&BindingsBuffer, sizeof(Bindings));
		}

		void GPUSimulation::OnImGuiRender(Timestep ts)
		{
			for (size_t i = 0; i < itterations_per_frame; i++)
			{
				// Provide Frame info Time
				FrameInfoBuffer.u_DeltaTime = ts;
				FrameInfoUniformBuffer->SetData(&FrameInfoBuffer, sizeof(FrameInfo));

				// Run the compute shaders
				g_BaseTexture->BindTexture(0);
				g_ComputeUpdate->Dispatch(AGENT_COUNT / 16 + 0, 1, 1);
				g_ProcessedTexture->BindTexture(1);
				g_ComputeProcess->Dispatch((WIDTH / 8) + 1, (HEIGHT / 8) + 1, 1);

				std::swap(g_BaseTexture, g_ProcessedTexture);
			}

			// Compute Color
			{
				g_ProcessedTexture->BindTexture(0);
				g_ColorTexture->BindTexture(1);
				g_ComputeColor->Dispatch((WIDTH / 8) + 1, (HEIGHT / 8) + 1, 1);
			}

			ImGui::Begin("GPU Simulation");
			auto width = glm::min(ImGui::GetContentRegionAvail().x, (float)WIDTH);
			auto height = HEIGHT * (width / WIDTH);
			ImGui::Image((ImTextureID)g_ColorTexture->GetRendererID(), { width, height }, { 0, 1 }, { 1, 0 });
			ImGui::End();

			ImGui::Begin("Simulation Controls");
			ImGui::InputFloat("Move Speed", &FrameInfoBuffer.u_MoveSpeed);
			ImGui::InputFloat("Evaporate Speed", &FrameInfoBuffer.u_EvaporateSpeed);
			ImGui::InputFloat("Diffuse Speed", &FrameInfoBuffer.u_DiffuseSpeed);
			ImGui::InputInt("Frame Iterations", &itterations_per_frame);
			ImGui::End();
		}
	}

}