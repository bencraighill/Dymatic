#include "dypch.h"

#include "Dymatic/Video/VideoReader.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <inttypes.h>
}

#include "Dymatic/Renderer/Renderer.h"

namespace Dymatic {

	namespace Utils {

		static const char* GetAVErrorString(int error)
		{
			static char errorString[AV_ERROR_MAX_STRING_SIZE];
			av_strerror(error, errorString, AV_ERROR_MAX_STRING_SIZE);
			return errorString;
		}

		static AVPixelFormat CorrectDeprecatedPixelFormat(AVPixelFormat format)
		{
			switch (format)
			{
			case AV_PIX_FMT_YUVJ420P: return AV_PIX_FMT_YUV420P;
			case AV_PIX_FMT_YUVJ422P: return AV_PIX_FMT_YUV422P;
			case AV_PIX_FMT_YUVJ444P: return AV_PIX_FMT_YUV444P;
			case AV_PIX_FMT_YUVJ440P: return AV_PIX_FMT_YUV440P;
			default:                  return format;
			}
		}

		static bool InitializeCodec(AVCodecContext** avCodecContext, const AVCodecParameters* avCodecParameters, const AVCodec* avCodec)
		{
			// Set up a codec context for the video decoder
			if (!(*avCodecContext = avcodec_alloc_context3(avCodec)))
			{
				DY_CORE_VERIFY(false, "Couldn't create AVVideoCodecContext");
				return false;
			}

			if (avcodec_parameters_to_context(*avCodecContext, avCodecParameters) < 0)
			{
				DY_CORE_VERIFY(false, "Could not initialize AVVideoCodecContext");
				return false;
			}

			if (avcodec_open2(*avCodecContext, avCodec, nullptr) < 0)
			{
				DY_CORE_VERIFY(false, "Could not open codec");
				return false;
			}

			return true;
		}

		static SubtitleData ASSFormatSubtitle(const char* assString)
		{
			SubtitleData subtitle;

			// Read in the data
			const char delimeter = ',';
			std::string line;
			std::istringstream assStream(assString);

			std::getline(assStream, line, delimeter);
			subtitle.Marked = std::stoi(line);

			std::getline(assStream, line, delimeter);
			subtitle.Layer = std::stoi(line);

			std::getline(assStream, subtitle.Style, delimeter);

			std::getline(assStream, subtitle.Name, delimeter);

			std::getline(assStream, line, delimeter);
			subtitle.MarginL = std::stoi(line);

			std::getline(assStream, line, delimeter);
			subtitle.MarginR = std::stoi(line);

			std::getline(assStream, line, delimeter);
			subtitle.MarginV = std::stoi(line);

			std::getline(assStream, subtitle.Effect, delimeter);

			std::getline(assStream, subtitle.Text, delimeter);

			return subtitle;
		}

		static SubtitleData GetSubtitle(const AVSubtitle& avSubtitle)
		{
			SubtitleData subtitle;

			for (uint32_t i = 0; i < avSubtitle.num_rects; i++)
			{
				const uint32_t beginTimestamp = avSubtitle.start_display_time;
				const uint32_t endTimestamp = avSubtitle.end_display_time;
				const AVSubtitleRect* rect = avSubtitle.rects[i];
				if (rect->type == SUBTITLE_ASS)
					subtitle = Utils::ASSFormatSubtitle(rect->ass);
				else if (rect->type == SUBTITLE_TEXT)
					subtitle.Text = rect->text;
			}

			subtitle.StartDisplayTime = avSubtitle.start_display_time;
			subtitle.EndDisplayTime = avSubtitle.end_display_time;

			return subtitle;
		}

		static int PacketReadCallback(void* opaque, uint8_t* buf, int bufSize)
		{
			MediaStream* media = (MediaStream*)opaque;
			FileStreamReader& stream = media->GetReader();

			size_t remainingBytes = media->GetStreamSize() - media->GetStreamPosition();
			int readSize = std::min<size_t>(bufSize, remainingBytes);

			if (readSize <= 0)
				return AVERROR_EOF;

			return stream.ReadDataLength((char*)buf, readSize);
		}

		static int64_t PacketSeekCallback(void* opaque, int64_t pos, int whence)
		{
			MediaStream* media = (MediaStream*)opaque;

			switch (whence)
			{
			case SEEK_SET:
				break;
			case SEEK_CUR:
				pos += media->GetStreamPosition();
				break;
			case SEEK_END:
				pos += media->GetStreamSize();
				break;
			default:
				return -1;
			}

			if (!media->SetStreamPosition(pos))
				return -1;

			return media->GetStreamPosition();
		}

