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
	return(0);
}