#pragma once
#include <stdint.h>
namespace MBMedia
{
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
	struct SampleFormatInfo
	{
		bool Interleaved = false;
		bool Signed = false;
		bool Integer = false;
		size_t SampleSize = -1;
	};
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

	class AudioStream
	{
	private:

	public:
		virtual MBMedia::AudioParameters GetAudioParameters() = 0;
		virtual size_t GetNextSamples(uint8_t** DataBuffer, size_t NumberOfSamples,size_t OutputSampleOffset) = 0;
		virtual bool IsFinished() = 0;
		virtual ~AudioStream() = 0
		{

		}
	};
	class AudioStreamFilter
	{
	public:
		virtual void InitializeInputParameters(MBMedia::AudioParameters const& InputParameters) = 0;
		virtual void InsertData(const uint8_t* const* InputData, size_t NumberOfFrames,size_t InputFrameOffset) = 0;
		virtual size_t AvailableSamples() = 0;
		virtual MBMedia::AudioParameters GetAudioParameters() = 0;
		virtual size_t GetNextSamples(uint8_t** DataBuffer, size_t NumberOfSamples,size_t OutputSampleOffset) = 0;
		//virtual void Flush() = 0;
		virtual ~AudioStreamFilter() = 0
		{

		}
	};
};