		static bool InitializeAVFormatContext(AVFormatContext** avFormatContext, const Ref<MediaStream> mediaStream)
		{
			// Allocate the format context
			if (!(*avFormatContext = avformat_alloc_context()))
			{
				DY_CORE_VERIFY(false, "Could not allocate AVFormatContext!");
				return false;
			}

			// Allocate buffer for AVIOContext
			constexpr int avioBufferSize = 4096;
			uint8_t* avioBuffer = (uint8_t*)av_mallocz(avioBufferSize + AV_INPUT_BUFFER_PADDING_SIZE);
			if (!avioBuffer)
			{
				DY_CORE_VERIFY(false, "Could not allocate AVIO buffer!");
				avformat_free_context(*avFormatContext);
				return false;
			}

			// Open the format input
			AVIOContext* avioContext = avio_alloc_context(avioBuffer, avioBufferSize, 0, mediaStream.get(), &PacketReadCallback, nullptr, &PacketSeekCallback);
			if (!avioContext)
			{
				DY_CORE_VERIFY(false, "Could not allocate AVIOContext!");
				av_free(avioBuffer);
				avformat_free_context(*avFormatContext);
				return false;
			}

			// Setup the custom AVIO context on the format context
			AVFormatContext* context = *avFormatContext;
			context->pb = avioContext;
			context->flags |= AVFMT_FLAG_CUSTOM_IO;

			// Open the format context using the custom AVIO context
			if (avformat_open_input(avFormatContext, nullptr, nullptr, nullptr) != 0)
			{
				DY_CORE_VERIFY(false, "Could not open video from file stream!");
				av_freep(&(*avFormatContext)->pb->buffer);
				avio_context_free(&(*avFormatContext)->pb);
				avformat_free_context(*avFormatContext);
				return false;
			}

			return true;
		}

		static void ShutdownAVFormatContext(AVFormatContext** avFormatContext)
		{
			if (*avFormatContext)
			{
				if ((*avFormatContext)->pb)
				{
					av_freep(&(*avFormatContext)->pb->buffer);
					avio_context_free(&(*avFormatContext)->pb);
				}

				avformat_close_input(avFormatContext);
			}
		}

		static void FreeAVFrameList(std::deque<AVFrame*>& frameList)
		{
			for (auto& frame : frameList)
				av_frame_free(&frame);

			frameList.clear();
		}

		static void FlipBufferVertical(uint8_t* data, const int rowSize, const int height)
		{
			uint8_t* tempRow = new uint8_t[rowSize];

			for (int i = 0; i < height / 2; i++)
			{
				uint8_t* topRow = data + i * rowSize;
				uint8_t* bottomRow = data + (height - 1 - i) * rowSize;
				memcpy(tempRow, topRow, rowSize);
				memcpy(topRow, bottomRow, rowSize);
				memcpy(bottomRow, tempRow, rowSize);
			}

			delete[] tempRow;
		}

		static uint32_t MakeEven(uint32_t n)
		{
			return n - n % 2;
		}
	}

	VideoReader::VideoReader(const VideoReaderSpecification& specification)
		: m_Specification(specification), m_Target(specification.Target)
	{
		DY_PROFILE_FUNCTION();
		Invalidate();
	}

	VideoReader::~VideoReader()
	{
		DY_PROFILE_FUNCTION();
		Release();
	}

