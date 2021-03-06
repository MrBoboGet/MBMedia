#include "MBMedia.h"
//#include "MBMedia2.h"
#include <filesystem>
#include <iostream>
extern "C"
{
#include <ffmpeg/libavcodec/avcodec.h>
}

#include "MBAudioUtility.h"

int main()
{
	std::cout << avcodec_find_encoder(AV_CODEC_ID_H265) << std::endl;
	//avcodec_register_all();
//	return 0;
	std::filesystem::current_path(std::filesystem::current_path().parent_path());
	
	std::unique_ptr<MBMedia::AudioFIFOBuffer> AudioBuffer;
	std::deque<MBMedia::StreamFrame> VideoFrames;
	for (size_t j = 0; j < 100; j++)
	{
		std::vector<MBMedia::StreamDecoder> TestLeekDecoders;
		MBMedia::ContainerDemuxer TestLeekDemuxer("./SonicSpeedruning.mkv");
		for (size_t i = 0; i < TestLeekDemuxer.NumberOfStreams(); i++)
		{
			TestLeekDecoders.push_back(MBMedia::StreamDecoder(TestLeekDemuxer.GetStreamInfo(i)));
		}
		while (true)
		{
			size_t Index;
			MBMedia::StreamPacket Packet = TestLeekDemuxer.GetNextPacket(&Index);
			if (Packet.GetType() == MBMedia::MediaType::Null)
			{
				break;
			}
			TestLeekDecoders[Index].InsertPacket(Packet);
			while (true)
			{
				MBMedia::StreamFrame NewFrame = TestLeekDecoders[Index].GetNextFrame();
				if (NewFrame.GetMediaType() == MBMedia::MediaType::Audio)
				{
					if (AudioBuffer == nullptr)
					{
						AudioBuffer = std::unique_ptr<MBMedia::AudioFIFOBuffer>(new MBMedia::AudioFIFOBuffer(NewFrame.GetAudioParameters(),4096));
					}
					MBMedia::AudioBuffer Buffer = MBMedia::AudioBuffer(NewFrame.GetAudioParameters(), NewFrame.GetAudioFrameInfo().NumberOfSamples);
					AudioBuffer->InsertData(NewFrame.GetData(), NewFrame.GetAudioFrameInfo().NumberOfSamples);
					AudioBuffer->ReadData(Buffer.GetData(), NewFrame.GetAudioFrameInfo().NumberOfSamples);
				}
				if (NewFrame.GetMediaType() == MBMedia::MediaType::Video)
				{
					VideoFrames.push_back(std::move(NewFrame));
				}
				if (NewFrame.GetMediaType() == MBMedia::MediaType::Null)
				{
					break;
				}
			}
		}
		VideoFrames.clear();
	}


	return(0);
	//MBMedia::Transcode("./SonicSpeedruning.mkv", "./SonicSpeedruning.mp4", MBMedia::MBVideoCodecs::H264, MBMedia::MBAudioCodecs::AAC);
	MBMedia::Transcode("./SonicSpeedruning.mkv", "./SonicSpeedruning.mp4", MBMedia::Codec::AAC, MBMedia::Codec::H264);
	MBMedia::Transcode("./SonicSpeedruning.mp4", "././SonicSpeedruning2.mkv", MBMedia::Codec::AAC, MBMedia::Codec::H264);


	//testar att skriva ren audio data till output streamen
	MBMedia::ContainerDemuxer AudioFile("SonicSpeedruning.mp4");
	size_t AudioStreamIndex = -1;
	for (size_t i = 0; i < AudioFile.NumberOfStreams(); i++)
	{
		if (AudioFile.GetStreamInfo(i).GetMediaType() == MBMedia::MediaType::Audio)
		{
			AudioStreamIndex = i;
			break;
		}
	}
	MBMedia::StreamDecoder AudioDecoder = MBMedia::StreamDecoder(AudioFile.GetStreamInfo(AudioStreamIndex));
	
	MBMedia::OutputContext OnlyAudioOutput = MBMedia::OutputContext("SonicSpeedruningAudio.mp4");
	
	MBMedia::StreamEncoder EncoderToInsert = MBMedia::StreamEncoder(MBMedia::Codec::AAC, AudioDecoder.GetAudioDecodeInfo());
	MBMedia::AudioEncodeInfo EncodingInfo = EncoderToInsert.GetAudioEncodeInfo();
	MBMedia::AudioToFrameConverter FrameConverter(EncodingInfo.AudioInfo, 0, EncodingInfo.StreamTimebase, EncodingInfo.FrameSize);
	OnlyAudioOutput.AddOutputStream(std::move(EncoderToInsert));
	OnlyAudioOutput.WriteHeader();
	bool DecoderFlushed = false;
	while (true)
	{
		size_t CurrentPacketIndex = -1;
		MBMedia::StreamPacket CurrentPacket = AudioFile.GetNextPacket(&CurrentPacketIndex);
		if (CurrentPacket.GetType() == MBMedia::MediaType::Null)
		{
			AudioDecoder.Flush();
			DecoderFlushed = true;
		}
		else if (CurrentPacket.GetType() == MBMedia::MediaType::Video)
		{
			continue;
		}
		else
		{
			AudioDecoder.InsertPacket(CurrentPacket);
		}
		MBMedia::StreamFrame NewAudioFrame = AudioDecoder.GetNextFrame();
		if (NewAudioFrame.GetMediaType() == MBMedia::MediaType::Null && !DecoderFlushed)
		{
			continue;
		}
		if (NewAudioFrame.GetMediaType() == MBMedia::MediaType::Null && DecoderFlushed)
		{
			FrameConverter.Flush();
		}
		if (!DecoderFlushed)
		{
			NewAudioFrame.SetDuration(NewAudioFrame.GetDuration());
			FrameConverter.InsertAudioData(NewAudioFrame.GetData(), EncodingInfo.FrameSize);
		}
		MBMedia::StreamFrame ConvertedAudioFrame = FrameConverter.GetNextFrame();
		if (ConvertedAudioFrame.GetMediaType() != MBMedia::MediaType::Null)
		{
			OnlyAudioOutput.InsertFrame(ConvertedAudioFrame,0);
		}
		if (ConvertedAudioFrame.GetMediaType() == MBMedia::MediaType::Null && DecoderFlushed)
		{
			break;
		}
	}
	OnlyAudioOutput.Finalize();
	return(0);
}