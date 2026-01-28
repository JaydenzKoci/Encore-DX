#pragma once
//
// Created by marie on 20/10/2024.
//

#include "OvershellMenu.h"
#include "assets.h"
#include "menu.h"
#include "uiUnits.h"

#include <vector>

struct StreakPopEffect {
    double triggerTime = 0.0;
    bool active = false;
    int lastCombo = 0;
};

struct NewHighScoreEffect {
    bool triggered = false;
    double triggerTime = 0.0;
    int previousHighScore = 0;
    bool highScoreLoaded = false;
};

// technically this IS a menu, but realistically, is it?
class GameplayMenu : public OvershellMenu {
    int CameraSelectionPerPlayer[4][4] {
        {0,0,0,0},
        {1,0,0,0},
        {2,1,0,0},
        {3,2,1,0}
    };
    int CameraPosPerPlayer[4][4] {
        {0,0,0,0},
        {8,-8,0,0},
        {4,0,-4,0},
        {4,12,-12,-4}
    };
    
    StreakPopEffect streakPopEffects[4];
    static constexpr double STREAK_POP_DURATION = 0.5;
    
    NewHighScoreEffect highScoreEffect;
    
    void DrawStreakPopEffect(int playerIndex, float centerX, float centerY, double currentTime);
    void DrawNewHighScoreNotification(Units &u, Assets &assets, double currentTime);
public:
    GameplayMenu();
    virtual ~GameplayMenu();
    void DrawScorebox(Units &u, Assets &assets, float scoreY);
    void DrawTimerbox(Units &u, Assets &assets, float scoreY);
    void DrawGameplayStars(Units &u, Assets &assets, float scorePos, float starY);
    void DrawInstrumentIcon(Units &u, Assets &assets, float scoreY, double songTime);
    void KeyboardInputCallback(int key, int scancode, int action, int mods);
    void ControllerInputCallback(int joypadID, GLFWgamepadstate state);
    void Draw() override;
    void Load() override;
    void CleanupAndSwitchToResults();
};
