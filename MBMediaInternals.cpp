#include "MBVideoDefinitions.h"
#include "MBMediaInternals.h"
#include "MBMediaDefinitions.h"
#include "MBAudioDefinitions.h"

#include <iostream>

#include <assert.h>

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
};