//
// Created by Jaydenz on 04/29/2025.
//

#pragma once

#include "OvershellMenu.h"
#include "settings-old.h"
#include "keybinds.h"
#include "assets.h"
#include "settings.h"

extern Encore::Settings TheGameSettings;
extern Encore::SettingsInit TheSettingsInitializer;

#ifndef SETTINGSKEYBOARD_H
#define SETTINGSKEYBOARD_H

namespace Encore {
    class SettingsKeyboard {
    };
}

class SettingsKeyboard : public OvershellMenu {
#define OPTION(type, value, default) type value = default;
        SETTINGSKEYBOARD_H;
#undef OPTION
public:
    SettingsKeyboard() = default;
    ~SettingsKeyboard() override = default;
    void Draw() override;
    static std::pair<std::string, int> getBindTypeAndIndex(size_t optionIndex);
    void KeyboardInputCallback(int key, int scancode, int action, int mods) override;
    void ControllerInputCallback(int joypadID, GLFWgamepadstate state) override;
    void Load();
    void Save();

private:
    Keybinds keybinds;
    int selectedIndex = 0;
    int bindingOption = -1;
    bool isHovering = false;
    const float boxWidthPct = 0.55f;
    std::vector<std::pair<std::string, int*>> options = {
        {"4K Lane 1", &TheGameSettings.Keybinds4K[0]},
        {"4K Lane 2", &TheGameSettings.Keybinds4K[1]},
        {"4K Lane 3", &TheGameSettings.Keybinds4K[2]},
        {"4K Lane 4", &TheGameSettings.Keybinds4K[3]},
        {"5K Lane 1", &TheGameSettings.Keybinds5K[0]},
        {"5K Lane 2", &TheGameSettings.Keybinds5K[1]},
        {"5K Lane 3", &TheGameSettings.Keybinds5K[2]},
        {"5K Lane 4", &TheGameSettings.Keybinds5K[3]},
        {"5K Lane 5", &TheGameSettings.Keybinds5K[4]},
        {"Overdrive", &TheGameSettings.KeybindOverdrive},
        {"Overdrive Alt", &TheGameSettings.KeybindOverdriveAlt},
        {"Pause", &TheGameSettings.KeybindPause}
    };
    struct SidebarContent {
        const char* header;
        const char* body;
    };
    std::vector<SidebarContent> sidebarContents = {
        {"Keyboard Bindings", "TBD"},
        {"4K Lane 1", "TBD"},
        {"4K Lane 2", "TBD"},
        {"4K Lane 3", "TBD"},
        {"4K Lane 4", "TBD"},
        {"5K Lane 1", "TBD"},
        {"5K Lane 2", "TBD"},
        {"5K Lane 3", "TBD"},
        {"5K Lane 4", "TBD"},
        {"5K Lane 5", "TBD"},
        {"Overdrive", "TBD"},
        {"Overdrive Alt", "TBD"},
        {"Pause", "TBD"}
    };
};

extern Encore::SettingsKeyboard TheKeyboardSettings;

#endif // SETTINGSKEYBOARD_H