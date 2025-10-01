#include "video_background.h"
#include "settings.h"
#include <raylib.h>

VideoBackgroundManager& TheVideoBackground = VideoBackgroundManager::getInstance();

VideoBackgroundManager& VideoBackgroundManager::getInstance() {
    static VideoBackgroundManager instance;
    return instance;
}

bool VideoBackgroundManager::LoadVideoBackground(const std::filesystem::path& songVideoPath) {
    // Unload any existing video
    Unload();
    
    // Use the song-specific video if it exists
    if (std::filesystem::exists(songVideoPath)) {
        TraceLog(LOG_INFO, "Using song-specific video: %s", songVideoPath.string().c_str());
        
        // Create and load video
        currentVideo = std::make_unique<VideoStream>();
        if (currentVideo->Load(songVideoPath)) {
            TraceLog(LOG_INFO, "Video background loaded successfully");
            return true;
        } else {
            TraceLog(LOG_ERROR, "Failed to load video background: %s", songVideoPath.string().c_str());
            currentVideo.reset();
            return false;
        }
    } else {
        TraceLog(LOG_INFO, "No video background available");
        return false;
    }
}

void VideoBackgroundManager::Update() {
    if (currentVideo && currentVideo->IsLoaded()) {
        currentVideo->Update();
    }
}

void VideoBackgroundManager::Draw() {
    if (currentVideo && currentVideo->IsLoaded()) {
        // Draw video with slight transparency so gameplay elements are visible
        currentVideo->Draw(0, 0, Color{255, 255, 255, 180});
    }
}

void VideoBackgroundManager::Play() {
    if (currentVideo && currentVideo->IsLoaded()) {
        currentVideo->Play();
        isPlaying = true;
    }
}

void VideoBackgroundManager::Pause() {
    if (currentVideo && currentVideo->IsLoaded()) {
        currentVideo->Pause();
        isPlaying = false;
    }
}

void VideoBackgroundManager::Stop() {
    if (currentVideo && currentVideo->IsLoaded()) {
        currentVideo->Stop();
        isPlaying = false;
    }
}

bool VideoBackgroundManager::IsLoaded() const {
    return currentVideo && currentVideo->IsLoaded();
}

bool VideoBackgroundManager::IsPlaying() const {
    return isPlaying && IsLoaded();
}

void VideoBackgroundManager::Unload() {
    if (currentVideo) {
        currentVideo->Unload();
        currentVideo.reset();
    }
    isPlaying = false;
}