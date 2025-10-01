//
// Created by marie on 02/08/2024.
//

#include "gameplayHudRenderer.h"
#include "settings.h"

void gameplayHudRenderer::RenderHud() {
    extern Encore::Settings TheGameSettings;
    if (!TheGameSettings.ShowHealthBar) {
        return;
    }
    
}
