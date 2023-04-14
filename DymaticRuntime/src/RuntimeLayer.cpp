#include "RuntimeLayer.h"
#include "Dymatic/Asset/AssetManager.h"
#include "Dymatic/Scene/SceneSerializer.h"
#include "Dymatic/Scripting/ScriptEngine.h"

#include "Dymatic/Renderer/SceneRenderer.h"

#include "Version.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace Dymatic {

	static const char* s_ProjectPath = "C:/dev/Dymatic/DymaticEditor/SandboxProject/Sandbox.dyproject";

	RuntimeLayer::RuntimeLayer()
		: Layer("RuntimeLayer")
	{
	}

	void RuntimeLayer::OnAttach()
	{
		DY_PROFILE_FUNCTION();

		// Setup the framebuffer and renderer
		FramebufferSpecification fbSpec;
		fbSpec.Attachments = { 
			TextureFormat::RGBA16F,			// Color
			TextureFormat::RED_INTEGER,		// EntityID
			TextureFormat::Depth,			// Depth
			TextureFormat::RGBA16F,			// Normal
			TextureFormat::RGBA16F,			// Emissive
			TextureFormat::RGBA8			// Roughness + Metallic + Specular + AO
		};
		fbSpec.Width = 1600;
		fbSpec.Height = 900;
		fbSpec.Samples = 1;
		m_Framebuffer = Framebuffer::Create(fbSpec);
		SceneRenderer::SetActiveFramebuffer(m_Framebuffer);

		Renderer2D::SetLineWidth(4.0f);

		// Load the project
		Project::Load(s_ProjectPath);
		AssetManager::Deserialize();
		
		Log::ShowConsole();
		ScriptEngine::SetCoreAssemblyPath(Project::GetCoreModulePath());
		ScriptEngine::SetAppAssemblyPath(Project::GetScriptModulePath());
		ScriptEngine::ReloadAssembly();

		// Open the start scene
		m_Scene = CreateRef<Scene>();
		SceneSerializer serializer(m_Scene);
		serializer.Deserialize(Project::GetAssetFileSystemPath(Project::GetActive()->GetConfig().StartScene).string());
		
		// Start the scene runtime
		m_Scene->OnRuntimeStart();

		// Finally, show window when ready (if it was hidden)
		if (Application::Get().GetSpecification().WindowStartHidden)
			Application::Get().GetWindow().ShowWindow();

		// Setup ImGui debug overlay
		{
			// Prevent ini settings dumping
			ImGui::GetIO().IniFilename = nullptr;

			ImGuiLayer* imguiLayer = Application::Get().GetImGuiLayer();
			imguiLayer->AddIconFont("assets/fonts/IconsFont.ttf", 20.0f, 0x00A9, 0x00A9); // Copyright Symbol
			imguiLayer->AddIconFont("assets/fonts/IconsFont.ttf", 20.0f, 0x00AE, 0x00AE); // Registered Symbol
		}
	}

	void RuntimeLayer::OnDetach()
	{
		DY_PROFILE_FUNCTION();
		
		m_Scene->OnRuntimeStop();
	}

	void RuntimeLayer::OnUpdate(Timestep ts)
	{
		DY_PROFILE_FUNCTION();

		m_DeltaTime = ts;
		
		const uint32_t width = Application::Get().GetWindow().GetWidth();
		const uint32_t height = Application::Get().GetWindow().GetHeight();

		m_Scene->OnViewportResize(width, height);

		// Resize
		if (FramebufferSpecification spec = m_Framebuffer->GetSpecification();
			width > 0.0f && height > 0.0f && // zero sized framebuffer is invalid
			(spec.Width != width || spec.Height != height))
		{
			m_Framebuffer->Resize(width, height);
			SceneRenderer::Resize();
		}

		// Render
		Renderer2D::ResetStats();
		SceneRenderer::ResetStats();
		m_Framebuffer->Bind();
		
		RenderCommand::SetClearColor(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		RenderCommand::Clear();

		//Clear entity ID attachment to -1
		int value = -1;
		m_Framebuffer->ClearAttachment(1, &value);

		m_Scene->OnUpdateRuntime(ts);

		// Draw framebuffer to main framebuffer
		m_Framebuffer->Copy(0);

		m_Framebuffer->Unbind();
	}

	void RuntimeLayer::OnImGuiRender()
	{
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
				const float spacing = ImGui::GetTextLineHeight() + style.FramePadding.y * 2.0f;

				ImVec2 drawPos = ImVec2(windowPos.x + style.FramePadding.x, windowPos.y + windowSize.y - spacing);
				auto& messages = Log::GetMessages();
				for (uint32_t index = messages.size() - 1; index > 0; index--)
				{
					auto& message = messages[index];

					if (message.Level == 5 /*Critical*/)
					{
						const ImVec2 size = ImGui::CalcTextSize(message.Text.c_str());
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

					if (drawPos.y > windowPos.y)
						drawPos.y -= spacing;
					else
						break;
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

				sprintf(buff, "%.2f ms", m_DeltaTime);
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
	}

	void RuntimeLayer::OnEvent(Event& e)
	{
	}

}