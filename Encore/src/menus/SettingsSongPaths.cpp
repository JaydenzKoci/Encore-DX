#include "SettingsSongPaths.h"

#include "MenuManager.h"
#include "gameMenu.h"
#include "raygui.h"
#include "assets.h"
#include "settings.h"
#include "uiUnits.h"
#include "OvershellMenu.h"
#include "song/songlist.h"
#include "util/settings-text.h"
#include <cstring>

extern Encore::SettingsInit TheSettingsInitializer;

void SettingsSongPaths::Draw() {
    if (!IsWindowReady()) return;

    Units& u = Units::getInstance();
    Assets& assets = Assets::getInstance();

    if (TheSongList.curSong != nullptr) {
        GameMenu::DrawAlbumArtBackground(TheSongList.curSong->albumArtBlur);
    } else {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);
    }
    DrawRectangle(u.LeftSide, 0, u.winpct(1.0f), GetScreenHeight(), Color{0, 0, 0});

    float TextPlacementTB = u.hpct(0.05f);
    float TextPlacementLR = u.wpct(0.05f);

    float entryBorderWidth = u.winpct(0.002f);
    float EntryFontSize = u.hinpct(0.03f);
    float EntryHeight = u.hinpct(0.06f);
    float EntryTop = TextPlacementTB + u.hinpct(0.125f) + u.hinpct(0.01f);
    float boxLeft = u.LeftSide + u.winpct(0.025f) + 23.0f;
    float boxWidth = u.wpct(0.60f);
    Color boxBackground = Color{31, 31, 50, 255};
    Color boxBorder = WHITE;
    Color glowColor = Color{142, 13, 148, 220};
    float highlightBorderWidth = 4.0f;
    Color activeColor = Color{255, 105, 180, 255};
    int defaultColor = GuiGetStyle(BUTTON, BASE_COLOR_PRESSED);

    Vector2 mousePos = GetMousePosition();

    while (TheGameSettings.EnabledSongPaths.size() < TheGameSettings.SongPaths.size()) {
        TheGameSettings.EnabledSongPaths.push_back(true);
    }

    float toggleButtonWidth = u.winpct(0.06f);
    float removeButtonWidth = u.winpct(0.08f);
    float buttonGap = u.winpct(0.01f);

    int maxVisibleEntries = 8;
    int totalEntries = (int)TheGameSettings.SongPaths.size();

    for (int i = 0; i < std::min(maxVisibleEntries, totalEntries - scrollOffset); i++) {
        int pathIndex = i + scrollOffset;
        if (pathIndex >= (int)TheGameSettings.SongPaths.size()) break;

        float entryTop = EntryTop + (EntryHeight + 5.0f) * i;
        
        DrawRectangle(boxLeft - entryBorderWidth, entryTop - entryBorderWidth, boxWidth + 2 * entryBorderWidth, EntryHeight + 2 * entryBorderWidth, boxBorder);
        DrawRectangle(boxLeft, entryTop, boxWidth, EntryHeight, boxBackground);

        std::string pathStr = TheGameSettings.SongPaths[pathIndex].string();
        float maxTextWidth = boxWidth - toggleButtonWidth - removeButtonWidth - buttonGap * 3 - u.winpct(0.02f);
        
        std::string displayPath = pathStr;
        Vector2 textSize = MeasureTextEx(assets.rubik, displayPath.c_str(), EntryFontSize, 0);
        while (textSize.x > maxTextWidth && displayPath.length() > 3) {
            displayPath = "..." + displayPath.substr(4);
            textSize = MeasureTextEx(assets.rubik, displayPath.c_str(), EntryFontSize, 0);
        }
        
        DrawTextEx(assets.rubik, displayPath.c_str(), {boxLeft + u.winpct(0.01f), entryTop + (EntryHeight - textSize.y) / 2}, EntryFontSize, 0, WHITE);

        Rectangle toggleRect = {boxLeft + boxWidth - toggleButtonWidth - removeButtonWidth - buttonGap * 2, entryTop, toggleButtonWidth, EntryHeight};
        Rectangle removeRect = {boxLeft + boxWidth - removeButtonWidth - buttonGap, entryTop, removeButtonWidth, EntryHeight};

        bool isEnabled = pathIndex < (int)TheGameSettings.EnabledSongPaths.size() && TheGameSettings.EnabledSongPaths[pathIndex];
        
        GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, isEnabled ? ColorToInt(activeColor) : defaultColor);
        if (GuiButton(toggleRect, isEnabled ? "On" : "Off")) {
            if (pathIndex < (int)TheGameSettings.EnabledSongPaths.size()) {
                TheGameSettings.EnabledSongPaths[pathIndex] = !TheGameSettings.EnabledSongPaths[pathIndex];
                needsRescan = true;
                TheGameSettings.SaveIfChanged(TheSettingsInitializer.GetSettingsFilePath());
            }
        }
        GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, defaultColor);

        if (isEnabled) {
            DrawRectangleLinesEx(toggleRect, 2.0f, glowColor);
        }

        if (GuiButton(removeRect, "Remove")) {
            TheGameSettings.SongPaths.erase(TheGameSettings.SongPaths.begin() + pathIndex);
            if (pathIndex < (int)TheGameSettings.EnabledSongPaths.size()) {
                TheGameSettings.EnabledSongPaths.erase(TheGameSettings.EnabledSongPaths.begin() + pathIndex);
            }
            needsRescan = true;
            TheGameSettings.SaveIfChanged(TheSettingsInitializer.GetSettingsFilePath());
        }

        Rectangle entryRect = {boxLeft - entryBorderWidth, entryTop - entryBorderWidth, boxWidth + 2 * entryBorderWidth, EntryHeight + 2 * entryBorderWidth};
        if (CheckCollisionPointRec(mousePos, entryRect)) {
            DrawRectangleLinesEx(entryRect, highlightBorderWidth, glowColor);
        }
    }

    if (totalEntries > maxVisibleEntries) {
        float scrollBarHeight = u.hpct(0.4f);
        float scrollBarWidth = u.winpct(0.01f);
        float scrollBarX = boxLeft + boxWidth + u.winpct(0.02f);
        float scrollBarY = EntryTop;
        
        DrawRectangle(scrollBarX, scrollBarY, scrollBarWidth, scrollBarHeight, Color{50, 50, 50, 255});
        
        float thumbHeight = scrollBarHeight * ((float)maxVisibleEntries / totalEntries);
        float thumbY = scrollBarY + (scrollBarHeight - thumbHeight) * ((float)scrollOffset / (totalEntries - maxVisibleEntries));
        DrawRectangle(scrollBarX, thumbY, scrollBarWidth, thumbHeight, WHITE);

        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            scrollOffset -= (int)wheel;
            if (scrollOffset < 0) scrollOffset = 0;
            if (scrollOffset > totalEntries - maxVisibleEntries) scrollOffset = totalEntries - maxVisibleEntries;
        }
    }

    float addButtonTop = EntryTop + (EntryHeight + 5.0f) * std::min(maxVisibleEntries, totalEntries) + u.hinpct(0.02f);
    float addButtonWidth = boxWidth;
    float addButtonHeight = EntryHeight + 10.0f;
    
    Rectangle addButtonRect = {boxLeft, addButtonTop, addButtonWidth, addButtonHeight};
    DrawRectangle(boxLeft - entryBorderWidth, addButtonTop - entryBorderWidth, addButtonWidth + 2 * entryBorderWidth, addButtonHeight + 2 * entryBorderWidth, boxBorder);
    DrawRectangle(boxLeft, addButtonTop, addButtonWidth, addButtonHeight, boxBackground);
    
    if (CheckCollisionPointRec(mousePos, addButtonRect)) {
        DrawRectangleLinesEx({boxLeft - entryBorderWidth, addButtonTop - entryBorderWidth, addButtonWidth + 2 * entryBorderWidth, addButtonHeight + 2 * entryBorderWidth}, highlightBorderWidth, glowColor);
    }
    
    if (GuiButton(addButtonRect, "Add New Path")) {
        showAddPathDialog = true;
        memset(newPathBuffer, 0, sizeof(newPathBuffer));
    }

    float SidebarLeft = u.LeftSide + u.winpct(0.70f);
    float SidebarWidth = u.wpct(0.16f);
    float SidebarTop = u.hinpct(0.10f);
    float SidebarHeight = u.hpct(0.85f);
    float SidebarHeaderHeight = u.hinpct(0.14f);
    float borderWidth = u.winpct(0.05f);
    float innerTop = SidebarTop + borderWidth;

    DrawRectangle(SidebarLeft - u.winpct(0.004f), SidebarTop - u.hinpct(0.08f) - u.winpct(0.004f),
                  SidebarWidth + u.winpct(0.008f), SidebarHeight + u.winpct(0.02f), WHITE);
    DrawRectangle(SidebarLeft, SidebarTop - u.hinpct(0.02f), SidebarWidth, SidebarHeight, Color{31, 31, 50, 255});

    struct SidebarContent {
        const char* header;
        const char* body;
    };
    SidebarContent sidebarContents[] = {
        {
            "Manage song folders",
            "Toggle paths on/off to\nfilter which songs appear\nin the song list.\n\nAdd new paths to include\nmore song folders.\n\nRemove paths you no\nlonger need."
        }
    };

    const char* headerText = sidebarContents[0].header;
    const char* sidebarBodyText = sidebarContents[0].body;

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
    float currentHeaderY = innerTop;
    for (const std::string& line : headerLines) {
        float lineX = SidebarLeft + (SidebarWidth - maxHeaderWidth) / 2;
        DrawTextEx(assets.rubikBold, line.c_str(), {lineX, currentHeaderY}, headerFontSize, 0, WHITE);
        currentHeaderY += headerLineSpacing;
    }

    float bodyFontSize = u.hinpct(0.030f);
    float lineSpacing = bodyFontSize * 1.2f;
    std::vector<std::string> lines = split(sidebarBodyText, "\n");
    float currentY = SidebarTop + SidebarHeaderHeight + u.hinpct(0.05f);
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

    DrawTextEx(assets.rubik, "Settings", {TextPlacementLR, u.hpct(0.027f)}, u.hinpct(0.042f), 0, LIGHTGRAY);
    GameMenu::mhDrawText(assets.redHatDisplayBlack, "SONG PATHS", {TextPlacementLR, TextPlacementTB}, u.hinpct(0.125f), WHITE, assets.sdfShader, LEFT);

    if (showAddPathDialog) {
        float dialogWidth = u.winpct(0.5f);
        float dialogHeight = u.hinpct(0.25f);
        float dialogX = (GetScreenWidth() - dialogWidth) / 2;
        float dialogY = (GetScreenHeight() - dialogHeight) / 2;

        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 180});
        DrawRectangle(dialogX - entryBorderWidth, dialogY - entryBorderWidth, dialogWidth + 2 * entryBorderWidth, dialogHeight + 2 * entryBorderWidth, WHITE);
        DrawRectangle(dialogX, dialogY, dialogWidth, dialogHeight, Color{31, 31, 50, 255});

        DrawTextEx(assets.rubikBold, "Add Song Path", {dialogX + u.winpct(0.02f), dialogY + u.hinpct(0.02f)}, u.hinpct(0.04f), 0, WHITE);

        Rectangle textBoxRect = {dialogX + u.winpct(0.02f), dialogY + u.hinpct(0.08f), dialogWidth - u.winpct(0.04f), u.hinpct(0.05f)};
        GuiTextBox(textBoxRect, newPathBuffer, sizeof(newPathBuffer), true);

        float buttonWidth = u.winpct(0.1f);
        float buttonHeight = u.hinpct(0.05f);
        float buttonY = dialogY + dialogHeight - buttonHeight - u.hinpct(0.02f);

        if (GuiButton({dialogX + dialogWidth / 2 - buttonWidth - u.winpct(0.01f), buttonY, buttonWidth, buttonHeight}, "Add")) {
            if (strlen(newPathBuffer) > 0) {
                std::filesystem::path newPath(newPathBuffer);
                if (std::filesystem::exists(newPath) && std::filesystem::is_directory(newPath)) {
                    TheGameSettings.SongPaths.push_back(newPath);
                    TheGameSettings.EnabledSongPaths.push_back(true);
                    needsRescan = true;
                    TheGameSettings.SaveIfChanged(TheSettingsInitializer.GetSettingsFilePath());
                }
            }
            showAddPathDialog = false;
        }

        if (GuiButton({dialogX + dialogWidth / 2 + u.winpct(0.01f), buttonY, buttonWidth, buttonHeight}, "Cancel")) {
            showAddPathDialog = false;
        }
    }
}

