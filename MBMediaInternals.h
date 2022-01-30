#pragma once
#define NOMINMAX

#include "MBVideoDefinitions.h"
#include "MBAudioDefinitions.h"
#include "MBMediaDefinitions.h"

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
namespace MBMedia
{
	void h_Print_ffmpeg_Error(int ReadFrameResponse);
	int FFMPEGCall(int ReadFrameResponse);
	inline void _FreeFormatContext(void* DataToFree)
	{
		avformat_free_context((AVFormatContext*)DataToFree);
		//FFMPEGCall();
	}
	inline void _FreeSwrContext(void* ContextToFree)
	{
		SwrContext* FFMPEGContext = (SwrContext*)ContextToFree;
		swr_free(&FFMPEGContext);
	}
	MediaType h_FFMPEGMediaTypeToMBMediaType(AVMediaType TypeToConvert);
	TimeBase h_RationalToTimebase(AVRational RationalToConvert);
	struct MBMediaTypeConnector
	{
		Codec AssociatdCodec = Codec::Null;
		MediaType AssociatedMediaType = MediaType::Null;
		AVCodecID AssoicatedCodecId = (AVCodecID)-1;
	};

	static const MBMediaTypeConnector ConnectedTypes[(size_t)Codec::Null] = {
		{Codec::AAC,MediaType::Audio,AV_CODEC_ID_AAC},
		{Codec::H264,MediaType::Video,AV_CODEC_ID_H264},
		{Codec::H265,MediaType::Video,AV_CODEC_ID_H265},
		{Codec::VP9,MediaType::Video,AV_CODEC_ID_VP9},
	};
	MediaType GetCodecMediaType(Codec InputCodec);
	Codec h_FFMPEGCodecTypeToMBCodec(AVCodecID TypeToConvert);
	SampleFormat h_FFMPEGAudioFormatToMBFormat(AVSampleFormat FormatToConvert);
	AVSampleFormat h_MBSampleFormatToFFMPEGSampleFormat(SampleFormat FormatToConvert);
	int64_t h_MBLayoutToFFMPEGLayout(ChannelLayout LayoutToConvert);
	ChannelLayout h_FFMPEGLayoutToMBLayout(int64_t LayoutToConvert);
	VideoFormat h_FFMPEGVideoFormatToMBVideoFormat(AVPixelFormat FormatToConvert);
	AVPixelFormat h_MBVideoFormatToFFMPEGVideoFormat(VideoFormat FormatToConvert);
	VideoFormat h_FFMPEGVideoFormatToMBVideoFormat(AVPixelFormat FormatToConvert);
}