	void VideoReader::Invalidate()
	{
#ifdef DY_DIST
		av_log_set_level(AV_LOG_QUIET);
#endif

		if (m_AVFormatContext)
			Release();

		if (!m_Specification.VideoStream)
			return;

		m_Specification.VideoStream->Reset();

		if (!Utils::InitializeAVFormatContext(&m_AVFormatContext, m_Specification.VideoStream))
		{
			DY_CORE_ASSERT(false, "Failed to initialize AVFormatContext");
			return;
		}

		// Check if an external subtitle file was specified
		if (m_Specification.SubtitleStream)
		{
			m_Specification.SubtitleStream->Reset();

			if (!Utils::InitializeAVFormatContext(&m_AVSubtitleFormatContext, m_Specification.SubtitleStream))
				DY_CORE_ASSERT(false, "Failed to initialize Subtitle AVFormatContext");
		}

		m_VideoStreamIndex = -1;
		m_SubtitleStreamIndex = -1;

		// Find the first valid stream indices inside the file
		for (uint32_t i = 0; i < m_AVFormatContext->nb_streams; i++)
		{
			const AVCodecParameters* avCodecParameters = m_AVFormatContext->streams[i]->codecpar;
			const AVCodec* avCodec = const_cast<AVCodec*>(avcodec_find_decoder(avCodecParameters->codec_id));

			if (!avCodec)
				continue;

			// Handle video stream
			if (avCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				m_VideoStreamIndex = i;
				m_SourceWidth = avCodecParameters->width;
				m_SourceHeight = avCodecParameters->height;

				AVRational framerate = m_AVFormatContext->streams[i]->avg_frame_rate;
				// We flip and do denominator / numerator as we want delta time not framerate.
				m_FrameDelta = (double)framerate.den / (double)framerate.num;

				// Convert internal rational type to a double
				AVRational timeBase = m_AVFormatContext->streams[i]->time_base;
				m_TimeBase = (double)timeBase.num / (double)timeBase.den;

				m_Duration = m_AVFormatContext->streams[i]->duration * m_TimeBase;

				if (!Utils::InitializeCodec(&m_AVVideoCodecContext, avCodecParameters, avCodec))
				{
					DY_CORE_VERIFY(false, "Failed to initialize video codec!");
					return;
				}

				break;
			}
		}

		// Find the subtitle stream
		// If we have a valid external subtitle file we will use it, otherwise use the internal one (if it exists)
		const AVFormatContext* avFormatContext = m_AVSubtitleFormatContext ? m_AVSubtitleFormatContext : m_AVFormatContext;
		for (uint32_t i = 0; i < avFormatContext->nb_streams; i++)
		{
			const AVCodecParameters* avCodecParameters = avFormatContext->streams[i]->codecpar;
			const AVCodec* avCodec = const_cast<AVCodec*>(avcodec_find_decoder(avCodecParameters->codec_id));

			if (avCodecParameters->codec_type == AVMEDIA_TYPE_SUBTITLE)
			{
				m_SubtitleStreamIndex = i;
				Utils::InitializeCodec(&m_AVSubtitleCodecContext, avCodecParameters, avCodec);

				m_SubtitleTimeBase = av_q2d(avFormatContext->streams[i]->time_base);

				break;
			}
		}

		// Ensure we actually loaded a valid video stream.
		if (m_VideoStreamIndex == -1)
		{
			DY_CORE_VERIFY(false, "Could not find valid video stream inside file");
			return;
		}

		// Allocate a buffer for storing CPU side texture buffer data for the frame
		const size_t bufferSize = GetTargetWidth() * GetTargetHeight() * 3;
		m_Buffer = Buffer(bufferSize);

		// Allocate a packet for shared usage when reading frames
		if (!(m_AVPacket = av_packet_alloc()))
		{
			DY_CORE_VERIFY(false, "Couldn't allocate AVPacket");
			return;
		}

		// We have successfully loaded the video asset
		m_IsLoaded = true;
	}

	void VideoReader::Release()
	{
		sws_freeContext(m_SwsScalerContext);
		m_SwsScalerContext = nullptr;

		Utils::ShutdownAVFormatContext(&m_AVFormatContext);
		avformat_free_context(m_AVFormatContext);
		m_AVFormatContext = nullptr;

		av_packet_free(&m_AVPacket);
		m_AVPacket = nullptr;

		avcodec_free_context(&m_AVVideoCodecContext);
		m_AVVideoCodecContext = nullptr;

		avcodec_free_context(&m_AVSubtitleCodecContext);
		m_AVSubtitleCodecContext = nullptr;

		Utils::FreeAVFrameList(m_AVFrames);

		if (m_AVSubtitleFormatContext)
		{
			avformat_close_input(&m_AVSubtitleFormatContext);
			avformat_free_context(m_AVSubtitleFormatContext);
			m_AVSubtitleFormatContext = nullptr;
		}
	}

