#pragma once
#include "PixelFormats.h"
namespace MBMedia
{
	struct VideoParameters
	{
		VideoFormat Format = VideoFormat::Null;
		int Width = 0;
		int Height = 0;
	};
};