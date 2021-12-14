#include "MBMedia.h"
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
	MBMedia::Transcode("./TheCap.mp4", "./TheCap.mkv", MBMedia::MBVideoCodecs::H264, MBMedia::MBAudioCodecs::AAC);
	return(0);
}