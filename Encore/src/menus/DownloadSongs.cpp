#include "DownloadSongs.h"
#include "MenuManager.h"
#include "gameMenu.h"
#include "raygui.h"
#include "assets.h"
#include "settings.h"
#include "uiUnits.h"
#include "OvershellMenu.h"
#include <algorithm>
#include <unordered_map>
#include <thread>
#include <filesystem>
#include <cstdlib>
#include <chrono>
#include <iostream>
#include <regex>

#include <raylib.h>

static std::string FilterEmojis(const std::string& text) {
    std::string filtered = text;
    
    filtered = std::regex_replace(filtered, std::regex("[\U0001F600-\U0001F64F]"), "");
    filtered = std::regex_replace(filtered, std::regex("[\U0001F300-\U0001F5FF]"), "");
    filtered = std::regex_replace(filtered, std::regex("[\U0001F680-\U0001F6FF]"), "");
    filtered = std::regex_replace(filtered, std::regex("[\U0001F1E0-\U0001F1FF]"), "");
    filtered = std::regex_replace(filtered, std::regex("[\U00002600-\U000027BF]"), "");
    filtered = std::regex_replace(filtered, std::regex("[\U0001F900-\U0001F9FF]"), "");
    
    filtered = std::regex_replace(filtered, std::regex("\\s+"), " ");
    
    size_t start = filtered.find_first_not_of(" \t");
    if (start == std::string::npos) return "";
    size_t end = filtered.find_last_not_of(" \t");
    return filtered.substr(start, end - start + 1);
}

DownloadSongs::DownloadSongs() {
}

DownloadSongs::~DownloadSongs() {
    for (auto& pair : coverCache) {
        if (pair.second.id > 0) {
            UnloadTexture(pair.second);
        }
    }
    
    for (auto& pair : fullQualityCache) {
        if (pair.second.id > 0) {
            bool isInRegularCache = false;
            for (const auto& regularPair : coverCache) {
                if (regularPair.second.id == pair.second.id) {
                    isInRegularCache = true;
                    break;
                }
            }
            if (!isInRegularCache) {
                UnloadTexture(pair.second);
            }
        }
    }
    
    coverCache.clear();
    fullQualityCache.clear();
    isPlaceholder.clear();
    downloadingCovers.clear();
}

void DownloadSongs::Load() {
    if (TheTrackDownloader.LoadTrackList()) {
        tracks = TheTrackDownloader.GetAllTracks();
        std::cout << "DownloadSongs: Loaded " << tracks.size() << " tracks" << std::endl;
        
        downloadingCovers.clear();
        isPlaceholder.clear();
        fullQualityCache.clear();
        
        if (!tracks.empty()) {
            std::cout << "DownloadSongs: Starting cover art downloads..." << std::endl;
            DownloadAllCovers();
        }
    } else {
        std::cout << "DownloadSongs: Failed to load track list" << std::endl;
    }
    
    selectedTrack = 0;
    hoveredTrack = -1;
    showDetails = false;
}

void DownloadSongs::Save() {
}

void DownloadSongs::Draw() {
    if (!IsWindowReady()) {
        return;
    }

    Units& u = Units::getInstance();
    Assets& assets = Assets::getInstance();

    if (TheSongList.curSong != nullptr) {
        GameMenu::DrawAlbumArtBackground(TheSongList.curSong->albumArtBlur);
    } else {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);
    }
    DrawRectangle(u.LeftSide, 0, u.winpct(1.0f), GetScreenHeight(), Color{0, 0, 0});
    encOS::DrawTopOvershell(0.15f);
    GameMenu::DrawVersion();
    GameMenu::DrawBottomOvershell();

    float TextPlacementTB = u.hpct(0.05f);
    float TextPlacementLR = u.wpct(0.05f);
    DrawTextEx(assets.rubik, "Settings", {TextPlacementLR, u.hpct(0.027f)}, u.hinpct(0.042f), 0, LIGHTGRAY);
    GameMenu::mhDrawText(assets.redHatDisplayBlack, "DOWNLOAD SONGS", {TextPlacementLR, TextPlacementTB}, u.hinpct(0.125f), WHITE, assets.sdfShader, LEFT);

    if (showDetails) {
        DrawTrackDetails();
    } else {
        DrawTrackGrid();
    }
}

