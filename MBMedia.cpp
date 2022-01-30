#include "MBMedia.h"
#include "MBAudioUtility.h"
#include <assert.h>
namespace MBMedia
{
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

	//BEGIN AudioOctetStreamer
	AudioOctetStreamer::AudioOctetStreamer(std::unique_ptr<MBUtility::MBSearchableInputStream> InputStream, MBMedia::AudioParameters const& OutputParameters)
	{
		m_StreamDemuxer = std::unique_ptr<MBMedia::ContainerDemuxer>(new MBMedia::ContainerDemuxer(std::move(InputStream)));
		size_t AudioIndex = -1;
		for (size_t i = 0; i < m_StreamDemuxer->NumberOfStreams(); i++)
		{
			if (m_StreamDemuxer->GetStreamInfo(i).GetMediaType() == MBMedia::MediaType::Audio)
			{
				AudioIndex = i;
			}
		}
		m_StreamDecoder = std::unique_ptr<MBMedia::StreamDecoder>(new MBMedia::StreamDecoder(m_StreamDemuxer->GetStreamInfo(AudioIndex)));
		m_StreamDecoder->SetAudioConversionParameters(OutputParameters, 2024);//godtycklig framesize
		m_AudioStreamIndex = AudioIndex;
	}
	AudioOctetStreamer::AudioOctetStreamer(std::unique_ptr<MBUtility::MBSearchableInputStream> InputStream)
	{
		m_StreamDemuxer = std::unique_ptr<MBMedia::ContainerDemuxer>(new MBMedia::ContainerDemuxer(std::move(InputStream)));
		size_t AudioIndex = -1;
		for (size_t i = 0; i < m_StreamDemuxer->NumberOfStreams(); i++)
		{
			if (m_StreamDemuxer->GetStreamInfo(i).GetMediaType() == MBMedia::MediaType::Audio)
			{
				AudioIndex = i;
			}
		}
		m_StreamDecoder = std::unique_ptr<MBMedia::StreamDecoder>(new MBMedia::StreamDecoder(m_StreamDemuxer->GetStreamInfo(AudioIndex)));
		//MBMedia::AudioParameters NewParameters = m_StreamDecoder->GetAudioDecodeInfo().AudioInfo;
		//NewParameters.AudioFormat = MBMedia::GetPlanarAudioFormat(NewParameters.AudioFormat);
		//m_StreamDecoder->SetAudioConversionParameters(NewParameters, 2024);//Godtycklig framesize
		m_AudioStreamIndex = AudioIndex;
	}
	MBMedia::AudioParameters AudioOctetStreamer::GetAudioParameters()
	{
		return(m_StreamDecoder->GetAudioDecodeInfo().AudioInfo);
	}
	size_t AudioOctetStreamer::p_LoadSamples(size_t MinimumSamplesToLoad)
	{
		size_t LoadedSamples = 0;
		while (!m_StreamEnded && LoadedSamples < MinimumSamplesToLoad)
		{
			size_t PacketIndex = 0;
			MBMedia::StreamPacket NewPacket = m_StreamDemuxer->GetNextPacket(&PacketIndex);
			if (PacketIndex == m_AudioStreamIndex)
			{
				m_StreamDecoder->InsertPacket(NewPacket);
			}
			if (NewPacket.GetType() == MBMedia::MediaType::Null)
			{
				m_StreamDecoder->Flush();
			}
			MBMedia::StreamFrame NewFrame = m_StreamDecoder->GetNextFrame();
			if (NewFrame.GetMediaType() == MBMedia::MediaType::Null && NewPacket.GetType() == MBMedia::MediaType::Null)
			{
				m_StreamEnded = true;
				break;
			}
			else if (NewFrame.GetMediaType() != MBMedia::MediaType::Null)
			{
				LoadedSamples += NewFrame.GetAudioFrameInfo().NumberOfSamples;
				m_DecodedFrames.push_back(std::move(NewFrame));
			}
		}
		//assert(LoadedSamples != 0);
		return LoadedSamples;
	}
	size_t AudioOctetStreamer::p_GetStoredSamples()
	{
		size_t ReturnValue = 0;
		for (auto const& Frame : m_DecodedFrames)
		{
			ReturnValue += Frame.GetAudioFrameInfo().NumberOfSamples;
		}
		ReturnValue -= m_CurrentFrameSampleOffset;
		return(ReturnValue);
	}
	size_t AudioOctetStreamer::p_WriteFrameSamples(uint8_t** OutData, const size_t SampleOffset, MBMedia::StreamFrame const& FrameToWriteFrom, size_t NumberOfSamples)
	{
		if (m_DecodedFrames.size() == 0)
		{
			throw std::exception();
		}
		size_t CurrentFrameSamples = m_DecodedFrames.front().GetAudioFrameInfo().NumberOfSamples - m_CurrentFrameSampleOffset;
		size_t SamplesToWrite = NumberOfSamples > CurrentFrameSamples ? CurrentFrameSamples : NumberOfSamples;
		MBMedia::AudioParameters AudioParameters = m_DecodedFrames.front().GetAudioParameters();
		MBMedia::SampleFormatInfo SampleFormatInfo = MBMedia::GetSampleFormatInfo(AudioParameters.AudioFormat);
		const size_t CurrentInputOffset = m_CurrentFrameSampleOffset * GetChannelFrameSize(m_StreamDecoder->GetAudioDecodeInfo().AudioInfo);
		const size_t CurrentOutputOffset = SampleOffset * GetChannelFrameSize(m_StreamDecoder->GetAudioDecodeInfo().AudioInfo);
		const size_t SamplesToWriteSize = SamplesToWrite* GetChannelFrameSize(m_StreamDecoder->GetAudioDecodeInfo().AudioInfo);
		for (size_t i = 0; i < GetParametersDataPlanes(m_StreamDecoder->GetAudioDecodeInfo().AudioInfo); i++)
		{
			float* TestPointer = (float*)m_DecodedFrames.front().GetData()[i];
			std::memcpy(OutData[i] + CurrentOutputOffset, m_DecodedFrames.front().GetData()[i] + CurrentInputOffset, SamplesToWriteSize);
		}
		size_t NumberOfFrames = m_DecodedFrames.front().GetAudioFrameInfo().NumberOfSamples;
		float* TestPointer = (float*)m_DecodedFrames.front().GetData()[0];
#ifdef MBMEDIA_VERIFY_AUDIO_DATA
		VerifySamples(OutData, m_StreamDecoder->GetAudioDecodeInfo().AudioInfo, SamplesToWrite);
#endif // MBMEDIA_VERIFY_AUDIO_DATA

		
		//h_ArrayIsAudiData(OutData[0] + SampleOffset * SampleFormatInfo.SampleSize, SamplesToWrite);
		//h_ArrayIsAudiData(OutData[1] + SampleOffset * SampleFormatInfo.SampleSize, SamplesToWrite);

		m_CurrentFrameSampleOffset += SamplesToWrite;
		assert(m_CurrentFrameSampleOffset <= m_DecodedFrames.front().GetAudioFrameInfo().NumberOfSamples);
		if (m_CurrentFrameSampleOffset == m_DecodedFrames.front().GetAudioFrameInfo().NumberOfSamples)
		{
			m_CurrentFrameSampleOffset = 0;
			m_DecodedFrames.pop_front();
		}
		return(SamplesToWrite);
	}
	size_t AudioOctetStreamer::GetNextSamples(uint8_t** DataBuffer, size_t NumberOfSamples,size_t OutputSamplesOffset)
	{
		if (m_StreamDecoder == nullptr)
		{
			throw std::exception();
		}
		if (p_GetStoredSamples() < NumberOfSamples)
		{
			p_LoadSamples(NumberOfSamples - p_GetStoredSamples());
		}
		std::vector<uint8_t*> OutputBuffer = std::vector<uint8_t*>(MBMedia::GetParametersDataPlanes(m_StreamDecoder->GetAudioDecodeInfo().AudioInfo), nullptr);
		for (size_t i = 0; i < MBMedia::GetParametersDataPlanes(m_StreamDecoder->GetAudioDecodeInfo().AudioInfo); i++)
		{
			OutputBuffer[i] = DataBuffer[i] + (GetChannelFrameSize(m_StreamDecoder->GetAudioDecodeInfo().AudioInfo) * OutputSamplesOffset);
		}

		MBMedia::AudioParameters OutputParameters = m_StreamDecoder->GetAudioDecodeInfo().AudioInfo;
		size_t ExtractedSamples = 0;
		while (!IsFinished())
		{
			ExtractedSamples += p_WriteFrameSamples(OutputBuffer.data(), ExtractedSamples, m_DecodedFrames.front(), NumberOfSamples - ExtractedSamples);
			if (ExtractedSamples == NumberOfSamples)
			{
				break;
			}
		}
		if (ExtractedSamples < NumberOfSamples)
		{
			size_t OutputByteOffset = ExtractedSamples * GetChannelFrameSize(OutputParameters);
			size_t TotalOutputWidth = NumberOfSamples * GetChannelFrameSize(OutputParameters);
			for (size_t i = 0; i < GetParametersDataPlanes(OutputParameters); i++)
			{
				std::memset(OutputBuffer.data()[i] + OutputByteOffset, 0, TotalOutputWidth - OutputByteOffset);
			}
		}
		//float* TestPointer1 = (float*)DataBuffer[0];
		//float* TestPointer2 = (float*)DataBuffer[1];
		//h_ArrayIsAudiData(DataBuffer[0], ExtractedSamples);
		//h_ArrayIsAudiData(DataBuffer[1], ExtractedSamples);
#ifdef MBMEDIA_VERIFY_AUDIO_DATA
		assert(MBMedia::VerifySamples(OutputBuffer.data(), GetAudioParameters(), ExtractedSamples, OutputSamplesOffset));
#endif // MBAE_VERIFY_AUDIO_DATA

		return(ExtractedSamples);
	}
	bool AudioOctetStreamer::IsFinished()
	{
		return(m_DecodedFrames.size() == 0 && m_StreamEnded);
	}
	//END AudioOctetStreamer



}