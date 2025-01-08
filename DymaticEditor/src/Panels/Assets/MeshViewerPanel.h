#pragma once
#include "Panels/EditorPanel.h"

#include "Dymatic/Renderer/Model.h"
#include "Panels/Assets/Utils/AssetPanelViewport.h"

namespace Dymatic {

	class MeshViewerPanel : public EditorPanel
	{
	public:
		MeshViewerPanel(Ref<Model> mesh);

		virtual void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender(bool& open) override;

		virtual void OnEvent(Event& e) override;

		inline virtual void Focus() override { m_Focus = true; }
		
	private:
		Ref<Model> m_Mesh;
		std::vector<Model::LODInfo> m_LODInfo;

		AssetPanelViewport m_Viewport;
		
		// Panel properties
		uint32_t m_VertexCount;
		uint32_t m_IndiciesCount;

		bool m_UsingVertexPaint = false;
		glm::vec4 m_VertexPaintColor = glm::vec4(1.0f);

		bool m_Focus = false;
	};

}