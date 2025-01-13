#include "Panels/Assets/MeshViewerPanel.h"

#include "Dymatic/Asset/AssetManager.h"

#include "Panels/UI.h"
#include "Panels/Assets/Utils/AssetViewerPanelUtils.h"

#include "Dymatic/Core/Application.h"
#include "Dymatic/Renderer/SceneRenderer.h"

#include <imgui/imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>

namespace Dymatic {

	// Note: This value needs to be reflected in editor shaders
	static constexpr uint32_t MaxVertexSelectionPerFrame = 256;

	struct VertexPaintData
	{
		glm::vec2 BrushPosition;
		float BrushRadius = 0.2f;
		bool Painting = false;
	};

	static Ref<Shader> s_VertexPaintShader = nullptr;
	static Ref<ShaderStorageBuffer> s_PaintedVerticesSSBO = nullptr;
	static VertexPaintData s_VertexPaintBuffer;

	static void InitSharedResources()
	{
		if (s_VertexPaintShader)
			return;

		s_VertexPaintShader = Shader::Create("Resources/Shaders/Editor/VertexPaint/Editor_VertexPaint.glsl");
		s_PaintedVerticesSSBO = ShaderStorageBuffer::Create(sizeof(uint32_t) + MaxVertexSelectionPerFrame * sizeof(uint32_t), ShaderStorageBufferUsage::DYNAMIC_COPY);
		s_PaintedVerticesSSBO->Bind(RendererConstants::Editor);
	}

	MeshViewerPanel::MeshViewerPanel(Ref<Model> mesh)
		: m_Mesh(mesh), m_LODInfo(mesh->GetLodInfo())
	{
		InitSharedResources();

		// Calculate display statistics
		m_VertexCount = 0;
		m_IndiciesCount = 0;

		for (const auto& submesh : mesh->GetMeshes())
		{
			m_VertexCount += submesh->GetVerticies().size();
			m_IndiciesCount += submesh->GetIndicies().size();
		}
	}