	bool VideoReader::ReadFrame()
	{
		DY_PROFILE_FUNCTION();

		if (!m_AVFormatContext)
			return false;

		// Check if we should loop
		if (m_Specification.Looping && m_CurrentTime > m_Duration)
			SeekFrame(0.0);

		// Decode frames
		if (!DecodeSingleFrame())
			return false;

		if (m_AVFrames.empty())
			return true;

		// Check if the first frame in our buffer is read to be rendered
		if (m_AVFrames.front()->pts * m_TimeBase > m_CurrentTime)
			return true;

		// We now convert this data to a format that can be uploaded to a GPU texture.
		// Set up sws scaler
 		if (!m_SwsScalerContext)
		{
			AVPixelFormat sourcePixelFormat = Utils::CorrectDeprecatedPixelFormat(m_AVVideoCodecContext->pix_fmt);
			m_SwsScalerContext = sws_getContext(m_SourceWidth, m_SourceHeight, sourcePixelFormat, GetTargetWidth(), GetTargetHeight(), AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);
		}
		
		if (!m_SwsScalerContext)
		{
			DY_CORE_VERIFY(false, "Could not initialize sws_scala context");
			return false;
		}
		
		// Copy the image to the local buffer in the desired format for rendering
		{
			DY_PROFILE_SCOPE("VideoReader::ReadFrame sws_scale");

			uint8_t* dest[] = { m_Buffer.Data, nullptr, nullptr, nullptr };
			int dest_linesize[] = { GetTargetWidth() * 3, 0, 0, 0 };
			AVFrame* avFrame = m_AVFrames.front();
			sws_scale(m_SwsScalerContext, avFrame->data, avFrame->linesize, 0, m_SourceHeight, dest, dest_linesize);

			// Consume the frame
			av_frame_free(&avFrame);
			m_AVFrames.pop_front();
		}

		// Flip the image vertically (assuming RGB8)
		{
			DY_PROFILE_SCOPE("VideoReader::ReadFrame FlipImage");
			Utils::FlipBufferVertical(m_Buffer.Data, GetTargetWidth() * 3, GetTargetHeight());
		}
		
		return true;
	}

	bool VideoReader::SetTime(double time)
	{
		DY_PROFILE_FUNCTION();

		SeekFrame(time);

		// av_seek_frame takes effect after one frame, so we decoding one here to ensure the next call 
		// to VideoReader::ReadFrame will be the correct frame.
		return ReadFrame();
	}

	bool VideoReader::SetTime(int64_t timestamp)
	{
		return false;
	}

	Ref<Texture2D> VideoReader::GetCurrentFrame()
	{
		DY_PROFILE_FUNCTION();

		if (!m_Target)
			return GetNextFrame();

		return m_Target;
	}

	Ref<Texture2D> VideoReader::GetNextFrame(Timestep ts)
	{
		DY_PROFILE_FUNCTION();

		if (!m_AVFormatContext)
			return nullptr;

		const uint32_t targetWidth = GetTargetWidth();
		const uint32_t targetHeight = GetTargetHeight();

		// Check if we have a texture allocated for this video
		if (!m_Target)
		{
			TextureSpecification spec;
			spec.Width = targetWidth;
			spec.Height = targetHeight;
			spec.Format = TextureFormat::RGB8;
			m_Target = Texture2D::Create(spec);
		}

		m_CurrentTime += ts.GetSeconds();

		// Update GPU texture size if needed
		if (m_Target->GetWidth() != targetWidth || m_Target->GetHeight() != targetHeight)
			m_Target->Resize(targetWidth, targetHeight);

		// Decode and copy frame data to the GPU
		ReadFrame();
		m_Target->SetData(m_Buffer);

		return m_Target;
	}

	Ref<Texture2D> VideoReader::GetNextFrame()
	{
		return GetCurrentFrame();
	}

	const std::string VideoReader::GetCurrentSubtitle()
	{
		if (m_CurrentTime >= m_Subtitle.StartDisplayTime && m_CurrentTime <= m_Subtitle.EndDisplayTime)
			return m_Subtitle.Text;

		return std::string();
	}

