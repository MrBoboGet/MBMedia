#include "MBMedia2.h"

namespace MBMedia
{


	void Transcode(std::string const& InputFile, std::string const& OutputFile, Codec NewAudioCodec, Codec NewVideoCodec)
	{
		ContainerDemuxer InputData(InputFile);
		std::vector<StreamDecoder> Decoders = {};
		OutputContext OutputData(OutputFile);
		for (size_t i = 0; i < InputData.NumberOfStreams(); i++)
		{
			if (InputData.GetStreamInfo(i).Type == MediaType::Audio)
			{
				Decoders.push_back(StreamDecoder(InputData.GetStreamInfo(i)));
				OutputData.AddOutputStream(StreamEncoder(NewAudioCodec,GetAudioEncodePresets(Decoders.back())));
			}
			if (InputData.GetStreamInfo(i).Type == MediaType::Video)
			{
				Decoders.push_back(StreamDecoder(InputData.GetStreamInfo(i)));
				OutputData.AddOutputStream(StreamEncoder(NewVideoCodec, GetVideoEncodePresets(Decoders.back())));
			}
		}
		while (!InputData.EndOfFile())
		{
			size_t PacketIndex = 0;
			StreamPacket NewPacket = InputData.GetNextPacket(&PacketIndex);
			Decoders[PacketIndex].InsertPacket(NewPacket);
			while (Decoders[PacketIndex].DataAvailable())
			{
				StreamFrame NewFrame = Decoders[PacketIndex].GetNextFrame();
				OutputData.InsertFrame(NewFrame, PacketIndex);
			}
		}
		for (size_t i = 0; i < Decoders.size(); i++)
		{
			Decoders[i].Flush();
			while (Decoders[i].DataAvailable())
			{
				StreamFrame NewFrame = Decoders[i].GetNextFrame();
				OutputData.InsertFrame(NewFrame, i);
			}
		}
		OutputData.Finalize();
	}
};