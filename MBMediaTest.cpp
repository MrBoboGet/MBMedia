#include "MBMedia.h"
//#include "MBMedia2.h"
#include <filesystem>
#include <iostream>
extern "C"
{
#include <ffmpeg/libavcodec/avcodec.h>
}
int main()
{
	std::cout << avcodec_find_encoder(AV_CODEC_ID_H265) << std::endl;
	avcodec_register_all();
//	return 0;
	std::filesystem::current_path(std::filesystem::current_path().parent_path());
	//MBMedia::Transcode("./SonicSpeedruning.mkv", "./SonicSpeedruning.mp4", MBMedia::MBVideoCodecs::H264, MBMedia::MBAudioCodecs::AAC);
	MBMedia::Transcode("./SonicSpeedruning.mkv", "./SonicSpeedruning.mp4", MBMedia::Codec::AAC, MBMedia::Codec::H264);
	MBMedia::Transcode("./SonicSpeedruning.mp4", "././SonicSpeedruning2.mkv", MBMedia::Codec::AAC, MBMedia::Codec::H264);


	//testar att skriva ren audio data till output streamen
	//MBMedia::ContainerDemuxer AudioFile("SonicSpeedruning.mp4");
	//size_t AudioStreamIndex = -1;
	//for (size_t i = 0; i < AudioFile.NumberOfStreams(); i++)
	//{
	//	if (AudioFile.GetStreamInfo(i).GetMediaType() == MBMedia::MediaType::Audio)
	//	{
	//		AudioStreamIndex = i;
	//		break;
	//	}
	//}
	//MBMedia::StreamDecoder AudioDecoder = MBMedia::StreamDecoder(AudioFile.GetStreamInfo(AudioStreamIndex));
	//
	//MBMedia::OutputContext RawAudioFile = MBMedia::OutputContext("SonicSpeedruningAudio.mp4");
	//MBMedia::StreamEncoder EncoderToInsert = MBMedia::StreamEncoder(MBMedia::Codec::AAC, MBMedia::GetAudioEncodePresets(AudioDecoder));
	//RawAudioFile.AddOutputStream(std::move(EncoderToInsert));

	//MBMedia::AudioToFrameConverter FrameConverter(EncoderToInsert.GetAudioParameters(),EncoderToInsert.GetAudioParameters().FrameSize)

	return(0);
}