	bool VideoReader::DecodeSingleFrame()
	{
		DY_PROFILE_FUNCTION();
	
		if (!m_AVFormatContext)
			return false;

		int response;
		// Only read a new frame if we don't already have a packet to check (i.e. one that was not consumed previously)
		while (m_AVFrames.size() <= m_Specification.BufferSize && (av_read_frame(m_AVFormatContext, m_AVPacket) >= 0))
		{
			// Only handle video and subtitle streams
			if (m_AVPacket->stream_index != m_VideoStreamIndex && m_AVPacket->stream_index != m_SubtitleStreamIndex)
			{
				av_packet_unref(m_AVPacket);
				continue;
			}

			// Check if we should present the current packet
			// Note: We do not call av_packet_unref if we do not consume it.
			const double presentationTime = m_TimeBase * m_AVPacket->pts;

			if (m_AVPacket->stream_index == m_VideoStreamIndex)
			{
				// TODO: Only send the packet and receive the frame when the resultant image will actually be rendered!

				response = avcodec_send_packet(m_AVVideoCodecContext, m_AVPacket);
				if (response < 0)
				{
					DY_CORE_VERIFY(false, fmt::format("Failed to decode packet: {}", Utils::GetAVErrorString(response)));
					return false;
				}

				// Allocate a new frame and add it to the list
				AVFrame* avFrame = av_frame_alloc();
				if (m_AVFrames.empty() || avFrame->pts < m_AVFrames.front()->pts)
					m_AVFrames.push_front(avFrame);
				else
					m_AVFrames.push_back(avFrame);

				// Receive the frame
				response = avcodec_receive_frame(m_AVVideoCodecContext, avFrame);

				if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
				{
					av_packet_unref(m_AVPacket);
				}
				else if (response < 0)
				{
					DY_CORE_VERIFY(false, fmt::format("Failed to decode packet: {}", Utils::GetAVErrorString(response)));
					return false;
				}
			}
			else if (!m_AVSubtitleFormatContext && m_AVPacket->stream_index == m_SubtitleStreamIndex)
			{
				// Decode embedded subtitle stream only if we did not specify/load an external one.
				int gotSubtitle = 0;
				AVSubtitle avSubtitle;

				avcodec_decode_subtitle2(m_AVSubtitleCodecContext, &avSubtitle, &gotSubtitle, m_AVPacket);

				// Unpack the subtitle packet here
				if (gotSubtitle)
				{
					m_Subtitle = Utils::GetSubtitle(avSubtitle);
					m_Subtitle.StartDisplayTime = m_AVPacket->pts * m_TimeBase;
					m_Subtitle.EndDisplayTime = m_Subtitle.StartDisplayTime + m_AVPacket->duration * m_TimeBase;
				}
			}

			av_packet_unref(m_AVPacket);
		}

		// Handle external subtitle file
		if (m_AVSubtitleFormatContext && m_CurrentTime > m_Subtitle.EndDisplayTime)
		{
			while (av_read_frame(m_AVSubtitleFormatContext, m_AVPacket) >= 0)
			{
				if (m_AVPacket->stream_index == m_SubtitleStreamIndex)
				{
					const double startTime = m_AVPacket->pts * m_SubtitleTimeBase;
					const double endTime = startTime + m_AVPacket->duration * m_SubtitleTimeBase;

					if (endTime < m_CurrentTime)
						continue;
					
					// Decode embedded subtitle stream only if we did not specify/load an external one.
					int gotSubtitle = 0;
					AVSubtitle avSubtitle;

					avcodec_decode_subtitle2(m_AVSubtitleCodecContext, &avSubtitle, &gotSubtitle, m_AVPacket);

					// Unpack the subtitle packet here
					if (gotSubtitle)
					{
						m_Subtitle = Utils::GetSubtitle(avSubtitle);
						m_Subtitle.StartDisplayTime = startTime;
						m_Subtitle.EndDisplayTime = endTime;
					}

					break;
				}
			}
		}

		return true;
	}

	void VideoReader::SeekFrame(double time)
	{
		if (!m_AVFormatContext)
			return;

		const int64_t videoTimestamp = time / m_TimeBase;

		// Call ffmpeg internal commands to seek the correct frame at the timestamp
		av_seek_frame(m_AVFormatContext, m_VideoStreamIndex, videoTimestamp, AVSEEK_FLAG_BACKWARD);

		if (m_AVSubtitleFormatContext)
		{
			const int64_t subtitleTimestamp = time / m_SubtitleTimeBase;
			av_seek_frame(m_AVSubtitleFormatContext, m_SubtitleStreamIndex, subtitleTimestamp, AVSEEK_FLAG_BACKWARD);
		}
		else if (m_SubtitleStreamIndex != -1)
		{
			// Note: Embedded subtitles share the same time base as the video, thus use the video timestamp
			av_seek_frame(m_AVFormatContext, m_SubtitleStreamIndex, videoTimestamp, AVSEEK_FLAG_BACKWARD);
		}

		// Reset all necessary variables
		m_CurrentTime = time;
		m_Subtitle = SubtitleData();
		Utils::FreeAVFrameList(m_AVFrames);
	}

