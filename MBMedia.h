#pragma once
#define NOMINMAX

#include <vector>
#include <memory>
#include <string>
#include <stddef.h>
#include <cstdint>
#include <stdint.h>
#include <queue>
#include "PixelFormats.h"
#include <MBUtility/MBInterfaces.h>

#include "MBMediaDefinitions.h"
#include "MBVideoDefinitions.h"
#include "MBAudioDefinitions.h"
#include "ContainerMuxing.h"
///*
namespace MBMedia
{
	void Transcode(std::string const& InputFile, std::string const& OutputFile,Codec NewAudioCodec,Codec NewVideoCodec);



	class AudioOctetStreamer : public AudioStream
	{
	private:
		std::unique_ptr<MBMedia::ContainerDemuxer> m_StreamDemuxer = nullptr;
		std::unique_ptr<MBMedia::StreamDecoder> m_StreamDecoder = nullptr;
		bool m_StreamEnded = false;
		std::deque<MBMedia::StreamFrame> m_DecodedFrames = {};
		size_t m_CurrentFrameSampleOffset = 0;

		size_t m_AudioStreamIndex = 0;

		size_t p_WriteFrameSamples(uint8_t** OutData, const size_t Offset, MBMedia::StreamFrame const& FrameToWriteFrom, size_t NumberOfSamples);

		size_t p_LoadSamples(size_t SamplesToLoad);
		size_t p_GetStoredSamples();

	public:
		AudioOctetStreamer(std::unique_ptr<MBUtility::MBSearchableInputStream> InputStream, MBMedia::AudioParameters const& OutputParameters);
		AudioOctetStreamer(std::unique_ptr<MBUtility::MBSearchableInputStream> InputStream);
		virtual MBMedia::AudioParameters GetAudioParameters() override;
		virtual size_t GetNextSamples(uint8_t** DataBuffer, size_t NumberOfSamples,size_t OutputSamplesOffset) override;
		virtual bool IsFinished() override;
		virtual ~AudioOctetStreamer() {};
	};
};
//*/