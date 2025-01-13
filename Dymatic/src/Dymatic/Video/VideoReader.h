#pragma once
#include "Dymatic/Core/Base.h"
#include "Dymatic/Core/Buffer.h"
#include "Dymatic/Core/Timestep.h"

#include "Dymatic/Renderer/Texture.h"
#include "Dymatic/Renderer/Framebuffer.h"
#include "Dymatic/Video/Video.h"
#include "Dymatic/Video/Subtitle.h"

#include <filesystem>
#include <deque>

// Forward declare structures from avformat.h
struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVSubtitle;
struct AVPacket;
struct AVStream;
struct SwsContext;

namespace Dymatic {

	struct VideoReaderSpecification
	{
		Ref<Video> VideoStream = nullptr;
		Ref<Subtitle> SubtitleStream = nullptr;
		Ref<Texture2D> Target = nullptr;

		// Note: Zero width/height indicates to use video default.
		uint32_t Width = 0;
		uint32_t Height = 0;

		bool Subtitles = true;
		bool Looping = true;
		
		// Warning: Buffer size must be larger than the number of buffered frames in a video (or they will be rendered out of order).
		uint32_t BufferSize = 16;
	};

	struct SubtitleData
	{
		double StartDisplayTime = 0.0;
		double EndDisplayTime = 0.0;

		uint32_t Marked;
		uint32_t Layer;
		std::string Style;
		std::string Name;
		uint32_t MarginL;
		uint32_t MarginR;
		uint32_t MarginV;
		std::string Effect;
		std::string Text;
	};

	class VideoReader : public Asset
	{
	public:
		static Ref<VideoReader> Create() { return CreateRef<VideoReader>(); }

		VideoReader() = default;
		VideoReader(const VideoReaderSpecification& specification);
		~VideoReader();

		bool ReadFrame();
		bool SetTime(double time);
		bool SetTime(int64_t timestamp);
		
		inline uint32_t GetSourceWidth() const { return m_SourceWidth; }
		inline uint32_t GetSourceHeight() const { return m_SourceHeight; }

		inline bool IsLoaded() const { return m_IsLoaded; }
		inline Buffer GetBuffer() const { return m_Buffer; }

		inline Ref<Video> GetSource() const { return m_Specification.VideoStream; }
		inline Ref<Video>& GetSource() { return m_Specification.VideoStream; }

		inline Ref<Subtitle> GetSubtitles() const { return m_Specification.SubtitleStream; }
		inline Ref<Subtitle>& GetSubtitles() { return m_Specification.SubtitleStream; }

		inline Ref<Texture2D> GetTarget() const { return m_Target; }
		inline Ref<Texture2D>& GetTarget() { return m_Target; }

		Ref<Texture2D> GetCurrentFrame();
		Ref<Texture2D> GetNextFrame();
		Ref<Texture2D> GetNextFrame(Timestep ts);

		void Invalidate();

		const std::string GetCurrentSubtitle();

		static AssetType GetStaticType() { return AssetType::VideoPlayer; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	private:
		void Release();

		bool DecodeSingleFrame();
		void SeekFrame(double time);

		inline const uint32_t GetTargetWidth() const { return m_Specification.Width == 0 ? m_SourceWidth : m_Specification.Width; }
		inline const uint32_t GetTargetHeight() const { return m_Specification.Height == 0 ? m_SourceHeight : m_Specification.Height; }

	private:
		uint32_t m_SourceWidth, m_SourceHeight;
		uint32_t m_PresentationTimestamp;
		bool m_IsLoaded = false;
		VideoReaderSpecification m_Specification;

		Ref<Texture2D> m_Target = nullptr;
		
		// Local buffer
		Buffer m_Buffer;

		// Playback state
		double m_CurrentTime = 0.0;
		double m_FrameDelta;
		double m_Duration;

		// Subtitles
		SubtitleData m_Subtitle;

		// Internal State

		// Video
		AVFormatContext* m_AVFormatContext = nullptr;
		AVPacket* m_AVPacket = nullptr;
		int m_VideoStreamIndex = -1;
		AVCodecContext* m_AVVideoCodecContext = nullptr;
		SwsContext* m_SwsScalerContext = nullptr;
		std::deque<AVFrame*> m_AVFrames;
		double m_TimeBase;

		// Subtitles (External Only)
		AVFormatContext* m_AVSubtitleFormatContext = nullptr;
		double m_SubtitleTimeBase;

		// Subtitles (Shared)
		int m_SubtitleStreamIndex = -1;
		AVCodecContext* m_AVSubtitleCodecContext = nullptr;
	};

	struct VideoWriterSpecification
	{
		std::filesystem::path Path;
		Ref<Framebuffer> Framebuffer = nullptr;

		uint32_t Width = 0, Height = 0;
		uint32_t Bitrate = 3000000;
		uint32_t FPS = 25;
	};

	class VideoWriter
	{
	public:
		VideoWriter(const VideoWriterSpecification& specification);
		~VideoWriter();

		bool WriteFrame(Timestep ts);
		bool WriteFrame(Timestep ts, Ref<Texture2D> frame);

		void Close();

		inline bool IsStreamValid() const { return m_IsStreamValid; }

	private:
		void RenderLocalBuffer();
		bool OutputFrame();

	private:
		VideoWriterSpecification m_Specification;

		double m_CurrentTimeMilliseconds = 0.0;
		bool m_IsStreamValid = false;

		// Local CPU side buffer
		uint32_t m_TargetWidth, m_TargetHeight;
		Ref<Framebuffer> m_DrawBuffer;
		uint32_t m_CurrentFrameIndex = 0;
		Buffer m_LocalBuffer;

		AVFormatContext* m_AVFormatContext = nullptr;
		AVCodecContext* m_AVCodecContext = nullptr;
		AVStream* m_VideoStream = nullptr;
		AVFrame* m_AVFrame = nullptr;
		AVPacket* m_AVPacket = nullptr;
		SwsContext* m_SwsContext = nullptr;
	};

}