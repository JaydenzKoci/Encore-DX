//
// Created by Jaydenz on 10/2/2025.
//

#include "downloadOverlay.h"
#include "util/trackDownloader.h"
#include "assets.h"
#include "menus/uiUnits.h"
#include <raylib.h>

void Encore::DownloadOverlay::Draw() {
    if (!TheTrackDownloader.IsDownloading() && TheTrackDownloader.GetDownloadStatus() == "Ready") {
        return;
    }
    
    Units& u = Units::getInstance();
    Assets& assets = Assets::getInstance();

    float overlayWidth = u.wpct(0.25f);
    float overlayHeight = u.hpct(0.12f);
    float overlayX = GetScreenWidth() - overlayWidth - u.wpct(0.02f);
    float overlayY = u.hpct(0.02f);

    Color bgColor = Color{31, 31, 50, 240};
    Color borderColor = Color{142, 13, 148, 255};
    float borderWidth = 2.0f;
    
    DrawRectangle(overlayX - borderWidth, overlayY - borderWidth, 
                  overlayWidth + 2 * borderWidth, overlayHeight + 2 * borderWidth, borderColor);
    DrawRectangle(overlayX, overlayY, overlayWidth, overlayHeight, bgColor);

    float coverSize = 100.0f;
    float coverX = overlayX - coverSize - u.wpct(0.01f);
    float coverY = overlayY + u.hpct(0.02f);
    
    DrawRectangle(coverX, coverY, coverSize, coverSize, Color{60, 60, 80, 255});
    DrawRectangleLinesEx({coverX, coverY, coverSize, coverSize}, 2.0f, borderColor);

    float noteSize = 40.0f;
    float noteX = coverX + (coverSize - noteSize) / 2;
    float noteY = coverY + (coverSize - noteSize) / 2;
    DrawCircle(noteX + noteSize * 0.3f, noteY + noteSize * 0.7f, noteSize * 0.15f, WHITE);
    DrawRectangle(noteX + noteSize * 0.25f, noteY, noteSize * 0.1f, noteSize * 0.8f, WHITE);

    float titleFontSize = u.hinpct(0.025f);
    Vector2 titlePos = {overlayX + u.wpct(0.01f), overlayY + u.hpct(0.01f)};
    DrawTextEx(assets.rubikBold, "Tracks Downloading...", titlePos, titleFontSize, 0, WHITE);

    float statusFontSize = u.hinpct(0.02f);
    Vector2 statusPos = {overlayX + u.wpct(0.01f), overlayY + u.hpct(0.04f)};
    std::string statusText = TheTrackDownloader.GetDownloadStatus();
    DrawTextEx(assets.rubik, statusText.c_str(), statusPos, statusFontSize, 0, LIGHTGRAY);

    if (TheTrackDownloader.IsDownloading()) {
        float progressBarY = overlayY + u.hpct(0.07f);
        float progressBarWidth = overlayWidth - u.wpct(0.02f);
        float progressBarHeight = u.hpct(0.015f);
        float progressBarX = overlayX + u.wpct(0.01f);

        DrawRectangle(progressBarX, progressBarY, progressBarWidth, progressBarHeight, Color{60, 60, 80, 255});
        
        float progress = TheTrackDownloader.GetDownloadProgress();
        float fillWidth = progressBarWidth * progress;
        DrawRectangle(progressBarX, progressBarY, fillWidth, progressBarHeight, Color{142, 13, 148, 255});

        float progressTextY = progressBarY + progressBarHeight + u.hpct(0.005f);
        std::string progressText = std::to_string(TheTrackDownloader.GetCompletedCount()) + 
                                  "/" + std::to_string(TheTrackDownloader.GetTotalCount()) + 
                                  " (" + std::to_string(int(progress * 100)) + "%)";
        DrawTextEx(assets.rubik, progressText.c_str(), 
                  {progressBarX, progressTextY}, u.hinpct(0.018f), 0, WHITE);
    }
}