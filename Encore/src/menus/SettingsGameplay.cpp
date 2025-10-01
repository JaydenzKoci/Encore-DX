#include "SettingsGameplay.h"

#include "MenuManager.h"
#include "gameMenu.h"
#include "raygui.h"
#include "assets.h"
#include "settings.h"
#include "settingsOptionRenderer.h"
#include "uiUnits.h"
#include "gameplay/enctime.h"
#include "OvershellMenu.h"
#include "util/settings-text.h"



bool ShowGameplaySettings = true;

void SettingsGameplay::Draw() {
    if (!IsWindowReady()) {
        return;
    }

    Units& u = Units::getInstance();
    if (&u == nullptr) {
        return;
    }

    Assets& assets = Assets::getInstance();
    if (&assets == nullptr) {
        return;
    }

    SettingsOld& settingsMain = SettingsOld::getInstance();
    if (&settingsMain == nullptr) {
        return;
    }

    SongTime& enctime = TheSongTime;
    if (&enctime == nullptr) {
    }

    settingsOptionRenderer sor;
    const float boxWidthPct = 0.55f;

    if (TheSongList.curSong != nullptr) {
        GameMenu::DrawAlbumArtBackground(TheSongList.curSong->albumArtBlur);
    } else {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);
    }
    DrawRectangle(u.LeftSide, 0, u.winpct(1.0f), GetScreenHeight(), Color{0, 0, 0});

    float SidebarLeft = u.LeftSide + u.winpct(0.70f);
    float SidebarWidth = u.wpct(0.235f);
    float SidebarTop = u.hinpct(0.15f);
    float SidebarHeight = u.hpct(0.85f);
    float SidebarHeaderHeight = u.hinpct(0.10f);
    float borderWidth = u.winpct(0.002f);
    float innerTop = SidebarTop + borderWidth;

    DrawLineEx({SidebarLeft - borderWidth, SidebarTop}, {SidebarLeft - borderWidth, SidebarTop + SidebarHeight}, borderWidth, WHITE);
    DrawLineEx({SidebarLeft + SidebarWidth, SidebarTop}, {SidebarLeft + SidebarWidth, SidebarTop + SidebarHeight}, borderWidth, WHITE);
    DrawLineEx({SidebarLeft - borderWidth, SidebarTop + SidebarHeight}, {SidebarLeft + SidebarWidth + borderWidth, SidebarTop + SidebarHeight}, borderWidth, WHITE);
    DrawRectangle(SidebarLeft, SidebarTop, SidebarWidth, SidebarHeight, Color{31, 31, 50, 255});

    struct SidebarContent {
        const char* header;
        const char* body;
    };
    SidebarContent sidebarContents[] = {
        // sidebar text
        // fullscreen
        {
            "Fullscreen",
            "TBD"
        },
        // scan Songs
        {
            "Scan Songs",
            "TBD"
        },

        {
            "Hit Window",
            "TBD"
        },
        {
            "Health",
            "TBD"
        },
        {
            "FPS Counter",
            "TBD"
        },
        {
            "Version Info",
            "TBD"
        },
        {
            "HUD Position",
            "TBD"
        }
    };

    static int selectedIndex = 0;
    Vector2 mousePos = GetMousePosition();
    bool isHovering = false;

    const char* headerText = sidebarContents[selectedIndex].header;
    const char* sidebarBodyText = sidebarContents[selectedIndex].body;
    float headerFontSize = u.hinpct(0.030f);
    float headerLineSpacing = headerFontSize * 1.2f;
    std::vector<std::string> headerLines = split(headerText, "\n");
    float maxHeaderWidth = 0;
    for (const std::string& line : headerLines) {
        Vector2 lineSize = MeasureTextEx(assets.rubikBold, line.c_str(), headerFontSize, 0);
        if (lineSize.x > maxHeaderWidth) {
            maxHeaderWidth = lineSize.x;
        }
    }
    float currentHeaderY = innerTop + u.hinpct(0.02f);
    for (const std::string& line : headerLines) {
        float lineX = SidebarLeft + (SidebarWidth - maxHeaderWidth) / 2;
        DrawTextEx(assets.rubikBold, line.c_str(), {lineX, currentHeaderY}, headerFontSize, 0, WHITE);
        currentHeaderY += headerLineSpacing;
    }
    float bodyFontSize = u.hinpct(0.030f);
    float lineSpacing = bodyFontSize * 1.2f;
    std::vector<std::string> lines = split(sidebarBodyText, "\n");
    float currentY = SidebarTop + SidebarHeaderHeight + u.hinpct(0.02f);
    for (const std::string& line : lines) {
        Vector2 lineSize = MeasureTextEx(assets.rubik, line.c_str(), bodyFontSize, 0);
        float lineX = SidebarLeft + (SidebarWidth - lineSize.x) / 2;
        DrawTextEx(assets.rubik, line.c_str(), {lineX, currentY}, bodyFontSize, 0, WHITE);
        currentY += lineSpacing;
    }

    encOS::DrawTopOvershell(0.15f);
    GameMenu::DrawVersion();
    GameMenu::DrawBottomOvershell();
    DrawOvershell();

    float TextPlacementTB = u.hpct(0.05f);
    float TextPlacementLR = u.wpct(0.05f);
    DrawTextEx(assets.rubik, "Settings", {TextPlacementLR, u.hpct(0.027f)}, u.hinpct(0.042f), 0, LIGHTGRAY);
    GameMenu::mhDrawText(assets.redHatDisplayBlack, "GAMEPLAY", {TextPlacementLR, TextPlacementTB}, u.hinpct(0.125f), WHITE, assets.sdfShader, LEFT);

    float settingsOffsetX = 0.0f;
    float settingsOffsetY = 0.0f;
    float EntryFontSize = u.hinpct(0.03f);
    float EntryHeight = u.hinpct(0.06f) + 30.0f - 50.0f + 10.0f + 7.0f;
    float EntryTop = TextPlacementTB + u.hinpct(0.125f) + u.hinpct(0.01f) + settingsOffsetY - 30.0f - 2.0f - 2.0f;
    float verticalGap = 0.0f;
    float boxLeft = u.LeftSide + u.winpct(0.025f) + settingsOffsetX + 75.0f - 50.0f - 2.0f;
    float boxWidth = u.wpct(boxWidthPct) + 50.0f + 17.0f + 7.0f - 2.0f - 2.0f;
    float OptionLeft = boxLeft;
    float OptionWidth = boxWidth;
    Color boxBackground = Color{31, 31, 50, 255};
    Color boxBorder = WHITE;
    Color glowColor = Color{142, 13, 148, 220};
    float highlightBorderWidth = 4.0f;

    float scanButtonWidth = OptionWidth;
    float scanButtonHeight = EntryHeight + 10.0f;
    float toggleButtonWidth = ((OptionWidth / 2) * 0.3f) - 30.0f;
    float toggleOffset = 50.0f;

    Color activeColor = Color{255, 105, 180, 255};
    int defaultColor = GuiGetStyle(BUTTON, BASE_COLOR_PRESSED);

    int settingOffset = 0;
    float fullscreenTop = EntryTop + (EntryHeight + verticalGap) * settingOffset;
    Rectangle fullscreenBoxRect = {boxLeft - borderWidth, fullscreenTop - borderWidth, boxWidth + 2 * borderWidth, EntryHeight + 2 * borderWidth};
    DrawRectangle(boxLeft - borderWidth, fullscreenTop - borderWidth, boxWidth + 2 * borderWidth, EntryHeight + 2 * borderWidth, boxBorder);
    DrawRectangle(boxLeft, fullscreenTop, boxWidth, EntryHeight, boxBackground);
    Vector2 fullscreenTextSize = MeasureTextEx(assets.rubikBold, "Fullscreen", EntryFontSize, 0);
    DrawTextEx(assets.rubikBold, "Fullscreen", {boxLeft + u.winpct(0.01f), fullscreenTop + (EntryHeight - fullscreenTextSize.y) / 2}, EntryFontSize, 0, WHITE);
    Rectangle offButtonRect = {OptionLeft + OptionWidth - 2 * toggleButtonWidth - toggleOffset, fullscreenTop, toggleButtonWidth, EntryHeight};
    Rectangle onButtonRect = {OptionLeft + OptionWidth - toggleButtonWidth - toggleOffset, fullscreenTop, toggleButtonWidth, EntryHeight};
    if (CheckCollisionPointRec(mousePos, offButtonRect) || CheckCollisionPointRec(mousePos, onButtonRect)) {
        selectedIndex = 0;
        isHovering = true;
        DrawRectangleLinesEx(fullscreenBoxRect, highlightBorderWidth, glowColor);
    }
    // might redo later - Jaydenz
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, settingsMain.fullscreen ? defaultColor : ColorToInt(activeColor));
    if (GuiButton(offButtonRect, "Off")) {
        if (settingsMain.fullscreen) {
            settingsMain.fullscreen = false;
            if (IsWindowFullscreen()) {
                ToggleFullscreen();
            }
            settingsMain.saveSettings(settingsMain.getDirectory() / "settings.json");
        }
    }
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, settingsMain.fullscreen ? ColorToInt(activeColor) : defaultColor);
    if (GuiButton(onButtonRect, "On")) {
        if (!settingsMain.fullscreen) {
            settingsMain.fullscreen = true;
            if (!IsWindowFullscreen()) {
                ToggleFullscreen();
            }
            settingsMain.saveSettings(settingsMain.getDirectory() / "settings.json");
        }
    }
    if (!settingsMain.fullscreen) {
        DrawRectangleLinesEx(offButtonRect, highlightBorderWidth, glowColor);
    } else {
        DrawRectangleLinesEx(onButtonRect, highlightBorderWidth, glowColor);
    }
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, defaultColor);

    settingOffset++;
    float scanSongsTop = EntryTop + (EntryHeight + verticalGap) * settingOffset;
    Rectangle scanSongsBoxRect = {boxLeft - borderWidth, scanSongsTop - borderWidth, boxWidth + 2 * borderWidth, scanButtonHeight + 2 * borderWidth};
    DrawRectangle(boxLeft - borderWidth, scanSongsTop - borderWidth, boxWidth + 2 * borderWidth, scanButtonHeight + 2 * borderWidth, boxBorder);
    DrawRectangle(boxLeft, scanSongsTop, boxWidth, scanButtonHeight, boxBackground);
    Vector2 scanSongsTextSize = MeasureTextEx(assets.rubikBold, "Scan Songs", EntryFontSize, 0);
    DrawTextEx(assets.rubikBold, "Scan Songs", {boxLeft + u.winpct(0.01f), scanSongsTop + (scanButtonHeight - scanSongsTextSize.y) / 2}, EntryFontSize, 0, WHITE);
    Rectangle scanButtonRect = {OptionLeft + OptionWidth - scanButtonWidth, scanSongsTop, scanButtonWidth, scanButtonHeight};
    if (CheckCollisionPointRec(mousePos, scanButtonRect)) {
        selectedIndex = 1;
        isHovering = true;
        DrawRectangleLinesEx(scanSongsBoxRect, highlightBorderWidth, glowColor);
    }
    if (GuiButton(scanButtonRect, "Scan Songs")) {
        if (TheGameSettings.SongPaths.empty()) {
            TraceLog(LOG_ERROR, "SongPaths is empty. Cannot scan songs.");
        } else {
            try {
                TraceLog(LOG_INFO, "Starting song scan with %d paths", TheGameSettings.SongPaths.size());
                for (const auto& path : TheGameSettings.SongPaths) {
                    TraceLog(LOG_INFO, "Scanning path: %s", path.c_str());
                }
                TheSongList.ScanSongs(TheGameSettings.SongPaths);
                TraceLog(LOG_INFO, "Song scan completed successfully");
            } catch (const std::exception& e) {
                TraceLog(LOG_ERROR, "Error during song scan: %s", e.what());
            } catch (...) {
                TraceLog(LOG_ERROR, "Unknown error during song scan");
            }
        }
    }



    settingOffset++;
    float hideHitWindowTop = EntryTop + (EntryHeight + verticalGap) * settingOffset;
    Rectangle hideHitWindowBoxRect = {boxLeft - borderWidth, hideHitWindowTop - borderWidth, boxWidth + 2 * borderWidth, EntryHeight + 2 * borderWidth};
    DrawRectangle(boxLeft - borderWidth, hideHitWindowTop - borderWidth, boxWidth + 2 * borderWidth, EntryHeight + 2 * borderWidth, boxBorder);
    DrawRectangle(boxLeft, hideHitWindowTop, boxWidth, EntryHeight, boxBackground);
    Vector2 hideHitWindowTextSize = MeasureTextEx(assets.rubikBold, "Hit Window", EntryFontSize, 0);
    DrawTextEx(assets.rubikBold, "Hit Window", {boxLeft + u.winpct(0.01f), hideHitWindowTop + (EntryHeight - hideHitWindowTextSize.y) / 2}, EntryFontSize, 0, WHITE);
    Rectangle hitWindowOffButtonRect = {OptionLeft + OptionWidth - 2 * toggleButtonWidth - toggleOffset, hideHitWindowTop, toggleButtonWidth, EntryHeight};
    Rectangle hitWindowOnButtonRect = {OptionLeft + OptionWidth - toggleButtonWidth - toggleOffset, hideHitWindowTop, toggleButtonWidth, EntryHeight};
    if (CheckCollisionPointRec(mousePos, hitWindowOffButtonRect) || CheckCollisionPointRec(mousePos, hitWindowOnButtonRect)) {
        selectedIndex = 2;
        isHovering = true;
        DrawRectangleLinesEx(hideHitWindowBoxRect, highlightBorderWidth, glowColor);
    }
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, !TheGameSettings.HideHitWindow ? defaultColor : ColorToInt(activeColor));
    if (GuiButton(hitWindowOffButtonRect, "Off")) {
        if (!TheGameSettings.HideHitWindow) {
            TheGameSettings.HideHitWindow = true;
            TheGameSettings.SaveToFile((settingsMain.getDirectory() / "settings.json").string());
        }
    }
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, !TheGameSettings.HideHitWindow ? ColorToInt(activeColor) : defaultColor);
    if (GuiButton(hitWindowOnButtonRect, "On")) {
        if (TheGameSettings.HideHitWindow) {
            TheGameSettings.HideHitWindow = false;
            TheGameSettings.SaveToFile((settingsMain.getDirectory() / "settings.json").string());
        }
    }
    if (TheGameSettings.HideHitWindow) {
        DrawRectangleLinesEx(hitWindowOffButtonRect, highlightBorderWidth, glowColor);
    } else {
        DrawRectangleLinesEx(hitWindowOnButtonRect, highlightBorderWidth, glowColor);
    }
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, defaultColor);

    settingOffset++;
    float showHealthBarTop = EntryTop + (EntryHeight + verticalGap) * settingOffset;
    Rectangle showHealthBarBoxRect = {boxLeft - borderWidth, showHealthBarTop - borderWidth, boxWidth + 2 * borderWidth, EntryHeight + 2 * borderWidth};
    DrawRectangle(boxLeft - borderWidth, showHealthBarTop - borderWidth, boxWidth + 2 * borderWidth, EntryHeight + 2 * borderWidth, boxBorder);
    DrawRectangle(boxLeft, showHealthBarTop, boxWidth, EntryHeight, boxBackground);
    Vector2 showHealthBarTextSize = MeasureTextEx(assets.rubikBold, "Health", EntryFontSize, 0);
    DrawTextEx(assets.rubikBold, "Health", {boxLeft + u.winpct(0.01f), showHealthBarTop + (EntryHeight - showHealthBarTextSize.y) / 2}, EntryFontSize, 0, WHITE);
    Rectangle healthBarOffButtonRect = {OptionLeft + OptionWidth - 2 * toggleButtonWidth - toggleOffset, showHealthBarTop, toggleButtonWidth, EntryHeight};
    Rectangle healthBarOnButtonRect = {OptionLeft + OptionWidth - toggleButtonWidth - toggleOffset, showHealthBarTop, toggleButtonWidth, EntryHeight};
    if (CheckCollisionPointRec(mousePos, healthBarOffButtonRect) || CheckCollisionPointRec(mousePos, healthBarOnButtonRect)) {
        selectedIndex = 3;
        isHovering = true;
        DrawRectangleLinesEx(showHealthBarBoxRect, highlightBorderWidth, glowColor);
    }
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, TheGameSettings.ShowHealthBar ? ColorToInt(activeColor) : defaultColor);
    if (GuiButton(healthBarOffButtonRect, "Off")) {
        if (TheGameSettings.ShowHealthBar) {
            TheGameSettings.ShowHealthBar = false;
            TheGameSettings.SaveToFile((settingsMain.getDirectory() / "settings.json").string());
        }
    }
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, TheGameSettings.ShowHealthBar ? ColorToInt(activeColor) : defaultColor);
    if (GuiButton(healthBarOnButtonRect, "On")) {
        if (!TheGameSettings.ShowHealthBar) {
            TheGameSettings.ShowHealthBar = true;
            TheGameSettings.SaveToFile((settingsMain.getDirectory() / "settings.json").string());
        }
    }
    if (!TheGameSettings.ShowHealthBar) {
        DrawRectangleLinesEx(healthBarOffButtonRect, highlightBorderWidth, glowColor);
    } else {
        DrawRectangleLinesEx(healthBarOnButtonRect, highlightBorderWidth, glowColor);
    }
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, defaultColor);

    settingOffset++;
    float hideFPSTop = EntryTop + (EntryHeight + verticalGap) * settingOffset;
    Rectangle hideFPSBoxRect = {boxLeft - borderWidth, hideFPSTop - borderWidth, boxWidth + 2 * borderWidth, EntryHeight + 2 * borderWidth};
    DrawRectangle(boxLeft - borderWidth, hideFPSTop - borderWidth, boxWidth + 2 * borderWidth, EntryHeight + 2 * borderWidth, boxBorder);
    DrawRectangle(boxLeft, hideFPSTop, boxWidth, EntryHeight, boxBackground);
    Vector2 hideFPSTextSize = MeasureTextEx(assets.rubikBold, "FPS Counter", EntryFontSize, 0);
    DrawTextEx(assets.rubikBold, "FPS Counter", {boxLeft + u.winpct(0.01f), hideFPSTop + (EntryHeight - hideFPSTextSize.y) / 2}, EntryFontSize, 0, WHITE);
    Rectangle fpsOffButtonRect = {OptionLeft + OptionWidth - 2 * toggleButtonWidth - toggleOffset, hideFPSTop, toggleButtonWidth, EntryHeight};
    Rectangle fpsOnButtonRect = {OptionLeft + OptionWidth - toggleButtonWidth - toggleOffset, hideFPSTop, toggleButtonWidth, EntryHeight};
    if (CheckCollisionPointRec(mousePos, fpsOffButtonRect) || CheckCollisionPointRec(mousePos, fpsOnButtonRect)) {
        selectedIndex = 4;
        isHovering = true;
        DrawRectangleLinesEx(hideFPSBoxRect, highlightBorderWidth, glowColor);
    }
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, !TheGameSettings.HideFPSCounter ? defaultColor : ColorToInt(activeColor));
    if (GuiButton(fpsOffButtonRect, "Off")) {
        if (!TheGameSettings.HideFPSCounter) {
            TheGameSettings.HideFPSCounter = true;
            TheGameSettings.SaveToFile((settingsMain.getDirectory() / "settings.json").string());
        }
    }
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, !TheGameSettings.HideFPSCounter ? ColorToInt(activeColor) : defaultColor);
    if (GuiButton(fpsOnButtonRect, "On")) {
        if (TheGameSettings.HideFPSCounter) {
            TheGameSettings.HideFPSCounter = false;
            TheGameSettings.SaveToFile((settingsMain.getDirectory() / "settings.json").string());
        }
    }
    if (TheGameSettings.HideFPSCounter) {
        DrawRectangleLinesEx(fpsOffButtonRect, highlightBorderWidth, glowColor);
    } else {
        DrawRectangleLinesEx(fpsOnButtonRect, highlightBorderWidth, glowColor);
    }
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, defaultColor);

    settingOffset++;
    float hideVersionTop = EntryTop + (EntryHeight + verticalGap) * settingOffset;
    Rectangle hideVersionBoxRect = {boxLeft - borderWidth, hideVersionTop - borderWidth, boxWidth + 2 * borderWidth, EntryHeight + 2 * borderWidth};
    DrawRectangle(boxLeft - borderWidth, hideVersionTop - borderWidth, boxWidth + 2 * borderWidth, EntryHeight + 2 * borderWidth, boxBorder);
    DrawRectangle(boxLeft, hideVersionTop, boxWidth, EntryHeight, boxBackground);
    Vector2 hideVersionTextSize = MeasureTextEx(assets.rubikBold, "Version Info", EntryFontSize, 0);
    DrawTextEx(assets.rubikBold, "Version Info", {boxLeft + u.winpct(0.01f), hideVersionTop + (EntryHeight - hideVersionTextSize.y) / 2}, EntryFontSize, 0, WHITE);
    Rectangle versionOffButtonRect = {OptionLeft + OptionWidth - 2 * toggleButtonWidth - toggleOffset, hideVersionTop, toggleButtonWidth, EntryHeight};
    Rectangle versionOnButtonRect = {OptionLeft + OptionWidth - toggleButtonWidth - toggleOffset, hideVersionTop, toggleButtonWidth, EntryHeight};
    if (CheckCollisionPointRec(mousePos, versionOffButtonRect) || CheckCollisionPointRec(mousePos, versionOnButtonRect)) {
        selectedIndex = 5;
        isHovering = true;
        DrawRectangleLinesEx(hideVersionBoxRect, highlightBorderWidth, glowColor);
    }
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, !TheGameSettings.HideVersionInfo ? defaultColor : ColorToInt(activeColor));
    if (GuiButton(versionOffButtonRect, "Off")) {
        if (!TheGameSettings.HideVersionInfo) {
            TheGameSettings.HideVersionInfo = true;
            TheGameSettings.SaveToFile((settingsMain.getDirectory() / "settings.json").string());
        }
    }
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, !TheGameSettings.HideVersionInfo ? ColorToInt(activeColor) : defaultColor);
    if (GuiButton(versionOnButtonRect, "On")) {
        if (TheGameSettings.HideVersionInfo) {
            TheGameSettings.HideVersionInfo = false;
            TheGameSettings.SaveToFile((settingsMain.getDirectory() / "settings.json").string());
        }
    }
    if (TheGameSettings.HideVersionInfo) {
        DrawRectangleLinesEx(versionOffButtonRect, highlightBorderWidth, glowColor);
    } else {
        DrawRectangleLinesEx(versionOnButtonRect, highlightBorderWidth, glowColor);
    }
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, defaultColor);

    settingOffset++;
    float hudPositionTop = EntryTop + (EntryHeight + verticalGap) * settingOffset;
    Rectangle hudPositionBoxRect = {boxLeft - borderWidth, hudPositionTop - borderWidth, boxWidth + 2 * borderWidth, EntryHeight + 2 * borderWidth};
    DrawRectangle(boxLeft - borderWidth, hudPositionTop - borderWidth, boxWidth + 2 * borderWidth, EntryHeight + 2 * borderWidth, boxBorder);
    DrawRectangle(boxLeft, hudPositionTop, boxWidth, EntryHeight, boxBackground);
    Vector2 hudPositionTextSize = MeasureTextEx(assets.rubikBold, "HUD Position", EntryFontSize, 0);
    DrawTextEx(assets.rubikBold, "HUD Position", {boxLeft + u.winpct(0.01f), hudPositionTop + (EntryHeight - hudPositionTextSize.y) / 2}, EntryFontSize, 0, WHITE);
    
    // HUD Position cycle button
    float cycleButtonWidth = toggleButtonWidth * 2 + toggleOffset;
    Rectangle hudCycleButtonRect = {OptionLeft + OptionWidth - cycleButtonWidth, hudPositionTop, cycleButtonWidth, EntryHeight};
    
    const char* hudPositionNames[] = {"Top-Right", "Top-Left", "Bottom-Right", "Bottom-Left"};
    const char* currentHUDPosition = hudPositionNames[TheGameSettings.HUDPosition % 4];
    
    if (CheckCollisionPointRec(mousePos, hudCycleButtonRect)) {
        selectedIndex = 6;
        isHovering = true;
        DrawRectangleLinesEx(hudPositionBoxRect, highlightBorderWidth, glowColor);
    }
    
    if (GuiButton(hudCycleButtonRect, currentHUDPosition)) {
        TheGameSettings.HUDPosition = (TheGameSettings.HUDPosition + 1) % 4;
        TheGameSettings.SaveToFile((settingsMain.getDirectory() / "settings.json").string());
    }

    if (!isHovering) {
        selectedIndex = 0;
    }
}

#include <raylib.h>

void SettingsGameplay::KeyboardInputCallback(int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        Save();
        TheMenuManager.SwitchScreen(SETTINGS);
    }
}

void SettingsGameplay::ControllerInputCallback(int joypadID, GLFWgamepadstate state) {
    if (state.buttons[GLFW_GAMEPAD_BUTTON_B] == GLFW_PRESS) {
        Save();
        TheMenuManager.SwitchScreen(SETTINGS);
    }
}

void SettingsGameplay::Load() {
    SettingsOld& settingsMain = SettingsOld::getInstance();
    // Ensure window state matches settings
    if (settingsMain.fullscreen && !IsWindowFullscreen()) {
        ToggleFullscreen();
    } else if (!settingsMain.fullscreen && IsWindowFullscreen()) {
        ToggleFullscreen();
    }
}

void SettingsGameplay::Save() {
    SettingsOld& settingsMain = SettingsOld::getInstance();
    settingsMain.saveSettings(settingsMain.getDirectory() / "settings.json");
}