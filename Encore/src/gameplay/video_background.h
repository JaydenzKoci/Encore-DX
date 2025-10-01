#pragma once
#include "video.h"
#include <filesystem>
#include <memory>

class VideoBackgroundManager {
public:
    static VideoBackgroundManager& getInstance();
    
    // Load video background for a song (will use custom video if set)
    bool LoadVideoBackground(const std::filesystem::path& songVideoPath);
    
    // Update video playback
    void Update();
    
    // Draw video background
    void Draw();
    
    // Start/stop playback
    void Play();
    void Pause();
    void Stop();
    
    // Check if video is loaded and playing
    bool IsLoaded() const;
    bool IsPlaying() const;
    
    // Unload current video
    void Unload();

private:
    VideoBackgroundManager() = default;
    std::unique_ptr<VideoStream> currentVideo;
    bool isPlaying = false;
};

extern VideoBackgroundManager& TheVideoBackground;