void DownloadSongs::DrawTrackGrid() {
    Units& u = Units::getInstance();
    Assets& assets = Assets::getInstance();
    
    if (tracks.empty()) {
        Vector2 messagePos = {u.wpct(0.4f), u.hpct(0.4f)};
        DrawTextEx(assets.rubikBold, "Loading tracks...", messagePos, u.hinpct(0.04f), 0, WHITE);
        
        Vector2 infoPos = {u.wpct(0.4f), u.hpct(0.45f)};
        DrawTextEx(assets.rubik, "Press ESC to go back", infoPos, u.hinpct(0.025f), 0, LIGHTGRAY);
        return;
    }
    std::string trackCountText = "Tracks loaded: " + std::to_string(tracks.size());
    Vector2 textSize = MeasureTextEx(assets.rubik, trackCountText.c_str(), u.hinpct(0.03f), 0);
    Vector2 bottomCenterPos = {
        (GetScreenWidth() - textSize.x) / 2,
        GetScreenHeight() - textSize.y - u.hpct(0.05f)
    };
    DrawTextEx(assets.rubik, trackCountText.c_str(), bottomCenterPos, u.hinpct(0.03f), 0, WHITE);
    int gridCols = 4;
    float gridStartX = u.LeftSide + u.winpct(0.05f);
    float gridStartY = u.hpct(0.20f);
    float gridWidth = u.wpct(0.85f);
    float gridHeight = u.hpct(0.65f);
    
    float cellWidth = 180.0f;
    float cellHeight = 200.0f;
    float padding = 10.0f;
    int actualCols = std::max(1, (int)((gridWidth - padding) / (cellWidth + padding)));

    float wheelMove = GetMouseWheelMove();
    if (wheelMove != 0) {
        scrollOffset -= wheelMove * 50.0f;

        int totalRows = (tracks.size() + actualCols - 1) / actualCols;
        float totalContentHeight = totalRows * (cellHeight + padding);
        float maxScroll = std::max(0.0f, totalContentHeight - gridHeight);
        
        scrollOffset = std::max(0.0f, std::min(scrollOffset, maxScroll));
    }

    BeginScissorMode(gridStartX, gridStartY, gridWidth, gridHeight);
    
    Vector2 mousePos = GetMousePosition();
    hoveredTrack = -1;

    for (int i = 0; i < static_cast<int>(tracks.size()); i++) {
        int col = i % actualCols;
        int row = i / actualCols;
        
        float cellX = gridStartX + col * (cellWidth + padding) + padding;
        float cellY = gridStartY + row * (cellHeight + padding) + padding - scrollOffset;

        if (cellY + cellHeight < gridStartY || cellY > gridStartY + gridHeight) {
            continue;
        }
        
        Rectangle cellRect = {cellX, cellY, cellWidth, cellHeight};

        bool isHovered = CheckCollisionPointRec(mousePos, cellRect) && 
                        mousePos.y >= gridStartY && mousePos.y <= gridStartY + gridHeight;
        bool isSelected = (i == selectedTrack);
        
        if (isHovered) {
            hoveredTrack = i;
        }

        Color cellBg = isSelected ? Color{142, 13, 148, 150} : Color{31, 31, 50, 220};
        Color cellBorder = isHovered ? Color{142, 13, 148, 255} : Color{100, 100, 120, 255};
        float borderWidth = isSelected ? 4.0f : 2.0f;

        DrawRectangle(cellX, cellY, cellWidth, cellHeight, cellBg);
        DrawRectangleLinesEx(cellRect, borderWidth, cellBorder);

        if (isSelected) {
            DrawRectangleLinesEx({cellX + 2, cellY + 2, cellWidth - 4, cellHeight - 4}, 1.0f, Color{142, 13, 148, 100});
        }

        float coverSize = 100.0f;
        float coverX = cellX + (cellWidth - coverSize) / 2;
        float coverY = cellY + 10.0f;

        std::string coverName = tracks[i].key;
        if (!tracks[i].cover.empty()) {
            coverName = tracks[i].cover;
            if (coverName.length() > 4 && coverName.substr(coverName.length() - 4) == ".png") {
                coverName = coverName.substr(0, coverName.length() - 4);
            }
        }

        Texture2D coverTexture = LoadCoverFromUrl(coverName);
        
        if (coverTexture.id > 0) {
            DrawTexture(coverTexture, coverX, coverY, WHITE);
        } else {
            DrawRectangle(coverX, coverY, coverSize, coverSize, Color{80, 80, 120, 255});
            DrawRectangleGradientV(coverX, coverY, coverSize, coverSize, Color{100, 100, 140, 255}, Color{60, 60, 100, 255});

            float noteSize = 40.0f;
            float noteX = coverX + (coverSize - noteSize) / 2;
            float noteY = coverY + (coverSize - noteSize) / 2;
            DrawCircle(noteX + noteSize * 0.3f, noteY + noteSize * 0.7f, noteSize * 0.15f, WHITE);
            DrawRectangle(noteX + noteSize * 0.25f, noteY, noteSize * 0.1f, noteSize * 0.8f, WHITE);
        }

        DrawRectangleLinesEx({coverX, coverY, coverSize, coverSize}, 3.0f, WHITE);

        float textY = coverY + coverSize + 5.0f;
        float titleFontSize = u.hinpct(0.018f);
        float artistFontSize = u.hinpct(0.015f);

        std::string titleText = tracks[i].title;
        if (titleText.length() > 20) {
            titleText = titleText.substr(0, 17) + "...";
        }
        Vector2 titleSize = MeasureTextEx(assets.rubikBold, titleText.c_str(), titleFontSize, 0);
        float titleX = cellX + (cellWidth - titleSize.x) / 2;
        DrawTextEx(assets.rubikBold, titleText.c_str(), {titleX, textY}, titleFontSize, 0, WHITE);

        textY += titleSize.y + 2.0f;
        std::string artistText = tracks[i].artist;
        if (artistText.length() > 22) {
            artistText = artistText.substr(0, 19) + "...";
        }
        Vector2 artistSize = MeasureTextEx(assets.rubik, artistText.c_str(), artistFontSize, 0);
        float artistX = cellX + (cellWidth - artistSize.x) / 2;
        DrawTextEx(assets.rubik, artistText.c_str(), {artistX, textY}, artistFontSize, 0, LIGHTGRAY);

        if ((isHovered || isSelected) && tracks[i].isDownloaded) {
            textY += artistSize.y + 2.0f;
            const char* statusText = "Downloaded";
            Color statusColor = GREEN;
            Vector2 statusSize = MeasureTextEx(assets.rubik, statusText, artistFontSize, 0);
            float statusX = cellX + (cellWidth - statusSize.x) / 2;
            DrawTextEx(assets.rubik, statusText, {statusX, textY}, artistFontSize, 0, statusColor);
        }

        if (isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selectedTrack = i;
            showDetails = true;
            detailsTrack = i;
        }
    }
    
    EndScissorMode();

    if (tracks.size() > actualCols * 3) {
        float scrollBarX = gridStartX + gridWidth - 10.0f;
        float scrollBarY = gridStartY;
        float scrollBarHeight = gridHeight;

        DrawRectangle(scrollBarX, scrollBarY, 8.0f, scrollBarHeight, Color{60, 60, 80, 255});

        int totalRows = (tracks.size() + actualCols - 1) / actualCols;
        float totalContentHeight = totalRows * (cellHeight + padding);
        float maxScroll = std::max(1.0f, totalContentHeight - gridHeight);
        float thumbHeight = std::max(20.0f, scrollBarHeight * (gridHeight / totalContentHeight));
        float thumbY = scrollBarY + (scrollOffset / maxScroll) * (scrollBarHeight - thumbHeight);
        
        DrawRectangle(scrollBarX, thumbY, 8.0f, thumbHeight, Color{142, 13, 148, 255});
    }
}
void
 DownloadSongs::DrawTrackDetails() {
    Units& u = Units::getInstance();
    Assets& assets = Assets::getInstance();
    
    if (detailsTrack < 0 || detailsTrack >= (int)tracks.size()) {
        showDetails = false;
        return;
    }
    
    const auto& track = tracks[detailsTrack];

    float panelWidth = u.wpct(0.6f);
    float panelHeight = u.hpct(0.7f) - 100.0f;
    float panelX = u.LeftSide + (u.winpct(1.0f) - panelWidth) / 2;
    float panelY = (GetScreenHeight() - panelHeight) / 2;

    DrawRectangle(panelX - 10, panelY - 10, panelWidth + 20, panelHeight + 20, Color{142, 13, 148, 255});
    DrawRectangle(panelX, panelY, panelWidth, panelHeight, Color{31, 31, 50, 255});

    float albumArtSize = 370.0f;
    float albumArtX = panelX + panelWidth - albumArtSize - u.wpct(0.01f);
    float albumArtY = panelY + u.hpct(0.03f);

    std::string coverName = track.key;
    if (!track.cover.empty()) {
        coverName = track.cover;
        if (coverName.length() > 4 && coverName.substr(coverName.length() - 4) == ".png") {
            coverName = coverName.substr(0, coverName.length() - 4);
        }
    }

    Texture2D albumTexture = LoadFullQualityCover(coverName);
    if (albumTexture.id > 0) {
        DrawTextureEx(albumTexture, {albumArtX, albumArtY}, 0.0f, 370.0f / albumTexture.width, WHITE);
    } else {
        DrawRectangle(albumArtX, albumArtY, albumArtSize, albumArtSize, Color{80, 80, 120, 255});
        DrawRectangleGradientV(albumArtX, albumArtY, albumArtSize, albumArtSize, Color{100, 100, 140, 255}, Color{60, 60, 100, 255});
    }
    DrawRectangleLinesEx({albumArtX, albumArtY, albumArtSize, albumArtSize}, 3.0f, WHITE);

    float leftContentX = panelX + u.wpct(0.01f);
    float leftContentY = panelY + u.hpct(0.02f);
    float leftContentWidth = albumArtX - leftContentX - 15.0f;
    float lineHeight = u.hinpct(0.025f);

    DrawTextEx(assets.rubikBold, track.title.c_str(), {leftContentX, leftContentY}, u.hinpct(0.03f), 0, WHITE);
    leftContentY += lineHeight * 1.2f;

    DrawTextEx(assets.rubik, ("Artist: " + track.artist).c_str(), {leftContentX, leftContentY}, u.hinpct(0.022f), 0, LIGHTGRAY);
    leftContentY += lineHeight * 1.5f; // 5 lines below artist as requested

    DrawTextEx(assets.rubik, ("Release Year: " + track.releaseYear).c_str(), {leftContentX, leftContentY}, u.hinpct(0.02f), 0, LIGHTGRAY);
    leftContentY += lineHeight;

    DrawTextEx(assets.rubik, ("Duration: " + track.duration).c_str(), {leftContentX, leftContentY}, u.hinpct(0.02f), 0, LIGHTGRAY);
    leftContentY += lineHeight;

    std::string cleanVerification = FilterEmojis(track.verification);
    DrawTextEx(assets.rubik, ("Verification: " + cleanVerification).c_str(), {leftContentX, leftContentY}, u.hinpct(0.02f), 0, LIGHTGRAY);
    leftContentY += lineHeight;

    DrawTextEx(assets.rubik, ("Charter: " + track.charter).c_str(), {leftContentX, leftContentY}, u.hinpct(0.02f), 0, LIGHTGRAY);
    leftContentY += lineHeight;

    DrawTextEx(assets.rubik, ("Genre: " + track.genre).c_str(), {leftContentX, leftContentY}, u.hinpct(0.02f), 0, LIGHTGRAY);
    leftContentY += lineHeight;

    DrawTextEx(assets.rubik, ("Format: " + track.format).c_str(), {leftContentX, leftContentY}, u.hinpct(0.02f), 0, LIGHTGRAY);
    leftContentY += lineHeight;

    DrawTextEx(assets.rubik, ("BPM: " + track.bpm).c_str(), {leftContentX, leftContentY}, u.hinpct(0.02f), 0, LIGHTGRAY);
    leftContentY += lineHeight;

    DrawTextEx(assets.rubik, ("Key: " + track.trackKey).c_str(), {leftContentX, leftContentY}, u.hinpct(0.02f), 0, LIGHTGRAY);
    leftContentY += lineHeight;

    if (!track.loadingPhrase.empty()) {
        DrawTextEx(assets.rubik, ("Loading Phrase: " + track.loadingPhrase).c_str(), {leftContentX, leftContentY}, u.hinpct(0.02f), 0, LIGHTGRAY);
        leftContentY += lineHeight;
    }

    DrawTextEx(assets.rubik, ("Size: " + track.filesize).c_str(), {leftContentX, leftContentY}, u.hinpct(0.02f), 0, LIGHTGRAY);
    leftContentY += lineHeight;

    if (track.isDownloaded) {
        DrawTextEx(assets.rubik, "Status: Downloaded", {leftContentX, leftContentY}, u.hinpct(0.02f), 0, GREEN);
    }

    float buttonY = panelY + panelHeight - u.hpct(0.08f);
    float buttonWidth = u.wpct(0.12f);
    float buttonHeight = u.hpct(0.05f);

    if (!track.isDownloaded) {
        Rectangle downloadBtn = {leftContentX, buttonY, buttonWidth, buttonHeight};
        if (GuiButton(downloadBtn, "Download")) {
            TheTrackDownloader.DownloadTrack(track.key);
        }
    }


    Rectangle backBtn = {panelX + panelWidth - buttonWidth - u.wpct(0.02f), buttonY, buttonWidth, buttonHeight};
    if (GuiButton(backBtn, "Back")) {
        showDetails = false;
    }
}

