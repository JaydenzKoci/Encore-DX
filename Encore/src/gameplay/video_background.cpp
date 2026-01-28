#include "video_background.h"
#include "settings.h"
#include <raylib.h>

VideoBackgroundManager& TheVideoBackground = VideoBackgroundManager::getInstance();

VideoBackgroundManager& VideoBackgroundManager::getInstance() {
    static VideoBackgroundManager instance;
    return instance;
}

bool VideoBackgroundManager::LoadVideoBackground(const std::filesystem::path& songVideoPath) {
    Unload();
    if (std::filesystem::exists(songVideoPath)) {
        TraceLog(LOG_INFO, "Using song-specific video: %s", songVideoPath.string().c_str());

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

void VideoBackgroundManager::Seek(double timeMs) {
    if (currentVideo && currentVideo->IsLoaded()) {
        currentVideo->Seek(timeMs);
    }
}

void VideoBackgroundManager::Resume() {
    if (currentVideo && currentVideo->IsLoaded()) {
        currentVideo->Resume();
        isPlaying = true;
    }
}

bool VideoBackgroundManager::HasEnded() const {
    return currentVideo && currentVideo->HasEnded();
}

double VideoBackgroundManager::GetCurrentPositionMs() const {
    if (currentVideo && currentVideo->IsLoaded()) {
        return currentVideo->GetCurrentPositionMs();
    }
    return 0.0;
}