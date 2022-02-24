#pragma once
#define NOMINMAX

#include <stdint.h>
#include <cstddef>
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
		bool operator==(AudioParameters const& OtherParameters)
		{
			bool ReturnValue = true;
			ReturnValue = ReturnValue && (Layout == OtherParameters.Layout);
			ReturnValue = ReturnValue && (AudioFormat == OtherParameters.AudioFormat);
			ReturnValue = ReturnValue && (SampleRate == OtherParameters.SampleRate);
			ReturnValue = ReturnValue && (NumberOfChannels == OtherParameters.NumberOfChannels);
			return(ReturnValue);
		}
		bool operator!=(AudioParameters const& OtherParameters)
		{
			return(!(*this == OtherParameters));
		}
	};

	class AudioStream
	{
	private:

	public:
		virtual MBMedia::AudioParameters GetAudioParameters() = 0;
		virtual size_t GetNextSamples(uint8_t* const* DataBuffer, size_t NumberOfSamples,size_t OutputSampleOffset) = 0;
		virtual bool IsFinished() = 0;
		virtual ~AudioStream()
		{

		}
	};
	class SeekableAudioStream : public AudioStream
	{
	private:
		virtual uint64_t CurrentSampleOffset() = 0;
		virtual void SeekToSampleOffset(uint64_t SampleOffset) = 0;
	};
	class AudioStreamFilter
	{
	public:
		virtual void InitializeInputParameters(MBMedia::AudioParameters const& InputParameters) = 0;
		virtual void InsertData(const uint8_t* const* InputData, size_t NumberOfFrames,size_t InputFrameOffset) = 0;
		virtual size_t AvailableSamples() = 0;
		virtual MBMedia::AudioParameters GetAudioParameters() = 0;
		virtual size_t GetNextSamples(uint8_t* const* DataBuffer, size_t NumberOfSamples,size_t OutputSampleOffset) = 0;
		//virtual void Flush() = 0;
		virtual ~AudioStreamFilter() 
		{

		}
	};
};