void DownloadSongs::KeyboardInputCallback(int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;
    
    if (showDetails) {
        if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_B) {
            showDetails = false;
        }
        return;
    }

    float gridWidth = Units::getInstance().wpct(0.85f);
    float cellWidth = 180.0f;
    float padding = 10.0f;
    int gridCols = std::max(1, (int)((gridWidth - padding) / (cellWidth + padding)));
    
    int oldSelected = selectedTrack;
    
    switch (key) {
        case GLFW_KEY_ESCAPE:
            Save();
            TheMenuManager.SwitchScreen(MAIN_MENU);
            break;
            
        case GLFW_KEY_LEFT:
            if (selectedTrack > 0) selectedTrack--;
            break;
            
        case GLFW_KEY_RIGHT:
            if (selectedTrack < static_cast<int>(tracks.size()) - 1) selectedTrack++;
            break;
            
        case GLFW_KEY_UP:
            if (selectedTrack >= gridCols) selectedTrack -= gridCols;
            break;
            
        case GLFW_KEY_DOWN:
            if (selectedTrack + gridCols < static_cast<int>(tracks.size())) selectedTrack += gridCols;
            break;
            
        case GLFW_KEY_ENTER:
        case GLFW_KEY_SPACE:
            if (selectedTrack < static_cast<int>(tracks.size())) {
                showDetails = true;
                detailsTrack = selectedTrack;
            }
            break;
            
        case GLFW_KEY_PAGE_UP:
            scrollOffset = std::max(0.0f, scrollOffset - 200.0f);
            break;
            
        case GLFW_KEY_PAGE_DOWN:
            int totalRows = (tracks.size() + gridCols - 1) / gridCols;
            float totalContentHeight = totalRows * 210.0f;
            float gridHeight = Units::getInstance().hpct(0.65f);
            float maxScroll = std::max(0.0f, totalContentHeight - gridHeight);
            scrollOffset = std::min(maxScroll, scrollOffset + 200.0f);
            break;
    }

    if (selectedTrack != oldSelected) {
        int selectedRow = selectedTrack / gridCols;
        float cellHeight = 200.0f;
        float selectedY = selectedRow * 210.0f;
        float gridHeight = Units::getInstance().hpct(0.65f);

        if (selectedY < scrollOffset) {
            scrollOffset = selectedY;
        }

        else if (selectedY + cellHeight > scrollOffset + gridHeight) {
            scrollOffset = selectedY + cellHeight - gridHeight;
        }
    }
}

