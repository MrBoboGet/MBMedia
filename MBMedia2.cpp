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

	//BEGIN StreamInfo
	StreamInfo::StreamInfo(std::shared_ptr<void> FFMPEGContainerData, size_t StreamIndex)
	{
		m_InternalData = FFMPEGContainerData;
		m_StreamIndex = -StreamIndex;
	}

	//END StreamInfo

	void _FreePacket(void* PacketToFree)
	{
		AVPacket* Packet = (AVPacket*)PacketToFree;
		av_packet_free(&Packet);
	}
	//BEGIN StreamPacket
	StreamPacket::StreamPacket(void* FFMPEGPacket,MediaType PacketType)
		//: m_ImplementationData(FFMPEGPacket, _FreePacket)
	{
		if (FFMPEGPacket != nullptr)
		{
			m_Type = PacketType;
			m_InternalData = std::unique_ptr<void, void(*)(void*)>(FFMPEGPacket, _FreePacket);
		}
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
		int ReadResponse = FFMPEGCall(av_read_frame((AVFormatContext*)m_InternalData.get(),NewPacket));
		AVFormatContext* InputContext = (AVFormatContext*)m_InternalData.get();
		if (ReadResponse >= 0)
		{
			return(StreamPacket(NewPacket, h_FFMPEGMediaTypeToMBMediaType(InputContext->streams[NewPacket->stream_index]->codecpar->codec_type)));
		}
		else 
		{
			return(StreamPacket(nullptr, MediaType::Null));
		}
	}
	//END ContainerDemuxer

	//BEGIN OutputContext
	OutputContext::OutputContext(std::string const& OutputFile)
	{

	}

	void OutputContext::AddOutputStream(StreamEncoder&& Encoder)
	{
		//m_OutputEncoders.push_back(Encoder);
	}
	void OutputContext::WriteHeader()
	{
		AVFormatContext* OutputFormatContext = (AVFormatContext*)m_InternalData.get();
		if (OutputFormatContext == nullptr)
		{
			return;
		}
		avformat_write_header(OutputFormatContext, NULL);
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
	StreamFrame::StreamFrame(void* FFMPEGData,MediaType FrameType)
	{
		if (FFMPEGData != nullptr)
		{
			m_InternalData = std::unique_ptr<void, void(*)(void*)>(FFMPEGData, _FreeFrame);
			m_MediaType = FrameType;
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
		//DecodeCodecContext.push_back(NewCodexContext);
	}
	void StreamDecoder::InsertPacket(StreamPacket const& PacketToDecode)
	{
		AVCodecContext* CodecContext = (AVCodecContext*)m_InternalData.get();
		FFMPEGCall(avcodec_send_packet(CodecContext, (const AVPacket*)PacketToDecode.m_InternalData.get()));
	}
	StreamFrame StreamDecoder::GetNextFrame()
	{
		AVCodecContext* CodecContext = (AVCodecContext*)m_InternalData.get();
		AVFrame* NewFrame = av_frame_alloc();
		int RecieveResult = FFMPEGCall(avcodec_receive_frame(CodecContext, NewFrame));
		MediaType FrameType = m_Type;
		if (RecieveResult < 0)
		{
			av_frame_free(&NewFrame);
			FrameType = MediaType::Null;
		}
		return(StreamFrame(NewFrame,m_Type));
	}
	void StreamDecoder::Flush()
	{
		AVCodecContext* CodecContext = (AVCodecContext*)m_InternalData.get();
		FFMPEGCall(avcodec_send_packet(CodecContext, nullptr));
	}

	//END StreamDecoder

	//BEGIN StreamEncoder

	StreamEncoder::StreamEncoder(Codec StreamType, AudioEncodeInfo const& EncodeInfo)
	{

	}
	StreamEncoder::StreamEncoder(Codec StreamType, VideoEncodeInfo const& EncodeInfo)
	{

	}
	void StreamEncoder::InsertFrame(StreamFrame const& FrameToEncode)
	{
		AVCodecContext* CodecContext = (AVCodecContext*)m_InternalData.get();
		FFMPEGCall(avcodec_send_frame(CodecContext,(const AVFrame*) FrameToEncode.m_InternalData.get()));
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
		return(StreamPacket(NewPacket, PacketType));
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
		while (true)
		{
			size_t PacketIndex = 0;
			StreamPacket NewPacket = InputData.GetNextPacket(&PacketIndex);
			if (NewPacket.GetType() == MediaType::Null)
			{
				break;
			}
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