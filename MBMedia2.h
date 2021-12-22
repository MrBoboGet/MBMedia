#pragma once
#include <vector>
#include <memory>
#include <string>
#include <stddef.h>
#include <cstdint>
#include <stdint.h>
///*
namespace MBMedia
{
	enum class Codec
	{
		AAC,
		H264,
		H265,
		VP9,
		Null,
	};
	enum class MediaType
	{
		Video,
		Audio,
		Subtitles,
		Null,
	};
	enum class ContainerFormat
	{
		Null,
	};
	struct TimeBase
	{
		int num = 0;
		int den = 0;
	};
	//har inte hittat entry pointen till free_steam, så gör något lite hacky med att låta streamen vara helt kopplad till format kontexten
	void _FreeFormatContext(void* DataToFree);
	struct StreamInfo
	{
	private:
		friend class ContainerDemuxer;
		friend class StreamDecoder;
		size_t m_StreamIndex = -1;//all detta förutsätter att stream datan  inte förändras
		std::shared_ptr<void> m_InternalData = nullptr;
		StreamInfo(std::shared_ptr<void> FFMPEGContainerData,size_t StreamIndex); //används av decoder, en decode context har en konstant mängd streams
		MediaType m_Type = MediaType::Null;
		Codec m_StreamCodec = Codec::Null;
	public:
		MediaType GetMediaType() const {return(m_Type);};
		Codec GetCodec() const {return(m_StreamCodec);};
	};

	struct VideoDecodeInfo
	{

	};
	struct VideoEncodeInfo
	{
	private:
		uint32_t m_PixelFormat = -1;

	public:
		int height = 0;
		int width = 0;
		size_t bit_rate = 0;
		size_t rc_buffer_size = 0;
		size_t rc_max_rate = 0;
		size_t rc_min_rate = 0;
		TimeBase time_base;
	};

	struct AudioDecodeInfo
	{

	};
	struct AudioEncodeInfo
	{
	private:
		friend 	VideoEncodeInfo GetVideoEncodePresets(StreamDecoder const& StreamToCopy);
		friend AudioEncodeInfo GetAudioEncodePresets(StreamDecoder const& StreamToCopy);
		friend class StreamEncoder;
		int m_channels = -1;
		int m_channels_layout = -1;
	public:
		size_t bit_rate = 0;
		size_t rc_buffer_size = 0;
		size_t rc_max_rate = 0;
		size_t rc_min_rate = 0;
		int sample_rate = 0;
		TimeBase time_base = { 0,0 };
	};
	AudioEncodeInfo GetAudioEncodePresets(StreamDecoder const& StreamToCopy);
	VideoEncodeInfo GetVideoEncodePresets(StreamDecoder const& StreamToCopy);
	inline void _DoNothing(void*)
	{
		return;
	}
	void _FreePacket(void*);
	
	class StreamPacket
	{
	private:
		friend class ContainerDemuxer;
		friend class OutputContext;
		friend class StreamDecoder;
		friend class StreamEncoder;
		std::unique_ptr<void, void (*)(void*)> m_InternalData = std::unique_ptr<void, void (*)(void*)>(nullptr, _DoNothing);
		MediaType m_Type = MediaType::Null;
		StreamPacket(void* FFMPEGPacket, TimeBase PacketTimebase,MediaType PacketType);
		TimeBase m_TimeBase;
	public:
		//float GetDuration();
		TimeBase GetTimebase();
		void Rescale(TimeBase NewTimebase );
		void Rescale(TimeBase OriginalTimebase, TimeBase NewTimebase );

		StreamPacket(StreamPacket const&) = delete;
		StreamPacket(StreamPacket&&) = default;
		StreamPacket& operator=(StreamPacket&&) = default;
		MediaType GetType() { return(m_Type); }
		//~StreamPacket();
	};
	class StreamFrame
	{
	private:
		friend class StreamDecoder;
		friend class StreamEncoder;
		std::unique_ptr<void, void (*)(void*)> m_InternalData = std::unique_ptr<void, void (*)(void*)>(nullptr, _DoNothing);
		MediaType m_MediaType = MediaType::Null;
		TimeBase m_TimeBase;
		StreamFrame(void* FFMPEGData,TimeBase FrameTimeBase,MediaType FrameType);
	public:
		TimeBase GetTimeBase()const {return(m_TimeBase);};
		MediaType GetMediaType() { return(m_MediaType); };
	};