void DownloadSongs::ControllerInputCallback(int joypadID, GLFWgamepadstate state) {
    if (state.buttons[GLFW_GAMEPAD_BUTTON_B] == GLFW_PRESS) {
        if (showDetails) {
            showDetails = false;
        } else {
            Save();
            TheMenuManager.SwitchScreen(MAIN_MENU);
        }
    }
    
    if (state.buttons[GLFW_GAMEPAD_BUTTON_A] == GLFW_PRESS) {
        if (!showDetails && selectedTrack < static_cast<int>(tracks.size())) {
            showDetails = true;
            detailsTrack = selectedTrack;
        }
    }
}

void DownloadSongs::LoadCoverArt(const std::string& coverUrl) {
}

std::string DownloadSongs::GetCoverCachePath(const std::string& coverName) {
    std::filesystem::path cacheDir = std::filesystem::current_path() / "cache" / "covers";
    std::filesystem::create_directories(cacheDir);
    
    return (cacheDir / (coverName + ".png")).string();
}

bool DownloadSongs::DownloadCoverImage(const std::string& coverName, const std::string& outputPath) {
    std::string coverUrl = "https://github.com/Encore-Developers/EncoreCustoms/blob/main/assets/covers/" + coverName + ".png?raw=true";

    std::string command = "curl -L \"" + coverUrl + "\" -o \"" + outputPath + "\"";
    int result = system(command.c_str());
    return result == 0;
}

