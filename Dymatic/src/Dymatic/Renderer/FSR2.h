#pragma once
#include "Dymatic/Core/Timestep.h"
#include "Dymatic/Renderer/Texture.h"

#include "Dymatic/Renderer/Camera.h"

namespace Dymatic {

	class FSRManager
	{
	public:
		struct FSRContext
		{
			uint32_t ColorRendererID, DepthRendererID, MotionVectorsRendererID, OutputRendererID;
			TextureFormat ColorFormat, DepthFormat, MotionVectorsFormat, OutputFormat;
			glm::uvec2 RenderSize, DisplaySize;;
		};
	public:
		static void Init();
		static void Shutdown();

		static void Dispatch(Timestep ts);
		static void UpdateCamera(const Camera& camera);
		static glm::vec2 GetJitterOffset();

		static void SetContext(const FSRContext& context);
		static Ref<Texture2D> GetOutput();

	private:
		static void CreateContext();
		static void DestroyContext();
		static void Resize(glm::uvec2 size);
	};

}