	void _FreeCodecContext(void*);
	class StreamDecoder
	{
	private:
		//void p_Flush();
		friend 	AudioEncodeInfo GetAudioEncodePresets(StreamDecoder const& StreamToCopy);
		friend VideoEncodeInfo GetVideoEncodePresets(StreamDecoder const& StreamToCopy);
		std::shared_ptr<void> m_InternalData = nullptr;
		MediaType m_Type = MediaType::Null;
		TimeBase m_CodecTimebase;
		TimeBase m_StreamTimebase;
	public:
		TimeBase GetCodecTimebase() const { return(m_CodecTimebase); };
		TimeBase GetStreamTimebase() const { return(m_CodecTimebase); };
		StreamDecoder(StreamDecoder const&) = delete;
		StreamDecoder(StreamDecoder&&) = default;
		StreamDecoder& operator=(StreamDecoder&&) = default;
		
		StreamDecoder(StreamInfo const& StreamToDecode);//implicit antagande här, att decoda en stream kan göras "korrekt", att omvandla eller omtolka datan bör göras från framesen vi får efter

		void InsertPacket(StreamPacket const& PacketToDecode);
		StreamFrame GetNextFrame();
		void Flush();
		//~StreamDecoder();
	};
	class StreamEncoder
	{
	private:
		friend class OutputContext;
		std::unique_ptr<void, void (*)(void*)> m_InternalData = std::unique_ptr<void, void (*)(void*)>(nullptr,_DoNothing);
		MediaType m_Type = MediaType::Null;
		Codec m_Codec = Codec::Null;
		TimeBase m_InputTimeBase;
	public:
		TimeBase GetTimebase();
		StreamEncoder(StreamEncoder const&) = delete;
		StreamEncoder(StreamEncoder&&) noexcept = default;
		StreamEncoder& operator=(StreamEncoder&&) = default;

		StreamEncoder(Codec StreamType, AudioEncodeInfo const& EncodeInfo);
		StreamEncoder(Codec StreamType, VideoEncodeInfo const& EncodeInfo);

		void InsertFrame(StreamFrame const& FrameToEncode);
		void Flush();
		StreamPacket GetNextPacket();
		
		MediaType GetMediaType() { return(m_Type); };
		//~StreamEncoder();
	};
	class ContainerDemuxer
	{
	private:
		std::shared_ptr<void> m_InternalData = nullptr;
		std::vector<StreamInfo> m_InputStreams = {};
		bool m_FileEnded = false;
	public:
		size_t NumberOfStreams() { return(m_InputStreams.size()); }
		StreamInfo const& GetStreamInfo(size_t StreamIndex);
		StreamPacket GetNextPacket(size_t* StreamIndex);
		//bool EndOfFile();

		ContainerDemuxer(std::string const& InputFile);
		//~ContainerDemuxer();
	};
	class OutputContext
	{
	private:
		//std::vector<std::unique_ptr<
		ContainerFormat m_OutputFormat = ContainerFormat::Null;
		std::unique_ptr<void, void (*)(void*)> m_InternalData = std::unique_ptr<void, void (*)(void*)>(nullptr, _DoNothing);
		std::vector<StreamEncoder> m_OutputEncoders = {};
		void p_WritePacket(StreamPacket& PacketToWrite, size_t StreamIndex);
		void p_WriteTrailer();
	public:
		OutputContext(std::string const& OutputFile);

		OutputContext(OutputContext const&) = delete;
		OutputContext(OutputContext&&) noexcept = default;
		OutputContext& operator=(OutputContext&&) = default;

		void AddOutputStream(StreamEncoder&& Encoder);
		void WriteHeader();
		
		//ffmpeg verkar inte ha någon stream data i framen, kanske innebär att framen på något sätt är atomisk...
		void InsertFrame(StreamFrame const& PacketToInsert, size_t StreamIndex);
		void Finalize();
	};


	class MediaStreamer
	{
	private:
	public:

	};
	MediaType GetCodecMediaType(Codec InputCodec);
	void Transcode(std::string const& InputFile, std::string const& OutputFile,Codec NewAudioCodec,Codec NewVideoCodec);
};
//*/