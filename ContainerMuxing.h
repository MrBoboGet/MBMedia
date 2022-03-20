#pragma once
#define NOMINMAX

#include <MBUtility/MBInterfaces.h>

#include <memory>
#include <vector>
#include <string>

#include "MBAudioDefinitions.h"
#include "MBVideoDefinitions.h"

#include "DoNothing.h"

namespace MBMedia
{
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
	};

	struct AudioDecodeInfo
	{
		AudioParameters AudioInfo;
		TimeBase StreamTimebase = { 0,0 };
		int64_t AverageBitrate = -1;
		size_t FrameSize = -1;
	};
	struct AudioEncodeInfo
	{
		AudioParameters AudioInfo;
		TimeBase StreamTimebase = { 0,0 };
		uint32_t TargetBitrate = 0;
		size_t FrameSize = -1;
	};


	class StreamPacket
	{
	private:
		friend class ContainerDemuxer;
		friend class OutputContext;
		friend class StreamDecoder;
		friend class StreamEncoder;
		std::unique_ptr<void, void (*)(void*)> m_InternalData = std::unique_ptr<void, void (*)(void*)>(nullptr, _DoNothing);
		MediaType m_Type = MediaType::Null;
		StreamPacket(void* FFMPEGPacket, TimeBase PacketTimebase, MediaType PacketType);
		StreamPacket() {};
		TimeBase m_TimeBase;
	public:
		//float GetDuration();
		TimeBase GetTimebase();
		void Rescale(TimeBase NewTimebase);
		void Rescale(TimeBase OriginalTimebase, TimeBase NewTimebase);

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
		friend class AudioFrameConverter;
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
		void SetPresentationTime(int64_t NewTimestamp);

		int64_t GetDuration() const;// <0 om inte känd
		void SetDuration(int64_t NewDuration);

		TimeBase GetTimeBase()const { return(m_TimeBase); };
		MediaType GetMediaType() const { return(m_MediaType); };

		VideoParameters GetVideoParameters() const;
		AudioParameters GetAudioParameters() const;

		AudioFrameInfo GetAudioFrameInfo() const;

		uint8_t** GetData();
	};
	struct StreamInfo
	{
	private:
		friend class ContainerDemuxer;
		friend class StreamDecoder;
		size_t m_StreamIndex = -1;//all detta f�ruts�tter att stream datan  inte f�r�ndras
		std::shared_ptr<void> m_InternalData = nullptr;
		StreamInfo(std::shared_ptr<void> FFMPEGContainerData, size_t StreamIndex); //anv�nds av decoder, en decode context har en konstant m�ngd streams
		MediaType m_Type = MediaType::Null;
		Codec m_StreamCodec = Codec::Null;

		AudioDecodeInfo m_AudioDecodeInfo;
		VideoDecodeInfo m_VideoDecodeInfo;
	public:
		TimeBase StreamTimebase;
		int64_t StreamDuration = -1;
		AudioDecodeInfo GetAudioInfo() const;
		VideoDecodeInfo GetVideoInfo() const;
		MediaType GetMediaType() const { return(m_Type); };
		Codec GetCodec() const { return(m_StreamCodec); };
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
		AudioToFrameConverter(AudioParameters const& InputParameters, int64_t InitialTimestamp, TimeBase OutputTimebase, size_t FrameSize);
		void InsertAudioData(const uint8_t* const*, size_t FrameSize);
		void Flush();
		StreamFrame GetNextFrame();
	};


	//Semantiken bakom b�da dem h�r struktsen blev aningen fucky och lite wucky, men den under konverterar per frame, och den �ver per audio data
	class AudioFrameConverter
	{
	private:
		friend void swap(AudioFrameConverter& LeftConverter, AudioFrameConverter& RightConverter);
		std::queue<StreamFrame> m_StoredFrames = {};
		std::unique_ptr<void, void (*)(void*)> m_ConversionContext = std::unique_ptr<void, void (*)(void*)>(nullptr, _DoNothing);
		std::unique_ptr<void, void (*)(void*)> m_AudioDataBuffer = std::unique_ptr<void, void (*)(void*)>(nullptr, _DoNothing);
		AudioParameters m_NewAudioParameters;
		AudioParameters m_OldAudioParameters;
		TimeBase m_InputTimebase;
		size_t m_NewFrameSize = -1;
		//DEBUG
		//ANTAGANDE varje input timestamp har monotont v�xande pts
		int64_t DEBUG_LastTimestamp = 0;
		//DEBUG
		int64_t m_CurrentTimestamp = 0;
		bool m_FirstTimestampSet = false;
		bool m_Flushed = false;
		void p_ConvertNewFrame();
		void p_FlushBufferedFrames();
	public:
		AudioFrameConverter(TimeBase InputTimebase, AudioParameters const& OldParameters, AudioParameters const& NewParameters, size_t NewFrameSize);
		void InsertFrame(StreamFrame const& FrameToInsert);
		StreamFrame GetNextFrame();
		void Flush();
		void Reset();
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
		void Reset();
	};
	////StreamFrame FlipPictureHorizontally(StreamFrame const& ImageToFlip);
	StreamFrame FlipRGBPictureHorizontally(StreamFrame const& ImageToFlip);
	class FrameConverter
	{
	private:
		friend void swap(FrameConverter& LeftConverter, FrameConverter& RightConverter);
		std::unique_ptr<AudioFrameConverter> m_AudioConverter = nullptr;
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

		FrameConverter(TimeBase InputTimebase, AudioParameters const& OldParameters, AudioParameters const& NewParameters, size_t NewFrameSize);
		FrameConverter(TimeBase InputTimebase, VideoParameters const& OldParameters, VideoParameters const& NewParameters);
		void Flush();
		void Reset();
		void InsertFrame(StreamFrame const& FrameToInsert);
		StreamFrame GetNextFrame();

		//SwrContext converting and storing at the same time motivated this interface, even tough it isn't consistent with the rest.
		//Passing nullptr flushes and returns the stored records untill the media type is null
		//StreamFrame ConvertFrame(const StreamFrame* FrameToConvert);
		//Because freeing a swrcontext before it is flushed we put this destructor here temporarily for debugging purposes
		~FrameConverter();
	};

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
		//uint64_t m_CurrentPts = -1;
		StreamFrame p_GetDecodedFrame();
		FrameConverter m_FrameConverter;
	public:
		void SetAudioConversionParameters(AudioParameters const& ParametersToConvertTo, size_t NewFrameSize);
		void SetVideoConversionParameters(VideoParameters const& ParametersToConvertTo);

		AudioDecodeInfo GetAudioDecodeInfo() const;
		VideoDecodeInfo GetVideoDecodeInfo() const;

		TimeBase GetCodecTimebase() const { return(m_CodecTimebase); };
		TimeBase GetStreamTimebase() const { return(m_CodecTimebase); };

		StreamDecoder(StreamDecoder const&) = delete;
		StreamDecoder(StreamDecoder&&) = default;
		StreamDecoder& operator=(StreamDecoder&&) = default;

		MediaType GetType() const { return(m_Type); };
		StreamDecoder(StreamInfo const& StreamToDecode);//implicit antagande h�r, att decoda en stream kan g�ras "korrekt", att omvandla eller omtolka datan b�r g�ras fr�n framesen vi f�r efter

		void InsertPacket(StreamPacket const& PacketToDecode);
		StreamFrame GetNextFrame();
		void Flush();
		void Reset();
		//~StreamDecoder();
	};

	class StreamEncoder
	{
	private:
		friend class OutputContext;
		std::unique_ptr<void, void (*)(void*)> m_InternalData = std::unique_ptr<void, void (*)(void*)>(nullptr, _DoNothing);
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

		bool m_IsValid = true;
	public:
		size_t NumberOfStreams() { if (!m_IsValid) { return(-1); }; return(m_InputStreams.size()); }
		bool StreamInfoAvailable();
		StreamInfo const& GetStreamInfo(size_t StreamIndex);
		StreamPacket GetNextPacket(size_t* StreamIndex);
		//bool Finished() const;

		//OBS kan vara tidiagre än specifcerat
		void SeekPosition(size_t StreamIndexToSearch, int64_t StreamTimestampPosition);

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


		//ffmpeg verkar inte ha n�gon stream data i framen, kanske inneb�r att framen p� n�got s�tt �r atomisk...
		void InsertFrame(StreamFrame const& PacketToInsert, size_t StreamIndex);
		void Finalize();
	};
}