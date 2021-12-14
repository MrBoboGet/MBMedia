
#ifdef _WIN32
#include <unknwn.h>
#include<strmif.h>
#endif // __WIN32__
#include <vector>
#include "MBMedia.h"
#include <iostream>
#include <filesystem>
#include <assert.h>
///*
extern "C"
{
	#include <ffmpeg/libavcodec/avcodec.h>
    #include <ffmpeg/libavformat/avformat.h>
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
		std::cout <<"FFMpeg error: "<< MessageBuffer << std::endl;
	}
    int FFMPEGCall(int ReadFrameResponse)
	{
		h_Print_ffmpeg_Error(ReadFrameResponse);
		return(ReadFrameResponse);
	}
class MBDecodeContext
{
public:
	AVFormatContext* InputFormatContext = nullptr;
	std::vector<AVCodecParameters*> InputCodecParameters = {};
	std::vector<AVCodec*> InputCodecs = {};
	std::vector<AVCodecContext*> DecodeCodecContext = {};
	MBDecodeContext(std::string InputFilepath)
	{
		//transmux exempel
		InputFormatContext = avformat_alloc_context();
		//allokerar format kontexten, information om filtyp och innehåll,läser bara headers och etc
		FFMPEGCall(avformat_open_input(&InputFormatContext, InputFilepath.c_str(), NULL, NULL));
		//läsar in data om själva datastreamsen
		FFMPEGCall(avformat_find_stream_info(InputFormatContext, NULL));
		for (size_t i = 0; i < InputFormatContext->nb_streams; i++)
		{
			AVCodecParameters* NewInputCodecParamters = InputFormatContext->streams[i]->codecpar;
			InputCodecParameters.push_back(NewInputCodecParamters);
			AVCodec* NewInputCodec = avcodec_find_decoder(NewInputCodecParamters->codec_id);
			//TEST
			//NewInputCodec->capabilities
			//TEST
			std::cout << NewInputCodec->name << std::endl;
			InputCodecs.push_back(NewInputCodec);
			//givet en codec och codec parameters så kan vi encoda/decoda data, men eftersom det är statefull kräver vi en encode/decode context
			AVCodecContext* NewCodexContext = avcodec_alloc_context3(NewInputCodec);
			FFMPEGCall(avcodec_parameters_to_context(NewCodexContext, NewInputCodecParamters));
			//sedan måste vi öppna den, vet inte riktigt varför, initializerar den kanske?
			FFMPEGCall(avcodec_open2(NewCodexContext, NewInputCodec, NULL));
			DecodeCodecContext.push_back(NewCodexContext);
		}
		//all data för att decoda insamlad
	}
	void FreeMemory()
	{
		avformat_close_input(&InputFormatContext);
		for (size_t i = 0; i < DecodeCodecContext.size(); i++)
		{
			avcodec_free_context(&DecodeCodecContext[i]);
		}
	}
};
class MBEncodeContext
{
public:
	std::string OutFileName = "";
	AVFormatContext* OutputFormatContext = nullptr;
	MBAudioCodecs TargetAudioCodec = MBAudioCodecs::Null;
	MBVideoCodecs TargetVideoCodec = MBVideoCodecs::Null;
	MBEncodeContext(std::string OutputFilepath)
	{
		avformat_alloc_output_context2(&OutputFormatContext, NULL, NULL, OutputFilepath.c_str());
		//all data för att decoda insamlad
	}
	MBEncodeContext(std::string OutputFilepath, MBAudioCodecs AudioCodecToUse, MBVideoCodecs VideoCodecToUse)
	{
		TargetAudioCodec = AudioCodecToUse;
		TargetVideoCodec = VideoCodecToUse;
		OutFileName = OutputFilepath;
		avformat_alloc_output_context2(&OutputFormatContext, NULL, NULL, OutputFilepath.c_str());
		//all data för att decoda insamlad
	}
	void FreeMemory()
	{

	}
};
AVCodecID MBVideoCodecToFFMpegVideoCodec(MBVideoCodecs CodecToDecode)
{
	if (CodecToDecode == MBVideoCodecs::H265)
	{
		return(AV_CODEC_ID_H265);
	}
	else if (CodecToDecode == MBVideoCodecs::H264)
	{
		return(AV_CODEC_ID_H264);
	}
}
std::string MBAudioCodecToString(MBAudioCodecs CodecToDecode)
{
	if (CodecToDecode == MBAudioCodecs::AAC)
	{
		return("aac");
	}
}


