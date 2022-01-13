#define NOMINMAX
#include "MBMedia.h"
#ifdef _WIN32
#include <unknwn.h>
#include<strmif.h>
#endif // __WIN32__
#include <vector>
#include "MBMedia.h"
#include <iostream>
#include <filesystem>
#include <assert.h>
#include <numeric>
extern "C"
{
#include <ffmpeg/libavcodec/avcodec.h>
#include <ffmpeg/libavformat/avformat.h>
#include <ffmpeg/libswresample/swresample.h>
#include <ffmpeg/libswscale/swscale.h>
#include <ffmpeg/libavutil/audio_fifo.h>
#include <ffmpeg/libavutil/imgutils.h>
//#include <ffmpeg/libavutil/>
}
///*
namespace MBMedia
{
	void h_Print_ffmpeg_Error(int ReadFrameResponse)
	{
		if (ReadFrameResponse >= 0)
		{
			return;
		}
		char MessageBuffer[512];
		av_strerror(ReadFrameResponse, MessageBuffer, 512);
		std::cout << "FFMpeg error: " << MessageBuffer << std::endl;
	}
	int FFMPEGCall(int ReadFrameResponse)
	{
		h_Print_ffmpeg_Error(ReadFrameResponse);
		return(ReadFrameResponse);
	}
	void _FreeFormatContext(void* DataToFree)
	{
		avformat_free_context((AVFormatContext*)DataToFree);
		//FFMPEGCall();
	}
	MediaType h_FFMPEGMediaTypeToMBMediaType(AVMediaType TypeToConvert)
	{
		MediaType ReturnValue = MediaType::Null;
		if (TypeToConvert == AVMEDIA_TYPE_VIDEO)
		{
			ReturnValue = MediaType::Video;
		}
		else if (TypeToConvert == AVMEDIA_TYPE_AUDIO)
		{
			ReturnValue = MediaType::Audio;
		}
		else if (TypeToConvert == AVMEDIA_TYPE_SUBTITLE)
		{
			ReturnValue = MediaType::Subtitles;
		}
		return(ReturnValue);
	}
	TimeBase h_RationalToTimebase(AVRational RationalToConvert)
	{
		TimeBase ReturnValue = { RationalToConvert.num,RationalToConvert.den };
		return(ReturnValue);
	}
	struct MBMediaTypeConnector
	{
		Codec AssociatdCodec = Codec::Null;
		MediaType AssociatedMediaType = MediaType::Null;
		AVCodecID AssoicatedCodecId = (AVCodecID)-1;
	};
	const MBMediaTypeConnector ConnectedTypes[(size_t)Codec::Null] = {
		{Codec::AAC,MediaType::Audio,AV_CODEC_ID_AAC},
		{Codec::H264,MediaType::Video,AV_CODEC_ID_H264},
		{Codec::H265,MediaType::Video,AV_CODEC_ID_H265},
		{Codec::VP9,MediaType::Video,AV_CODEC_ID_VP9},
	};
	MediaType GetCodecMediaType(Codec InputCodec)
	{
		return(ConnectedTypes[size_t(InputCodec)].AssociatedMediaType);
	}
	Codec h_FFMPEGCodecTypeToMBCodec(AVCodecID TypeToConvert)
	{
		Codec ReturnValue = Codec::Null;
		for (size_t i = 0; i < (size_t)Codec::Null; i++)
		{
			if (ConnectedTypes[i].AssoicatedCodecId == TypeToConvert)
			{
				ReturnValue = ConnectedTypes[i].AssociatdCodec;
			}
		}
		return(ReturnValue);
	}
	SampleFormat h_FFMPEGAudioFormatToMBFormat(AVSampleFormat FormatToConvert)
	{
		return(SampleFormat(FormatToConvert));
	}
	AVSampleFormat h_MBSampleFormatToFFMPEGSampleFormat(SampleFormat FormatToConvert)
	{
		return(AVSampleFormat(FormatToConvert));
	}
	int64_t h_MBLayoutToFFMPEGLayout(ChannelLayout LayoutToConvert)
	{
		return(int64_t(LayoutToConvert));
	}
	ChannelLayout h_FFMPEGLayoutToMBLayout(int64_t LayoutToConvert)
	{
		return(ChannelLayout(LayoutToConvert));
	}
	VideoFormat h_FFMPEGVideoFormatToMBVideoFormat(AVPixelFormat FormatToConvert);
	AVPixelFormat h_MBVideoFormatToFFMPEGVideoFormat(VideoFormat FormatToConvert)
	{
		AVPixelFormat ReturnValue = AVPixelFormat::AV_PIX_FMT_NONE;
		int64_t MB_VAAPI = (uint64_t)VideoFormat::AV_PIX_FMT_VAAPI;
		int64_t FFMPEG_VAAPI = (uint64_t)AVPixelFormat::AV_PIX_FMT_VAAPI;
		if (MB_VAAPI == FFMPEG_VAAPI)
		{
			ReturnValue = AVPixelFormat(FormatToConvert);
		}
		else
		{
			int64_t Lowest_VAAPI = MB_VAAPI < FFMPEG_VAAPI ? MB_VAAPI : FFMPEG_VAAPI;
			int64_t VAAPI_Distance = std::abs(MB_VAAPI - FFMPEG_VAAPI);
			int64_t FormatToConvertDistance = int64_t(FormatToConvert) - Lowest_VAAPI;
			if (FormatToConvertDistance <= VAAPI_Distance && FormatToConvertDistance >= 0)
			{
				throw std::exception();
			}
			if (FormatToConvertDistance < 0)
			{
				ReturnValue = AVPixelFormat(int64_t(FormatToConvert));
			}
			else
			{
				ReturnValue = MB_VAAPI < FFMPEG_VAAPI ? AVPixelFormat(int64_t(FormatToConvert) + VAAPI_Distance) : AVPixelFormat(int64_t(FormatToConvert) - VAAPI_Distance);
			}
		}
		assert(h_FFMPEGVideoFormatToMBVideoFormat(ReturnValue) == FormatToConvert);
		return(ReturnValue);
	}
	VideoFormat h_FFMPEGVideoFormatToMBVideoFormat(AVPixelFormat FormatToConvert)
	{
		VideoFormat ReturnValue = VideoFormat::Null;
		int64_t MB_VAAPI = (uint64_t)VideoFormat::AV_PIX_FMT_VAAPI;
		int64_t FFMPEG_VAAPI = (uint64_t)AVPixelFormat::AV_PIX_FMT_VAAPI;
		if (MB_VAAPI == FFMPEG_VAAPI)
		{
			ReturnValue = VideoFormat(FormatToConvert);
		}
		else
		{
			int64_t Lowest_VAAPI = MB_VAAPI < FFMPEG_VAAPI ? MB_VAAPI : FFMPEG_VAAPI;
			int64_t VAAPI_Distance = std::abs(MB_VAAPI - FFMPEG_VAAPI);
			int64_t FormatToConvertDistance = int64_t(FormatToConvert) - Lowest_VAAPI;
			if (FormatToConvertDistance <= VAAPI_Distance && FormatToConvertDistance >= 0)
			{
				throw std::exception();
			}
			if (FormatToConvertDistance < 0)
			{
				ReturnValue = VideoFormat(int64_t(FormatToConvert));
			}
			else
			{
				ReturnValue = MB_VAAPI > FFMPEG_VAAPI ? VideoFormat(int64_t(FormatToConvert) + VAAPI_Distance) : VideoFormat(int64_t(FormatToConvert) - VAAPI_Distance);
			}
		}
		return(ReturnValue);
	}
	//BEGIN StreamInfo
	StreamInfo::StreamInfo(std::shared_ptr<void> FFMPEGContainerData, size_t StreamIndex)
	{
		m_InternalData = FFMPEGContainerData;
		m_StreamIndex = StreamIndex;
		AVStream* StreamData = ((AVFormatContext*)FFMPEGContainerData.get())->streams[StreamIndex];
		m_StreamCodec = h_FFMPEGCodecTypeToMBCodec( StreamData->codecpar->codec_id);
		m_Type = h_FFMPEGMediaTypeToMBMediaType(StreamData->codecpar->codec_type);
	}

	//END StreamInfo