void SettingsSongPaths::KeyboardInputCallback(int key, int scancode, int action, int mods) {
    if (showAddPathDialog) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            showAddPathDialog = false;
            return;
        }
        
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            size_t len = strlen(newPathBuffer);
            
            if (key == GLFW_KEY_BACKSPACE && len > 0) {
                newPathBuffer[len - 1] = '\0';
                return;
            }
            
            if (key == GLFW_KEY_V && (mods & GLFW_MOD_CONTROL)) {
                const char* clipboard = GetClipboardText();
                if (clipboard) {
                    size_t clipLen = strlen(clipboard);
                    size_t spaceLeft = sizeof(newPathBuffer) - len - 1;
                    size_t copyLen = (clipLen < spaceLeft) ? clipLen : spaceLeft;
                    strncat(newPathBuffer, clipboard, copyLen);
                }
                return;
            }
            
            if (key == GLFW_KEY_C && (mods & GLFW_MOD_CONTROL)) {
                SetClipboardText(newPathBuffer);
                return;
            }
        }
        return;
    }
    
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (needsRescan) {
            auto enabledPaths = TheGameSettings.GetEnabledSongPaths();
            if (!enabledPaths.empty()) {
                TheSongList.ScanSongs(enabledPaths);
            }
        }
        TheMenuManager.SwitchScreen(SETTINGSGAMEPLAY);
    }
}

void SettingsSongPaths::ControllerInputCallback(int joypadID, GLFWgamepadstate state) {
    if (showAddPathDialog) {
        if (state.buttons[GLFW_GAMEPAD_BUTTON_B] == GLFW_PRESS) {
            showAddPathDialog = false;
        }
        return;
    }
    
    if (state.buttons[GLFW_GAMEPAD_BUTTON_B] == GLFW_PRESS) {
        if (needsRescan) {
            auto enabledPaths = TheGameSettings.GetEnabledSongPaths();
            if (!enabledPaths.empty()) {
                TheSongList.ScanSongs(enabledPaths);
            }
        }
        TheMenuManager.SwitchScreen(SETTINGSGAMEPLAY);
    }
}

void SettingsSongPaths::Load() {
    scrollOffset = 0;
    showAddPathDialog = false;
    needsRescan = false;
    memset(newPathBuffer, 0, sizeof(newPathBuffer));
}