	VideoWriter::VideoWriter(const VideoWriterSpecification& specification)
		: m_Specification(specification)
	{
		DY_PROFILE_FUNCTION();

#ifdef DY_DIST
		av_log_set_level(AV_LOG_QUIET);
#endif
		
		// Determine the appropriate output size
		m_TargetWidth  = Utils::MakeEven(m_Specification.Width  ? m_Specification.Width  : m_Specification.Framebuffer ? m_Specification.Framebuffer->GetSpecification().Width  : 1920);
		m_TargetHeight = Utils::MakeEven(m_Specification.Height ? m_Specification.Height : m_Specification.Framebuffer ? m_Specification.Framebuffer->GetSpecification().Height : 1080);
		
		// Allocate the output media context
		avformat_alloc_output_context2(&m_AVFormatContext, nullptr, nullptr, m_Specification.Path.string().c_str());
		if (!m_AVFormatContext)
		{
			DY_CORE_ASSERT(false, fmt::format("Could not deduce output format from file extension '{}'", m_Specification.Path.extension().string()));
			return;
		}

		// Find the encoder for the video stream
		const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!codec)
		{
			DY_CORE_ASSERT(false, "Codec not found");
			return;
		}

		// Create a new video stream
		m_VideoStream = avformat_new_stream(m_AVFormatContext, codec);
		if (!m_VideoStream)
		{
			DY_CORE_ASSERT(false, "Could not allocate stream");
			return;
		}

		m_AVCodecContext = avcodec_alloc_context3(codec);
		if (!m_AVCodecContext)
		{
			DY_CORE_ASSERT(false, "Could not allocate video codec context");
			return;
		}

		// Set codec parameters
		m_AVCodecContext->bit_rate = m_Specification.Bitrate;
		m_AVCodecContext->width = m_TargetWidth;
		m_AVCodecContext->height = m_TargetHeight;
		m_AVCodecContext->time_base = { 1, (int)m_Specification.FPS };
		m_AVCodecContext->framerate = { (int)m_Specification.FPS, 1 };
		m_AVCodecContext->gop_size = 10;
		m_AVCodecContext->max_b_frames = 1;
		m_AVCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;

		if (m_AVFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
			m_AVCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		int result = avcodec_open2(m_AVCodecContext, codec, nullptr);
		if (result < 0)
		{
			DY_CORE_ASSERT(false, fmt::format("Could not open codec: {}", Utils::GetAVErrorString(result)));
			return;
		}

		m_VideoStream->time_base = m_AVCodecContext->time_base;
		avcodec_parameters_from_context(m_VideoStream->codecpar, m_AVCodecContext);

		// Open the output file
		if (!(m_AVFormatContext->oformat->flags & AVFMT_NOFILE))
		{
			if (avio_open(&m_AVFormatContext->pb, m_Specification.Path.string().c_str(), AVIO_FLAG_WRITE) < 0) 
			{
				DY_CORE_ASSERT(false, fmt::format("Could not open output file '{}'", m_Specification.Path.string()));
				return;
			}
		}

		// Write the stream header
		if (avformat_write_header(m_AVFormatContext, nullptr) < 0)
		{
			DY_CORE_ASSERT(false, fmt::format("Error occured when opening output file '{}'", m_Specification.Path.string()));
			return;
		}

		m_AVFrame = av_frame_alloc();
		if (!m_AVFrame)
		{
			DY_CORE_ASSERT(false, "Could not allocate video frame");
			return;
		}

		m_AVFrame->format = m_AVCodecContext->pix_fmt;
		m_AVFrame->width = m_AVCodecContext->width;
		m_AVFrame->height = m_AVCodecContext->height;

		result = av_image_alloc(m_AVFrame->data, m_AVFrame->linesize, m_TargetWidth, m_TargetHeight, m_AVCodecContext->pix_fmt, 32);
		if (result < 0)
		{
			DY_CORE_ASSERT(false, fmt::format("Could not allocate raw picture buffer: {}", Utils::GetAVErrorString(result)));
			return;
		}

		m_AVPacket = av_packet_alloc();
		if (!m_AVPacket)
		{
			DY_CORE_ASSERT(false, "Could not allocate packet");
			return;
		}
		
		m_SwsContext = sws_getContext(m_TargetWidth, m_TargetHeight, AV_PIX_FMT_RGBA, m_TargetWidth, m_TargetHeight, m_AVCodecContext->pix_fmt, SWS_BILINEAR, nullptr, nullptr, nullptr);
		if (!m_SwsContext)
		{
			DY_CORE_ASSERT(false, "Failed to get sws context");
			return;
		}

		// Allocate GPU resources for rendering and a local CPU buffer
		m_LocalBuffer = Buffer(m_TargetWidth * m_TargetHeight * 4);

		FramebufferSpecification framebufferSpecification;
		framebufferSpecification.Attachments = { TextureFormat::RGBA8 };
		framebufferSpecification.Width = m_TargetWidth;
		framebufferSpecification.Height = m_TargetHeight;
		m_DrawBuffer = Framebuffer::Create(framebufferSpecification);

		m_IsStreamValid = true;
	}

