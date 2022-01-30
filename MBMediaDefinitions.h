#pragma once
#define NOMINMAX

#define MBMEDIA_VERIFY_AUDIO_DATA

namespace MBMedia
{
	enum class Codec
	{
		AAC,
		H264,
		H265,
		VP9,
		Opus,
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
	MediaType GetCodecMediaType(Codec InputCodec);
};