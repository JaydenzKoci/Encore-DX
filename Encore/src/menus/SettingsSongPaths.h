#ifndef SETTINGS_SONG_PATHS_H
#define SETTINGS_SONG_PATHS_H

#include <GLFW/glfw3.h>
#include "OvershellMenu.h"
#include <string>

class SettingsSongPaths : public OvershellMenu {
public:
    void Draw();
    void KeyboardInputCallback(int key, int scancode, int action, int mods);
    void ControllerInputCallback(int joypadID, GLFWgamepadstate state);
    void Load();

private:
    int scrollOffset = 0;
    bool showAddPathDialog = false;
    char newPathBuffer[512] = {0};
    bool needsRescan = false;
};

#endif