	VideoWriter::~VideoWriter()
	{
		DY_PROFILE_FUNCTION();

		Close();
	}

	bool VideoWriter::WriteFrame(Timestep ts)
	{
		DY_PROFILE_FUNCTION();

		m_CurrentTimeMilliseconds += ts.GetSeconds();

		m_Specification.Framebuffer->Bind();
		m_Specification.Framebuffer->BindColorSampler(0);
		m_Specification.Framebuffer->Unbind();
		RenderLocalBuffer();
		
		return OutputFrame();
	}

	bool VideoWriter::WriteFrame(Timestep ts, Ref<Texture2D> frame)
	{
		DY_PROFILE_FUNCTION();

		m_CurrentTimeMilliseconds += ts.GetSeconds();

		frame->Bind();
		RenderLocalBuffer();

		return OutputFrame();
	}

	void VideoWriter::Close()
	{
		DY_PROFILE_FUNCTION();
		
		// Write the trailer
		av_write_trailer(m_AVFormatContext);

		// Free resources
		avcodec_free_context(&m_AVCodecContext);
		av_frame_free(&m_AVFrame);
		av_packet_free(&m_AVPacket);
		avio_closep(&m_AVFormatContext->pb);
		avformat_free_context(m_AVFormatContext);

		m_IsStreamValid = false;
	}

	void VideoWriter::RenderLocalBuffer()
	{
		DY_PROFILE_FUNCTION();

		m_DrawBuffer->Bind();
		Renderer::DrawFullscreenTexture();
		m_LocalBuffer = m_DrawBuffer->CopyColorBuffer(0);
		m_DrawBuffer->Unbind();
	}

	bool VideoWriter::OutputFrame()
	{
		DY_PROFILE_FUNCTION();

		// Flip the image vertically (assuming RGBA8)
		{
			DY_PROFILE_SCOPE("VideoWriter::OutputFrame FlipImage");
			Utils::FlipBufferVertical(m_LocalBuffer.Data, m_TargetWidth * 4, m_TargetHeight);
		}

		uint8_t* rgbSrc[3] = { m_LocalBuffer.Data, nullptr, nullptr };
		int rgbSrcStride[3] = { m_TargetWidth * 4, 0, 0 };
		sws_scale(m_SwsContext, rgbSrc, rgbSrcStride, 0, m_TargetHeight, m_AVFrame->data, m_AVFrame->linesize);

		m_AVFrame->pts = m_CurrentFrameIndex;

		int result = avcodec_send_frame(m_AVCodecContext, m_AVFrame);
		if (result < 0)
		{
			DY_CORE_ASSERT(false, fmt::format("Error sending a frame for encoding: {}", Utils::GetAVErrorString(result)));
			return false;
		}

		while (result >= 0)
		{
			result = avcodec_receive_packet(m_AVCodecContext, m_AVPacket);

			if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
				break;
			else if (result < 0)
			{
				DY_CORE_ASSERT(false, fmt::format("Error during encoding: {}", Utils::GetAVErrorString(result)));
				return false;
			}

			av_packet_rescale_ts(m_AVPacket, m_AVCodecContext->time_base, m_VideoStream->time_base);
			m_AVPacket->stream_index = m_VideoStream->index;

			if (av_interleaved_write_frame(m_AVFormatContext, m_AVPacket) < 0)
			{
				DY_CORE_ASSERT(false, fmt::format("Error writing video frame: {}", Utils::GetAVErrorString(result)));
				return false;
			}

			av_packet_unref(m_AVPacket);
		}

		m_CurrentFrameIndex++;

		return true;
	}

}