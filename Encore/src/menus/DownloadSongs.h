//
// Created by Jaydenz on 10/2/2025.
//

#ifndef DOWNLOADSONGS_H
#define DOWNLOADSONGS_H

#include "menu.h"
#include "util/trackDownloader.h"
#include <vector>
#include <unordered_map>

#include "raylib.h"

class DownloadSongs : public Menu {
public:
    DownloadSongs();
    ~DownloadSongs();
    
    void Draw() override;
    void KeyboardInputCallback(int key, int scancode, int action, int mods) override;
    void ControllerInputCallback(int joypadID, GLFWgamepadstate state) override;
    void Load() override;
    void Save();

private:
    std::vector<Encore::TrackInfo> tracks;
    int selectedTrack = 0;
    int hoveredTrack = -1;
    bool showDetails = false;
    int detailsTrack = -1;
    float scrollOffset = 0.0f;
    
    void DrawTrackGrid();
    void DrawTrackDetails();
    void LoadCoverArt(const std::string& coverUrl);

    std::unordered_map<std::string, Texture2D> coverCache;
    std::unordered_map<std::string, Texture2D> fullQualityCache;
    std::unordered_map<std::string, bool> downloadingCovers;
    std::unordered_map<std::string, bool> isPlaceholder;
    
    Texture2D LoadCoverFromUrl(const std::string& coverName);
    Texture2D LoadFullQualityCover(const std::string& coverName);
    void DownloadAllCovers();
    bool DownloadCoverImage(const std::string& coverName, const std::string& outputPath);
    std::string GetCoverCachePath(const std::string& coverName);
};

#endif //DOWNLOADSONGS_H