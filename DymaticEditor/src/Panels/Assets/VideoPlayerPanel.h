#pragma once
#include "Panels/EditorPanel.h"

#include "Dymatic/Video/VideoReader.h"

namespace Dymatic {

	class VideoPlayerPanel : public EditorPanel
	{
	public:
		VideoPlayerPanel(Ref<VideoReader> videoPlayer);

		virtual void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender(bool& open) override;
		inline virtual void Focus() override { m_Focused = true; }

	private:
		void OnModify();
	private:
		Ref<VideoReader> m_VideoPlayer;
		bool m_Focused = false;
	};

}