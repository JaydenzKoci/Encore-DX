#pragma once
#include "video.h"
#include <filesystem>
#include <memory>

class VideoBackgroundManager {
public:
    static VideoBackgroundManager& getInstance();
    bool LoadVideoBackground(const std::filesystem::path& songVideoPath);
    void Update();
    void Draw();
    void Play();
    void Pause();
    void Stop();
    bool IsLoaded() const;
    bool IsPlaying() const;
    void Unload();

private:
    VideoBackgroundManager() = default;
    std::unique_ptr<VideoStream> currentVideo;
    bool isPlaying = false;
};

extern VideoBackgroundManager& TheVideoBackground;