MBError InternalTranscode(MBDecodeContext* DecodeData, MBEncodeContext* EncodeData)
{
	AVCodec* VideoCodec = avcodec_find_encoder(MBVideoCodecToFFMpegVideoCodec(EncodeData->TargetVideoCodec));
	AVCodec* AudioCodec = avcodec_find_encoder_by_name(MBAudioCodecToString(EncodeData->TargetAudioCodec).c_str());
	AVCodecContext* VideoEncodeContext = avcodec_alloc_context3(VideoCodec);
	AVCodecContext* AudioEncodeContext = avcodec_alloc_context3(AudioCodec);
	//letar fram den första video codexen
	int FirstVideoIndex = -1;
	AVCodecParameters* FirstVideoParameters = nullptr;
	for (size_t i = 0; i < DecodeData->InputCodecParameters.size(); i++)
	{
		if (DecodeData->InputCodecParameters[i]->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			FirstVideoIndex = i;
			FirstVideoParameters = DecodeData->InputCodecParameters[i];
			break;
		}
	}
	AVRational input_framerate = av_guess_frame_rate(DecodeData->InputFormatContext, DecodeData->InputFormatContext->streams[FirstVideoIndex], NULL);
	AVCodecContext* FirstVideoContext = DecodeData->DecodeCodecContext[FirstVideoIndex];
	VideoEncodeContext->height = FirstVideoParameters->height;
	VideoEncodeContext->width = FirstVideoParameters->width;
	VideoEncodeContext->pix_fmt = VideoCodec->pix_fmts[0];
	//control rate
	VideoEncodeContext->bit_rate		= 2 * 1000 * 1000;
	VideoEncodeContext->rc_buffer_size	= 4 * 1000 * 1000;
	VideoEncodeContext->rc_max_rate		= 2 * 1000 * 10000;
	VideoEncodeContext->rc_min_rate		= 2.5 * 1000 * 1000;
	//timebase
	//testar för enkelhetens och vetenskapens skull att bara kopiera över alla data
	VideoEncodeContext->time_base = av_inv_q(input_framerate);

	int FirstAudioIndex = -1;
	for (size_t i = 0; i < DecodeData->InputCodecParameters.size(); i++)
	{
		if (DecodeData->InputCodecParameters[i]->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			FirstAudioIndex = i;
			break;
		}
	}
	AVCodecContext* FirstAudioContext = DecodeData->DecodeCodecContext[FirstAudioIndex];
	//AudioEncodeContext->height = FirstAudioContext->height;
	//AudioEncodeContext->width = FirstAudioContext->width;
	//AudioEncodeContext->pix_fmt = AudioCodec->pix_fmts[0];
	//control rate
	AudioEncodeContext->bit_rate		= FirstAudioContext->bit_rate;
	AudioEncodeContext->rc_buffer_size	= FirstAudioContext->rc_buffer_size;
	AudioEncodeContext->rc_max_rate		= FirstAudioContext->rc_max_rate;
	AudioEncodeContext->rc_min_rate		= FirstAudioContext->rc_min_rate;
	//timebase
	//testar för enkelhetens och vetenskapens skull att bara kopiera över alla data
	AudioEncodeContext->time_base = FirstAudioContext->time_base;
	AudioEncodeContext->sample_fmt = AudioCodec->sample_fmts[0];
	AudioEncodeContext->channels = FirstAudioContext->channels;
	AudioEncodeContext->channel_layout = FirstAudioContext->channel_layout;
	//sample rate vad det nu betyder wtf
	AudioEncodeContext->sample_rate = FirstAudioContext->sample_rate;
	//nu har vi två förhoppningsvis fungerande decoders för ljud, då är det bara att faktiskt encoda våran frame
	avcodec_open2(VideoEncodeContext, VideoCodec, NULL);
	avcodec_open2(AudioEncodeContext, AudioCodec, NULL);


	//öppnar filen med nya streams som vi sedan kan skriva med

	for (size_t i = 0; i < DecodeData->InputFormatContext->nb_streams; i++)
	{
		AVStream* InputStream = DecodeData->InputFormatContext->streams[i];
		AVStream* OutputStream = NULL;
		//AVStream* InputStream = InputFormatContext->streams[i];
		OutputStream = avformat_new_stream(EncodeData->OutputFormatContext, NULL);
		//ser till att streamsen har rätt codex data som vi vill byta till
		if (InputStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			FFMPEGCall(avcodec_parameters_from_context(OutputStream->codecpar, VideoEncodeContext));
		}
		else if (InputStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			FFMPEGCall(avcodec_parameters_from_context(OutputStream->codecpar, AudioEncodeContext));
		}
		else
		{
			FFMPEGCall(avcodec_parameters_copy(OutputStream->codecpar, DecodeData->InputCodecParameters[i]));
			//ser till att streamen tar global headers om format contexten gör det
			if (EncodeData->OutputFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
			{
				OutputStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			}
		}
		if (EncodeData->OutputFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
		{
			//OutputStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			EncodeData->OutputFormatContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}
	}
	avio_open(&EncodeData->OutputFormatContext->pb, EncodeData->OutFileName.c_str(), AVIO_FLAG_WRITE);
	FFMPEGCall(avformat_write_header(EncodeData->OutputFormatContext, NULL));
	AVFrame* InputFrame = av_frame_alloc();
	AVPacket* InputPacket = av_packet_alloc();
	AVStream* InStream = NULL;
	AVStream* OutStream = NULL;
	int ReadFrameResponse = 0;
	while (ReadFrameResponse = av_read_frame(DecodeData->InputFormatContext, InputPacket) >= 0)
	{
		int StreamIndex = InputPacket->stream_index;
		//DEBUG
		//if (ReadFrameResponse < 0)
		//{
		//	StreamIndex = 1;
		//	InputPacket = NULL;
		//}
		//else
		//{
		//	StreamIndex = InputPacket->stream_index;
		//}
		//
		AVMediaType PacketMediaType = DecodeData->InputCodecParameters[StreamIndex]->codec_type;
		InStream = DecodeData->InputFormatContext->streams[StreamIndex];
		OutStream = EncodeData->OutputFormatContext->streams[StreamIndex];
		
		if (PacketMediaType == AVMEDIA_TYPE_AUDIO || PacketMediaType == AVMEDIA_TYPE_VIDEO)
		{
			AVPacket* OutputPacket = av_packet_alloc();
			AVCodecContext* DecodeContextToUse = DecodeData->DecodeCodecContext[StreamIndex];
			//TEST
			//av_packet_rescale_ts(InputPacket,DecodeData->InputFormatContext.)
			//
			int SendPacketResponse = FFMPEGCall(avcodec_send_packet(DecodeContextToUse, InputPacket));
			while (SendPacketResponse >= 0)
			{
				SendPacketResponse = avcodec_receive_frame(DecodeContextToUse, InputFrame);
				if (SendPacketResponse == AVERROR(EAGAIN) || SendPacketResponse == AVERROR_EOF)
				{
					//SendPacketResponse = 0;
					if (SendPacketResponse == AVERROR_EOF)
					{
						std::cout << "Hmmm, kanske behöver flusha?" << std::endl;
					}
					break;
				}
				else if (SendPacketResponse < 0)
				{
					return -1;
				}
				//TEST
				int64_t PTSBefore = InputFrame->pts;
				//std::cout << "Inputframe pts: " << InputFrame->pts << std::endl;
				InputFrame->pts = av_frame_get_best_effort_timestamp(InputFrame);
				assert(PTSBefore == InputFrame->pts);
				//std::cout << "pts after get best effort "<<InputFrame->pts << std::endl;
				//TEST


				//avcodec_send_frame(EncodeData->OutputFormatContext->streams[InputPacket->stream_index], InputFrame);
				int SendFrameResponse = -1;
				if (PacketMediaType == AVMEDIA_TYPE_AUDIO)
				{
					//std::cout << InputFrame->pict_type << std::endl;
					SendFrameResponse = FFMPEGCall(avcodec_send_frame(AudioEncodeContext, InputFrame));
				}
				else
				{
					std::cout << InputFrame->pict_type << std::endl;
					//InputFrame->pict_type = AV_PICTURE_TYPE_NONE;
					//InputFrame->key_frame = true;
					//std::cout << InputFrame->pict_type << std::endl;
					SendFrameResponse = FFMPEGCall(avcodec_send_frame(VideoEncodeContext, InputFrame));
				}
				int DEBUG_PacketPerFrame = 0;
				int RecievePacketResponse = 0;
				while(RecievePacketResponse >= 0)
				{
					if (PacketMediaType == AVMEDIA_TYPE_AUDIO)
					{
						RecievePacketResponse = avcodec_receive_packet(AudioEncodeContext, OutputPacket);
					}
					else if(PacketMediaType == AVMEDIA_TYPE_VIDEO)
					{
						RecievePacketResponse = avcodec_receive_packet(VideoEncodeContext, OutputPacket);
					}
					if (RecievePacketResponse == AVERROR(EAGAIN) || RecievePacketResponse == AVERROR_EOF)
					{
						//RecievePacketResponse = 0;
						if (RecievePacketResponse == AVERROR_EOF)
						{
							std::cout << "Hmmm, kanske behöver flusha?" << std::endl;
						}
						break;
					}
					else if (RecievePacketResponse < 0)
					{
						return -1;
					}
					if (PacketMediaType == AVMEDIA_TYPE_VIDEO)
					{
						//std::cout << InputFrame->pict_type << std::endl;
						//std::cout << InputFrame->pict_type << std::endl;
						//InputFrame->pict_type = AV_PICTURE_TYPE_I;
						//std::cout << "InDuration" << InputPacket->duration << std::endl;
						//OutputPacket->duration =((OutStream->time_base.den / OutStream->time_base.num) / (InStream->avg_frame_rate.num * InStream->avg_frame_rate.den));
						//std::cout << "OutDuration" << OutputPacket->duration << std::endl;
						//std::cout << "BildData Som skrivs" << std::endl;
					}
					else
					{
						//std::cout << InputFrame->pict_type << std::endl;
					}
					DEBUG_PacketPerFrame += 1;
					OutputPacket->stream_index = StreamIndex;
					//OutputPacket->duration = av_rescale_q(OutputPacket->duration, InStream->time_base, OutStream->time_base);
					//std::cout << "Input Timestamp: " << InputPacket->pts << std::endl;
					av_packet_rescale_ts(OutputPacket, InStream->time_base, OutStream->time_base);
					if (PacketMediaType == AVMEDIA_TYPE_VIDEO)
					{
						//std::cout << OutputPacket->pts << std::endl;
						//OutputPacket->duration = 20;
						//OutputPacket->pts = InputFrame->pts;
						//if (InputFrame->pict_type != AV_PICTURE_TYPE_I)
						//{
						//	continue;
						//}
						continue;
					}
					//std::cout << "Output Timestamp: " << OutputPacket->pts << std::endl;
					FFMPEGCall(av_interleaved_write_frame(EncodeData->OutputFormatContext, OutputPacket));
				}
				if (DEBUG_PacketPerFrame > 1)
				{
					std::cout << "PacketPerFrame > 1" << std::endl;
				}
			}
			//h_Print_ffmpeg_Error(response);
			av_packet_unref(OutputPacket);
			av_packet_free(&OutputPacket);
		}
		if (ReadFrameResponse < 0)
		{
			break;
		}
	}
	h_Print_ffmpeg_Error(ReadFrameResponse);
    av_frame_unref(InputFrame);
	av_write_trailer(EncodeData->OutputFormatContext);
	return(MBError(false));
}
MBError Transcode(std::string InputFilepath, std::string OutputFilepath, MBVideoCodecs NewVideoCodec, MBAudioCodecs NewAudioCodec)
{
	MBDecodeContext DecodeContextToUse(InputFilepath);
	MBEncodeContext EncodeContextToUse(OutputFilepath, NewAudioCodec, NewVideoCodec);
	MBError ReturnValue = InternalTranscode(&DecodeContextToUse, &EncodeContextToUse);
	return(ReturnValue);
}
MBError Remux(std::string InputFilepath, std::string OutputFilepath)
{
	//transmux exempel
	AVFormatContext* InputFormatContext = avformat_alloc_context();
	//allokerar format kontexten, information om filtyp och innehåll,läser bara headers och etc
	avformat_open_input(&InputFormatContext, InputFilepath.c_str(), NULL, NULL);
	//läsar in data om själva datastreamsen
	avformat_find_stream_info(InputFormatContext, NULL);
	std::vector<AVCodecParameters*> InputCodecParameters = {};
	std::vector<AVCodec*> InputCodecs = {};
	std::vector<AVCodecContext*> DecodeCodecContext = {};
	for (size_t i = 0; i < InputFormatContext->nb_streams; i++)
	{
		AVCodecParameters* NewInputCodecParamters = InputFormatContext->streams[i]->codecpar;
		InputCodecParameters.push_back(NewInputCodecParamters);
		AVCodec* NewInputCodec = avcodec_find_decoder(NewInputCodecParamters->codec_id);
		std::cout << NewInputCodec->name << std::endl;
		InputCodecs.push_back(NewInputCodec);
		//givet en codec och codec parameters så kan vi encoda/decoda data, men eftersom det är statefull kräver vi en encode/decode context
		AVCodecContext* NewCodexContext = avcodec_alloc_context3(NewInputCodec);
		avcodec_parameters_to_context(NewCodexContext, NewInputCodecParamters);
		//sedan måste vi öppna den, vet inte riktigt varför, initializerar den kanske?
		avcodec_open2(NewCodexContext, NewInputCodec, NULL);
		DecodeCodecContext.push_back(NewCodexContext);
	}
	//all data för att decoda insamlad



	AVFormatContext* OutputFormatContext;
	avformat_alloc_output_context2(&OutputFormatContext, NULL, NULL, OutputFilepath.c_str());
	//skapar format kontexten som är vår out fil, sedan måste vi lägga till vad denna fil ska innehålla
	for (size_t i = 0; i < InputFormatContext->nb_streams; i++)
	{
		AVStream* OutputStream = NULL;
		//AVStream* InputStream = InputFormatContext->streams[i];
		OutputStream = avformat_new_stream(OutputFormatContext, NULL);
		avcodec_parameters_copy(OutputStream->codecpar, InputCodecParameters[i]);
		//ser till att streamen tar global headers om format contexten gör det
		if (OutputFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
		{
			OutputStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}
	}

	//tror den bara kommer gissa output format pga filename
	int ReadFrameResponse = avio_open(&OutputFormatContext->pb, OutputFilepath.c_str(), AVIO_FLAG_WRITE);
	if (ReadFrameResponse < 0)
	{
		std::cout << "Error opening output file" << std::endl;
	}
	ReadFrameResponse = avformat_write_header(OutputFormatContext, NULL);
	if (ReadFrameResponse < 0)
	{
		std::cout << "Error occurred when opening output file" << std::endl;
	}

	AVPacket ReadPacket;
	while (true)
	{
		ReadFrameResponse = av_read_frame(InputFormatContext, &ReadPacket);
		if (ReadFrameResponse < 0)
		{
			//all data är inläst
			break;
		}
		//vi skriver till den nya filen packet för packet
		AVStream* InStream = NULL;
		AVStream* OutStream = NULL;
		InStream = InputFormatContext->streams[ReadPacket.stream_index];
		OutStream = OutputFormatContext->streams[ReadPacket.stream_index];

		//Copy packet
		ReadPacket.pts = av_rescale_q_rnd(ReadPacket.pts, InStream->time_base, OutStream->time_base, static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		ReadPacket.dts = av_rescale_q_rnd(ReadPacket.dts, InStream->time_base, OutStream->time_base, static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		ReadPacket.duration = av_rescale_q(ReadPacket.duration, InStream->time_base, OutStream->time_base);
		ReadPacket.pos = -1;

		ReadFrameResponse = av_interleaved_write_frame(OutputFormatContext, &ReadPacket);
		if (ReadFrameResponse < 0)
		{
			std::cout << "Error demuxin packet" << std::endl;
			break;
		}
		av_packet_unref(&ReadPacket);
	}
	av_write_trailer(OutputFormatContext);

	avformat_close_input(&InputFormatContext);
	avio_closep(&OutputFormatContext->pb);

	avformat_free_context(OutputFormatContext);
	return(MBError(true));
}

MBError CreateHLSStream(std::string InputFilePath,std::string OutputFolderName, float TargetDuration)
{
	std::filesystem::create_directory(OutputFolderName);
	clock_t Timer = clock();
	std::string CommandString = "ffmpeg -hide_banner -loglevel error -i ";
	CommandString += InputFilePath;
	std::string Resolution = "720p";
	CommandString += " -c:a aac -ar 48000 -b:a 128k -c:v h264 -profile:v main -crf 20 -g 48 -keyint_min 96 -sc_threshold 0 -b:v 2500k -maxrate 2675k -bufsize 3750k -hls_time 8 -hls_playlist_type vod -hls_segment_filename ";
	//skapar ett nytt directory som vi sparar filerna i
	std::string HLS_FileName = OutputFolderName + "/" + Resolution + "_%03d.ts";
	std::string PlayListFilename = OutputFolderName + "/" + "MasterPlaylist" + ".m3u8";
	CommandString += HLS_FileName;
	CommandString += " " + PlayListFilename;
	std::system(CommandString.c_str());
	std::cout << (clock() - Timer) / double(CLOCKS_PER_SEC) << std::endl;
	return(MBError(false));
}
};
//*/