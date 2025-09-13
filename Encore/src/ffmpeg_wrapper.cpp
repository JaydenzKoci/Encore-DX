#include "ffmpeg_wrapper.h"
#include <iostream>

FFmpegWrapper::FFmpegWrapper()
    : format_context_(nullptr)
    , codec_context_(nullptr)
    , frame_(nullptr)
    , rgb_frame_(nullptr)
    , sws_context_(nullptr)
    , packet_(nullptr)
    , video_stream_index_(-1)
    , width_(0)
    , height_(0)
    , frame_rate_(0.0)
    , duration_(0.0)
{
}

FFmpegWrapper::~FFmpegWrapper() {
    CloseFile();
}

void FFmpegWrapper::Initialize() {
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
    av_register_all();
#endif

    av_log_set_level(AV_LOG_WARNING);
}

bool FFmpegWrapper::OpenFile(const std::string& filename) {
    CloseFile();

    if (avformat_open_input(&format_context_, filename.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return false;
    }

    if (avformat_find_stream_info(format_context_, nullptr) < 0) {
        std::cerr << "Could not find stream information" << std::endl;
        CloseFile();
        return false;
    }

    video_stream_index_ = av_find_best_stream(format_context_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_stream_index_ < 0) {
        std::cerr << "Could not find video stream" << std::endl;
        CloseFile();
        return false;
    }
    
    if (!InitializeDecoder()) {
        CloseFile();
        return false;
    }

    AVStream* video_stream = format_context_->streams[video_stream_index_];
    width_ = codec_context_->width;
    height_ = codec_context_->height;

    if (video_stream->avg_frame_rate.den != 0) {
        frame_rate_ = av_q2d(video_stream->avg_frame_rate);
    } else if (video_stream->r_frame_rate.den != 0) {
        frame_rate_ = av_q2d(video_stream->r_frame_rate);
    }

    if (format_context_->duration != AV_NOPTS_VALUE) {
        duration_ = static_cast<double>(format_context_->duration) / AV_TIME_BASE;
    }
    
    return true;
}

void FFmpegWrapper::CloseFile() {
    Cleanup();
}

bool FFmpegWrapper::ReadFrame(uint8_t** frame_data, int& width, int& height) {
    if (!IsOpen()) {
        return false;
    }
    
    while (av_read_frame(format_context_, packet_) >= 0) {
        if (packet_->stream_index == video_stream_index_) {
            int ret = avcodec_send_packet(codec_context_, packet_);
            if (ret < 0) {
                av_packet_unref(packet_);
                continue;
            }

            ret = avcodec_receive_frame(codec_context_, frame_);
            if (ret == 0) {
                if (!sws_context_) {
                    sws_context_ = sws_getContext(
                        codec_context_->width, codec_context_->height, codec_context_->pix_fmt,
                        codec_context_->width, codec_context_->height, AV_PIX_FMT_RGB24,
                        SWS_BILINEAR, nullptr, nullptr, nullptr
                    );
                    
                    if (!sws_context_) {
                        av_packet_unref(packet_);
                        return false;
                    }

                    rgb_frame_ = av_frame_alloc();
                    if (!rgb_frame_) {
                        av_packet_unref(packet_);
                        return false;
                    }
                    
                    int rgb_buffer_size = codec_context_->width * codec_context_->height * 3;
                    uint8_t* rgb_buffer = static_cast<uint8_t*>(av_malloc(rgb_buffer_size));

                    rgb_frame_->data[0] = rgb_buffer;
                    rgb_frame_->linesize[0] = codec_context_->width * 3;
                    rgb_frame_->width = codec_context_->width;
                    rgb_frame_->height = codec_context_->height;
                    rgb_frame_->format = AV_PIX_FMT_RGB24;
                }

                sws_scale(sws_context_, frame_->data, frame_->linesize, 0, codec_context_->height,
                         rgb_frame_->data, rgb_frame_->linesize);
                
                *frame_data = rgb_frame_->data[0];
                width = codec_context_->width;
                height = codec_context_->height;
                
                av_packet_unref(packet_);
                return true;
            }
        }
        av_packet_unref(packet_);
    }
    
    return false;
}

bool FFmpegWrapper::InitializeDecoder() {
    AVStream* video_stream = format_context_->streams[video_stream_index_];

    const AVCodec* codec = avcodec_find_decoder(video_stream->codecpar->codec_id);
    if (!codec) {
        std::cerr << "Unsupported codec" << std::endl;
        return false;
    }

    codec_context_ = avcodec_alloc_context3(codec);
    if (!codec_context_) {
        std::cerr << "Could not allocate codec context" << std::endl;
        return false;
    }

    if (avcodec_parameters_to_context(codec_context_, video_stream->codecpar) < 0) {
        std::cerr << "Could not copy codec parameters" << std::endl;
        return false;
    }

    if (avcodec_open2(codec_context_, codec, nullptr) < 0) {
        std::cerr << "Could not open codec" << std::endl;
        return false;
    }

    frame_ = av_frame_alloc();
    packet_ = av_packet_alloc();
    
    if (!frame_ || !packet_) {
        std::cerr << "Could not allocate frame or packet" << std::endl;
        return false;
    }
    
    return true;
}

void FFmpegWrapper::Cleanup() {
    if (sws_context_) {
        sws_freeContext(sws_context_);
        sws_context_ = nullptr;
    }
    
    if (rgb_frame_) {
        if (rgb_frame_->data[0]) {
            av_free(rgb_frame_->data[0]);
        }
        av_frame_free(&rgb_frame_);
    }
    
    if (frame_) {
        av_frame_free(&frame_);
    }
    
    if (packet_) {
        av_packet_free(&packet_);
    }
    
    if (codec_context_) {
        avcodec_free_context(&codec_context_);
    }
    
    if (format_context_) {
        avformat_close_input(&format_context_);
    }
    
    video_stream_index_ = -1;
    width_ = height_ = 0;
    frame_rate_ = duration_ = 0.0;
}