	void MeshViewerPanel::OnUpdate(Timestep ts)
	{
		// Update Vertex Painting UBO
		s_VertexPaintBuffer.Painting = m_UsingVertexPaint && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered() && m_Viewport.IsHovered();
		if (s_VertexPaintBuffer.Painting)
		{
			// Convert mouse position to NDC space
			s_VertexPaintBuffer.BrushPosition = m_Viewport.GetHoveredUV() * 2.0f - 1.0f;

			uint32_t vertexCount = 0;
			s_PaintedVerticesSSBO->SetData(&vertexCount, sizeof(uint32_t));
		}

		Renderer::SetEditorScratchBufferData(&s_VertexPaintBuffer, sizeof(VertexPaintData));

		// Draw the viewport
		m_Viewport.BeginViewportRender(ts);

		if (m_UsingVertexPaint)
		{
			m_Viewport.ClearViewport(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
			SceneRenderer::SubmitDrawShaderOverride(s_VertexPaintShader);
		}

		SceneRenderer::SubmitModel(glm::mat4(1.0f), m_Mesh);
		m_Viewport.SubmitViewportRender();
		m_Viewport.EndViewportRender();

		// Read back vertex painting data if it occurred
		if (s_VertexPaintBuffer.Painting)
		{
			uint32_t vertexCount;
			s_PaintedVerticesSSBO->GetData(&vertexCount, sizeof(uint32_t));
			vertexCount = glm::min(vertexCount, MaxVertexSelectionPerFrame);

			if (vertexCount)
			{
				ScopedBuffer buffer(vertexCount * sizeof(uint32_t));
				s_PaintedVerticesSSBO->GetData(buffer.Data, buffer.Size, sizeof(uint32_t));

				const auto& meshes = m_Mesh->GetMeshesEditable();
				std::unordered_set<Ref<Mesh>> modifiedMeshes;

				for (uint32_t index = 0; index < vertexCount; index++)
				{
					const uint32_t globalIndex = buffer.As<uint32_t>()[index];

					// TODO: We could optimize this drastically by storing the SubmeshIndex and LocalIndex on the GPU (although we would need a a_LocalIndex vertex attribute)
					// Determine which mesh we are in
					uint32_t accumulatedVertices = 0;
					for (auto& mesh : meshes)
					{
						const uint32_t nextAccumulated = accumulatedVertices + mesh->GetVertexCount();

						if (globalIndex < nextAccumulated)
						{
							// Once we have found the mesh based on global index, push it
							const uint32_t localIndex = globalIndex - accumulatedVertices;
							mesh->GetVerticesEditable()[localIndex].Color = m_VertexPaintColor;
							modifiedMeshes.insert(mesh);
							break;
						}

						accumulatedVertices = nextAccumulated;
					}
				}

				// Update all affected meshes
				for (const auto& mesh : modifiedMeshes)
					mesh->UpdateVertexData();
			}
		}
	}

	static void SetupDockspace(ImGuiID dockspace, uint64_t handle)
	{
		// Clear the current dockspace layout
		ImGui::DockBuilderRemoveNodeChildNodes(dockspace);

		const float viewportPanelRatio = 0.75f;

		ImGuiID mainPanel = ImGui::DockBuilderGetNode(dockspace)->ID;

		// Split dockspace
		ImGuiID leftPanel, rightPanel;
		ImGui::DockBuilderSplitNode(mainPanel, ImGuiDir_Left, viewportPanelRatio, &leftPanel, &rightPanel);
		ImGui::DockBuilderGetNode(leftPanel)->LocalFlags  |= ImGuiDockNodeFlags_HiddenTabBar;
		ImGui::DockBuilderGetNode(rightPanel)->LocalFlags |= ImGuiDockNodeFlags_HiddenTabBar;
		ImGui::DockBuilderDockWindow(UI::GetFixedWindowNameWithID("Viewport", handle).c_str(), leftPanel);
		ImGui::DockBuilderDockWindow(UI::GetFixedWindowNameWithID("Details",  handle).c_str(), rightPanel);
	}

	void MeshViewerPanel::OnImGuiRender(bool& open)
	{
		m_Viewport.OnPreImGuiRender();

		const AssetMetadata& metadata = AssetManager::GetMetadata(m_Mesh->Handle);

		if (m_Focus)
		{
			ImGui::SetNextWindowFocus();
			m_Focus = false;
		}

		const ImGuiStyle& style = ImGui::GetStyle();
		ImGuiWindowClass windowClass = UI::CreateDockingRestrictionClass(fmt::format("##MeshDockClass{}", m_Mesh->Handle).c_str());
		UI::CenterAppearingWindow(0.75f);
		const bool freezeViewport = (m_Viewport.IsHovered() && ImGui::GetIO().KeyAlt) || (m_UsingVertexPaint && ImGui::IsMouseDown(ImGuiMouseButton_Left) && m_Viewport.IsHovered());
		const ImGuiID dockspace = UI::BeginDockspaceWindow(Utils::GetViewerWindowName(FILE_ICON_MESH, m_Mesh->Handle, metadata.FilePath, "Mesh Viewer").c_str(), &open, ImGuiWindowFlags_MenuBar | (freezeViewport ? ImGuiWindowFlags_NoMove : 0), &windowClass);

		// Setup dockspace
		if (ImGui::IsWindowAppearing())
			SetupDockspace(dockspace, m_Mesh->Handle);

		// Draw Menu Bar
		ImGui::BeginMenuBar();
		ImGui::MenuItem(FA_FLOPPY_DISK " Save");

		if (ImGui::BeginMenu(FA_EYE " View"))
		{
			ImGui::MenuItem(FA_PAINTBRUSH " Vertex Paint Mode", "", &m_UsingVertexPaint);
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();

		ImGui::End();

		ImGui::SetNextWindowClass(&windowClass);
		ImGui::Begin(UI::GetFixedWindowNameWithID("Viewport", m_Mesh->Handle).c_str());
		m_Viewport.OnImGuiRender();

		if (m_UsingVertexPaint)
		{
			ImGui::SameLine(ImGui::GetWindowWidth() * 0.5f);
			ImGui::ColorEdit4("##ViewportVertexPaintColor", glm::value_ptr(m_VertexPaintColor), ImGuiColorEditFlags_NoInputs);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(100.0f);
			ImGui::DragFloat("##ViewportVertexPaintRadius", &s_VertexPaintBuffer.BrushRadius, 0.025f, 0.0f, 1.0f);
		}

		ImGui::End();

		ImGui::SetNextWindowClass(&windowClass);
		ImGui::Begin(UI::GetFixedWindowNameWithID("Details", m_Mesh->Handle).c_str());

		ImGui::TextDisabled(FA_LAYER_GROUP " LOD Settings");

		// Special handling of mesh source (LOD 0)
		if (UI::CollapsingHeader(FA_SQUARE " LOD 0"))
		{
			ImGui::TextDisabled(FA_EXPAND " Screen Size");
			ImGui::SameLine();
			ImGui::Text("1.0");

			AssetHandle sourceHandle = m_Mesh->GetSourceHandle();
			ImGui::TextDisabled(FA_CIRCLE_DOT " Target");
			ImGui::SameLine();
			ImGui::Text("Source Mesh");

			ImGui::Separator();

			ImGui::TextDisabled(FILE_ICON_MESH_SOURCE " Source Mesh");
			ImGui::SameLine();
			ImGui::Text(fmt::format("{} ({})", sourceHandle, AssetManager::GetMetadata(sourceHandle).FilePath.string()).c_str());
			ImGui::TextDisabled(FA_CODE_MERGE " Vertex Count");
			ImGui::SameLine();
			ImGui::Text("%d", m_VertexCount);
			ImGui::TextDisabled(FA_LIST_TREE "Index Count");
			ImGui::SameLine();
			ImGui::Text("%d", m_IndiciesCount);

			ImGui::TreePop();
		}

		const float buttonHeight = 35.0f;
		bool reload = false;

		// LOD Settings
		const auto& lodInfo = m_Mesh->GetLodInfo();
		for (size_t lodIndex = 0; lodIndex < lodInfo.size(); lodIndex++)
		{
			ImGui::PushID(lodIndex);

			auto& info = m_LODInfo[lodIndex];
			if (UI::CollapsingHeader(fmt::format(FA_SQUARE " LOD {}", lodIndex + 1).c_str()))
			{
				ImGui::Text(FA_EXPAND "Target Screen Size");
				ImGui::SameLine();
				ImGui::DragFloat("##LODScreenSizeInput", &info.ScreenSize, 1.0f, 0.0f, 1.0f);

				ImGui::TextDisabled(FA_CIRCLE_DOT " Target");
				ImGui::SameLine();
				int target = info.UseAutoLOD;
				const char* const targetOptions[] = { "Custom/Authored LOD", "Auto Generated LOD" };
				if (ImGui::Combo("##LODTargetDropdown", &target, targetOptions, IM_ARRAYSIZE(targetOptions)) && info.UseAutoLOD != target)
				{
					info.UseAutoLOD = target;

					if (info.UseAutoLOD)
						info.ScreenSize = 1.0f;
					else
						info.SourceHandle = 0;
				}

				if (info.UseAutoLOD)
				{
					ImGui::Text("LOD Reduction Factor");
					ImGui::SameLine();
					ImGui::DragFloat("##LODReducationFactorInput", &info.LODReductionFactor, 1.0f, 0.0f, 1.0f);
				}
				else
				{
					UI::DrawAssetSelectionDropdown(AssetType::MeshSource, info.SourceHandle);
				}

				const ImVec2 buttonSize = ImVec2(ImGui::GetContentRegionAvailWidth() * 0.5f, buttonHeight);
				if (ImGui::Button(FA_ROTATE " Apply", buttonSize))
				{
					m_Mesh->UpdateLod(lodIndex + 1, info);
					reload = true;
				}

				ImGui::SameLine();

				if (ImGui::Button(FA_LAYER_MINUS " Delete", buttonSize))
				{
					m_Mesh->DeleteLod(lodIndex + 1);
					reload = true;
				}

				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		ImGui::Dummy(ImVec2(0.0f, ImGui::GetContentRegionAvailHeight() - buttonHeight - style.WindowPadding.y * 2.0f));

		// Create LOD UI
		if (ImGui::Button(FA_LAYER_PLUS " Create New LOD", ImVec2(ImGui::GetContentRegionAvailWidth(), buttonHeight)))
		{
			Model::LODInfo newLod;
			newLod.UseAutoLOD = true;
			newLod.ScreenSize = (m_LODInfo.empty() ? 1.0f : m_LODInfo.back().ScreenSize) * 0.5f;
			newLod.LODReductionFactor = (m_LODInfo.empty() ? 1.0f : m_LODInfo.back().LODReductionFactor) * 0.75f;
			m_Mesh->CreateLod(newLod);

			reload = true;
		}

		if (reload)
		{
			// A change was made, reload the lod info
			AssetManager::SerializeAsset(m_Mesh);
			m_LODInfo = m_Mesh->GetLodInfo();
		}

		ImGui::End();
	}

	void MeshViewerPanel::OnEvent(Event& e)
	{
		m_Viewport.OnEvent(e);
	}

}