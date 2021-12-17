#pragma once
#include <vector>
#include <memory>
#include <string>
#include <stddef.h>
#include <cstdint>
#include <stdint.h>
namespace MBMedia
{
	enum class Codec
	{
		MP4,
		MKV,
		WEBM,
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

	};
	struct AudioDecodeInfo
	{

	};
	struct AudioEncodeInfo
	{

	};
	inline void _DoNothing(void*)
	{
		return;
	}
	void _FreePacket(void*);
	class StreamPacket
	{
	private:
		friend class ContainerDemuxer;
		friend class StreamDecoder;
		friend class StreamEncoder;
		std::unique_ptr<void, void (*)(void*)> m_InternalData = std::unique_ptr<void, void (*)(void*)>(nullptr, _DoNothing);
		MediaType m_Type = MediaType::Null;
		StreamPacket(void* FFMPEGPacket,MediaType PacketType);
	public:
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
		StreamFrame(void* FFMPEGData,MediaType FrameType);
	public:
		MediaType GetMediaType() { return(m_MediaType); };
	};

	void _FreeCodecContext(void*);
	class StreamDecoder
	{
	private:
		//void p_Flush();
		std::shared_ptr<void> m_InternalData = nullptr;
		MediaType m_Type = MediaType::Null;
	public:
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

		std::unique_ptr<void, void (*)(void*)> m_InternalData = std::unique_ptr<void, void (*)(void*)>(nullptr,_DoNothing);
		MediaType m_Type = MediaType::Null;
	public:
		StreamEncoder(StreamEncoder const&) = delete;
		StreamEncoder(StreamEncoder&&) = default;
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
		void p_WritePacket(StreamPacket const& PacketToWrite, size_t StreamIndex);
		void p_WriteTrailer();
	public:
		OutputContext(std::string const& OutputFile);


		void AddOutputStream(StreamEncoder&& Encoder);
		void WriteHeader();
		
		//ffmpeg verkar inte ha någon stream data i framen, kanske innebär att framen på något sätt är atomisk...
		void InsertFrame(StreamFrame const& PacketToInsert, size_t StreamIndex);
		void Finalize();
		~OutputContext();
	};


	class MediaStreamer
	{
	private:
	public:

	};

	AudioEncodeInfo GetAudioEncodePresets(StreamDecoder const& StreamToCopy);
	VideoEncodeInfo GetVideoEncodePresets(StreamDecoder const& StreamToCopy);
	MediaType GetCodecMediaType(Codec InputCodec);
	void Transcode(std::string const& InputFile, std::string const& OutputFile,Codec NewAudioCodec,Codec NewVideoCodec);
};