	void _FreePacket(void* PacketToFree)
	{
		AVPacket* Packet = (AVPacket*)PacketToFree;
		av_packet_free(&Packet);
	}
	//BEGIN StreamPacket
	StreamPacket::StreamPacket(void* FFMPEGPacket,TimeBase PacketTimebase,MediaType PacketType)
		//: m_ImplementationData(FFMPEGPacket, _FreePacket)
	{
		if (FFMPEGPacket != nullptr)
		{
			m_Type = PacketType;
			m_InternalData = std::unique_ptr<void, void(*)(void*)>(FFMPEGPacket, _FreePacket);
			m_TimeBase = PacketTimebase;
		}
	}
	//float StreamPacket::GetDuration()
	//{
	//	return()
	//}
	TimeBase StreamPacket::GetTimebase()
	{
		return(m_TimeBase);
	}
	void StreamPacket::Rescale(TimeBase NewTimebase)
	{
		AVPacket* FFMPEGPacket = (AVPacket*)m_InternalData.get();
		av_packet_rescale_ts(FFMPEGPacket, { m_TimeBase.num,m_TimeBase.den }, { NewTimebase.num,NewTimebase.den });
		m_TimeBase = NewTimebase;
	}
	void StreamPacket::Rescale(TimeBase OriginalTimebase, TimeBase NewTimebase)
	{
		AVPacket* FFMPEGPacket = (AVPacket*)m_InternalData.get();
		av_packet_rescale_ts(FFMPEGPacket, { OriginalTimebase.num,OriginalTimebase.den }, { NewTimebase.num,NewTimebase.den });
		m_TimeBase = NewTimebase;
	}
	//END StreamPacket
	SampleFormatInfo s_SampleFormatInfoTable[] = 
	{
		{false,false,false,-1},

		{ true,false,true,1 },
		{ true,false,true,2 },
		{true,true,true,4},
		{true,false,false,sizeof(float)},
		{true,false,false,sizeof(double)},

		{false,false,true,1 },
		{ false,false,true,2 },
		{false,true,true,4},
		{false,false,false,sizeof(float)},
		{false,false,false,sizeof(double)},

		{false,false,false,-1},
	};
	SampleFormatInfo GetSampleFormatInfo(SampleFormat FormatToInspect)
	{
		size_t FormatIndex = size_t(FormatToInspect)+ 1;
		if(FormatIndex > size_t(SampleFormat::Null))
		{
			throw std::exception();
		}
		return(s_SampleFormatInfoTable[FormatIndex]);
	}
	SampleFormat GetPlanarAudioFormat(SampleFormat FormatToConvert)
	{
		SampleFormat ReturnValue = SampleFormat::Null;
		if (FormatToConvert == SampleFormat::Null || FormatToConvert == SampleFormat::NONE)
		{
			return(ReturnValue);
		}
		size_t FormatIndex = size_t(FormatToConvert);
		if (FormatIndex <= 4)
		{
			ReturnValue = SampleFormat(FormatIndex + 5);
		}
		else
		{
			ReturnValue = FormatToConvert;
		}
		return(ReturnValue);
	}
	SampleFormat GetInterleavedAudioFormat(SampleFormat FormatToConvert)
	{
		SampleFormat ReturnValue = SampleFormat::Null;
		if (FormatToConvert == SampleFormat::Null || FormatToConvert == SampleFormat::NONE)
		{
			return(ReturnValue);
		}
		size_t FormatIndex = size_t(FormatToConvert);
		if (FormatIndex <= 4)
		{
			ReturnValue = FormatToConvert;
		}
		else
		{
			ReturnValue = SampleFormat(FormatIndex - 5);
		}
		return(ReturnValue);
	}


	//BEGIN ContainerDemuxer
	ContainerDemuxer::ContainerDemuxer(std::string const& InputFile)
	{
		AVFormatContext* InputFormatContext;
		InputFormatContext = avformat_alloc_context();
		//avformat_new_stream
		//allokerar format kontexten, information om filtyp och innehåll,läser bara headers och etc
		FFMPEGCall(avformat_open_input(&InputFormatContext, InputFile.c_str(), NULL, NULL));
		//läsar in data om själva datastreamsen
		FFMPEGCall(avformat_find_stream_info(InputFormatContext, NULL));
		m_InternalData = std::shared_ptr<void>(InputFormatContext, _FreeFormatContext);
		for (size_t i = 0; i < InputFormatContext->nb_streams; i++)
		{
			m_InputStreams.push_back(StreamInfo(m_InternalData, i));//hacky af, sparar hela decode contexten eftersom free_stream inte är en del av en public header
		}
	}
	int h_ReadSearchableInputData(void* UserData, uint8_t* OutputBuffer, int buf_size)
	{
		ContainerDemuxer* Demuxer =(ContainerDemuxer*) UserData;
		size_t BytesRead = 0;
		bool ShouldReadFromStream = false;
		if (Demuxer->m_ProbedData.size() > Demuxer->m_ReadProbeData)
		{
			size_t BytesToRead = Demuxer->m_ProbedData.size() - Demuxer->m_ReadProbeData >= buf_size ? buf_size : Demuxer->m_ProbedData.size() - Demuxer->m_ReadProbeData;
			BytesRead = BytesToRead;
			if (BytesRead < buf_size)
			{
				ShouldReadFromStream = true;
			}
		}
		else
		{
			ShouldReadFromStream = true;
		}
		if (ShouldReadFromStream)
		{
			BytesRead += Demuxer->m_CostumIO->Read(OutputBuffer + BytesRead, buf_size - BytesRead);
		}
		if (BytesRead == 0)
		{
			return(AVERROR_EOF);
		}
		return(BytesRead);
	}
	int64_t h_SeekSearchableInputStream(void* UserData, int64_t SeekCount, int whence)
	{
		ContainerDemuxer* Demuxer = (ContainerDemuxer*) UserData;
		int64_t ReturnValue = -1;
		if (whence==AVSEEK_SIZE)
		{
			ReturnValue = -1;
		}
		if (whence == SEEK_SET)
		{
			ReturnValue = Demuxer->m_CostumIO->SetInputPosition(SeekCount, whence);
		}
		if (whence == SEEK_CUR)
		{
			ReturnValue = Demuxer->m_CostumIO->SetInputPosition(SeekCount, whence);
		}
		if (whence == SEEK_END)
		{	
			ReturnValue = Demuxer->m_CostumIO->SetInputPosition(SeekCount, whence);
		}
		return(ReturnValue);
	}
	ContainerDemuxer::ContainerDemuxer(std::unique_ptr<MBUtility::MBSearchableInputStream>&& InputStream)
	{
		AVFormatContext* InputFormatContext;
		InputFormatContext = avformat_alloc_context();
		//avformat_new_stream
		//InputFormatContext->pb
		m_CostumIO = std::move(InputStream);
		InputFormatContext->flags|=AVFMT_FLAG_CUSTOM_IO;
		InputFormatContext->pb = avio_alloc_context((unsigned char*)av_malloc(8192), 8192, 0, this, h_ReadSearchableInputData, NULL, h_SeekSearchableInputStream);
		//allokerar format kontexten, information om filtyp och innehåll,läser bara headers och etc
		//InputFormatContext->ifo

		const size_t ProbeDataSize = 1000000;
		//uint8_t* ProbeData[ProbeDataSize + AVPROBE_PADDING_SIZE];
		//memset(ProbeData, 0, ProbeDataSize + AVPROBE_PADDING_SIZE);
		m_ProbedData = std::string(ProbeDataSize, 0);
		size_t ReadBytes = m_CostumIO->Read(m_ProbedData.data(), ProbeDataSize);
		m_ProbedData.resize(ReadBytes);
		AVProbeData ProbeStruct;
		ProbeStruct.buf = (unsigned char*)m_ProbedData.data();
		ProbeStruct.buf_size = ProbeDataSize;
		ProbeStruct.filename = "";
		ProbeStruct.mime_type = "";
		InputFormatContext->iformat = av_probe_input_format(&ProbeStruct, 1);
		//InputFormatContext->iformat =(AVInputFormat*)123123123;
		m_ProbedData = "";//OBS efersom vi antar att streamen är searchable, kanske inte alltid är det?
		m_CostumIO->SetInputPosition(0);
		FFMPEGCall(avformat_open_input(&InputFormatContext, "", NULL, NULL));
		//läsar in data om själva datastreamsen
		FFMPEGCall(avformat_find_stream_info(InputFormatContext, NULL));
		m_InternalData = std::shared_ptr<void>(InputFormatContext, _FreeFormatContext);
		for (size_t i = 0; i < InputFormatContext->nb_streams; i++)
		{
			m_InputStreams.push_back(StreamInfo(m_InternalData, i));//hacky af, sparar hela decode contexten eftersom free_stream inte är en del av en public header
		}
	}
	ContainerDemuxer::~ContainerDemuxer()
	{
		if (m_CostumIO != nullptr)
		{
			AVFormatContext*  FormatContext= (AVFormatContext *)m_InternalData.get();
			av_free(FormatContext->pb->buffer);
		}
	}
	//bool ContainerDemuxer::EndOfFile()
	//{
	//	return(true);
	//}
	StreamInfo const& ContainerDemuxer::GetStreamInfo(size_t StreamIndex)
	{
		return(m_InputStreams[StreamIndex]);
	}
	StreamPacket ContainerDemuxer::GetNextPacket(size_t* StreamIndex)
	{
		AVPacket* NewPacket = av_packet_alloc();
		AVFormatContext* InputContext = (AVFormatContext*)m_InternalData.get();
		int ReadResponse = av_read_frame(InputContext, NewPacket);
		if (ReadResponse >= 0)
		{
			AVStream* AssociatedStream = InputContext->streams[NewPacket->stream_index];
			*StreamIndex = NewPacket->stream_index;
			return(StreamPacket(NewPacket, { AssociatedStream->time_base.num,AssociatedStream->time_base.den }, h_FFMPEGMediaTypeToMBMediaType(AssociatedStream->codecpar->codec_type)));
		}
		else 
		{
			*StreamIndex = -1;
			return(StreamPacket(nullptr, { 0,0 }, MediaType::Null));
		}
	}
	//bool ContainerDemuxer::Finished() const
	//{
	//	return(m_FileEnded);
	//}
	//END ContainerDemuxer

