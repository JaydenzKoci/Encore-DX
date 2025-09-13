#include "ffmpeg_wrapper.h"
#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

void test_ffmpeg_basic() {
    std::cout << "=== FFmpeg Basic Test ===" << std::endl;

    std::cout << "FFmpeg version: " << av_version_info() << std::endl;
    std::cout << "libavformat version: " << avformat_version() << std::endl;
    std::cout << "libavcodec version: " << avcodec_version() << std::endl;
    std::cout << "libavutil version: " << avutil_version() << std::endl;

    std::cout << "\nInitializing FFmpeg..." << std::endl;
    FFmpegWrapper::Initialize();
    std::cout << "FFmpeg initialized successfully!" << std::endl;

    std::cout << "\nCreating FFmpeg wrapper..." << std::endl;
    FFmpegWrapper wrapper;
    std::cout << "Wrapper created successfully!" << std::endl;

    std::cout << "\nAvailable codecs (first 5):" << std::endl;
    const AVCodec* codec = nullptr;
    void* opaque = nullptr;
    int count = 0;
    while ((codec = av_codec_iterate(&opaque)) && count < 5) {
        if (codec->type == AVMEDIA_TYPE_VIDEO) {
            std::cout << "  - " << codec->name << " (" << codec->long_name << ")" << std::endl;
            count++;
        }
    }
    
    std::cout << "\n=== FFmpeg test completed successfully! ===" << std::endl;
}