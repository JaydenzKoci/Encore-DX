#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

#include <string>
#include <memory>

/**
 * @brief Simple FFmpeg wrapper for video processing
 * 
 * This class provides a simplified interface for common FFmpeg operations
 * such as opening video files, reading frames, and basic format conversion.
 */
class FFmpegWrapper {
public:
    FFmpegWrapper();
    ~FFmpegWrapper();
    
    /**
     * @brief Initialize FFmpeg (call once at startup)
     */
    static void Initialize();
    
    /**
     * @brief Open a video file for reading
     * @param filename Path to the video file
     * @return true if successful, false otherwise
     */
    bool OpenFile(const std::string& filename);
    
    /**
     * @brief Close the currently opened file
     */
    void CloseFile();
    
    /**
     * @brief Read the next frame from the video
     * @param frame_data Output buffer for frame data (RGB format)
     * @param width Output width of the frame
     * @param height Output height of the frame
     * @return true if frame was read successfully, false if EOF or error
     */
    bool ReadFrame(uint8_t** frame_data, int& width, int& height);
    
    /**
     * @brief Get video information
     */
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    double GetFrameRate() const { return frame_rate_; }
    double GetDuration() const { return duration_; }
    
    /**
     * @brief Check if a file is currently open
     */
    bool IsOpen() const { return format_context_ != nullptr; }

private:
    AVFormatContext* format_context_;
    AVCodecContext* codec_context_;
    AVFrame* frame_;
    AVFrame* rgb_frame_;
    SwsContext* sws_context_;
    AVPacket* packet_;
    
    int video_stream_index_;
    int width_;
    int height_;
    double frame_rate_;
    double duration_;
    
    bool InitializeDecoder();
    void Cleanup();
};

// test function
void test_ffmpeg_basic();