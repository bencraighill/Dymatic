#include "dypch.h"
#include "Dymatic/Renderer/FSR2.h"

#include "Dymatic/Core/Buffer.h"
#include "Dymatic/Scene/SceneCamera.h"

#include "Platform/OpenGL/OpenGLTextureFormat.h"

#include <ffx-fsr2-api/ffx_fsr2.h>
#include <ffx-fsr2-api/gl/ffx_fsr2_gl.h>

#include <codecvt>
#include <locale>

#include <GLFW/glfw3.h>

namespace Dymatic {

	FfxFsr2Context s_FSRContext;
	Buffer s_ScratchBuffer;

	Ref<Texture2D> s_OutputTexture = nullptr;

	glm::uvec2 s_RenderSize;
	glm::uvec2 s_DisplaySize;

	static FfxFsr2DispatchDescription s_DispatchDescription;
	static glm::vec2 s_JitterOffset;
	static int32_t s_JitterPhaseIndex = 0;

#ifndef DY_DIST
	static void FfxFsr2MessageCallback(FfxFsr2MsgType type, const wchar_t* messageString)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		std::string message = converter.to_bytes(messageString);

		switch (type)
		{
		case FFX_FSR2_MESSAGE_TYPE_WARNING: DY_CORE_WARN("[ffx-fsr2]: {}", message);  return;
		case FFX_FSR2_MESSAGE_TYPE_ERROR:	DY_CORE_ERROR("[ffx-fsr2]: {}", message); return;
		}

		DY_CORE_INFO("[ffx-fsr2]: {}", message);
	}