void DownloadSongs::DownloadAllCovers() {
    std::thread([this]() {
        std::cout << "Starting fast cover art download for " << tracks.size() << " tracks..." << std::endl;
        
        std::vector<std::thread> downloadThreads;
        const int maxThreads = 8;
        int threadCount = 0;
        
        for (const auto& track : tracks) {
            std::string coverName = track.key;

            if (!track.cover.empty()) {
                coverName = track.cover;

                if (coverName.length() > 4 && coverName.substr(coverName.length() - 4) == ".png") {
                    coverName = coverName.substr(0, coverName.length() - 4);
                }
            }

            if (downloadingCovers[coverName]) {
                continue;
            }
            
            std::string cachePath = GetCoverCachePath(coverName);
            if (std::filesystem::exists(cachePath)) {
                continue;
            }

            downloadingCovers[coverName] = true;
            
            downloadThreads.emplace_back([this, coverName, cachePath]() {
                bool success = DownloadCoverImage(coverName, cachePath);
                if (success) {
                    std::cout << "Downloaded: " << coverName << std::endl;
                }
                downloadingCovers[coverName] = false;
            });
            
            threadCount++;

            if (threadCount >= maxThreads) {
                for (auto& t : downloadThreads) {
                    if (t.joinable()) {
                        t.join();
                    }
                }
                downloadThreads.clear();
                threadCount = 0;
            }
        }

        for (auto& t : downloadThreads) {
            if (t.joinable()) {
                t.join();
            }
        }
        
        std::cout << "Fast cover download complete!" << std::endl;
    }).detach();
}

