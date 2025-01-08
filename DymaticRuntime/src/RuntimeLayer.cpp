#include "RuntimeLayer.h"
#include "Dymatic/Asset/AssetManager.h"
#include "Dymatic/Scripting/ScriptEngine.h"
#include "Dymatic/Scripting/ScriptGlue.h"

#include "Dymatic/Renderer/SceneRenderer.h"

#include "Version.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace Dymatic {

	// Client should modify this at compile time
	static const char* s_ProjectPath = "Packaging.dyproject";

	RuntimeLayer::RuntimeLayer()
		: Layer("RuntimeLayer")
	{
	}

	void RuntimeLayer::OnAttach()
	{
		DY_PROFILE_FUNCTION();

#ifndef DY_DIST
		Log::SetCallback([&](const Log::Message& message)
		{
			m_DebugMessages.push_back({ message.FormattedText, message.Level });
		});
#endif

		const glm::vec2 defaultViewportSize = glm::vec2(1600, 900);
		m_SceneRendererContext = SceneRendererContext::Create(defaultViewportSize);
		SceneRenderer::SetActiveContext(m_SceneRendererContext);

		Renderer2D::SetLineWidth(4.0f);

		// Load the project
		Project::Load(s_ProjectPath);
		
		Log::ShowConsole();
		ScriptEngine::SetCoreAssemblyPath(Project::GetCoreModulePath());
		ScriptEngine::SetAppAssemblyPath(Project::GetScriptModulePath());
		ScriptEngine::ReloadAssembly();

		ScriptGlue::SetOpenSceneCallback([&](UUID handle)
		{
			m_PostUpdateQueue.push_back(handle);
		});

		// Open the start scene
		m_Scene = AssetManager::GetAsset<Scene>(Project::GetActive()->GetConfig().StartScene);
		
		// Startup Logo
		m_StartupLogo = Texture2D::Create("Resources/Icons/Branding/DymaticIconCircular.png");

		// Start the scene runtime
		m_Scene->OnRuntimeStart();

		// Finally, show window when ready (if it was hidden)
		if (Application::Get().GetSpecification().WindowStartHidden)
			Application::Get().GetWindow().ShowWindow();

#ifndef DY_DIST
		// Setup ImGui debug overlay
		{
			// Prevent ini settings dumping
			ImGui::GetIO().IniFilename = nullptr;

			ImGuiLayer* imguiLayer = Application::Get().GetImGuiLayer();
			imguiLayer->AddIconFont("Resources/Fonts/OpenSans-Regular.ttf", 20.0f, 0x00A9, 0x00A9); // Copyright Symbol
			imguiLayer->AddIconFont("Resources/Fonts/OpenSans-Regular.ttf", 20.0f, 0x00AE, 0x00AE); // Registered Symbol
		}
#endif
	}

	void RuntimeLayer::OnDetach()
	{
		DY_PROFILE_FUNCTION();
		
		m_Scene->OnRuntimeStop();
	}

	void RuntimeLayer::OnUpdate(Timestep ts)
	{
		DY_PROFILE_FUNCTION();

		static constexpr float fadeInStart = 3.0f;
		static constexpr float fadeInEnd = 5.0f;
		static constexpr float fadeOutStart = 8.0f;
		static constexpr float fadeOutEnd = 10.0f;

#ifndef DY_DIST
		m_DeltaTime = ts;
#endif
		
		const uint32_t width = Application::Get().GetWindow().GetWidth();
		const uint32_t height = Application::Get().GetWindow().GetHeight();

		m_Scene->OnViewportResize(width, height);

		// Resize
		if (FramebufferSpecification spec = m_SceneRendererContext->ActiveFramebuffer->GetSpecification();
			width > 0.0f && height > 0.0f && // zero sized framebuffer is invalid
			(spec.Width != width || spec.Height != height))
		{
			m_SceneRendererContext->Resize(width, height);
		}

		// Render
		Renderer2D::ResetStats();
		SceneRenderer::ResetStats();
		m_SceneRendererContext->ActiveFramebuffer->Bind();
		
		RenderCommand::SetClearColor(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		RenderCommand::Clear();

		//Clear entity ID attachment to -1
		const int value = -1;
		m_SceneRendererContext->ActiveFramebuffer->ClearAttachment(1, &value);

		// Manage Startup Logo
		if (m_StartupLogo)
		{
			m_StartupDisplayTime += ts;

			if (m_StartupDisplayTime >= fadeInStart)
			{
				// Animate alpha with cubic
				const float alpha =
					m_StartupDisplayTime <= fadeInEnd ? std::pow((m_StartupDisplayTime - fadeInStart) / (fadeInEnd - fadeInStart), 3.0f) :
					m_StartupDisplayTime >= fadeOutStart ? 1.0f - std::pow((m_StartupDisplayTime - fadeOutStart) / (fadeOutEnd - fadeOutStart), 3.0f) :
					1.0f;

				SceneCamera camera;
				camera.SetProjectionType(SceneCamera::ProjectionType::Orthographic);
				camera.SetOrthographicSize(2.0f);
				camera.SetViewportSize(m_SceneRendererContext->ActiveWidth, m_SceneRendererContext->ActiveHeight);

				Renderer2D::BeginScene(camera, glm::mat4(1.0f));
				Renderer2D::DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.2f, 0.0f)), m_StartupLogo, 1.0f, glm::vec4(alpha));
				Renderer2D::DrawText(glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.65f, 0.0f)), glm::vec3(0.2f)), "Dymatic Engine", TextAlignment::Center, nullptr, glm::vec4(alpha));
				Renderer2D::DrawText(glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.80f, 0.0f)), glm::vec3(0.035f)), "Copyright Dymatic Technologies 2024", TextAlignment::Center, nullptr, glm::vec4(alpha));
				Renderer2D::EndScene();

				if (m_StartupDisplayTime >= fadeOutEnd)
					m_StartupLogo = nullptr;
			}
		}
		else
		{
			// Render Scene
			m_Scene->OnUpdateRuntime(ts);
		}

		// Draw framebuffer to main framebuffer
		m_SceneRendererContext->ActiveFramebuffer->CopyColor(0);

		m_SceneRendererContext->ActiveFramebuffer->Unbind();

		// Iterate through the post update command list
		// (Which at the moment is just a list of scenes to potentially switch)
		if (!m_PostUpdateQueue.empty())
		{
			for (auto& command : m_PostUpdateQueue)
			{
				// Instruct the current scene to terminate then request the new scene from the asset manager.
				m_Scene->OnRuntimeStop();
				m_Scene = Scene::Copy(AssetManager::GetAsset<Scene>(command));
				m_Scene->OnRuntimeStart();
			}
			
			m_PostUpdateQueue.clear();
		}
	}

	void RuntimeLayer::OnImGuiRender()
	{
#ifndef DY_DIST
		static bool s_DebugOpen = false;
		static bool s_PressedLastFrame = false;
		
		const bool pressed = Input::IsKeyPressed(Key::LeftControl) && Input::IsKeyPressed(Key::LeftShift) && Input::IsKeyPressed(Key::LeftAlt) && Input::IsKeyPressed(Key::Z);
		if (pressed && !s_PressedLastFrame)
			s_DebugOpen = !s_DebugOpen;
		s_PressedLastFrame = pressed;

		
		if (s_DebugOpen)
		{
			auto& style = ImGui::GetStyle();

			const auto& window = Application::Get().GetWindow();
			const ImVec2 windowPos = ImVec2(window.GetPositionX(), window.GetPositionY());
			const ImVec2 windowSize = ImVec2(window.GetWidth(), window.GetHeight());

			auto drawList = ImGui::GetForegroundDrawList();

			// Draw debug log overlay
			{
				ImVec2 drawPos = ImVec2(windowPos.x + style.FramePadding.x, windowPos.y + windowSize.y);
				for (auto it = std::rbegin(m_DebugMessages); it != std::rend(m_DebugMessages); ++it)
				{
					auto& message = *it;

					if (drawPos.y > windowPos.y)
						drawPos.y -= ImGui::CalcTextSize(message.Text.c_str()).y + style.FramePadding.y * 2.0f;
					else
						break;

					if (message.Level == 5 /*Critical*/)
					{
						ImVec2 size = ImGui::CalcTextSize(message.Text.c_str());
						drawList->AddRectFilled(
							ImVec2(drawPos.x - style.FramePadding.x, drawPos.y - style.FramePadding.y),
							ImVec2(drawPos.x + size.x + style.FramePadding.x, drawPos.y + size.y + style.FramePadding.y),
							ImGui::GetColorU32(ImGuiCol_LogCritical)
						);
					}

					drawList->AddText(drawPos,
						// Get log level color
						ImGui::GetColorU32(
							message.Level == 0 ? ImGuiCol_LogTrace :
							message.Level == 2 ? ImGuiCol_LogInfo :
							message.Level == 3 ? ImGuiCol_LogWarn :
							message.Level == 4 ? ImGuiCol_LogError :
							message.Level == 5 ? ImGuiCol_Text :
							ImGuiCol_TextDisabled
						),
						// Access log message text
						message.Text.c_str()
					);
				}
			}

			// Draw FPS Counter
			{
				char buff[256];

				const float fps = 1.0f / m_DeltaTime;
				const ImU32 color = ImGui::GetColorU32(fps > 55.0f ? (ImVec4(0.1f, 0.8f, 0.2f, 1.0f)) : (fps > 25.0f ? (ImVec4(1.0f, 0.95f, 0.85f, 1.0f)) : (ImVec4(0.8f, 0.1f, 0.2f, 1.0f))));

				sprintf(buff, "%.2f FPS", fps);
				drawList->AddText(ImVec2(windowPos.x + windowSize.x - style.FramePadding.x - ImGui::CalcTextSize(buff).x, windowPos.y + style.FramePadding.y), color, buff);

				memset(buff, 0, 256);

				sprintf(buff, "%.2f ms", m_DeltaTime * 100.0f);
				drawList->AddText(ImVec2(windowPos.x + windowSize.x - style.FramePadding.x - ImGui::CalcTextSize(buff).x, windowPos.y + style.FramePadding.y * 3.0f + ImGui::GetTextLineHeight()), color, buff);
			}

			// Draw Version Information
			{
				const char* text[] = {
					DY_APPLICATION_NAME,
					u8"Version: " DY_APPLICATION_VERSION,
					u8"Legal: " DY_VERSION_COPYRIGHT ", " DY_VERSION_TRADEMARK " " DY_APPLICATION_COPYRIGHT " " DY_APPLICATION_TRADEMARK,
					u8"Build Information: " DY_APPLICATION_EXECUTABLE " (" __DATE__ " " __TIME__ ")",
					u8"Dymatic Engine Version " DY_VERSION
				};
				uint32_t count = sizeof(text) / sizeof(text[0]);

				for (uint32_t index = 0; index < count; index++)
					drawList->AddText(windowPos + windowSize - ImVec2(style.FramePadding.x + ImGui::CalcTextSize(text[index]).x, style.FramePadding.y + ImGui::GetTextLineHeightWithSpacing() * ((count) - index)), ImGui::GetColorU32(ImGuiCol_TextDisabled), text[index]);
			}
		}
#endif
	}

	void RuntimeLayer::OnEvent(Event& e)
	{
	}

}