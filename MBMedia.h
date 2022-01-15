#pragma once
#include <vector>
#include <memory>
#include <string>
#include <stddef.h>
#include <cstdint>
#include <stdint.h>
#include <queue>
#include "PixelFormats.h"
#include <MBUtility/MBInterfaces.h>
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
	enum class SampleFormat
	{
		NONE = -1,
		U8,   
		S16,  
		S32,  
		FLT,  
		DBL,  
	
		U8P,  
		S16P, 
		S32P, 
		FLTP, 
		DBLP, 

		Null,
	};
	SampleFormat GetPlanarAudioFormat(SampleFormat FormatToConvert);
	SampleFormat GetInterleavedAudioFormat(SampleFormat FormatToConvert);
	
	bool FormatIsPlanar(SampleFormat FormatToInspect);
	//förutsätter att outputten har korrekt mängd allokerade bytes


	struct SampleFormatInfo
	{
		bool Interleaved = false;
		bool Signed = false;
		bool Integer = false;
		size_t SampleSize = -1;
	};
	SampleFormatInfo GetSampleFormatInfo(SampleFormat FormatToInspect);
	enum class ChannelLayout : int64_t
	{

		Null
	};
	struct AudioParameters
	{
		ChannelLayout Layout = ChannelLayout::Null;
		SampleFormat AudioFormat = SampleFormat::Null;
		uint64_t SampleRate = -1;
		size_t NumberOfChannels = -1;
	};


	size_t GetChannelFrameSize(AudioParameters const& ParametersToInspect);
	size_t GetParametersDataPlanes(AudioParameters const& ParametersToInspect);

	//OBS Förutsätter att output datan är har utrymmer den behöver. Låg nivå konvertering. Kanske borde lägga till en Converssion context klass...
	void ConvertSampleData(const uint8_t** InputData, AudioParameters const& InputParameters, uint8_t** OutputBuffer, AudioParameters const& OutputParameters,size_t SamplesToConvert);
	//enum class VideoFormat
	//{
	//	Null
	//};
	struct VideoParameters
	{
		VideoFormat Format = VideoFormat::Null;
		int Width = 0;
		int Height = 0;
	};
	//class StreamDecoder;
	//class StreamEncoder;
	//AudioFormatParameters GetAudioParameters(StreamDecoder const&);
	//VideoFormatParameters GetVideoParameters(StreamDecoder const&);
	//AudioFormatParameters GetAudioParameters(StreamEncoder const&);
	//VideoFormatParameters GetVideoParameters(StreamEncoder const&);
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
		VideoParameters VideoInfo;
		TimeBase StreamTimebase = { 0,0 };
		int64_t AverageBitrate = -1;
	};

	struct VideoEncodeInfo
	{
	public:
		VideoParameters VideoInfo;
		TimeBase StreamTimebase = { 0,0 };
		uint32_t TargetBitrate = 0;
		//size_t rc_buffer_size = 0;
		//size_t rc_max_rate = 0;
		//size_t rc_min_rate = 0;
	};

	struct AudioDecodeInfo
	{
		AudioParameters AudioInfo;
		TimeBase StreamTimebase = { 0,0 };
		size_t FrameSize = -1;
		int64_t AverageBitrate = -1;
	};
	struct AudioEncodeInfo
	{
	private:
		friend class StreamEncoder;
	public:
		AudioParameters AudioInfo;
		TimeBase StreamTimebase = { 0,0 };
		uint32_t TargetBitrate = 0;
		size_t FrameSize = -1;
		//size_t rc_buffer_size = 0;
		//size_t rc_max_rate = 0;
		//size_t rc_min_rate = 0;
	};
	inline void _DoNothing(void*)
	{
		return;
	}
	void _FreePacket(void*);
	
	//struct PresentationData
	//{
	//private:
	//
	//};

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
		MediaType GetType() const { return(m_Type); }
		//~StreamPacket();
	};
	struct AudioFrameInfo
	{
		size_t NumberOfSamples = -1;
	};
	class StreamFrame
	{
	private:
		friend class StreamDecoder;
		friend class StreamEncoder;
		friend class AudioConverter;
		friend class VideoConverter;
		friend StreamFrame FlipRGBPictureHorizontally(StreamFrame const& ImageToFlip);
		//friend StreamFrame FlipPictureHorizontally(StreamFrame const&);
		std::unique_ptr<void, void (*)(void*)> m_InternalData = std::unique_ptr<void, void (*)(void*)>(nullptr, _DoNothing);
		MediaType m_MediaType = MediaType::Null;
		TimeBase m_TimeBase;
	public:
		StreamFrame(void* FFMPEGData, TimeBase FrameTimeBase, MediaType FrameType);
		StreamFrame();
		int64_t GetPresentationTime() const;
		TimeBase GetTimeBase()const {return(m_TimeBase);};
		MediaType GetMediaType() const { return(m_MediaType); };

		VideoParameters GetVideoParameters() const;
		AudioParameters GetAudioParameters() const;

		AudioFrameInfo GetAudioFrameInfo() const;

		uint8_t** GetData();
	};

	class AudioFIFOBuffer
	{
	private:
		std::vector<std::vector<uint8_t>> m_InternalBuffers = {};
		static constexpr float m_GrowthFactor = 1.5;
		SampleFormatInfo m_InputFormatInfo;
		AudioParameters m_InputParameters;
		size_t m_StoredSamples = 0;
		size_t m_CurrentBuffersOffset = 0;

		bool m_IsInitialized = false;
		void p_ResizeBuffers();
		size_t p_GetChannelFrameSize();
	public:
		AudioFIFOBuffer() {};
		AudioFIFOBuffer(AudioParameters const& InputParameters, size_t InitialNumberOfSamples);
		
		void Initialize(AudioParameters const& InputParameters, size_t InitialNumberOfSamples);
		
		void InsertData(const uint8_t* const* AudioData, size_t NumberOfSamples);
		void InsertData(const uint8_t* const* AudioData, size_t NumberOfSamples,size_t InputSampleOffset);
		size_t ReadData(uint8_t** OutputBuffers, size_t NumberOfSamplesToRead);
		size_t ReadData(uint8_t** OutputBuffers, size_t NumberOfSamplesToRead,size_t OutputSampleOffset);
		size_t AvailableSamples();
	};

	class AudioToFrameConverter
	{
	private:
		AudioParameters m_FrameParameters;
		TimeBase m_OutputTimebase;
		size_t m_FrameSize = 0;
		int64_t m_CurrentTimeStamp = 0;
		std::unique_ptr<void, void (*)(void*)> m_AudioFifoBuffer = std::unique_ptr<void, void (*)(void*)>(nullptr, _DoNothing);
		std::deque<StreamFrame> m_StoredFrames = {};
		void p_ConvertStoredSamples();
		bool m_Flushed = false;
	public:
		AudioToFrameConverter(AudioParameters const& InputParameters,int64_t InitialTimestamp,TimeBase OutputTimebase, size_t FrameSize);
		void InsertAudioData(const uint8_t*const*,size_t FrameSize);
		void Flush();
		StreamFrame GetNextFrame();
	};
	class VideoFrameConverter
	{
	private:

	public:
	};
	class AudioConverter
	{
	private:
		friend void swap(AudioConverter& LeftConverter, AudioConverter& RightConverter);
		std::queue<StreamFrame> m_StoredFrames = {};
		std::unique_ptr<void, void (*)(void*)> m_ConversionContext = std::unique_ptr<void, void (*)(void*)>(nullptr, _DoNothing);
		std::unique_ptr<void, void (*)(void*)> m_AudioDataBuffer = std::unique_ptr<void, void (*)(void*)>(nullptr, _DoNothing);
		AudioParameters m_NewAudioParameters;
		AudioParameters m_OldAudioParameters;
		TimeBase m_InputTimebase;
		size_t m_NewFrameSize = -1;
		//DEBUG
		//ANTAGANDE varje input timestamp har monotont växande pts
		int64_t DEBUG_LastTimestamp = 0;
		//DEBUG
		int64_t m_CurrentTimestamp = 0;
		bool m_FirstTimestampSet = false;
		bool m_Flushed = false;
		void p_ConvertNewFrame();
		void p_FlushBufferedFrames();
	public:
		AudioConverter(TimeBase InputTimebase, AudioParameters const& OldParameters, AudioParameters const& NewParameters,size_t NewFrameSize);
		void InsertFrame(StreamFrame const& FrameToInsert);
		StreamFrame GetNextFrame();
		void Flush();
	};
	class VideoConverter
	{
	private:
		friend void swap(VideoConverter& LeftConverter, VideoConverter& RightConverter);
		std::queue<StreamFrame> m_StoredFrames = {};
		std::unique_ptr<void, void (*)(void*)> m_ConversionContext = std::unique_ptr<void, void (*)(void*)>(nullptr, _DoNothing);
		VideoParameters m_NewVideoParameters;
		VideoParameters m_OldVideoParameters;
		TimeBase m_InputTimebase;
	public:
		VideoConverter(TimeBase InputTimebase, VideoParameters const& OldParameters, VideoParameters const& NewParameters);
		void InsertFrame(StreamFrame const& FrameToInsert);
		StreamFrame GetNextFrame();
		void Flush();
	};
	//StreamFrame FlipPictureHorizontally(StreamFrame const& ImageToFlip);
	StreamFrame FlipRGBPictureHorizontally(StreamFrame const& ImageToFlip);
	class FrameConverter
	{
	private:
		friend void swap(FrameConverter& LeftConverter, FrameConverter& RightConverter);
		std::unique_ptr<AudioConverter> m_AudioConverter = nullptr;
		std::unique_ptr<VideoConverter> m_VideoConverter = nullptr;
		bool m_Flushed = false;
		MediaType m_Type = MediaType::Null;
		
	public:
		FrameConverter() {};

		FrameConverter(FrameConverter&&) noexcept;
		FrameConverter& operator=(FrameConverter&&) noexcept;

		FrameConverter(FrameConverter const&) = delete;
		FrameConverter& operator=(FrameConverter const&) = delete;

		bool IsInitialised();

		FrameConverter(TimeBase InputTimebase,AudioParameters const& OldParameters,AudioParameters const& NewParameters,size_t NewFrameSize);
		FrameConverter(TimeBase InputTimebase,VideoParameters const& OldParameters, VideoParameters const& NewParameters);
		void Flush();
		void InsertFrame(StreamFrame const& FrameToInsert);
		StreamFrame GetNextFrame();

		//SwrContext converting and storing at the same time motivated this interface, even tough it isn't consistent with the rest.
		//Passing nullptr flushes and returns the stored records untill the media type is null
		//StreamFrame ConvertFrame(const StreamFrame* FrameToConvert);
		//Because freeing a swrcontext before it is flushed we put this destructor here temporarily for debugging purposes
		~FrameConverter()
		{
			if (!m_Flushed && IsInitialised())
			{
				throw std::exception();
			}
		}
	};

	void _FreeCodecContext(void*);
	class StreamDecoder
	{
	private:
		//void p_Flush();
		std::shared_ptr<void> m_InternalData = nullptr;
		MediaType m_Type = MediaType::Null;
		TimeBase m_CodecTimebase;
		TimeBase m_StreamTimebase;

		bool m_DecodeStreamFinished = false;
		bool m_Flushing = false;
		uint64_t m_CurrentPts = -1;
		StreamFrame p_GetDecodedFrame();
		FrameConverter m_FrameConverter;
	public:
		void SetAudioConversionParameters(AudioParameters const& ParametersToConvertTo,size_t NewFrameSize);
		void SetVideoConversionParameters(VideoParameters const& ParametersToConvertTo);
		AudioDecodeInfo GetAudioDecodeInfo() const;
		VideoDecodeInfo GetVideoDecodeInfo() const;

		TimeBase GetCodecTimebase() const { return(m_CodecTimebase); };
		TimeBase GetStreamTimebase() const { return(m_CodecTimebase); };
		StreamDecoder(StreamDecoder const&) = delete;
		StreamDecoder(StreamDecoder&&) = default;
		StreamDecoder& operator=(StreamDecoder&&) = default;
		
		MediaType GetType() const { return(m_Type); };

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
		TimeBase GetTimebase() const;
		StreamEncoder(StreamEncoder const&) = delete;
		StreamEncoder(StreamEncoder&&) noexcept = default;
		StreamEncoder& operator=(StreamEncoder&&) = default;

		StreamEncoder(Codec StreamType, VideoDecodeInfo const& EncodeInfo);
		StreamEncoder(Codec StreamType, AudioDecodeInfo const& EncodeInfo);
		StreamEncoder(Codec StreamType, AudioEncodeInfo const& EncodeInfo);
		StreamEncoder(Codec StreamType, VideoEncodeInfo const& EncodeInfo);

		AudioEncodeInfo GetAudioEncodeInfo() const;
		VideoEncodeInfo GetVideoEncodeInfo() const;

		void InsertFrame(StreamFrame const& FrameToEncode);
		void Flush();
		StreamPacket GetNextPacket();
		
		MediaType GetMediaType() const { return(m_Type); };
		//~StreamEncoder();
	};
	class ContainerDemuxer
	{
	private:
		friend 	int64_t h_SeekSearchableInputStream(void* UserData, int64_t SeekCount, int whence);
		friend int h_ReadSearchableInputData(void* UserData, uint8_t* OutputBuffer, int buf_size);
		std::shared_ptr<void> m_InternalData = nullptr;
		std::unique_ptr<MBUtility::MBSearchableInputStream> m_CostumIO = {};
		std::vector<StreamInfo> m_InputStreams = {};
		std::string m_ProbedData = "";
		size_t m_ReadProbeData = 0;
		bool m_FileEnded = false;
	public:
		size_t NumberOfStreams() { return(m_InputStreams.size()); }
		StreamInfo const& GetStreamInfo(size_t StreamIndex);
		StreamPacket GetNextPacket(size_t* StreamIndex);
		//bool Finished() const;

		ContainerDemuxer(std::string const& InputFile);
		ContainerDemuxer(std::unique_ptr<MBUtility::MBSearchableInputStream>&& InputStream);
		~ContainerDemuxer();
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

		StreamEncoder const& GetOutputEncoder(size_t EncoderIndex);
		
		
		//ffmpeg verkar inte ha någon stream data i framen, kanske innebär att framen på något sätt är atomisk...
		void InsertFrame(StreamFrame const& PacketToInsert, size_t StreamIndex);
		void Finalize();
	};
	MediaType GetCodecMediaType(Codec InputCodec);
	void Transcode(std::string const& InputFile, std::string const& OutputFile,Codec NewAudioCodec,Codec NewVideoCodec);
};
//*/