#endif

	void FSRManager::Init()
	{
	}

	void FSRManager::Shutdown()
	{
		DestroyContext();
	}

	void FSRManager::Dispatch(Timestep ts)
	{
		GetJitterOffset();
		s_DispatchDescription.jitterOffset = { s_JitterOffset.x, s_JitterOffset.y };
		s_DispatchDescription.motionVectorScale = { (float)s_RenderSize.x, (float)s_RenderSize.y };
		s_DispatchDescription.renderSize = { s_RenderSize.x, s_RenderSize.y };

		// Should be configurable by user
		s_DispatchDescription.enableSharpening = false;
		s_DispatchDescription.sharpness = 0.0f;

		s_DispatchDescription.frameTimeDelta = ts.GetMilliseconds();
		s_DispatchDescription.preExposure = 1.0f;

		// Should be set to true for the first frame after a discontinuous camera cut
		s_DispatchDescription.reset = false;

		const FfxErrorCode code = ffxFsr2ContextDispatch(&s_FSRContext, &s_DispatchDescription);
		DY_CORE_VERIFY(code == FFX_OK);
	}

	void FSRManager::UpdateCamera(const Camera& camera)
	{
		s_DispatchDescription.cameraNear = camera.GetNearClip();
		s_DispatchDescription.cameraFar = camera.GetFarClip();
		s_DispatchDescription.cameraFovAngleVertical = glm::radians(camera.GetFOV());
	}

	glm::vec2 FSRManager::GetJitterOffset()
	{
		const int32_t phaseCount = ffxFsr2GetJitterPhaseCount(s_RenderSize.x, s_DisplaySize.x);
		s_JitterPhaseIndex = (s_JitterPhaseIndex + 1) % phaseCount;
		ffxFsr2GetJitterOffset(&s_JitterOffset.x, &s_JitterOffset.y, s_JitterPhaseIndex, phaseCount);

		return { 2.0f * s_JitterOffset.x / static_cast<float>(s_RenderSize.x), 2.0f * s_JitterOffset.y / static_cast<float>(s_RenderSize.y) };
	}

	static const FfxResource GetFfxTexture(uint32_t rendererID, const glm::uvec2 size, const TextureFormat format, const wchar_t* debugName)
	{
		const GLenum internalFormat = Utils::DymaticTextureFormatToGLInternalFormat(format);
		return ffxGetTextureResourceGL(rendererID, size.x, size.y, internalFormat, debugName);
	}

	void FSRManager::SetContext(const FSRContext& context)
	{
		if (s_RenderSize != context.RenderSize)
			Resize(context.RenderSize);

		// Get internal texture objects
		s_DispatchDescription.color			= GetFfxTexture(context.ColorRendererID,			context.RenderSize,  context.ColorFormat,			L"FSR Dymatic Color Texture");
		s_DispatchDescription.depth			= GetFfxTexture(context.DepthRendererID,			context.RenderSize,  context.DepthFormat,			L"FSR Dymatic Depth Texture");
		s_DispatchDescription.motionVectors = GetFfxTexture(context.MotionVectorsRendererID,	context.RenderSize,  context.MotionVectorsFormat,	L"FSR Dymatic Motion Vector Texture");
	}

	Ref<Texture2D> FSRManager::GetOutput()
	{
		return s_OutputTexture;
	}

	void FSRManager::CreateContext()
	{
		// Create the output texture
		TextureSpecification outputSpecification;
		outputSpecification.Width = s_DisplaySize.x;
		outputSpecification.Height = s_DisplaySize.y;
		outputSpecification.Format = TextureFormat::RGBA16F;

		// Create default data
		Buffer buffer = Buffer(s_DisplaySize.x * s_DisplaySize.y * Utils::GetDymaticTextureFormatBPP(outputSpecification.Format));

		for (uint32_t i = 0; i < s_DisplaySize.x * s_DisplaySize.y; i++)
			((glm::vec4*)buffer.Data)[i] = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

		s_OutputTexture = Texture2D::Create(outputSpecification, buffer);
		buffer.Release();

		s_DispatchDescription.output = GetFfxTexture(s_OutputTexture->GetRendererID(), s_DisplaySize, outputSpecification.Format, L"FSR Dymatic Output Texture");

		// Allocate a scratch buffer
		const size_t scratchBufferSize = ffxFsr2GetScratchMemorySizeGL();
		s_ScratchBuffer = Buffer(scratchBufferSize);

		// Get the FSR2 OpenGL interface
		FfxFsr2Interface fsrInterface;
		DY_CORE_VERIFY(ffxFsr2GetInterfaceGL(&fsrInterface, s_ScratchBuffer.Data, s_ScratchBuffer.Size, glfwGetProcAddress) == FFX_OK, "Failed to get FSR2 OpenGL interface!");

		// Setup the FSR2 context
		FfxFsr2ContextDescription contextDescription;
		contextDescription.device;
		contextDescription.callbacks = fsrInterface;
		contextDescription.maxRenderSize = { s_RenderSize.x, s_RenderSize.y };
		contextDescription.displaySize = { s_DisplaySize.x, s_DisplaySize.y };
		contextDescription.flags = FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE | FFX_FSR2_ENABLE_AUTO_EXPOSURE | FFX_FSR2_ALLOW_NULL_DEVICE_AND_COMMAND_LIST;

#ifdef DY_DEBUG
		contextDescription.flags |= FFX_FSR2_ENABLE_DEBUG_CHECKING;
#endif

#ifndef DY_DIST
		contextDescription.fpMessage = FfxFsr2MessageCallback;
#endif

		DY_CORE_VERIFY(ffxFsr2ContextCreate(&s_FSRContext, &contextDescription) == FFX_OK, "Failed to create FSR2 context!");

		// Dispatch Description Setup (for fixed values)
		s_DispatchDescription.commandList;
		s_DispatchDescription.exposure = {};
		s_DispatchDescription.reactive = {};
		s_DispatchDescription.transparencyAndComposition = {};
		s_DispatchDescription.viewSpaceToMetersFactor = 1.0f;

		s_DispatchDescription.deviceDepthNegativeOneToOne = false;
	}

	void FSRManager::DestroyContext()
	{
		if (!s_OutputTexture)
			return;

		// Note: This should be called during complete engine shutdown OR when the user disables FSR
		// Note: The GPU should be idle before this is called (does not account for in-flight resources)
		ffxFsr2ContextDestroy(&s_FSRContext);

		s_OutputTexture = nullptr;

		s_ScratchBuffer.Release();
	}

	void FSRManager::Resize(glm::uvec2 size)
	{
		s_RenderSize = size;
		s_DisplaySize = glm::vec2(size) * 1.7f;

		DestroyContext();
		CreateContext();
	}

}