Texture2D DownloadSongs::LoadCoverFromUrl(const std::string& coverName) {
    std::string cachePath = GetCoverCachePath(coverName);
    bool fileExistsOnDisk = std::filesystem::exists(cachePath);

    auto it = coverCache.find(coverName);
    if (it != coverCache.end()) {
        if (isPlaceholder[coverName] && fileExistsOnDisk) {
            std::cout << "Reloading real cover for: " << coverName << std::endl;
            UnloadTexture(it->second);
            coverCache.erase(coverName);
            isPlaceholder.erase(coverName);
        } else {
            return it->second;
        }
    }

    if (fileExistsOnDisk) {
        Image coverImage = LoadImage(cachePath.c_str());
        
        if (coverImage.data != nullptr) {
            std::cout << "Loaded real cover from disk: " << coverName << std::endl;

            if (coverImage.width != 100 || coverImage.height != 100) {
                ImageResize(&coverImage, 100, 100);
            }
            
            Texture2D texture = LoadTextureFromImage(coverImage);
            UnloadImage(coverImage);

            coverCache[coverName] = texture;
            isPlaceholder[coverName] = false;
            return texture;
        }
    }

    Image placeholder = GenImageColor(100, 100, Color{80, 80, 120, 255});

    int hash = 0;
    for (char c : coverName) {
        hash += c;
    }
    Color accentColor = Color{
        (unsigned char)(100 + (hash * 37) % 155),
        (unsigned char)(100 + (hash * 73) % 155), 
        (unsigned char)(100 + (hash * 109) % 155),
        255
    };
    
    ImageDrawRectangle(&placeholder, 10, 10, 80, 80, accentColor);
    ImageDrawRectangleLines(&placeholder, {5, 5, 90, 90}, 2, WHITE);

    
    Texture2D texture = LoadTextureFromImage(placeholder);
    UnloadImage(placeholder);

    coverCache[coverName] = texture;
    isPlaceholder[coverName] = true;
    
    return texture;
}

Texture2D DownloadSongs::LoadFullQualityCover(const std::string& coverName) {
    auto it = fullQualityCache.find(coverName);
    if (it != fullQualityCache.end()) {
        return it->second;
    }
    
    std::string cachePath = GetCoverCachePath(coverName);

    if (std::filesystem::exists(cachePath)) {
        Image coverImage = LoadImage(cachePath.c_str());
        
        if (coverImage.data != nullptr) {
            Texture2D texture = LoadTextureFromImage(coverImage);
            UnloadImage(coverImage);

            fullQualityCache[coverName] = texture;
            return texture;
        }
    }

    Texture2D fallbackTexture = LoadCoverFromUrl(coverName);

    if (fallbackTexture.id > 0) {
        fullQualityCache[coverName] = fallbackTexture;
    }
    
    return fallbackTexture;
}