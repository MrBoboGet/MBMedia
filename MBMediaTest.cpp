#include "MBMedia.h"
#include <filesystem>
#include <iostream>
extern "C"
{
#include <ffmpeg/libavcodec/avcodec.h>
}
int main()
{
	std::cout << avcodec_find_encoder(AV_CODEC_ID_H264) << std::endl;
//	return 0;
	std::filesystem::current_path(std::filesystem::current_path().parent_path());
	MBMedia::Transcode("./CatBoom.mp4", "./CatBoom.mkv", MBMedia::MBVideoCodecs::H264, MBMedia::MBAudioCodecs::AAC);
}