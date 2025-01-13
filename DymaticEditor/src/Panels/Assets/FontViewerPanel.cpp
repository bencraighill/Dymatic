#include "Panels/Assets/FontViewerPanel.h"

#include "Panels/Assets/Utils/AssetViewerPanelUtils.h"

#include "Dymatic/Asset/AssetManager.h"

#include "Panels/UI.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>

namespace Dymatic {

	static const uint32_t s_GlyphPreviewSize = 128;

	FontViewerPanel::FontViewerPanel(const AssetHandle handle)
	{
		m_Font = AssetManager::RequestAsset<Font>(handle);

		// Setup renderer resources
		FramebufferSpecification specification;
		specification.Width = m_ViewportSize.x;
		specification.Height = m_ViewportSize.y;
		specification.Attachments = { TextureFormat::RGBA8, TextureFormat::Depth };
		m_Framebuffer = Framebuffer::Create(specification);

		m_Camera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
		m_Camera.SetOrthographic(2.0f, 0.0f, 1.0f);
		m_CameraTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f));

		m_PreviewInputBuffer = "Preview";
	}

	void FontViewerPanel::OnUpdate(Timestep ts)
	{
		if (!m_Font->IsLoaded())
			return;

		// Render the font preview

		// Check if we need to resize
		if (m_TabBarIndex == 0)
		{
			const bool resize = m_Framebuffer->GetSpecification().Width != m_ViewportSize.x || m_Framebuffer->GetSpecification().Height != m_ViewportSize.y;

			if (resize)
			{
				m_Framebuffer->Resize(m_ViewportSize.x, m_ViewportSize.y);
				m_Camera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
			}

			// Bind, Render, then Unbind
			if (resize || m_Invalidate)
			{
				m_Framebuffer->Bind();
				RenderCommand::SetClearColor(glm::vec4(0.0f));
				RenderCommand::Clear();
				Renderer2D::BeginScene(m_Camera, m_CameraTransform);
				Renderer2D::DrawText(glm::scale(glm::mat4(1.0f), glm::vec3(m_Scale)), m_PreviewInputBuffer, TextAlignment::Center, m_Font, glm::vec4(1.0f));
				Renderer2D::EndScene();
				m_Framebuffer->Unbind();

				m_Invalidate = false;
			}
		}
		else if (m_TabBarIndex == 1 && m_GlyphThumbnails.empty())
		{
			// Generate glyph thumbnail cache
			
			FramebufferSpecification specification;
			specification.Width = s_GlyphPreviewSize;
			specification.Height = s_GlyphPreviewSize;
			specification.Attachments = { TextureFormat::RGBA8, TextureFormat::Depth };
			Ref<Framebuffer> framebuffer = Framebuffer::Create(specification);
			
			SceneCamera camera;
			glm::mat4 cameraTransform;
			camera.SetViewportSize(s_GlyphPreviewSize, s_GlyphPreviewSize);
			camera.SetOrthographic(2.0f, 0.0f, 1.0f);
			cameraTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f));

			const auto& glyphs = m_Font->GetGlyphs();
			for (auto& [codepoint, glyph] : glyphs)
			{
				// Render glyph to framebuffer
				framebuffer->Bind();
				RenderCommand::SetClearColor({ 0.0f, 0.0f, 0.0f, 0.5f });
				RenderCommand::Clear();
				Renderer2D::BeginScene(camera, cameraTransform);
				Renderer2D::DrawText(glm::mat4(1.0f), std::string(1, (char)codepoint), TextAlignment::Center, m_Font, glm::vec4(1.0f));
				Renderer2D::EndScene();

				// Copy framebuffer to texture
				TextureSpecification textureSpecification;
				textureSpecification.Width = s_GlyphPreviewSize;
				textureSpecification.Height = s_GlyphPreviewSize;
				Ref<Texture2D> texture = Texture2D::Create(textureSpecification, framebuffer->CopyColorBuffer(0));

				framebuffer->Unbind();

				m_GlyphThumbnails[codepoint] = texture;
			}
		}
	}

	void FontViewerPanel::OnImGuiRender(bool& open)
	{
		const AssetMetadata& metadata = AssetManager::GetMetadata(m_Font->Handle);

		ImGui::PushID(m_Font->Handle);

		if (m_Focus)
		{
			ImGui::SetNextWindowFocus();
			m_Focus = false;
		}

		UI::CenterAppearingWindow(ImVec2(512.0f, 512.0f));
		ImGui::Begin(Utils::GetViewerWindowName(FA_FONT, m_Font->Handle, metadata.FilePath, "Font Viewer").c_str(), &open);

		const char* tabBarOptions[3] = { FA_EYE " Preview", FA_FONT " Glyphs", FA_GRID_4 " Atlas" };
		ImGui::SwitchButtonEx("##FontViewerPanelTabSwitchButton", tabBarOptions, IM_ARRAYSIZE(tabBarOptions), &m_TabBarIndex, ImVec2(ImGui::GetContentRegionAvailWidth(), 0.0f));

		ImGui::Separator();

		ImGui::BeginChild("##FontViewerPanelTabRegion");

		if (m_TabBarIndex == 0)
		{
			// Preview Tab
			// A live preview where example text can be entered with a resizing framebuffer

			// Draw the image
			m_ViewportSize = ImVec2(ImGui::GetContentRegionAvailWidth(), ImGui::GetContentRegionAvailHeight() * 0.5f);
			ImGui::Image((ImTextureID)m_Framebuffer->GetColorAttachmentRendererID(0), m_ViewportSize, { 0, 1 }, { 1, 0 });

			ImGui::Separator();

			if (ImGui::InputTextMultiline("##FontViewerPanelPreviewInput", &m_PreviewInputBuffer, ImGui::GetContentRegionAvail()))
				m_Invalidate = true;

			const ImGuiIO& io = ImGui::GetIO();
			if (ImGui::IsWindowHovered() && io.KeyCtrl && io.MouseWheel != 0.0f)
			{
				m_Scale = std::clamp(m_Scale * (1.0f + io.MouseWheel * 0.1f), 0.01f, 5.0f);
				m_Invalidate = true;
			}
		}
		else if (m_TabBarIndex == 1)
		{
			// Glyphs Tab
			const auto& glyphs = m_Font->GetGlyphs();

			// Table View with icon (cached) and details laid out underneath
			const ImVec2 buttonSize = ImVec2(s_GlyphPreviewSize, s_GlyphPreviewSize);
			const ImGuiTableFlags flags = ImGuiTableFlags_PadOuterX | ImGuiTableFlags_NoBordersInBody;
			const int columnCount = std::clamp((int)(ImGui::GetContentRegionAvailWidth() / s_GlyphPreviewSize), 1, IMGUI_TABLE_MAX_COLUMNS);
			ImGui::BeginTable("##FontViewerPanelGlyphTable", columnCount, flags, ImVec2(ImGui::GetContentRegionAvailWidth(), 0.0f));

			ImDrawList* drawList = ImGui::GetWindowDrawList();

			for (const auto& [codepoint, glyph] : glyphs)
			{
				ImGui::TableNextColumn();

				ImGui::InvisibleButton("##InvisibleGlyphButton", buttonSize);

				if (m_GlyphThumbnails.find(codepoint) != m_GlyphThumbnails.end())
					drawList->AddImage((ImTextureID)m_GlyphThumbnails.at(codepoint)->GetRendererID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), { 0, 1 }, { 1, 0 });
				
				if (UI::IsItemHoveredTooltip())
				{
					ImGui::BeginTooltip();
					ImGui::TextDisabled(FA_SLIDERS " Glyph Properties:");

					ImGui::TextDisabled(FA_TEXT " Codepoint: ");
					ImGui::SameLine();
					ImGui::Text("%d (%c)", glyph.Codepoint, (char)glyph.Codepoint);

					ImGui::TextDisabled(FA_KERNING " Type: ");
					ImGui::SameLine();
					ImGui::Text(glyph.IsWhitespace ? "Whitespace" : "Character");

					ImGui::TextDisabled(FA_DISTRIBUTE_SPACING_HORIZONTAL " Advance: ");
					ImGui::SameLine();
					ImGui::Text("%.2f", glyph.Advance);

					ImGui::Separator();

					ImGui::TextDisabled(FA_EXPAND " Sizing");

					ImGui::TextDisabled("Atlas Bounds: ");
					ImGui::SameLine();
					ImGui::Text("(%.2f, %.2f) - (%.2f, %.2f)", glyph.Min.x, glyph.Min.y, glyph.Max.x, glyph.Max.y);

					ImGui::TextDisabled("Quad Plane Bounds: ");
					ImGui::SameLine();
					ImGui::Text("Left: %.2f, Bottom: %.2f, Right: %.2f, Top: %.2f", glyph.Left, glyph.Bottom, glyph.Right, glyph.Top);

					ImGui::TextDisabled("Quad Plane Size: ");
					ImGui::SameLine();
					ImGui::Text("(%.2f, %.2f)", glyph.Size.x, glyph.Size.y);

					ImGui::EndTooltip();
				}
			}

			ImGui::EndTable();
		}
		else if (m_TabBarIndex == 2 && m_Font->IsLoaded())
		{
			// Atlas Tab
			Ref<Texture2D> atlas = m_Font->GetAtlas();
			ImGui::Image((ImTextureID)atlas->GetRendererID(), Utils::GetMaximizedSize(atlas->GetSize(), ImGui::GetWindowSize()), { 0, 1 }, { 1, 0 });
		}

		ImGui::EndChild();

		ImGui::End();

		ImGui::PopID();
	}

}