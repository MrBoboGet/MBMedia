#include "MBMedia2.h"
#ifdef _WIN32
#include <unknwn.h>
#include<strmif.h>
#endif // __WIN32__
#include <vector>
#include "MBMedia.h"
#include <iostream>
#include <filesystem>
#include <assert.h>
extern "C"
{
#include <ffmpeg/libavcodec/avcodec.h>
#include <ffmpeg/libavformat/avformat.h>
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
		int ReadResponse = FFMPEGCall(av_read_frame(InputContext, NewPacket));
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
		std::cout << "Stream time: " << FFMpegPacket->pts * (double(OutputFormat->streams[StreamIndex]->time_base.num) / double(OutputFormat->streams[StreamIndex]->time_base.den)) << std::endl;
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
		av_frame_free(&Frame);
	}
	//StreamFrame
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
		AVCodecContext* CodecContext = (AVCodecContext*)m_InternalData.get();
		const AVPacket* PacketToInsert = (const AVPacket*)PacketToDecode.m_InternalData.get();
		FFMPEGCall(avcodec_send_packet(CodecContext, (const AVPacket*)PacketToDecode.m_InternalData.get()));
	}
	StreamFrame StreamDecoder::GetNextFrame()
	{
		AVCodecContext* CodecContext = (AVCodecContext*)m_InternalData.get();
		AVFrame* NewFrame = av_frame_alloc();
		int RecieveResult = avcodec_receive_frame(CodecContext, NewFrame);
		MediaType FrameType = m_Type;
		if (RecieveResult < 0)
		{
			av_frame_free(&NewFrame);
			FrameType = MediaType::Null;
		}
		return(StreamFrame(NewFrame,m_StreamTimebase,m_Type));
	}
	void StreamDecoder::Flush()
	{
		AVCodecContext* CodecContext = (AVCodecContext*)m_InternalData.get();
		FFMPEGCall(avcodec_send_packet(CodecContext, nullptr));
	}

	//END StreamDecoder

	//BEGIN StreamEncoder
	TimeBase StreamEncoder::GetTimebase()
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
	VideoEncodeInfo GetVideoEncodePresets(StreamDecoder const& StreamToCopy)
	{
		VideoEncodeInfo ReturnValue;
		ReturnValue.bit_rate		= 2 * 1000 * 10000;
		ReturnValue.rc_buffer_size	= 4 * 1000 * 10000;
		ReturnValue.rc_max_rate		= 2 * 1000 * 10000;
		ReturnValue.rc_min_rate		= 2.5 * 1000 * 100;
		//
		AVCodecContext* CodecContextToCopy = (AVCodecContext*)StreamToCopy.m_InternalData.get();
		ReturnValue.height = CodecContextToCopy->height;
		ReturnValue.width = CodecContextToCopy->width;
		ReturnValue.time_base = StreamToCopy.GetCodecTimebase();
		return(ReturnValue);
	}
	StreamEncoder::StreamEncoder(Codec StreamType, VideoEncodeInfo const& EncodeInfo)
	{
		AVCodecID CodecToUse = ConnectedTypes[(size_t)StreamType].AssoicatedCodecId;
		AVCodec* FFMpegCodec = avcodec_find_encoder(CodecToUse);
		AVCodecContext* VideoEncodeContext = avcodec_alloc_context3(FFMpegCodec);

		VideoEncodeContext->height			= EncodeInfo.height;
		VideoEncodeContext->width			= EncodeInfo.width;
		VideoEncodeContext->bit_rate		= EncodeInfo.bit_rate;
		VideoEncodeContext->rc_buffer_size	= EncodeInfo.rc_buffer_size;
		VideoEncodeContext->rc_max_rate		= EncodeInfo.rc_max_rate;
		VideoEncodeContext->rc_min_rate		= EncodeInfo.rc_min_rate;
		VideoEncodeContext->time_base		= { EncodeInfo.time_base.num,EncodeInfo.time_base.den };
		size_t Offset = 0;
		AVPixelFormat FormatToUse = AV_PIX_FMT_NONE;
		while (FFMpegCodec->pix_fmts[Offset] != -1)
		{
			FormatToUse = FFMpegCodec->pix_fmts[Offset];
			Offset += 1;
		}
		VideoEncodeContext->pix_fmt = FormatToUse;

		FFMPEGCall(avcodec_open2(VideoEncodeContext, FFMpegCodec, NULL));
		m_InternalData = std::unique_ptr<void, void (*)(void*)>(VideoEncodeContext, _FreeCodecContext);
		m_Type = MediaType::Video;
	}
	AudioEncodeInfo GetAudioEncodePresets(StreamDecoder const& StreamToCopy)
	{
		AudioEncodeInfo ReturnValue;
		ReturnValue.bit_rate		=  2 * 1000 * 10000;
		ReturnValue.rc_buffer_size	=  4 * 1000 * 10000;
		ReturnValue.rc_max_rate		=  2 * 10000 * 100000;
		ReturnValue.rc_min_rate		=  2.5 * 100 * 100;
		//
		AVCodecContext* CodecContextToCopy = (AVCodecContext*)StreamToCopy.m_InternalData.get();
		ReturnValue.time_base = StreamToCopy.GetCodecTimebase();
		ReturnValue.m_channels = CodecContextToCopy->channels;
		ReturnValue.m_channels_layout = CodecContextToCopy->channel_layout;
		ReturnValue.sample_rate = CodecContextToCopy->sample_rate;
		//
		return(ReturnValue);
		//avcodec_parameters_from_context
	}
	StreamEncoder::StreamEncoder(Codec StreamType, AudioEncodeInfo const& EncodeInfo)
	{
		AVCodecID CodecToUse = ConnectedTypes[(size_t)StreamType].AssoicatedCodecId;
		AVCodec* FFMpegCodec = avcodec_find_encoder(CodecToUse);
		AVCodecContext* AudioEncodeContext = avcodec_alloc_context3(FFMpegCodec);
		AudioEncodeContext->bit_rate		= EncodeInfo.bit_rate;
		AudioEncodeContext->rc_buffer_size	= EncodeInfo.rc_buffer_size;
		AudioEncodeContext->rc_max_rate		= EncodeInfo.rc_max_rate;
		AudioEncodeContext->rc_min_rate		= EncodeInfo.rc_min_rate;
		//
		AudioEncodeContext->time_base		= { EncodeInfo.time_base.num,EncodeInfo.time_base.den };
		//AudioEncodeContext->sample_fmt		=(AVSampleFormat) EncodeInfo.m_SampleFormat;
		AudioEncodeContext->channels		= EncodeInfo.m_channels;
		AudioEncodeContext->channel_layout	= EncodeInfo.m_channels_layout;
		//sample rate vad det nu betyder wtf
		AudioEncodeContext->sample_rate		= EncodeInfo.sample_rate;

		AudioEncodeContext->sample_fmt = FFMpegCodec->sample_fmts[0];

		avcodec_open2(AudioEncodeContext, FFMpegCodec, NULL);
		m_InternalData = std::unique_ptr<void, void (*)(void*)>(AudioEncodeContext, _FreeCodecContext);
		m_Type = MediaType::Audio;
	}
	void StreamEncoder::InsertFrame(StreamFrame const& FrameToEncode)
	{
		AVCodecContext* CodecContext = (AVCodecContext*)m_InternalData.get();
		m_InputTimeBase = FrameToEncode.GetTimeBase();
		avcodec_send_frame(CodecContext,(const AVFrame*) FrameToEncode.m_InternalData.get());
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
		int FFmpegResult = FFMPEGCall(avcodec_receive_packet(CodecContext,NewPacket));
		if (FFmpegResult < 0)
		{
			av_packet_free(&NewPacket);
		}
		return(StreamPacket(NewPacket,m_InputTimeBase, PacketType));
	}

	//END StreamEncoder
	void Transcode(std::string const& InputFile, std::string const& OutputFile, Codec NewAudioCodec, Codec NewVideoCodec)
	{
		ContainerDemuxer InputData(InputFile);
		std::vector<StreamDecoder> Decoders = {};
		OutputContext OutputData(OutputFile);
		for (size_t i = 0; i < InputData.NumberOfStreams(); i++)
		{
			if (InputData.GetStreamInfo(i).GetMediaType() == MediaType::Audio)
			{
				Decoders.push_back(StreamDecoder(InputData.GetStreamInfo(i)));
				OutputData.AddOutputStream(StreamEncoder(NewAudioCodec,GetAudioEncodePresets(Decoders.back())));
			}
			if (InputData.GetStreamInfo(i).GetMediaType() == MediaType::Video)
			{
				Decoders.push_back(StreamDecoder(InputData.GetStreamInfo(i)));
				OutputData.AddOutputStream(StreamEncoder(NewVideoCodec, GetVideoEncodePresets(Decoders.back())));
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
			//if (NewPacket.GetType() == MediaType::Video)
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