	//BEGIN OutputContext
	OutputContext::OutputContext(std::string const& OutputFile)
	{
		AVFormatContext* OutputFormatContext = nullptr;
		FFMPEGCall(avformat_alloc_output_context2(&OutputFormatContext, NULL, NULL, OutputFile.c_str()));
		FFMPEGCall(avio_open(&OutputFormatContext->pb, OutputFile.c_str(), AVIO_FLAG_WRITE));
		if (OutputFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
		{
			OutputFormatContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}
		m_InternalData = std::unique_ptr<void, void (*)(void*)>(OutputFormatContext, _FreeFormatContext);
	}

	void OutputContext::AddOutputStream(StreamEncoder&& Encoder)
	{
		AVFormatContext* OutputFormatContext = (AVFormatContext*)m_InternalData.get();
		AVCodecContext* EncoderContext = (AVCodecContext*)Encoder.m_InternalData.get();
		const AVCodec* EncoderCodec = EncoderContext->codec;
		m_OutputEncoders.push_back(std::move(Encoder));
		AVStream* NewStream = avformat_new_stream(OutputFormatContext, EncoderCodec);
		FFMPEGCall(avcodec_parameters_from_context(NewStream->codecpar, EncoderContext));
	}
	StreamEncoder const& OutputContext::GetOutputEncoder(size_t Index)
	{
		return(m_OutputEncoders[Index]);
	}
	void OutputContext::p_WritePacket(StreamPacket& PacketToWrite, size_t StreamIndex)
	{
		AVFormatContext* OutputFormat = (AVFormatContext*)m_InternalData.get();
		PacketToWrite.Rescale({OutputFormat->streams[StreamIndex]->time_base.num, OutputFormat->streams[StreamIndex]->time_base.den	});
		AVPacket* FFMpegPacket = (AVPacket*)PacketToWrite.m_InternalData.get();
		//vet inte om det står någonstans, men man måste specifiera vilket index packetet är när man ska skriva till streamen...
		FFMpegPacket->stream_index = StreamIndex;
		if (StreamIndex == 1)
		{
			OutputFormat->streams[1]->codecpar->frame_size;
			int hej = 2;
		}
		//FFMpegPacket->
		//std::cout << "Stream time: " << FFMpegPacket->pts * (double(OutputFormat->streams[StreamIndex]->time_base.num) / double(OutputFormat->streams[StreamIndex]->time_base.den)) << std::endl;
		FFMPEGCall(av_interleaved_write_frame(OutputFormat, FFMpegPacket));
	}
	void OutputContext::p_WriteTrailer()
	{
		AVFormatContext* OutputFormat = (AVFormatContext*)m_InternalData.get();
		FFMPEGCall(av_write_trailer(OutputFormat));
	}
	void OutputContext::WriteHeader()
	{
		AVFormatContext* OutputFormatContext = (AVFormatContext*)m_InternalData.get();
		if (OutputFormatContext == nullptr)
		{
			return;
		}
		FFMPEGCall(avformat_write_header(OutputFormatContext, NULL));
	}
	void OutputContext::InsertFrame(StreamFrame const& FrameToInsert, size_t StreamIndex)
	{
		m_OutputEncoders[StreamIndex].InsertFrame(FrameToInsert);
		while (true)
		{
			StreamPacket NewPacket = m_OutputEncoders[StreamIndex].GetNextPacket();
			if (NewPacket.GetType() == MediaType::Null)
			{
				break;
			}
			//int hej = 0 / 0;
			//NewPacket.Rescale(m_OutputEncoders[);
			p_WritePacket(NewPacket, StreamIndex);
		}
	}
	void OutputContext::Finalize()
	{
		for (size_t i = 0; i < m_OutputEncoders.size(); i++)
		{
			m_OutputEncoders[i].Flush();
			while (true)
			{
				StreamPacket NewPacket = m_OutputEncoders[i].GetNextPacket();
				if (NewPacket.GetType() == MediaType::Null)
				{
					break;
				}
				p_WritePacket(NewPacket, i);
			}
		}
		p_WriteTrailer();
	}
	//END OutputContext
	void _FreeFrame(void* FFMPEGFrameToFree) 
	{
		AVFrame* Frame =(AVFrame*)FFMPEGFrameToFree;
		AVFrame* Frame2 =(AVFrame*)FFMPEGFrameToFree;
		if (Frame->buf[0] == NULL)
		{
			//TODO herre gud vad ass, hacky sätt att få en frame att kunna freea även om jag allokera den med avpicture_fill, men då ska man bara ta bort försat pointer...
			av_freep(&Frame->data[0]);
			//size_t Offset = 0;
			//while (Frame->data[Offset] != nullptr && Offset < AV_NUM_DATA_POINTERS)
			//{
			//	av_free(Frame->data[Offset]);
			//	Offset++;
			//}
		}
		av_frame_free(&Frame);
	}
	//StreamFrame
	StreamFrame::StreamFrame()
	{

	}
	uint8_t** StreamFrame::GetData()
	{
		if (m_MediaType == MediaType::Null)
		{
			return(nullptr);
		}
		AVFrame* FFMPEGFrame = (AVFrame*)m_InternalData.get();
		return(FFMPEGFrame->data);
	}
	VideoParameters StreamFrame::GetVideoParameters() const
	{
		if (m_MediaType != MediaType::Video || m_InternalData.get() == nullptr)
		{
			throw std::exception();
		}
		const AVFrame* FFMPEGFrame = ( const AVFrame*)m_InternalData.get();
		VideoParameters ReturnValue;
		ReturnValue.Width = FFMPEGFrame->width;
		ReturnValue.Height = FFMPEGFrame->height;
		ReturnValue.Format = h_FFMPEGVideoFormatToMBVideoFormat((AVPixelFormat) FFMPEGFrame->format);
		return(ReturnValue);
	}
	int64_t StreamFrame::GetPresentationTime() const
	{
		const AVFrame* FFMPEGFrame = (const AVFrame*)m_InternalData.get();
		return(FFMPEGFrame->pts);
	}
	AudioParameters StreamFrame::GetAudioParameters() const
	{
		const AVFrame* FFMPEGData = (const AVFrame*)m_InternalData.get();
		AudioParameters ReturnValue;
		ReturnValue.SampleRate = FFMPEGData->sample_rate;
		ReturnValue.NumberOfChannels = FFMPEGData->channels;
		ReturnValue.AudioFormat = h_FFMPEGAudioFormatToMBFormat((AVSampleFormat) FFMPEGData->format);
		ReturnValue.Layout = ChannelLayout::Null;
		return(ReturnValue);
	}
	AudioFrameInfo StreamFrame::GetAudioFrameInfo() const
	{
		if (m_InternalData == nullptr)
		{
			throw std::exception();
		}
		AudioFrameInfo ReturnValue;
		AVFrame* FrameData = (AVFrame*)m_InternalData.get();
		ReturnValue.NumberOfSamples = FrameData->nb_samples;
		return(ReturnValue);
	}
	StreamFrame::StreamFrame(void* FFMPEGData,TimeBase FrameTimeBase,MediaType FrameType)
	{
		if (FFMPEGData != nullptr)
		{
			m_InternalData = std::unique_ptr<void, void(*)(void*)>(FFMPEGData, _FreeFrame);
			m_MediaType = FrameType;
			m_TimeBase = FrameTimeBase;
		}
	}
	//StreamFrame


	//BEGIN StreamDecoder
	void _FreeCodecContext(void* PointerToFree)
	{
		AVCodecContext* CodecContext = (AVCodecContext*)PointerToFree;
		avcodec_free_context(&CodecContext);
	}
	void StreamDecoder::SetAudioConversionParameters(AudioParameters const& NewParameters,size_t NewFrameSize)
	{
		m_FrameConverter = FrameConverter(m_StreamTimebase,GetAudioDecodeInfo().AudioInfo, NewParameters,NewFrameSize);
	}
	void StreamDecoder::SetVideoConversionParameters(VideoParameters const& NewParameters)
	{
		m_FrameConverter = FrameConverter(m_StreamTimebase, GetVideoDecodeInfo().VideoInfo, NewParameters);
	}
	AudioDecodeInfo StreamDecoder::GetAudioDecodeInfo() const
	{
		AudioDecodeInfo ReturnValue;
		const AVCodecContext* CodecContext = (const AVCodecContext*)m_InternalData.get();
		ReturnValue.AudioInfo.AudioFormat = h_FFMPEGAudioFormatToMBFormat(CodecContext->sample_fmt);
		ReturnValue.AudioInfo.SampleRate = CodecContext->sample_rate;
		ReturnValue.AudioInfo.NumberOfChannels = CodecContext->channels;
		ReturnValue.AudioInfo.Layout = h_FFMPEGLayoutToMBLayout(CodecContext->channel_layout);
		ReturnValue.FrameSize = CodecContext->frame_size;
		ReturnValue.AverageBitrate = CodecContext->bit_rate;
		ReturnValue.StreamTimebase = GetStreamTimebase();

		ReturnValue.StreamTimebase = this->GetStreamTimebase();//ganska trassligt, kommer inte ihåg varför jag har 2 stycken...
		return(ReturnValue);
	}
	VideoDecodeInfo StreamDecoder::GetVideoDecodeInfo() const
	{
		VideoDecodeInfo ReturnValue;
		const AVCodecContext* CodecContext = (const AVCodecContext*)m_InternalData.get();
		ReturnValue.VideoInfo.Format = h_FFMPEGVideoFormatToMBVideoFormat(CodecContext->pix_fmt);
		ReturnValue.VideoInfo.Width = CodecContext->width;
		ReturnValue.VideoInfo.Height = CodecContext->height;
		ReturnValue.AverageBitrate = CodecContext->bit_rate;
		ReturnValue.StreamTimebase = GetStreamTimebase();
		return(ReturnValue);
	}
	StreamDecoder::StreamDecoder(StreamInfo const& StreamToDecode)
	{
		AVFormatContext* ContainerFormat = (AVFormatContext*)StreamToDecode.m_InternalData.get();
		AVCodecParameters* NewInputCodecParamters = ContainerFormat->streams[StreamToDecode.m_StreamIndex]->codecpar;
		AVCodec* NewInputCodec = avcodec_find_decoder(NewInputCodecParamters->codec_id);
		AVCodecContext* NewCodexContext = avcodec_alloc_context3(NewInputCodec);
		FFMPEGCall(avcodec_parameters_to_context(NewCodexContext, NewInputCodecParamters));
		//sedan måste vi öppna den, vet inte riktigt varför, initializerar den kanske?
		FFMPEGCall(avcodec_open2(NewCodexContext, NewInputCodec, NULL));
		m_InternalData = std::shared_ptr<void>(NewCodexContext, _FreeCodecContext);
		m_Type = h_FFMPEGMediaTypeToMBMediaType(NewInputCodec->type);
		if (m_Type == MediaType::Audio)
		{
			m_CodecTimebase = { ContainerFormat->streams[StreamToDecode.m_StreamIndex]->time_base.num,ContainerFormat->streams[StreamToDecode.m_StreamIndex]->time_base.den };
			m_StreamTimebase = m_CodecTimebase;
		}
		else if (m_Type == MediaType::Video)
		{
			//m_TimeBase = { ContainerFormat->streams[StreamToDecode.m_StreamIndex]->time_base.num,ContainerFormat->streams[StreamToDecode.m_StreamIndex]->time_base.den };
			AVRational FrameRate = av_guess_frame_rate(ContainerFormat, ContainerFormat->streams[StreamToDecode.m_StreamIndex], NULL);
			m_CodecTimebase = { FrameRate.den,FrameRate.num };
			m_StreamTimebase = { ContainerFormat->streams[StreamToDecode.m_StreamIndex]->time_base.num,ContainerFormat->streams[StreamToDecode.m_StreamIndex]->time_base.den };
		}
		//DecodeCodecContext.push_back(NewCodexContext);
	}
	void StreamDecoder::InsertPacket(StreamPacket const& PacketToDecode)
	{
		if (PacketToDecode.GetType() != GetType())
		{
			throw std::exception();
		}
		AVCodecContext* CodecContext = (AVCodecContext*)m_InternalData.get();
		const AVPacket* PacketToInsert = (const AVPacket*)PacketToDecode.m_InternalData.get();
		FFMPEGCall(avcodec_send_packet(CodecContext, (const AVPacket*)PacketToDecode.m_InternalData.get()));
	}
	StreamFrame StreamDecoder::p_GetDecodedFrame()
	{
		StreamFrame ReturnValue = StreamFrame();
		if (m_DecodeStreamFinished)
		{
			return(ReturnValue);
		}
		AVCodecContext* CodecContext = (AVCodecContext*)m_InternalData.get();
		AVFrame* NewFrame = av_frame_alloc();
		int RecieveResult = avcodec_receive_frame(CodecContext, NewFrame);
		MediaType FrameType = m_Type;
		if (RecieveResult < 0)
		{
			av_frame_free(&NewFrame);
			FrameType = MediaType::Null;
		}
		if (m_Flushing && FrameType == MediaType::Null)
		{
			m_DecodeStreamFinished = true;
		}
		return(StreamFrame(NewFrame, m_StreamTimebase, m_Type));
	}
	StreamFrame StreamDecoder::GetNextFrame()
	{
		StreamFrame ReturnValue = p_GetDecodedFrame();
		bool FrameConverted = false;
		if (ReturnValue.GetMediaType() != MediaType::Null && m_FrameConverter.IsInitialised())
		{
			FrameConverted = true;
			m_FrameConverter.InsertFrame(ReturnValue);
			ReturnValue = m_FrameConverter.GetNextFrame();
		}
		//^ kan inte hända samtidigt
		if (m_DecodeStreamFinished == true && m_FrameConverter.IsInitialised())
		{
			ReturnValue = m_FrameConverter.GetNextFrame();
		}
		return(ReturnValue);
	}
	void StreamDecoder::Flush()
	{
		if (m_Flushing)
		{
			return;
		}
		m_Flushing = true;
		AVCodecContext* CodecContext = (AVCodecContext*)m_InternalData.get();
		FFMPEGCall(avcodec_send_packet(CodecContext, nullptr));
		if (m_FrameConverter.IsInitialised())
		{
			m_FrameConverter.Flush();
		}
	}

	//END StreamDecoder

	void _FreeSwrContext(void* ContextToFree)
	{
		SwrContext* FFMPEGContext = (SwrContext*)ContextToFree;
		swr_free(&FFMPEGContext);
	}
	void _FreeAudioFifo(void* BufferToFree)
	{
		AVAudioFifo* FFMPEGBuffer = (AVAudioFifo*)BufferToFree;
		av_audio_fifo_free(FFMPEGBuffer);
	}
	void ConvertSampleData(const uint8_t** InputData, AudioParameters const& InputParameters, uint8_t** OutputBuffer, AudioParameters const& OutputParameters,size_t InputSamplesToConvert)
	{
		SwrContext* ConversionContext = swr_alloc_set_opts(NULL,
			h_MBLayoutToFFMPEGLayout(OutputParameters.Layout),
			h_MBSampleFormatToFFMPEGSampleFormat(OutputParameters.AudioFormat),
			OutputParameters.SampleRate,
			h_MBLayoutToFFMPEGLayout(InputParameters.Layout),
			h_MBSampleFormatToFFMPEGSampleFormat(InputParameters.AudioFormat),
			InputParameters.SampleRate,
			0,
			NULL);
		swr_init(ConversionContext);
		size_t OutputSamples = (OutputParameters.SampleRate * InputSamplesToConvert) / (InputParameters.SampleRate);
		int ConvertedSamples = swr_convert(ConversionContext, OutputBuffer, OutputSamples, InputData, InputSamplesToConvert);
		swr_free(&ConversionContext);
		assert(ConvertedSamples == OutputSamples);
	}
	//BEGIN AudioConverter
	void swap(AudioConverter& LeftConverter, AudioConverter& RightConverter)
	{
		std::swap(LeftConverter.m_Flushed, RightConverter.m_Flushed);
		std::swap(LeftConverter.m_ConversionContext, RightConverter.m_ConversionContext);
		std::swap(LeftConverter.m_AudioDataBuffer, RightConverter.m_AudioDataBuffer);
		std::swap(LeftConverter.m_NewAudioParameters, RightConverter.m_NewAudioParameters);
		std::swap(LeftConverter.DEBUG_LastTimestamp, RightConverter.DEBUG_LastTimestamp);
		std::swap(LeftConverter.m_CurrentTimestamp, RightConverter.m_CurrentTimestamp);
		std::swap(LeftConverter.m_FirstTimestampSet, RightConverter.m_FirstTimestampSet);
		std::swap(LeftConverter.m_InputTimebase, RightConverter.m_InputTimebase);
	}
	AudioConverter::AudioConverter(TimeBase InputTimebase, AudioParameters const& OldParameters, AudioParameters const& NewParameters,size_t NewFrameSize)
	{
		m_InputTimebase = InputTimebase;
		m_NewAudioParameters = NewParameters;
		m_OldAudioParameters = OldParameters;
		SwrContext* ConversionContext = swr_alloc_set_opts(NULL,
			h_MBLayoutToFFMPEGLayout(NewParameters.Layout),
			h_MBSampleFormatToFFMPEGSampleFormat(NewParameters.AudioFormat),
			NewParameters.SampleRate,
			h_MBLayoutToFFMPEGLayout(OldParameters.Layout),
			h_MBSampleFormatToFFMPEGSampleFormat(OldParameters.AudioFormat),
			OldParameters.SampleRate,
			0,
			NULL);
		m_NewFrameSize = NewFrameSize;
		FFMPEGCall(swr_init(ConversionContext));

		m_ConversionContext = std::unique_ptr<void, void (*)(void*)>(ConversionContext, _FreeSwrContext);
		//FIFO Buffer
		//TODO fix cases of output frame_size = 0 or input fram_size = 0
		AVAudioFifo* AudioBuffer = av_audio_fifo_alloc(h_MBSampleFormatToFFMPEGSampleFormat(OldParameters.AudioFormat), OldParameters.NumberOfChannels, m_NewFrameSize * 2);
		m_AudioDataBuffer = std::unique_ptr<void, void(*)(void*)>(AudioBuffer, _FreeAudioFifo);
	}
	AVFrame* h_GetFFMPEGFrame(AudioParameters const& AssociatedParameters, size_t NumberOfSamples)
	{
		AVFrame* ReturnValue = av_frame_alloc();
		if (ReturnValue == NULL)
		{
			throw std::exception();
		}
		ReturnValue->format = h_MBSampleFormatToFFMPEGSampleFormat(AssociatedParameters.AudioFormat);
		ReturnValue->channel_layout = h_MBLayoutToFFMPEGLayout(AssociatedParameters.Layout);
		ReturnValue->pict_type = AV_PICTURE_TYPE_NONE;
		ReturnValue->sample_rate = AssociatedParameters.SampleRate;
		//TODO Frame size equal to zero means that it supports variable frames, but nb samples shouldn.t be zero
		//assert(AssociatedParameters.FrameSize > 0);
		ReturnValue->nb_samples = NumberOfSamples;
		if (FFMPEGCall(av_frame_get_buffer(ReturnValue, 0)) < 0)
		{
			throw std::exception();
		}
		return(ReturnValue);
	}
	void AudioConverter::p_ConvertNewFrame()
	{
		AVFrame* ConvertedFrame = h_GetFFMPEGFrame(m_NewAudioParameters, m_NewFrameSize);

		AVAudioFifo* AudioBuffer = (AVAudioFifo*)m_AudioDataBuffer.get();
		size_t InputFrameSize = m_NewFrameSize;
		if (InputFrameSize > av_audio_fifo_size(AudioBuffer))
		{
			InputFrameSize = av_audio_fifo_size(AudioBuffer);
		}
		if (InputFrameSize == 0)
		{
			_FreeFrame(ConvertedFrame);
			return;
		}
		AVFrame* InputFrame = h_GetFFMPEGFrame(m_OldAudioParameters, InputFrameSize);
		FFMPEGCall(av_audio_fifo_read(AudioBuffer, (void**)InputFrame->data, InputFrame->nb_samples));

		SwrContext* ConversionContext = (SwrContext*)m_ConversionContext.get();
		int ConversionResult = 0;
		int64_t TimestampIncrease = ((m_InputTimebase.den / m_InputTimebase.num) * InputFrame->nb_samples) / InputFrame->sample_rate;
		ConversionResult = swr_convert(ConversionContext, ConvertedFrame->data, ConvertedFrame->nb_samples, (uint8_t const**)InputFrame->data, InputFrame->nb_samples);
		if (ConversionResult > 0)
		{
			ConvertedFrame->pts = m_CurrentTimestamp;
			ConvertedFrame->pkt_dts = m_CurrentTimestamp;
			ConvertedFrame->pkt_pts = m_CurrentTimestamp;
			//ReturnValue = StreamFrame(ConvertedFrame, FrameToConvert->GetTimeBase(), m_Type);
			ConvertedFrame->nb_samples = ConversionResult;
			m_StoredFrames.push(StreamFrame(ConvertedFrame, m_InputTimebase, MediaType::Audio));
			_FreeFrame(InputFrame);
		}
		else
		{
			std::cout << "Converting Audioframe: ";
			h_Print_ffmpeg_Error(ConversionResult);
			_FreeFrame(ConvertedFrame);
			_FreeFrame(InputFrame);
		}
		m_CurrentTimestamp += TimestampIncrease;
	}
	void AudioConverter::p_FlushBufferedFrames()
	{
		SwrContext* ConversionContext = (SwrContext*)m_ConversionContext.get();
		//while (swr_get_delay(ConversionContext, m_NewAudioParameters.SampleRate) > m_NewAudioParameters.FrameSize) 
		//{
		//
		//	if (swr_convert(swrContext, audioFrame->data,
		//		audioFrame->nb_samples, NULL, 0) < 0) {
		//		// handle error
		//	}
		//	// do stuff with your audioFrame
		//}
		size_t FlushedSamples = 0;
		while (true)
		{
			AVFrame* NewFrame = h_GetFFMPEGFrame(m_NewAudioParameters, m_NewFrameSize);
			FlushedSamples = swr_convert(ConversionContext, NewFrame->data, NewFrame->nb_samples, NULL, 0);
			if (FlushedSamples < 0)
			{
				_FreeFrame(NewFrame);
				std::cout << "Problem flushing Audio conversion: "; 
				FFMPEGCall(FlushedSamples);
				break;
			}
			else if (FlushedSamples == 0)
			{
				_FreeFrame(NewFrame);
			}
			else
			{
				NewFrame->nb_samples = FlushedSamples;
				m_StoredFrames.push(std::move(StreamFrame(NewFrame, m_InputTimebase, MediaType::Audio)));	
			}
			if (FlushedSamples < m_NewFrameSize || FlushedSamples == 0)
			{
				break;
			}
		}
	}
	void AudioConverter::InsertFrame(StreamFrame const& FrameToInsert)
	{
		AVAudioFifo* AudioBuffer = (AVAudioFifo*) m_AudioDataBuffer.get();
		AVFrame* InputFrame = (AVFrame*)FrameToInsert.m_InternalData.get();
		if (m_FirstTimestampSet == false)
		{
			m_CurrentTimestamp = InputFrame->pts;
			m_FirstTimestampSet = true;
		}
		av_audio_fifo_write(AudioBuffer, (void**)InputFrame->data, InputFrame->nb_samples);
		while (av_audio_fifo_size(AudioBuffer) >= m_NewFrameSize)
		{
			p_ConvertNewFrame();
		}
	}
	StreamFrame AudioConverter::GetNextFrame()
	{
		StreamFrame ReturnValue;
		if (m_StoredFrames.size() > 0)
		{
			ReturnValue = std::move(m_StoredFrames.front());
			m_StoredFrames.pop();
		}
		return(ReturnValue);
	}
	void AudioConverter::Flush()
	{
		p_ConvertNewFrame();
		p_FlushBufferedFrames();
	}

	//END AudioConverter

	//BEGIN AudioToFrameConverter
	AudioToFrameConverter::AudioToFrameConverter(AudioParameters const& InputParameters, int64_t InitialTimestamp, TimeBase OutputTimebase, size_t FrameSize)
	{
		m_FrameParameters = InputParameters;
		m_OutputTimebase = OutputTimebase;
		m_CurrentTimeStamp = InitialTimestamp;
		m_FrameSize = FrameSize;
		AVAudioFifo* AudioBuffer = av_audio_fifo_alloc(h_MBSampleFormatToFFMPEGSampleFormat(InputParameters.AudioFormat), InputParameters.NumberOfChannels, FrameSize * 2);
		m_AudioFifoBuffer = std::unique_ptr<void, void(*)(void*)>(AudioBuffer, _FreeAudioFifo);
	}
	void AudioToFrameConverter::InsertAudioData(const uint8_t* const* AudioData, size_t NumberOfSamples)
	{
		//Borde inte hända men men
		if (m_AudioFifoBuffer == nullptr || m_Flushed)
		{
			throw std::exception();
		}
		AVAudioFifo* AudioBuffer = (AVAudioFifo*)m_AudioFifoBuffer.get();
		//lite bruh att det inte är const pointer?
		av_audio_fifo_write(AudioBuffer,(void**) AudioData, NumberOfSamples);
		while (av_audio_fifo_size(AudioBuffer) > NumberOfSamples)
		{
			p_ConvertStoredSamples();
		}
	}
	void AudioToFrameConverter::p_ConvertStoredSamples()
	{
		//Borde inte hända men men
		if (m_AudioFifoBuffer == nullptr)
		{
			throw std::exception();
		}
		AVAudioFifo* AudioBuffer = (AVAudioFifo*)m_AudioFifoBuffer.get();
		size_t SamplesToExtract = std::min((size_t)av_audio_fifo_size(AudioBuffer), m_FrameSize);
		AVFrame* NewFrame = h_GetFFMPEGFrame(m_FrameParameters, SamplesToExtract);
		FFMPEGCall(av_audio_fifo_read(AudioBuffer, (void**)NewFrame->data, NewFrame->nb_samples));
		NewFrame->pts = m_CurrentTimeStamp;
		NewFrame->pkt_dts = m_CurrentTimeStamp;
		NewFrame->pkt_pts = m_CurrentTimeStamp;
		int64_t  TimestampIncrease = (SamplesToExtract * m_OutputTimebase.den) / (m_OutputTimebase.num * m_FrameParameters.SampleRate);
		m_CurrentTimeStamp += TimestampIncrease;
		m_StoredFrames.push_back(StreamFrame(NewFrame, m_OutputTimebase, MediaType::Audio));
	}
	void AudioToFrameConverter::Flush()
	{
		if (m_AudioFifoBuffer == nullptr)
		{
			throw std::exception();
		}
		AVAudioFifo* AudioBuffer = (AVAudioFifo*)m_AudioFifoBuffer.get();
		//gör inget mer än att man kan extrahera sista framen även om den inte är stor nog
		while (av_audio_fifo_size(AudioBuffer) > 0)
		{
			p_ConvertStoredSamples();
		}
		m_Flushed = true;
	}
	StreamFrame AudioToFrameConverter::GetNextFrame()
	{
		StreamFrame ReturnValue;
		if (m_StoredFrames.size() > 0)
		{
			ReturnValue = std::move(m_StoredFrames.front());
			m_StoredFrames.pop_front();
		}
		return(ReturnValue);
	}
	//END AudioToFrameConverter


	//BEGIN VideoConverter
	void _FreeSwsContext(void* ContextToFree)
	{
		SwsContext* FFMPEGContext = (SwsContext*)ContextToFree;
		sws_freeContext(FFMPEGContext);
	}
	void swap(VideoConverter& LeftConverter, VideoConverter& RightConverter)
	{
		std::swap(LeftConverter.m_OldVideoParameters, RightConverter.m_OldVideoParameters);
		std::swap(LeftConverter.m_NewVideoParameters, RightConverter.m_NewVideoParameters);
		std::swap(LeftConverter.m_InputTimebase, RightConverter.m_InputTimebase);
		std::swap(LeftConverter.m_ConversionContext, RightConverter.m_ConversionContext);
	}
	VideoConverter::VideoConverter(TimeBase InputTimebase, VideoParameters const& OldParameters, VideoParameters const& NewParameters)
	{
		m_OldVideoParameters = OldParameters;
		m_NewVideoParameters = NewParameters;
		m_InputTimebase = InputTimebase;
		//Swsc
		SwsContext* ConversionContext = sws_getContext(OldParameters.Width, OldParameters.Height,h_MBVideoFormatToFFMPEGVideoFormat(OldParameters.Format),
			NewParameters.Width,NewParameters.Height, h_MBVideoFormatToFFMPEGVideoFormat(NewParameters.Format), SWS_BILINEAR,NULL,NULL,NULL);
		m_ConversionContext = std::unique_ptr<void, void (*)(void*)>(ConversionContext, _FreeSwsContext);
	}
	AVFrame* h_GetFFMPEGFrame(int Width, int Height, VideoFormat FormatToUse)
	{
		AVFrame* NewFrame = av_frame_alloc();
		NewFrame->width = Width;
		NewFrame->height = Height;
		NewFrame->format = h_MBVideoFormatToFFMPEGVideoFormat(FormatToUse);
		int numBytes = avpicture_get_size(AVPixelFormat(NewFrame->format), NewFrame->width, NewFrame->height);
		//assert(numBytes);
		uint8_t* dataBuffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
		//NewFrame->data[0] = dataBuffer;
		//av_image_filla
		//av_image_fi
		avpicture_fill((AVPicture*)NewFrame, dataBuffer, AVPixelFormat(NewFrame->format), NewFrame->width, NewFrame->height);
		//av_image_fill_arrays(NewFrame->data, NewFrame->linesize, dataBuffer,h_MBVideoFormatToFFMPEGVideoFormat(FormatToUse), Width, Height, 0);
		//av_free(dataBuffer);
		//FFMPEGCall(av_image_alloc(NewFrame->data,NewFrame->linesize,NewFrame->width,NewFrame->height,h_MBVideoFormatToFFMPEGVideoFormat(FormatToUse),0));
		//_FreeFrame(NewFrame);
		//av_free(dataBuffer);
		//int hej = dataBuffer[0];
		//av_free(dataBuffer);
		return(NewFrame);
	}
	void VideoConverter::InsertFrame(StreamFrame const& FrameToInsert)
	{
		AVFrame* InputFrame = (AVFrame*)FrameToInsert.m_InternalData.get();
		SwsContext* ConversionContext = (SwsContext*)m_ConversionContext.get();

		//Kod snodd från https://lists.ffmpeg.org/pipermail/libav-user/2015-September/008473.html
		AVFrame* NewFrame = h_GetFFMPEGFrame(m_NewVideoParameters.Width, m_NewVideoParameters.Height, m_NewVideoParameters.Format);
		//NewFrame->data
		int Result = FFMPEGCall(sws_scale(ConversionContext, (const uint8_t* const*)InputFrame->data, InputFrame->linesize, 0, InputFrame->height, NewFrame->data, NewFrame->linesize));
		if (Result < 0)
		{
			throw std::exception(); //leakar, mest gjord för debugging
		}
		NewFrame->pts = InputFrame->pts;
		NewFrame->pkt_dts = InputFrame->pkt_dts;
		NewFrame->pkt_duration = InputFrame->pkt_duration;
		NewFrame->pkt_pts = InputFrame->pkt_pts;
		m_StoredFrames.push(StreamFrame(NewFrame, FrameToInsert.GetTimeBase(), MediaType::Video));
	}
	StreamFrame VideoConverter::GetNextFrame()
	{
		StreamFrame ReturnValue;
		if (m_StoredFrames.size() > 0)
		{
			ReturnValue = std::move(m_StoredFrames.front());
			m_StoredFrames.pop();
		}
		return(ReturnValue);
	}
	void VideoConverter::Flush()
	{
		//Do nothing, conversion can be done completely frame by frame basis
	}
	//END VideoConverter
	//StreamFrame FlipPictureHorizontally(StreamFrame const& ImageToFlip)
	//{
	//	StreamFrame ReturnValue;
	//	const AVFrame* InputFrame = (const AVFrame*)ImageToFlip.m_InternalData.get();
	//	if (InputFrame == nullptr || ImageToFlip.GetMediaType() != MediaType::Video)
	//	{
	//		throw std::exception();
	//	}
	//	VideoParameters ImageParameters = ImageToFlip.GetVideoParameters();
	//	AVFrame* NewFrame = h_GetFFMPEGFrame(ImageParameters.Width, ImageParameters.Height, ImageParameters.Format);
	//	//SwsContext* ConversionContext = sws_getContext(ImageParameters.Width, ImageParameters.Height, h_MBVideoFormatToFFMPEGVideoFormat(ImageParameters.Format),
	//	//	ImageParameters.Width, ImageParameters.Height, h_MBVideoFormatToFFMPEGVideoFormat(ImageParameters.Format), SWS_BILINEAR, NULL, NULL, NULL);
	//	//int Result = FFMPEGCall(sws_scale(ConversionContext, (const uint8_t* const*)InputFrame->data, InputFrame->linesize, 0, InputFrame->height, NewFrame->data, NewFrame->linesize));
	//	int Result = 0;
	//	size_t Offset = 0;
	//	while (InputFrame->data[Offset] != NULL)
	//	{
	//		for (size_t i = 0; i < InputFrame->height; i++)
	//		{
	//			memcpy(NewFrame->data[Offset] + (((InputFrame->height-1-i) * InputFrame->linesize[Offset]))
	//				,InputFrame->data[Offset] + (i * InputFrame->linesize[Offset]) ,InputFrame->linesize[Offset]);
	//		}
	//		//Offset += 1;
	//		break;
	//	}
	//	
	//	if (Result >= 0)
	//	{
	//		NewFrame->pts = InputFrame->pts;
	//		NewFrame->pkt_dts = InputFrame->pkt_dts;
	//		NewFrame->pkt_duration = InputFrame->pkt_duration;
	//		NewFrame->pkt_pts = InputFrame->pkt_pts;
	//		ReturnValue = StreamFrame(NewFrame, ImageToFlip.GetTimeBase(), MediaType::Video);
	//	}
	//	else
	//	{
	//		_FreeFrame(NewFrame);
	//	}
	//	return(ReturnValue);
	//}
	StreamFrame FlipRGBPictureHorizontally(StreamFrame const& ImageToFlip)
	{
		StreamFrame ReturnValue;
		//const AVFrame* InputFrame = (const AVFrame*)ImageToFlip.m_InternalData.get();
		if (ImageToFlip.GetMediaType() != MediaType::Video)
		{
			throw std::exception();
		}
		VideoParameters ImageParameters = ImageToFlip.GetVideoParameters();
		AVFrame* NewFrame = h_GetFFMPEGFrame(ImageParameters.Width, ImageParameters.Height, ImageParameters.Format);
		const AVFrame* InputFrame = (const AVFrame*)ImageToFlip.m_InternalData.get();
		//SwsContext* ConversionContext = sws_getContext(ImageParameters.Width, ImageParameters.Height, h_MBVideoFormatToFFMPEGVideoFormat(ImageParameters.Format),
		//	ImageParameters.Width, ImageParameters.Height, h_MBVideoFormatToFFMPEGVideoFormat(ImageParameters.Format), SWS_BILINEAR, NULL, NULL, NULL);
		//int Result = FFMPEGCall(sws_scale(ConversionContext, (const uint8_t* const*)InputFrame->data, InputFrame->linesize, 0, InputFrame->height, NewFrame->data, NewFrame->linesize));
		int Result = 0;
		for (size_t i = 0; i < InputFrame->height; i++)
		{
			memcpy(NewFrame->data[0] + (((InputFrame->height - 1 - i) * InputFrame->linesize[0]))
				, InputFrame->data[0] + (i * InputFrame->linesize[0]), InputFrame->linesize[0]);
		}
		NewFrame->pts = InputFrame->pts;
		NewFrame->pkt_dts = InputFrame->pkt_dts;
		NewFrame->pkt_duration = InputFrame->pkt_duration;
		NewFrame->pkt_pts = InputFrame->pkt_pts;
		return(StreamFrame(NewFrame,ImageToFlip.GetTimeBase(),MediaType::Video));
	}
	//BEGIN FrameConverter
	void swap(FrameConverter& LeftConverter, FrameConverter& RightConverter)
	{
		std::swap(LeftConverter.m_AudioConverter, RightConverter.m_AudioConverter);
		std::swap(LeftConverter.m_VideoConverter, RightConverter.m_VideoConverter);
		std::swap(LeftConverter.m_Flushed, RightConverter.m_Flushed);
		std::swap(LeftConverter.m_Type, RightConverter.m_Type);
	}
	FrameConverter::FrameConverter(FrameConverter&& FrameConverterToSteal) noexcept
	{
		swap(*this,FrameConverterToSteal);
	}
	FrameConverter& FrameConverter::operator=(FrameConverter&& FrameConverterToSteal) noexcept
	{
		swap(*this, FrameConverterToSteal);
		return(*this);
	}
	bool FrameConverter::IsInitialised()
	{
		return(m_AudioConverter != nullptr || m_VideoConverter != nullptr);
	}
	void FrameConverter::Flush()
	{
		m_Flushed = true;
		if (m_Type == MediaType::Audio)
		{
			m_AudioConverter->Flush();
		}
		else
		{
			m_VideoConverter->Flush();
		}
	}
	void FrameConverter::InsertFrame(StreamFrame const& FrameToInsert)
	{
		if (m_Type == MediaType::Audio)
		{
			m_AudioConverter->InsertFrame(FrameToInsert);
		}
		else
		{
			m_VideoConverter->InsertFrame(FrameToInsert);
		}
	}
	StreamFrame FrameConverter::GetNextFrame()
	{
		if (m_Type == MediaType::Audio)
		{
			return(m_AudioConverter->GetNextFrame());
		}
		else
		{
			return(m_VideoConverter->GetNextFrame());
		}
	}
	FrameConverter::FrameConverter(TimeBase InputTimebase,AudioParameters const& OldParameters, AudioParameters const& NewParameters,size_t NewFrameSize)
	{
		m_Type = MediaType::Audio;
		m_AudioConverter = std::unique_ptr<AudioConverter>(new AudioConverter(InputTimebase, OldParameters, NewParameters,NewFrameSize));
	}
	FrameConverter::FrameConverter(TimeBase InputTimebase, VideoParameters const& OldParameters, VideoParameters const& NewParameters)
	{
		m_Type = MediaType::Video;
		m_VideoConverter = std::unique_ptr<VideoConverter>(new VideoConverter(InputTimebase, OldParameters, NewParameters));
	}
	//StreamFrame FrameConverter::ConvertFrame(const StreamFrame* FrameToConvert)
	//{
		//TODO implement proper error handling in FrameConverter
		//StreamFrame ReturnValue;
		//if (m_Type == MediaType::Audio)
		//{
		//	if (FrameToConvert != nullptr && FrameToConvert->GetMediaType() != MediaType::Audio)
		//	{
		//		throw std::exception();
		//	}
		//	AVFrame* ConvertedFrame = av_frame_alloc();
		//	if (ConvertedFrame == NULL)
		//	{
		//		throw std::exception();
		//	}
		//	ConvertedFrame->format = h_MBSampleFormatToFFMPEGSampleFormat(m_NewAudioParameters.AudioFormat);
		//	ConvertedFrame->channel_layout = m_NewAudioParameters.m_ChannelLayout;
		//	ConvertedFrame->pict_type = AV_PICTURE_TYPE_NONE;
		//	//TODO Frame size equal to zero means that it supports variable frames, but nb samples shouldn.t be zero
		//	assert(m_NewAudioParameters.FrameSize > 0);
		//	ConvertedFrame->nb_samples = m_NewAudioParameters.FrameSize;
		//
		//
		//	if (FFMPEGCall(av_frame_get_buffer(ConvertedFrame, 0)) < 0) 
		//	{
		//		throw std::exception();
		//	}
		//	AVFrame* InputFrame = nullptr;
		//
		//
		//	SwrContext* ConversionContext = (SwrContext*)m_InternalData.get();
		//	int ConversionResult = 0;
		//	if (FrameToConvert != nullptr)
		//	{
		//		InputFrame = ( AVFrame *)FrameToConvert->m_InternalData.get();
		//		if (m_FirstTimestampSet == false)
		//		{
		//			DEBUG_LastTimestamp = InputFrame->pts;
		//			m_FirstTimestampSet = true;
		//			m_CurrentTimestamp = InputFrame->pts;
		//		}
		//		else
		//		{
		//			assert(DEBUG_LastTimestamp < InputFrame->pts);
		//			DEBUG_LastTimestamp = InputFrame->pts;
		//			int64_t TimestampIncrease = ((m_InputTimebase.den/m_InputTimebase.num)*InputFrame->nb_samples)/InputFrame->sample_rate;
		//			m_CurrentTimestamp += TimestampIncrease;
		//		}
		//		ConversionResult = swr_convert(ConversionContext, ConvertedFrame->data, ConvertedFrame->nb_samples, (uint8_t const**)InputFrame->data,InputFrame->nb_samples);
		//	}
		//	else
		//	{
		//		//mainly here for debugging, to be removed
		//		m_Flushed = true;
		//		ConversionResult = swr_convert(ConversionContext, ConvertedFrame->data, ConvertedFrame->nb_samples, NULL, 0);
		//	}
		//
		//	
		//	if (ConversionResult > 0)
		//	{
		//		ConvertedFrame->pts = m_CurrentTimestamp;
		//		ConvertedFrame->pkt_dts = m_CurrentTimestamp;
		//		ConvertedFrame->pkt_pts = m_CurrentTimestamp;
		//		ReturnValue = StreamFrame(ConvertedFrame, FrameToConvert->GetTimeBase(), m_Type);
		//	}
		//	else
		//	{
		//		std::cout << "Converting Audioframe: ";
		//		h_Print_ffmpeg_Error(ConversionResult);
		//		_FreeFrame(ConvertedFrame);
		//	}
		//}
		//else
		//{
		//	throw std::exception();
		//}
		//return(ReturnValue);
	//}
	//END FrameConverter

	//BEGIN StreamEncoder
	TimeBase StreamEncoder::GetTimebase() const
	{
		TimeBase ReturnValue;
		if (m_InternalData.get() == nullptr)
		{
			return(ReturnValue);
		}
		const AVCodecContext* CodecContext = (const AVCodecContext*)m_InternalData.get();
		ReturnValue = { CodecContext->time_base.num,CodecContext->time_base.den };
		return(ReturnValue);
	}
	//void _FreeCodecContext(void* CodecToFree)
	//{
	//	avcodec_free_context((AVCodecContext**)&CodecToFree);
	//}
	//VideoEncodeInfo GetVideoEncodePresets(StreamDecoder const& StreamToCopy)
	//{
	//	VideoEncodeInfo ReturnValue;
	//	ReturnValue.bit_rate		= 2 * 1000 * 1000;
	//	ReturnValue.rc_buffer_size	= 4 * 1000 * 10000;
	//	ReturnValue.rc_max_rate		= 2 * 1000 * 10000;
	//	ReturnValue.rc_min_rate		= 2.5 * 1000 * 100;
	//	//
	//	AVCodecContext* CodecContextToCopy = (AVCodecContext*)StreamToCopy.m_InternalData.get();
	//	ReturnValue.height = CodecContextToCopy->height;
	//	ReturnValue.width = CodecContextToCopy->width;
	//	ReturnValue.time_base = StreamToCopy.GetCodecTimebase();
	//	return(ReturnValue);
	//}
	StreamEncoder::StreamEncoder(Codec StreamType, VideoDecodeInfo const& EncodeInfo)
	{
		AVCodecID CodecToUse = ConnectedTypes[(size_t)StreamType].AssoicatedCodecId;
		AVCodec* FFMpegCodec = avcodec_find_encoder(CodecToUse);
		AVCodecContext* VideoEncodeContext = avcodec_alloc_context3(FFMpegCodec);

		VideoEncodeContext->height = EncodeInfo.VideoInfo.Height;
		VideoEncodeContext->width = EncodeInfo.VideoInfo.Width;
		VideoEncodeContext->bit_rate = EncodeInfo.AverageBitrate;
		if (VideoEncodeContext->bit_rate == 0)
		{
			VideoEncodeContext->bit_rate = 2 * 1000 * 1000;
		}
		VideoEncodeContext->rc_buffer_size = 4 * 1000 * 10000;
		VideoEncodeContext->rc_max_rate = 2 * 1000 * 10000;
		VideoEncodeContext->rc_min_rate = 2.5 * 1000 * 100;
		VideoEncodeContext->time_base = { EncodeInfo.StreamTimebase.num,EncodeInfo.StreamTimebase.den };
		size_t Offset = 0;
		AVPixelFormat FormatToUse = FFMpegCodec->pix_fmts[Offset];
		//while (FFMpegCodec->pix_fmts[Offset] != -1)
		//{
		//	FormatToUse = FFMpegCodec->pix_fmts[Offset];
		//	Offset += 1;
		//}
		VideoEncodeContext->pix_fmt = FormatToUse;

		FFMPEGCall(avcodec_open2(VideoEncodeContext, FFMpegCodec, NULL));
		m_InternalData = std::unique_ptr<void, void (*)(void*)>(VideoEncodeContext, _FreeCodecContext);
		m_Type = MediaType::Video;
	}
	StreamEncoder::StreamEncoder(Codec StreamType, AudioDecodeInfo const& EncodeInfo)
	{
		AVCodecID CodecToUse = ConnectedTypes[(size_t)StreamType].AssoicatedCodecId;
		AVCodec* FFMpegCodec = avcodec_find_encoder(CodecToUse);
		AVCodecContext* AudioEncodeContext = avcodec_alloc_context3(FFMpegCodec);
		AudioEncodeContext->bit_rate = EncodeInfo.AverageBitrate;
		if (AudioEncodeContext->bit_rate == 0)
		{
			AudioEncodeContext->bit_rate = 2 * 1000 * 1000;
		}
		AudioEncodeContext->rc_buffer_size = 4 * 1000 * 10000;
		AudioEncodeContext->rc_max_rate = 2 * 1000 * 10000;
		AudioEncodeContext->rc_min_rate = 2.5 * 1000 * 100;
		//
		AudioEncodeContext->time_base = { EncodeInfo.StreamTimebase.num,EncodeInfo.StreamTimebase.den };
		//AudioEncodeContext->sample_fmt		=(AVSampleFormat) EncodeInfo.m_SampleFormat;
		AudioEncodeContext->channels = EncodeInfo.AudioInfo.NumberOfChannels;
		AudioEncodeContext->channel_layout = h_MBLayoutToFFMPEGLayout(EncodeInfo.AudioInfo.Layout);
		//sample rate vad det nu betyder wtf
		AudioEncodeContext->sample_rate = EncodeInfo.AudioInfo.SampleRate;

		AudioEncodeContext->sample_fmt = FFMpegCodec->sample_fmts[0];

		avcodec_open2(AudioEncodeContext, FFMpegCodec, NULL);
		m_InternalData = std::unique_ptr<void, void (*)(void*)>(AudioEncodeContext, _FreeCodecContext);
		m_Type = MediaType::Audio;
	}
	StreamEncoder::StreamEncoder(Codec StreamType, VideoEncodeInfo const& EncodeInfo)
	{
		AVCodecID CodecToUse = ConnectedTypes[(size_t)StreamType].AssoicatedCodecId;
		AVCodec* FFMpegCodec = avcodec_find_encoder(CodecToUse);
		AVCodecContext* VideoEncodeContext = avcodec_alloc_context3(FFMpegCodec);

		VideoEncodeContext->height			= EncodeInfo.VideoInfo.Height;
		VideoEncodeContext->width			= EncodeInfo.VideoInfo.Width;
		VideoEncodeContext->bit_rate		= EncodeInfo.TargetBitrate;
		VideoEncodeContext->rc_buffer_size	= 4 * 1000 * 10000;
		VideoEncodeContext->rc_max_rate		= 2 * 1000 * 10000;
		VideoEncodeContext->rc_min_rate		= 2.5 * 1000 * 100;
		VideoEncodeContext->time_base		= { EncodeInfo.StreamTimebase.num,EncodeInfo.StreamTimebase.den };
		size_t Offset = 0;
		AVPixelFormat FormatToUse = FFMpegCodec->pix_fmts[Offset];
		//while (FFMpegCodec->pix_fmts[Offset] != -1)
		//{
		//	FormatToUse = FFMpegCodec->pix_fmts[Offset];
		//	Offset += 1;
		//}
		VideoEncodeContext->pix_fmt = FormatToUse;

		FFMPEGCall(avcodec_open2(VideoEncodeContext, FFMpegCodec, NULL));
		m_InternalData = std::unique_ptr<void, void (*)(void*)>(VideoEncodeContext, _FreeCodecContext);
		m_Type = MediaType::Video;
	}
	//AudioEncodeInfo GetAudioEncodePresets(StreamDecoder const& StreamToCopy)
	//{
	//	AudioEncodeInfo ReturnValue;
	//	ReturnValue.bit_rate		=  2 * 1000 * 10000;
	//	ReturnValue.rc_buffer_size	=  4 * 1000 * 10000;
	//	ReturnValue.rc_max_rate		=  2 * 10000 * 100000;
	//	ReturnValue.rc_min_rate		=  2.5 * 100 * 100;
	//	//
	//	AVCodecContext* CodecContextToCopy = (AVCodecContext*)StreamToCopy.m_InternalData.get();
	//	ReturnValue.time_base = StreamToCopy.GetCodecTimebase();
	//	ReturnValue.m_channels = CodecContextToCopy->channels;
	//	ReturnValue.m_channels_layout = CodecContextToCopy->channel_layout;
	//	ReturnValue.sample_rate = CodecContextToCopy->sample_rate;
	//	//
	//	return(ReturnValue);
	//	//avcodec_parameters_from_context
	//}
	StreamEncoder::StreamEncoder(Codec StreamType, AudioEncodeInfo const& EncodeInfo)
	{
		AVCodecID CodecToUse = ConnectedTypes[(size_t)StreamType].AssoicatedCodecId;
		AVCodec* FFMpegCodec = avcodec_find_encoder(CodecToUse);
		AVCodecContext* AudioEncodeContext = avcodec_alloc_context3(FFMpegCodec);
		AudioEncodeContext->bit_rate		= EncodeInfo.TargetBitrate;
		AudioEncodeContext->rc_buffer_size	= 4 * 1000 * 10000;
		AudioEncodeContext->rc_max_rate		= 2 * 1000 * 10000;
		AudioEncodeContext->rc_min_rate		= 2.5 * 1000 * 100;
		//
		AudioEncodeContext->time_base		= { EncodeInfo.StreamTimebase.num,EncodeInfo.StreamTimebase.den };
		//AudioEncodeContext->sample_fmt		=(AVSampleFormat) EncodeInfo.m_SampleFormat;
		AudioEncodeContext->channels		= EncodeInfo.AudioInfo.NumberOfChannels;
		AudioEncodeContext->channel_layout	= h_MBLayoutToFFMPEGLayout(EncodeInfo.AudioInfo.Layout);
		//sample rate vad det nu betyder wtf
		AudioEncodeContext->sample_rate		= EncodeInfo.AudioInfo.SampleRate;

		AudioEncodeContext->sample_fmt = FFMpegCodec->sample_fmts[0];

		avcodec_open2(AudioEncodeContext, FFMpegCodec, NULL);
		m_InternalData = std::unique_ptr<void, void (*)(void*)>(AudioEncodeContext, _FreeCodecContext);
		m_Type = MediaType::Audio;
	}
	AudioEncodeInfo StreamEncoder::GetAudioEncodeInfo() const
	{
		AudioEncodeInfo ReturnValue;
		const AVCodecContext* CodecContext = (const AVCodecContext*)m_InternalData.get();
		ReturnValue.AudioInfo.AudioFormat = h_FFMPEGAudioFormatToMBFormat(CodecContext->sample_fmt);
		ReturnValue.AudioInfo.SampleRate = CodecContext->sample_rate;
		ReturnValue.AudioInfo.NumberOfChannels = CodecContext->channels;
		ReturnValue.FrameSize = CodecContext->frame_size;
		ReturnValue.AudioInfo.Layout = h_FFMPEGLayoutToMBLayout(CodecContext->channel_layout);
		ReturnValue.TargetBitrate = CodecContext->bit_rate;
		ReturnValue.StreamTimebase = { CodecContext->time_base.num,CodecContext->time_base.den };
		return(ReturnValue);
	}
	VideoEncodeInfo StreamEncoder::GetVideoEncodeInfo() const
	{
		VideoEncodeInfo ReturnValue;
		const AVCodecContext* CodecContext = (const AVCodecContext*)m_InternalData.get();
		ReturnValue.VideoInfo.Format = h_FFMPEGVideoFormatToMBVideoFormat(CodecContext->pix_fmt);
		ReturnValue.VideoInfo.Width = CodecContext->width;
		ReturnValue.VideoInfo.Height = CodecContext->height;
		ReturnValue.TargetBitrate = CodecContext->bit_rate;
		ReturnValue.StreamTimebase = { CodecContext->time_base.num,CodecContext->time_base.den };
		return(ReturnValue);
	}
	void StreamEncoder::InsertFrame(StreamFrame const& FrameToEncode)
	{
		AVCodecContext* CodecContext = (AVCodecContext*)m_InternalData.get();
		m_InputTimeBase = FrameToEncode.GetTimeBase();
		const AVFrame* FrameToSend = (const AVFrame*)FrameToEncode.m_InternalData.get();
		if (m_Type == MediaType::Video)
		{
			int hej = 2;
		}
		//DEBUG
		//FrameToSend->pict_type = AV_PICTURE_TYPE_NONE;
		//
		avcodec_send_frame(CodecContext,FrameToSend);
	}
	void StreamEncoder::Flush()
	{
		AVCodecContext* CodecContext = (AVCodecContext*)m_InternalData.get();
		FFMPEGCall(avcodec_send_frame(CodecContext, nullptr));
	}
	StreamPacket StreamEncoder::GetNextPacket()
	{
		AVCodecContext* CodecContext = (AVCodecContext*)m_InternalData.get();
		AVPacket* NewPacket = av_packet_alloc();
		MediaType PacketType = m_Type;
		int FFmpegResult = avcodec_receive_packet(CodecContext,NewPacket);
		if (FFmpegResult < 0)
		{
			av_packet_free(&NewPacket);
		}
		else
		{
			
		}
		return(StreamPacket(NewPacket,m_InputTimeBase, PacketType));
	}

	//END StreamEncoder
	void Transcode(std::string const& InputFile, std::string const& OutputFile, Codec NewAudioCodec, Codec NewVideoCodec)
	{
		//ContainerDemuxer InputData(InputFile);
		ContainerDemuxer InputData(std::unique_ptr<MBUtility::MBFileInputStream>(new MBUtility::MBFileInputStream(InputFile)));
		//ContainerDemuxer InputData(InputFile);
		std::vector<StreamDecoder> Decoders = {};
		OutputContext OutputData(OutputFile);
		for (size_t i = 0; i < InputData.NumberOfStreams(); i++)
		{
			if (InputData.GetStreamInfo(i).GetMediaType() == MediaType::Audio)
			{
				Decoders.push_back(StreamDecoder(InputData.GetStreamInfo(i)));
				StreamEncoder NewStreamEncoder = StreamEncoder(NewAudioCodec, Decoders.back().GetAudioDecodeInfo());
				Decoders.back().SetAudioConversionParameters(NewStreamEncoder.GetAudioEncodeInfo().AudioInfo, NewStreamEncoder.GetAudioEncodeInfo().FrameSize);
				OutputData.AddOutputStream(std::move(NewStreamEncoder));
			}
			if (InputData.GetStreamInfo(i).GetMediaType() == MediaType::Video)
			{
				Decoders.push_back(StreamDecoder(InputData.GetStreamInfo(i)));
				StreamEncoder NewStreamEncoder = StreamEncoder(NewVideoCodec, Decoders.back().GetVideoDecodeInfo());
				Decoders.back().SetVideoConversionParameters(NewStreamEncoder.GetVideoEncodeInfo().VideoInfo);
				OutputData.AddOutputStream(std::move(NewStreamEncoder));
			}
		}
		OutputData.WriteHeader();
		while (true)
		{
			size_t PacketIndex = 0;
			StreamPacket NewPacket = InputData.GetNextPacket(&PacketIndex);
			if (NewPacket.GetType() == MediaType::Null)
			{
				break;
			}
			//if (NewPacket.GetType() == MediaType::Audio)
			//{
			//	continue;
			//}
			Decoders[PacketIndex].InsertPacket(NewPacket);
			while (true)
			{
				StreamFrame NewFrame = Decoders[PacketIndex].GetNextFrame();
				if (NewFrame.GetMediaType() == MediaType::Null)
				{
					break;
				}
				//DEBUG
				//if (NewFrame.GetMediaType() == MediaType::Video)
				//{
				//	NewFrame = FlipPictureHorizontally(NewFrame);
				//}
				//DEBUG
				OutputData.InsertFrame(NewFrame, PacketIndex);
			}
		}
		for (size_t i = 0; i < Decoders.size(); i++)
		{
			Decoders[i].Flush();
			while (true)
			{
				StreamFrame NewFrame = Decoders[i].GetNextFrame();
				if (NewFrame.GetMediaType() == MediaType::Null)
				{
					break;
				}
				OutputData.InsertFrame(NewFrame, i);
			}
		}
		OutputData.Finalize();
	}
};
//*/