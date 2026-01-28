//
// Created by marie on 16/11/2024.
//

#ifndef SONGSELECTMENU_H
#define SONGSELECTMENU_H
#include "OvershellMenu.h"
#include "uiUnits.h"
#include "song/song.h"
#include "settings.h"
#include <filesystem>
#include <map>

enum class ScoreInstrumentFilter {
    Vocals = 4,
    Bass = 1,
    Lead = 2,
    Drums = 0
};

class SongSelectMenu : public OvershellMenu {
public:
    SongSelectMenu() = default;
    ~SongSelectMenu() override;
    void KeyboardInputCallback(int key, int scancode, int action, int mods) override;
    void ControllerInputCallback(int joypadID, GLFWgamepadstate state) override;
    void Draw() override;
    void Load() override;
    void Unload();
    void UpdatePreviewVolume(double currentTime);

private:
    std::filesystem::path directory = GetPrevDirectoryPath(GetApplicationDirectory());
    std::filesystem::path getDirectory() const {
        return directory;
    }
    
    ScoreInstrumentFilter GetScoreInstrumentFilter() const {
        int val = TheGameSettings.ScoreInstrumentFilter;
        if (val == 4 || val == 1 || val == 2 || val == 0) {
            return static_cast<ScoreInstrumentFilter>(val);
        }
        return ScoreInstrumentFilter::Vocals; 
    }
    
    void CycleScoreInstrumentFilter() {
        ScoreInstrumentFilter current = GetScoreInstrumentFilter();
        ScoreInstrumentFilter next;
        switch (current) {
            case ScoreInstrumentFilter::Vocals: next = ScoreInstrumentFilter::Bass; break;
            case ScoreInstrumentFilter::Bass: next = ScoreInstrumentFilter::Lead; break;
            case ScoreInstrumentFilter::Lead: next = ScoreInstrumentFilter::Drums; break;
            case ScoreInstrumentFilter::Drums: next = ScoreInstrumentFilter::Vocals; break;
            default: next = ScoreInstrumentFilter::Vocals; break;
        }
        TheGameSettings.ScoreInstrumentFilter = static_cast<int>(next);
    }
    
    int GetScoreInstrumentIconIndex() const {
        return static_cast<int>(GetScoreInstrumentFilter());
    }
    
    const char* GetScoreInstrumentName() const {
        switch (GetScoreInstrumentFilter()) {
            case ScoreInstrumentFilter::Vocals: return "Vocals";
            case ScoreInstrumentFilter::Bass: return "Bass";
            case ScoreInstrumentFilter::Lead: return "Lead";
            case ScoreInstrumentFilter::Drums: return "Drums";
            default: return "Unknown";
        }
    }
    double previewStartTime = 0.0;
    float currentPreviewVolume = 0.0f;
    enum class PreviewState { FadeIn, Playing, FadeOut, Pause } previewState = PreviewState::FadeIn;
    const float fadeDuration = 2.5f;
    const float previewPlayDuration = 30.0f;
    const float pauseDuration = 2.5f;
    double phaseStartTime = 0.0;
    int animatingSongID = -1;
    int prevAnimatingSongID = -1;
    double animationStartTime = 0.0;
    const float animationDuration = 0.5f;
    int pendingSongID = -1;
    double selectionTime = 0.0;
    double seekPendingTime = -1.0;
    struct TextMetrics {
        float titleFontSize;
        float artistFontSize;
        float titleTextWidth;
        float artistTextWidth;
        bool titleNeedsScroll;
        bool artistNeedsScroll;
    };
    std::map<int, TextMetrics> songTextMetrics;
    std::map<int, double> scrollStartTimes;
    std::map<int, bool> scrollDirections;
    bool isDraggingScrollbar = false;

    void ComputeSongTextMetrics(Song& song);
    float GetScrollOffset(int songID, float textWidth, float maxWidth, double currentTime, bool isTitle, bool isSelected);
    static void DrawAlbumArtBackgroundPro(const Texture2D& texture, const Rectangle sourceRect) {
        Units u = Units::getInstance();
        if (IsTextureValid(texture)) {
            float diagonalLength = sqrtf((float)(GetScreenWidth() * GetScreenWidth()) + (float)(GetScreenHeight() * GetScreenHeight()));
            float RectXPos = GetScreenWidth() / 2;
            float RectYPos = diagonalLength / 2;
            DrawTexturePro(
                texture,
                sourceRect,
                { RectXPos, -RectYPos * 2, diagonalLength * 2, diagonalLength * 2 },
                { 0, 0 },
                45,
                Color{255, 255, 255, 128}
            );
        }
    }
};

#endif //SONGSELECTMENU_H
