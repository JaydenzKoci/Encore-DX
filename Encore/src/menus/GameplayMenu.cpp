//
// Created by marie on 20/10/2024.
//

#include "GameplayMenu.h"
#include <raylib.h>
#include <filesystem>
#include "gameplay/GameplayInputHandler.h"
#include "gameMenu.h"
#include "overshellRenderer.h"
#include "uiUnits.h"
#include "song/audio.h"
#include "song/songlist.h"
#include "song/chart.h"
#include "raymath.h"
#include "raygui.h"
#include "gameplay/enctime.h"
#include "styles.h"
#include "easing/easing.h"
#include "gameplay/gameplayRenderer.h"
#include "users/playerManager.h"
#include "MenuManager.h"
#include "OvershellHelper.h"
#include "settings-old.h"
#include "settings.h"
#include "leaderboard/leaderboard.h"


GameplayMenu::GameplayMenu() {}
GameplayMenu::~GameplayMenu() {}

void GameplayMenu::DrawStreakPopEffect(int playerIndex, float centerX, float centerY, double currentTime) {
    if (playerIndex < 0 || playerIndex >= 4) return;
    
    StreakPopEffect& effect = streakPopEffects[playerIndex];
    if (!effect.active) return;
    
    double timeSinceTrigger = currentTime - effect.triggerTime;
    if (timeSinceTrigger > STREAK_POP_DURATION) {
        effect.active = false;
        return;
    }
    
    float progress = static_cast<float>(timeSinceTrigger / STREAK_POP_DURATION);
    float easedProgress = getEasingFunction(EaseOutQuart)(progress);
    
    Units &u = Units::getInstance();
    float minRadius = u.hinpct(0.015f);
    float maxRadius = u.hinpct(0.055f);
    float currentRadius = minRadius + (maxRadius - minRadius) * easedProgress;
    
    float maxThickness = u.hinpct(0.01f);
    float minThickness = u.hinpct(0.002f);
    float currentThickness = maxThickness - (maxThickness - minThickness) * easedProgress;
    
    unsigned char alpha = static_cast<unsigned char>(255 * (1.0f - easedProgress));
    
    Color ringColor = { 255, 255, 255, alpha };
    DrawRing(
        { centerX, centerY },
        currentRadius - currentThickness,
        currentRadius,
        0.0f,
        360.0f,
        36,
        ringColor
    );
}

void GameplayMenu::CleanupAndSwitchToResults() {
    TheGameRenderer.backgroundVideo.Stop();
    TheGameRenderer.backgroundVideo.Unload();
    
    TheSongList.curSong->LoadAlbumArt();
    
    TheGameRenderer.midiLoaded = false;
    TheGameRenderer.highwayInAnimation = false;
    TheGameRenderer.highwayInEndAnim = false;
    TheGameRenderer.songPlaying = false;
    TheGameRenderer.highwayLevel = 0;
    TheGameRenderer.streamsLoaded = false;
    
    TheSongTime.Stop();
    TheSongTime.Reset();
    
    TheAudioManager.unloadStreams();
    
    for (int playerNum = 0; playerNum < ThePlayerManager.PlayersActive; playerNum++) {
        Player& player = ThePlayerManager.GetActivePlayer(playerNum);
        
        if (TheSongList.curSong && TheSongList.curSong->parts[player.Instrument]) {
            TheSongList.curSong->parts[player.Instrument]
                ->charts[player.Difficulty].resetNotes();
        }
        
        if (player.stats) {
            player.stats->Paused = false;
            player.stats->Overdrive = false;
            player.stats->Mute = false;
            player.stats->FAS = false;
            player.stats->Overstrum = false;
            player.stats->UpStrum = false;
            player.stats->DownStrum = false;
            player.stats->StrumNoFretTime = -1.0;
            
            std::fill(player.stats->HeldFrets.begin(), player.stats->HeldFrets.end(), false);
            std::fill(player.stats->HeldFretsAlt.begin(), player.stats->HeldFretsAlt.end(), false);
            std::fill(player.stats->OverhitFrets.begin(), player.stats->OverhitFrets.end(), false);
            std::fill(player.stats->TapRegistered.begin(), player.stats->TapRegistered.end(), false);
            std::fill(player.stats->LiftRegistered.begin(), player.stats->LiftRegistered.end(), false);
            std::fill(player.stats->overdriveLanesHit.begin(), player.stats->overdriveLanesHit.end(), false);
            
            std::fill(player.stats->axesValues.begin(), player.stats->axesValues.end(), 0.0f);
            std::fill(player.stats->buttonValues.begin(), player.stats->buttonValues.end(), 0);
            std::fill(player.stats->axesValues2.begin(), player.stats->axesValues2.end(), 0.0f);
        }
    }
    
    if (ThePlayerManager.BandStats) {
        ThePlayerManager.BandStats->ResetBandGameplayStats();
        ThePlayerManager.BandStats->Paused = false;
        ThePlayerManager.BandStats->PlayersInOverdrive = 0;
    }
    
    TheMenuManager.SwitchScreen(RESULTS);
}

void ManagePausedGame(GameplayInputHandler inputHandler, Player &player) {
    PlayerGameplayStats *&stats = player.stats;
    stats->Paused = !stats->Paused;
    ThePlayerManager.BandStats->Paused = !ThePlayerManager.BandStats->Paused;
    
    if (ThePlayerManager.BandStats->Paused) {
        Encore::EncoreLog(LOG_INFO, TextFormat("PAUSED at songTime=%.2f s", TheSongTime.GetSongTime()));
        
        TheAudioManager.pauseStreams();
        TheSongTime.Pause();
        TheGameRenderer.backgroundVideo.Pause();
    } else {
        if (TheSongTime.IsInResumeGracePeriod()) {
            Encore::EncoreLog(LOG_INFO, "UNPAUSING during grace period - extending by 3 seconds");
            TheSongTime.ExtendGracePeriod();
        } else {
            Encore::EncoreLog(LOG_INFO, "UNPAUSING - starting 3-second grace period");
            TheSongTime.Resume();
        }
        
        TheAudioManager.unpauseStreams();
        TheGameRenderer.ClearHeldInputs(player);
    }
}

void GameplayMenu::KeyboardInputCallback(int key, int scancode, int action, int mods) {
    Encore::EncoreLog(LOG_DEBUG, TextFormat("Keyboard key %01i inputted on menu %s, action ", key, ToString(TheMenuManager.currentScreen), action) );
    Player &player = ThePlayerManager.GetActivePlayer(0);
    PlayerGameplayStats *&stats = player.stats;
    GameplayInputHandler inputHandler;
    if (!TheGameRenderer.streamsLoaded) {
        return;
    }

    if (action < 2) {
        // if the key action is NOT repeat (release is 0, press is 1)
        int lane = -2;
        if (key == TheGameSettings.KeybindPause && action == GLFW_PRESS) {
            ManagePausedGame(inputHandler, player);
        } else {
            float rendererAlpha = TheGameRenderer.GetRendererAlpha(player.ActiveSlot);
            if (rendererAlpha < 0.95f) {
                return;
            }
        }
            if ((key == TheGameSettings.KeybindOverdrive
                        || key == TheGameSettings.KeybindOverdriveAlt)) {
                inputHandler.handleInputs(player, -1, action);
            } else if (!player.Bot) {
            if (player.Instrument != PlasticDrums) {
                if (player.Difficulty == 3 || player.ClassicMode) {
                    for (int i = 0; i < 5; i++) {
                        if (key == TheGameSettings.Keybinds5K[i]
                            && !stats->HeldFretsAlt[i]) {
                            if (action == GLFW_PRESS) {
                                stats->HeldFrets[i] = true;
                            } else if (action == GLFW_RELEASE) {
                                stats->HeldFrets[i] = false;
                                stats->OverhitFrets[i] = false;
                            }
                            lane = i;
                        } else if (key == TheGameSettings.Keybinds5KAlt[i]
                                   && !stats->HeldFrets[i]) {
                            if (action == GLFW_PRESS) {
                                stats->HeldFretsAlt[i] = true;
                            } else if (action == GLFW_RELEASE) {
                                stats->HeldFretsAlt[i] = false;
                                stats->OverhitFrets[i] = false;
                            }
                            lane = i;
                        }
                    }
                } else {
                    for (int i = 0; i < 4; i++) {
                        if (key == TheGameSettings.Keybinds4K[i]
                            && !stats->HeldFretsAlt[i]) {
                            if (action == GLFW_PRESS) {
                                stats->HeldFrets[i] = true;
                            } else if (action == GLFW_RELEASE) {
                                stats->HeldFrets[i] = false;
                                stats->OverhitFrets[i] = false;
                            }
                            lane = i;
                        } else if (key == TheGameSettings.Keybinds4KAlt[i]
                                   && !stats->HeldFrets[i]) {
                            if (action == GLFW_PRESS) {
                                stats->HeldFretsAlt[i] = true;
                            } else if (action == GLFW_RELEASE) {
                                stats->HeldFretsAlt[i] = false;
                                stats->OverhitFrets[i] = false;
                            }
                            lane = i;
                        }
                    }
                }
                if (player.ClassicMode) {
                    if (key == TheGameSettings.KeybindStrumUp) {
                        if (action == GLFW_PRESS) {
                            lane = 8008135;
                            stats->UpStrum = true;
                        } else if (action == GLFW_RELEASE) {
                            stats->UpStrum = false;
                            stats->Overstrum = false;
                        }
                    }
                    if (key == TheGameSettings.KeybindStrumDown) {
                        if (action == GLFW_PRESS) {
                            lane = 8008135;
                            stats->DownStrum = true;
                        } else if (action == GLFW_RELEASE) {
                            stats->DownStrum = false;
                            stats->Overstrum = false;
                        }
                    }
                }
                Encore::EncoreLog(LOG_DEBUG, TextFormat("Keyboard key lane %01i", lane) );
                if (lane != -1 && lane != -2) {
                    inputHandler.handleInputs(player, lane, action);
                    Encore::EncoreLog(LOG_DEBUG, "Sent key input");
                }
            }
        }
    }
}
void GameplayMenu::ControllerInputCallback(int joypadID, GLFWgamepadstate state) {
    GameplayInputHandler inputHandler;

    if (TheMenuManager.currentScreen == SONG_SELECT) {
        if (state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] == GLFW_PRESS) {
            TheSongList.SongSelectOffset -= 1;
        }
        if (state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] == GLFW_PRESS) {
            TheSongList.SongSelectOffset += 1;
        }
    }
    if (TheMenuManager.currentScreen == GAMEPLAY) {
        Encore::EncoreLog(
            LOG_DEBUG, TextFormat("Attempted input on joystick %01i", joypadID)
        );
        if (!ThePlayerManager.IsGamepadActive(joypadID))
            return;

        Player &player = ThePlayerManager.GetPlayerGamepad(joypadID);
        PlayerGameplayStats *&stats = player.stats;

        if (!TheGameRenderer.streamsLoaded) {
            return;
        }

        double eventTime = TheSongTime.GetSongTime();
        if (TheGameSettings.ControllerPause >= 0) {
            if (state.buttons[TheGameSettings.ControllerPause]
                != stats->buttonValues[TheGameSettings.ControllerPause]) {
                stats->buttonValues[TheGameSettings.ControllerPause] =
                    state.buttons[TheGameSettings.ControllerPause];
                if (state.buttons[TheGameSettings.ControllerPause] == 1) {
                    ManagePausedGame(inputHandler, player); // && !player.Bot
                }
            }
        } else if (!player.Bot) {
            if (state.axes[-(TheGameSettings.ControllerPause + 1)]
                != stats->axesValues[-(TheGameSettings.ControllerPause + 1)]) {
                stats->axesValues[-(TheGameSettings.ControllerPause + 1)] =
                    state.axes[-(TheGameSettings.ControllerPause + 1)];
                if (state.axes[-(TheGameSettings.ControllerPause + 1)]
                    == 1.0f * (float)TheGameSettings.ControllerPauseAxisDirection) {
                }
            }
        } //  && !player.Bot
        float rendererAlpha = TheGameRenderer.GetRendererAlpha(player.ActiveSlot);
        if (rendererAlpha >= 0.95f) {
            if (TheGameSettings.ControllerOverdrive >= 0) {
                if (state.buttons[TheGameSettings.ControllerOverdrive]
                    != stats->buttonValues[TheGameSettings.ControllerOverdrive]) {
                    stats->buttonValues[TheGameSettings.ControllerOverdrive] =
                        state.buttons[TheGameSettings.ControllerOverdrive];
                    inputHandler.handleInputs(
                        player, -1, state.buttons[TheGameSettings.ControllerOverdrive]
                    );
                } // // if (!player.Bot)
            } else {
                if (state.axes[-(TheGameSettings.ControllerOverdrive + 1)]
                    != stats->axesValues[-(TheGameSettings.ControllerOverdrive + 1)]) {
                    stats->axesValues[-(TheGameSettings.ControllerOverdrive + 1)] =
                        state.axes[-(TheGameSettings.ControllerOverdrive + 1)];
                    if (state.axes[-(TheGameSettings.ControllerOverdrive + 1)]
                        == 1.0f * (float)TheGameSettings.ControllerOverdriveAxisDirection) {
                        inputHandler.handleInputs(player, -1, GLFW_PRESS);
                    } else {
                        inputHandler.handleInputs(player, -1, GLFW_RELEASE);
                    }
                }
            }
        }
        if ((player.Difficulty == 3 || player.ClassicMode) && !player.Bot && rendererAlpha >= 0.95f) {
            int lane = -2;
            int action = -2;
            for (int i = 0; i < 5; i++) {
                if (TheGameSettings.Controller5K[i] >= 0) {
                    if (state.buttons[TheGameSettings.Controller5K[i]]
                        != stats->buttonValues[TheGameSettings.Controller5K[i]]) {
                        if (state.buttons[TheGameSettings.Controller5K[i]] == 1
                            && !stats->HeldFrets[i])
                            stats->HeldFrets[i] = true;
                        else if (stats->HeldFrets[i]) {
                            stats->HeldFrets[i] = false;
                            stats->OverhitFrets[i] = false;
                        }
                        inputHandler.handleInputs(
                            player, i, state.buttons[TheGameSettings.Controller5K[i]]
                        );
                        stats->buttonValues[TheGameSettings.Controller5K[i]] =
                            state.buttons[TheGameSettings.Controller5K[i]];
                        lane = i;
                    }
                } else {
                    if (state.axes[-(TheGameSettings.Controller5K[i] + 1)]
                        != stats->axesValues[-(TheGameSettings.Controller5K[i] + 1)]) {
                        if (state.axes[-(TheGameSettings.Controller5K[i] + 1)]
                                == 1.0f * (float)TheGameSettings.Controller5KAxisDirection[i]
                            && !stats->HeldFrets[i]) {
                            stats->HeldFrets[i] = true;
                            inputHandler.handleInputs(player, i, GLFW_PRESS);
                        } else if (stats->HeldFrets[i]) {
                            stats->HeldFrets[i] = false;
                            stats->OverhitFrets[i] = false;
                            inputHandler.handleInputs(player, i, GLFW_RELEASE);
                        }
                        stats->axesValues[-(TheGameSettings.Controller5K[i] + 1)] =
                            state.axes[-(TheGameSettings.Controller5K[i] + 1)];
                        lane = i;
                    }
                }
            }

            if (state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] == GLFW_PRESS
                && player.ClassicMode && !stats->UpStrum) {
                stats->UpStrum = true;
                stats->Overstrum = false;
                inputHandler.handleInputs(player, 8008135, GLFW_PRESS);
            } else if (state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] == GLFW_RELEASE
                       && player.ClassicMode && stats->UpStrum) {
                stats->UpStrum = false;
                inputHandler.handleInputs(player, 8008135, GLFW_RELEASE);
            }
            if (state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] == GLFW_PRESS
                && player.ClassicMode && !stats->DownStrum) {
                stats->DownStrum = true;
                stats->Overstrum = false;
                inputHandler.handleInputs(player, 8008135, GLFW_PRESS);
            } else if (state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] == GLFW_RELEASE
                       && player.ClassicMode && stats->DownStrum) {
                stats->DownStrum = false;
                inputHandler.handleInputs(player, 8008135, GLFW_RELEASE);
            }
        } else if (!player.Bot && rendererAlpha >= 0.95f) {
            for (int i = 0; i < 4; i++) {
                if (TheGameSettings.Controller4K[i] >= 0) {
                    if (state.buttons[TheGameSettings.Controller4K[i]]
                        != stats->buttonValues[TheGameSettings.Controller4K[i]]) {
                        if (state.buttons[TheGameSettings.Controller4K[i]] == 1)
                            stats->HeldFrets[i] = true;
                        else {
                            stats->HeldFrets[i] = false;
                            stats->OverhitFrets[i] = false;
                        }
                        inputHandler.handleInputs(
                            player, i, state.buttons[TheGameSettings.Controller4K[i]]
                        );
                        stats->buttonValues[TheGameSettings.Controller4K[i]] =
                            state.buttons[TheGameSettings.Controller4K[i]];
                    }
                } else {
                    if (state.axes[-(TheGameSettings.Controller4K[i] + 1)]
                        != stats->axesValues[-(TheGameSettings.Controller4K[i] + 1)]) {
                        if (state.axes[-(TheGameSettings.Controller4K[i] + 1)]
                            == 1.0f * (float)TheGameSettings.Controller4KAxisDirection[i]) {
                            stats->HeldFrets[i] = true;
                            inputHandler.handleInputs(player, i, GLFW_PRESS);
                        } else {
                            stats->HeldFrets[i] = false;
                            stats->OverhitFrets[i] = false;
                            inputHandler.handleInputs(player, i, GLFW_RELEASE);
                        }
                        stats->axesValues[-(TheGameSettings.Controller4K[i] + 1)] =
                            state.axes[-(TheGameSettings.Controller4K[i] + 1)];
                    }
                }
            }
        }
    }
};
void GameplayMenu::DrawScorebox(Units &u, Assets &assets, float scoreY) {
    Rectangle scoreboxSrc {
        0, 0, float(assets.Scorebox.width), float(assets.Scorebox.height)
    };
    extern Encore::Settings TheGameSettings;
    float WidthOfScorebox = u.hinpct(0.28);
    float HeightOfScorebox = WidthOfScorebox / 4;
    
    float hudOffsetX = 0.0f;
    float hudOffsetY = 0.0f;
    
    int effectiveHUDPosition = (ThePlayerManager.PlayersActive > 1) ? 0 : TheGameSettings.HUDPosition;
    
    switch (effectiveHUDPosition) {
        case 0:
            hudOffsetX = 0.0f;
            hudOffsetY = 0.0f;
            break;
        case 1:
            hudOffsetX = -(GetScreenWidth() - u.hinpct(0.28f) - u.hinpct(0.02f)) + 250.0f;
            hudOffsetY = 0.0f;
            break;
        case 2:
            hudOffsetX = 0.0f;
            hudOffsetY = GetScreenHeight() - u.hpct(0.30f);
            break;
        case 3:
            hudOffsetX = -(GetScreenWidth() - u.hinpct(0.28f) - u.hinpct(0.02f)) + 250.0f;
            hudOffsetY = GetScreenHeight() - u.hpct(0.30f);
            break;
        default:
            hudOffsetX = 0.0f;
            hudOffsetY = 0.0f;
            break;
    }
    
    float ScoreboxX = u.RightSide + hudOffsetX;
    float ScoreboxY = u.hpct(0.1425f) + hudOffsetY;
    
    Rectangle scoreboxDraw { ScoreboxX, ScoreboxY, WidthOfScorebox, HeightOfScorebox };
    Vector2 origin = Vector2{WidthOfScorebox, 0};
    DrawTexturePro(
        assets.Scorebox, scoreboxSrc, scoreboxDraw, origin, 0, WHITE
    );
    
    Vector2 textPos = { ScoreboxX - u.winpct(0.0145f), scoreY + u.hinpct(0.0025) };
    int textAlign = RIGHT;
    
    GameMenu::mhDrawText(
        assets.redHatMono,
        GameMenu::scoreCommaFormatter(ThePlayerManager.BandStats->Score),
        textPos,
        u.hinpct(0.05),
        Color { 107, 161, 222, 255 },
        assets.sdfShader,
        textAlign
    );
}

void GameplayMenu::DrawTimerbox(Units &u, Assets &assets, float scoreY) {
    Rectangle TimerboxSrc {
        0, 0, float(assets.Timerbox.width), float(assets.Timerbox.height)
    };
    extern Encore::Settings TheGameSettings;
    float WidthOfTimerbox = u.hinpct(0.14);
    float HeightOfTimerbox = WidthOfTimerbox / 4;
    
    float hudOffsetX = 0.0f;
    float hudOffsetY = 0.0f;
    
    int effectiveHUDPosition = (ThePlayerManager.PlayersActive > 1) ? 0 : TheGameSettings.HUDPosition;
    
    switch (effectiveHUDPosition) {
        case 0:
            hudOffsetX = 0.0f;
            hudOffsetY = 0.0f;
            break;
        case 1:
            hudOffsetX = -(GetScreenWidth() - u.hinpct(0.28f) - u.hinpct(0.02f)) + 250.0f;
            hudOffsetY = 0.0f;
            break;
        case 2:
            hudOffsetX = 0.0f;
            hudOffsetY = GetScreenHeight() - u.hpct(0.30f);
            break;
        case 3:
            hudOffsetX = -(GetScreenWidth() - u.hinpct(0.28f) - u.hinpct(0.02f)) + 250.0f;
            hudOffsetY = GetScreenHeight() - u.hpct(0.30f);
            break;
        default:
            hudOffsetX = 0.0f;
            hudOffsetY = 0.0f;
            break;
    }
    
    float TimerboxX = u.RightSide + hudOffsetX;
    float TimerboxY = u.hpct(0.1425f) + hudOffsetY;
    
    Rectangle TimerboxDraw { TimerboxX, TimerboxY, WidthOfTimerbox, HeightOfTimerbox };
    Vector2 origin = Vector2{WidthOfTimerbox, HeightOfTimerbox};
    DrawTexturePro(
        assets.Timerbox,
        TimerboxSrc,
        TimerboxDraw,
        origin,
        0,
        WHITE
    );
    int played = TheSongTime.GetSongTime();
    int length = TheSongTime.GetSongLength();
    float Width = Remap(played, 0, length, 0, WidthOfTimerbox);
    
    float scissorX = TimerboxX - WidthOfTimerbox;
    float scissorY = TimerboxY - HeightOfTimerbox;
    
    BeginScissorMode(
        scissorX,
        scissorY,
        Width + 1,
        HeightOfTimerbox + 1
    );
    DrawTexturePro(
        assets.TimerboxOutline,
        TimerboxSrc,
        TimerboxDraw,
        origin,
        0,
        WHITE
    );
    EndScissorMode();
    int playedMinutes = played / 60;
    int playedSeconds = played % 60;
    int songMinutes = length / 60;
    int songSeconds = length % 60;
    const char *textTime = TextFormat(
        "%i:%02i / %i:%02i", playedMinutes, playedSeconds, songMinutes, songSeconds
    );
    
    Vector2 textPos = { TimerboxX - (WidthOfTimerbox / 2), scoreY - u.hinpct(SmallHeader) };
    
    GameMenu::mhDrawText(
        assets.rubik,
        textTime,
        textPos,
        u.hinpct(SmallHeader * 0.66),
        WHITE,
        assets.sdfShader,
        CENTER
    );
}

void GameplayMenu::DrawGameplayStars(
    Units &u, Assets &assets, float scorePos, float starY
) {
    int starsval = ThePlayerManager.BandStats->Stars();
    float starPercent = (float)ThePlayerManager.BandStats->Score
        / (float)ThePlayerManager.BandStats->BaseScore;
    extern Encore::Settings TheGameSettings;
    for (int i = 0; i < 5; i++) {
        bool firstStar = (i == 0);
        float starX;
        starX = scorePos - u.hinpct(0.26) + (i * u.hinpct(0.0525));
        float starWH = u.hinpct(0.05);
        Rectangle emptyStarWH = {
            0, 0, (float)assets.emptyStar.width, (float)assets.emptyStar.height
        };
        Rectangle starRect = { starX, starY, starWH, starWH };
        DrawTexturePro(assets.emptyStar, emptyStarWH, starRect, { 0, 0 }, 0, WHITE);
        float yMaskPos = Remap(
            starPercent,
            firstStar ? 0 : BAND_STAR_THRESHOLD[i - 1],
            BAND_STAR_THRESHOLD[i],
            0,
            u.hinpct(0.05)
        );
        BeginScissorMode(starX, (starY + starWH) - yMaskPos, starWH, yMaskPos);
        DrawTexturePro(
            assets.star,
            emptyStarWH,
            starRect,
            { 0, 0 },
            0,
            i == starsval ? Color { 192, 192, 192, 128 } : WHITE
        );
        EndScissorMode();
    }
    if (starPercent >= BAND_STAR_THRESHOLD[4]
        && ThePlayerManager.BandStats->EligibleForGoldStars) {
        float starWH = u.hinpct(0.05);
        Rectangle emptyStarWH = {
            0, 0, (float)assets.goldStar.width, (float)assets.goldStar.height
        };
        float yMaskPos = Remap(
            starPercent, BAND_STAR_THRESHOLD[4], BAND_STAR_THRESHOLD[5], 0, u.hinpct(0.05)
        );
        float scissorX = scorePos - (starWH * 6);
        float scissorWidth = starWH * 6;
        
        BeginScissorMode(
            scissorX, (starY + starWH) - yMaskPos, scissorWidth, yMaskPos
        );
        for (int i = 0; i < 5; i++) {
            float starX = scorePos - u.hinpct(0.26) + (i * u.hinpct(0.0525));
            Rectangle starRect = { starX, starY, starWH, starWH };
            DrawTexturePro(
                ThePlayerManager.BandStats->GoldStars() ? assets.goldStar
                                                        : assets.goldStarUnfilled,
                emptyStarWH,
                starRect,
                { 0, 0 },
                0,
                WHITE
            );
        }
        EndScissorMode();
    }
}

void GameplayMenu::DrawInstrumentIcon(Units &u, Assets &assets, float scoreY, double songTime) {
    extern Encore::Settings TheGameSettings;
    
    if (!TheGameSettings.ShowInstrumentIcon) {
        return;
    }
    
    if (ThePlayerManager.PlayersActive != 1) {
        return;
    }
    
    Player &player = ThePlayerManager.GetActivePlayer(0);
    
    if (player.Instrument != PartGuitar && player.Instrument != PlasticGuitar) {
        return;
    }
    
    Chart &curChart = TheSongList.curSong->parts[player.Instrument]->charts[player.Difficulty];
    
    if (!curChart.hasInstrumentTextEvents()) {
        return;
    }
    
    InstrumentType currentType = curChart.getCurrentInstrumentType(songTime);
    
    float iconSize = u.hinpct(0.10f);
    
    float iconAlpha = 1.0f;
    if (TheGameSettings.TrackFading && ThePlayerManager.PlayersActive == 1) {
        iconAlpha = TheGameRenderer.GetRendererAlpha(0);
    }
    
    if (iconAlpha < 0.01f) {
        return;
    }
    
    float iconX, iconY;
    
    Camera3D worldCamera = TheGameRenderer.cameraVectors[ThePlayerManager.PlayersActive - 1][TheGameRenderer.cameraSel];
    Vector2 trackRightEdge = GetWorldToScreen({ 2.5f, 0.0f, 2.0f }, worldCamera);
    Vector2 trackLeftEdge = GetWorldToScreen({ -2.5f, 0.0f, 2.0f }, worldCamera);
    float trackRightX = trackRightEdge.x - TheGameRenderer.renderPos;
    float trackLeftX = trackLeftEdge.x - TheGameRenderer.renderPos;
    
    int iconPosition = TheGameSettings.InstrumentIconPosition;
    
    float padding = u.hinpct(0.02f);
    
    if (iconPosition == 2 || iconPosition == 3) {
        iconSize = u.hinpct(0.09f);
    }
    
    switch (iconPosition) {
        case 0:
            iconX = trackLeftX - iconSize - padding;
            iconY = GetScreenHeight() * 0.5f - iconSize * 0.5f;
            break;
        case 1:
            iconX = trackRightX + padding;
            iconY = GetScreenHeight() * 0.5f - iconSize * 0.5f;
            break;
        case 2:
            iconX = trackLeftX - iconSize - padding;
            iconY = GetScreenHeight() - iconSize;
            break;
        case 3:
            iconX = trackRightX + padding;
            iconY = GetScreenHeight() - iconSize;
            break;
        default:
            iconX = trackLeftX - iconSize - padding;
            iconY = GetScreenHeight() * 0.5f - iconSize * 0.5f;
            break;
    }
    
    Texture2D iconTexture;
    if (currentType == InstrumentType::Guitar) {
        iconTexture = assets.InstIcons[2];
    } else {
        iconTexture = assets.InstIcons[3];
    }
    
    unsigned char alpha = (unsigned char)(iconAlpha * 255);
    Color iconColor = { 255, 255, 255, alpha };
    
    Rectangle srcRect = { 0, 0, (float)iconTexture.width, (float)iconTexture.height };
    Rectangle destRect = { iconX, iconY, iconSize, iconSize };
    DrawTexturePro(iconTexture, srcRect, destRect, { 0, 0 }, 0, iconColor);
}

void GameplayMenu::DrawNewHighScoreNotification(Units &u, Assets &assets, double currentTime) {
    if (!highScoreEffect.triggered) {
        return;
    }
    
    double timeSinceTrigger = currentTime - highScoreEffect.triggerTime;
    
    float displayDuration = 3.0f;
    float fadeOutDuration = 0.5f;
    
    if (timeSinceTrigger > displayDuration) {
        return;
    }
    
    float bgAlpha = 1.0f;
    if (timeSinceTrigger > displayDuration - fadeOutDuration) {
        bgAlpha = (displayDuration - timeSinceTrigger) / fadeOutDuration;
        if (bgAlpha < 0) bgAlpha = 0;
    }
    
    const char* text = "New High Score";
    float fontSize = u.hinpct(0.05f);
    Vector2 textSize = MeasureTextEx(assets.rubikBold, text, fontSize, 0);
    
    float textX = (GetScreenWidth() - textSize.x) / 2;
    float textY = GetScreenHeight() * 0.20f;
    
    float padding = u.hinpct(0.015f);
    float bgY = textY - padding;
    float bgHeight = textSize.y + padding * 2;
    
    unsigned char bgAlphaChar = (unsigned char)(bgAlpha * 180);
    DrawRectangle(0, bgY, GetScreenWidth(), bgHeight, Color{0, 0, 0, bgAlphaChar});
    
    unsigned char textAlpha = (unsigned char)(bgAlpha * 255);
    DrawTextEx(assets.rubikBold, text, {textX, textY}, fontSize, 0, Color{255, 255, 255, textAlpha});
}

unsigned char BeatToCharViaTickThing(
    int tick, int MinBrightness, int MaxBrightness, int QuarterNoteLength
) {
    float TickModulo = tick % QuarterNoteLength;
    return Remap(
        TickModulo / float(QuarterNoteLength), 0, 1.0f, MaxBrightness, MinBrightness
    );
}

void GameplayMenu::Draw() {
    Units &u = Units::getInstance();
    Assets &assets = Assets::getInstance();
    double curTime = GetTime();

    ClearBackground(BLACK);

    if (TheSongTime.ShouldResumeVideoAfterGracePeriod() && TheGameRenderer.backgroundVideo.IsLoaded()) {
        Encore::EncoreLog(LOG_INFO, "Grace period ended - resuming video playback");
        TheGameRenderer.backgroundVideo.Play();
        TheSongTime.SetVideoResumedAfterGracePeriod(true);
    }

    TheGameRenderer.backgroundVideo.Update();

    if (TheGameRenderer.backgroundVideo.IsLoaded()) {
        if (TheSongTime.IsInResumeGracePeriod()) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);
        } else if (TheGameRenderer.backgroundVideo.HasEnded()) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);
        } else {
            float totalAlpha = 0.0f;
            int activePlayerCount = 0;
            for (int i = 0; i < ThePlayerManager.PlayersActive; i++) {
                totalAlpha += TheGameRenderer.GetRendererAlpha(i);
                activePlayerCount++;
            }
            float averageAlpha = (activePlayerCount > 0) ? (totalAlpha / activePlayerCount) : 1.0f;
            
            float fadeAmount = TheGameSettings.BackgroundFade / 100.0f;
            float minBrightness = 1.0f - fadeAmount;
            float videoBrightness = minBrightness + (fadeAmount * (1.0f - averageAlpha));
            unsigned char brightness = (unsigned char)(videoBrightness * 255);
            Color videoTint = { brightness, brightness, brightness, 255 };
            TheGameRenderer.backgroundVideo.Draw(0, 0, videoTint);
        }
    } else {
        GameMenu::DrawAlbumArtBackground(TheSongList.curSong->albumArtBlur);
    }

    unsigned char BackgroundColor = 0;
    if (ThePlayerManager.BandStats->PlayersInOverdrive > 0) {
        BackgroundColor = BeatToCharViaTickThing(TheGameRenderer.CurrentTick, 0, 8, 960);
    }

    if (TheGameSettings.BackgroundTint) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color { 0, 0, 0, 128 });
    }
    DrawRectangle(
        0, 0, GetScreenWidth(), GetScreenHeight(), Color { 255, 255, 255, BackgroundColor }
    );

    if (!TheGameRenderer.streamsLoaded) {
        TheAudioManager.loadStreams(TheSongList.curSong->stemsPath);
        TheGameRenderer.streamsLoaded = true;
    } else {
        for (auto &stream : TheAudioManager.loadedStreams) {
            bool streamHandled = false;
            
            if (stream.instrument == 5) {
                TheAudioManager.SetAudioStreamVolume(
                    stream.handle,
                    TheGameSettings.avMainVolume * TheGameSettings.avBackingTrackVolume
                );
                streamHandled = true;
            }
            else if (stream.instrument == 4) {
                TheAudioManager.SetAudioStreamVolume(
                    stream.handle,
                    TheGameSettings.avMainVolume * TheGameSettings.avBackingTrackVolume
                );
                streamHandled = true;
            }
            
            if (!streamHandled) {
                for (int i = 0; i < ThePlayerManager.PlayersActive; i++) {
                    Player &player = ThePlayerManager.GetActivePlayer(i);
                    int playerInstrument = player.ClassicMode ? player.Instrument - 5 : player.Instrument;
                    
                    bool isPlayerStream = false;
                    
                    if (playerInstrument == stream.instrument) {
                        isPlayerStream = true;
                    }
                    
                    if (playerInstrument == 4) {
                        if (stream.instrument == 3 || stream.instrument == 4) {
                            isPlayerStream = true;
                        }
                    }
                    
                    if (isPlayerStream) {
                        TheAudioManager.SetAudioStreamVolume(
                            stream.handle,
                            player.stats->Mute
                                ? TheGameSettings.avMainVolume * TheGameSettings.avMuteVolume
                                : TheGameSettings.avMainVolume
                                    * TheGameSettings.avActiveInstrumentVolume
                        );
                        streamHandled = true;
                        break;
                    }
                }
                
                if (!streamHandled) {
                    TheAudioManager.SetAudioStreamVolume(
                        stream.handle,
                        TheGameSettings.avMainVolume * TheGameSettings.avInactiveInstrumentVolume
                    );
                }
            }
        }

        float songPlayed = TheSongTime.GetSongLength();

        if (TheSongTime.SongComplete()) {
            TheGameRenderer.LowerHighway();
        }
        if (TheSongTime.SongComplete()) {
            float songPlayed = TheSongTime.GetSongLength();
            CleanupAndSwitchToResults();
            Encore::EncoreLog(LOG_INFO, TextFormat("Song ended at at %f", songPlayed));
            return;
        }
    }

    for (int pnum = 0; pnum < ThePlayerManager.PlayersActive; pnum++) {
        TheGameRenderer.cameraSel =
            CameraSelectionPerPlayer[ThePlayerManager.PlayersActive - 1][pnum];
        int pos = CameraPosPerPlayer[ThePlayerManager.PlayersActive - 1][pnum];
        if (pos == 0)
            TheGameRenderer.renderPos =
                CameraPosPerPlayer[ThePlayerManager.PlayersActive - 1][pnum];
        else
            TheGameRenderer.renderPos = GetScreenWidth()
                / CameraPosPerPlayer[ThePlayerManager.PlayersActive - 1][pnum];

        TheGameRenderer.RenderGameplay(
            ThePlayerManager.GetActivePlayer(pnum),
            TheSongTime.GetSongTime(),
            *TheSongList.curSong
        );
        
        Player& player = ThePlayerManager.GetActivePlayer(pnum);
        if (pnum < 4) {
            int currentCombo = player.stats->Combo;
            StreakPopEffect& effect = streakPopEffects[pnum];
            
            int maxComboForMeter = player.stats->maxMultForMeter() * 10;
            
            if (currentCombo > 0 && currentCombo % 10 == 0 && currentCombo != effect.lastCombo 
                && currentCombo <= maxComboForMeter) {
                if (currentCombo > effect.lastCombo) {
                    effect.active = true;
                    effect.triggerTime = curTime;
                }
            }
            effect.lastCombo = currentCombo;
            
            if (effect.active) {
                Vector3 multMeterWorldPos = { 0.0f, 0.0f, 1.1f };
                Camera3D worldCamera = TheGameRenderer.cameraVectors[ThePlayerManager.PlayersActive - 1][TheGameRenderer.cameraSel];
                Vector2 multMeterScreenPos = GetWorldToScreen(multMeterWorldPos, worldCamera);
                
                float effectCenterX = multMeterScreenPos.x - TheGameRenderer.renderPos;
                float effectCenterY = multMeterScreenPos.y;
                
                DrawStreakPopEffect(pnum, effectCenterX, effectCenterY, curTime);
            }
        }
        
        std::string NameText = ThePlayerManager.GetActivePlayer(pnum).Name;
        if (ThePlayerManager.GetActivePlayer(pnum).Bot) NameText.append(" - AUTOPLAY");
        float CenterPosForText =
            GetWorldToScreen(
                { 0, 0, 0 },
                TheGameRenderer.cameraVectors[ThePlayerManager.PlayersActive - 1]
                                             [TheGameRenderer.cameraSel]
            )
                .x;
        float fontSize = u.hinpct(0.035);
        float textWidth = MeasureTextEx(
                              assets.rubikBold,
                              NameText.c_str(),
                              fontSize,
                              0
        )
                              .x;
        Color headerUsernameColor;
        if (ThePlayerManager.GetActivePlayer(pnum).Bot)
            headerUsernameColor = SKYBLUE;
        else {
            if (ThePlayerManager.GetActivePlayer(pnum).BrutalMode)
                headerUsernameColor = RED;
            else
                headerUsernameColor = WHITE;
        }
        DrawTextEx(
            assets.rubikBold,
            NameText.c_str(),
            { (CenterPosForText - (textWidth / 2)) - (TheGameRenderer.renderPos),
              GetScreenHeight() - u.hinpct(0.04) },
            fontSize,
            0,
            headerUsernameColor
        );
        
        if (TheGameSettings.TrackFading && pnum < TheGameRenderer.playerFadeStates.size()) {
            auto& fadeState = TheGameRenderer.playerFadeStates[pnum];
            if (fadeState.showCountdown && fadeState.nextNoteTime > 0 && fadeState.isFadedOut) {
                double currentTime = TheSongTime.GetSongTime();
                double timeUntilNextNote = fadeState.nextNoteTime - currentTime;
                
                double countdownTime = timeUntilNextNote - 3.0;
                
                if (timeUntilNextNote > 0 && countdownTime > 0) {
                    if (fadeState.countdownStartTime == 0.0) {
                        fadeState.countdownStartTime = currentTime;
                    }
                    
                    float ringSize = u.hinpct(0.08f);
                    float ringX = CenterPosForText - (ringSize / 2) - TheGameRenderer.renderPos;
                    float ringY = GetScreenHeight() - u.hinpct(0.04) - ringSize - u.hinpct(0.02f);
                    
                    int seconds = (int)ceil(countdownTime);
                    if (seconds < 1) seconds = 1;
                    if (seconds > 99) seconds = 99;
                    
                    float alpha = 255.0f;
                    
                    float fadeInDuration = 0.5f;
                    double timeSinceCountdownStart = currentTime - fadeState.countdownStartTime;
                    if (timeSinceCountdownStart < fadeInDuration) {
                        float fadeInAlpha = 255.0f * (timeSinceCountdownStart / fadeInDuration);
                        alpha = fadeInAlpha;
                    }
                    
                    if (countdownTime <= 1.0) {
                        float fadeOutAlpha = 255.0f * countdownTime;
                        if (fadeOutAlpha < alpha) {
                            alpha = fadeOutAlpha;
                        }
                    }
                    
                    if (alpha < 0) alpha = 0;
                    if (alpha > 255) alpha = 255;
                    
                    Color ringColor = { 255, 255, 255, (unsigned char)alpha };
                    Color textColor = { 255, 255, 255, (unsigned char)alpha };
                    
                    if (assets.CountInTexture.id > 0) {
                        DrawTexturePro(
                            assets.CountInTexture,
                            { 0, 0, (float)assets.CountInTexture.width, (float)assets.CountInTexture.height },
                            { ringX, ringY, ringSize, ringSize },
                            { 0, 0 },
                            0.0f,
                            ringColor
                        );
                    } else {
                        DrawRectangle(ringX, ringY, ringSize, ringSize, { 255, 0, 0, (unsigned char)alpha });
                    }
                    
                    const char* countdownText = TextFormat("%d", seconds);
                    float countdownFontSize = u.hinpct(0.03f);
                    Vector2 countdownTextSize = MeasureTextEx(assets.rubikBold, countdownText, countdownFontSize, 0);
                    
                    DrawTextEx(
                        assets.rubikBold,
                        countdownText,
                        { ringX + (ringSize - countdownTextSize.x) / 2,
                          ringY + (ringSize - countdownTextSize.y) / 2 },
                        countdownFontSize,
                        0,
                        textColor
                    );
                } else {
                    fadeState.countdownStartTime = 0.0;
                }
            }
        }
    }

    extern Encore::Settings TheGameSettings;
    
    float hudOffsetX = 0.0f;
    float hudOffsetY = 0.0f;
    
    int effectiveHUDPosition = (ThePlayerManager.PlayersActive > 1) ? 0 : TheGameSettings.HUDPosition;
    
    switch (effectiveHUDPosition) {
        case 0:
            hudOffsetX = 0.0f;
            hudOffsetY = 0.0f;
            break;
        case 1:
            hudOffsetX = -(GetScreenWidth() - u.hinpct(0.28f) - u.hinpct(0.02f)) + 250.0f;
            hudOffsetY = 0.0f;
            break;
        case 2:
            hudOffsetX = 0.0f;
            hudOffsetY = GetScreenHeight() - u.hpct(0.30f);
            break;
        case 3:
            hudOffsetX = -(GetScreenWidth() - u.hinpct(0.28f) - u.hinpct(0.02f)) + 250.0f;
            hudOffsetY = GetScreenHeight() - u.hpct(0.30f);
            break;
        default:
            hudOffsetX = 0.0f;
            hudOffsetY = 0.0f;
            break;
    }
    
    float scorePos = u.RightSide - u.hinpct(0.01f) + hudOffsetX;
    float scoreY = u.hpct(0.15f) + hudOffsetY;
    float starY = scoreY + u.hinpct(0.065f);
    
    DrawGameplayStars(u, assets, scorePos, starY);
    DrawTimerbox(u, assets, scoreY);
    DrawScorebox(u, assets, scoreY);
    DrawInstrumentIcon(u, assets, scoreY, TheSongTime.GetSongTime());
    
    if (highScoreEffect.highScoreLoaded && !highScoreEffect.triggered && highScoreEffect.previousHighScore > 0) {
        if (ThePlayerManager.BandStats->Score > highScoreEffect.previousHighScore) {
            highScoreEffect.triggered = true;
            highScoreEffect.triggerTime = curTime;
        }
    }
    DrawNewHighScoreNotification(u, assets, curTime);

    float SongNameWidth = MeasureTextEx(
                              assets.rubikBoldItalic,
                              TheSongList.curSong->title.c_str(),
                              u.hinpct(MediumHeader),
                              0
    )
                              .x;
    std::string SongArtistString = TheSongList.curSong->artist + ", "
        + TheSongList.curSong->releaseYear;
    float SongArtistWidth =
        MeasureTextEx(
            assets.rubikBoldItalic, SongArtistString.c_str(), u.hinpct(SmallHeader), 0
        )
            .x;

    float SongExtrasWidth = MeasureTextEx(
                                assets.rubikBoldItalic,
                                TheSongList.curSong->charters[0].c_str(),
                                u.hinpct(SmallHeader),
                                0
    )
                                .x;

    double SongNameDuration = 0.75f;
    unsigned char SongNameAlpha = 255;
    float SongNamePosition = 35;
    unsigned char SongArtistAlpha = 255;
    float SongArtistPosition = 35;
    unsigned char SongExtrasAlpha = 255;
    float SongExtrasPosition = 35;
    float SongNameBackgroundWidth =
        SongNameWidth >= SongArtistWidth ? SongNameWidth : SongArtistWidth;
    float SongBackgroundWidth = SongNameBackgroundWidth;
    if (curTime > TheSongTime.GetStartTime() + 7.5
        && curTime < TheSongTime.GetStartTime() + 7.5 + SongNameDuration) {
        double timeSinceStart = GetTime() - (TheSongTime.GetStartTime() + 7.5);
        SongNameAlpha = static_cast<unsigned char>(Remap(
            static_cast<float>(
                getEasingFunction(EaseOutCirc)(timeSinceStart / SongNameDuration)
            ),
            0,
            1.0,
            255,
            0
        ));
        SongNamePosition = Remap(
            static_cast<float>(
                getEasingFunction(EaseInOutBack)(timeSinceStart / SongNameDuration)
            ),
            0,
            1.0,
            35,
            -SongNameWidth
        );
    } else if (curTime > TheSongTime.GetStartTime() + 7.5 + SongNameDuration)
        SongNameAlpha = 0;

    if (curTime > TheSongTime.GetStartTime() + 7.75
        && curTime < TheSongTime.GetStartTime() + 7.75 + SongNameDuration) {
        double timeSinceStart = GetTime() - (TheSongTime.GetStartTime() + 7.75);
        SongArtistAlpha = static_cast<unsigned char>(Remap(
            static_cast<float>(
                getEasingFunction(EaseOutCirc)(timeSinceStart / SongNameDuration)
            ),
            0,
            1.0,
            255,
            0
        ));

        SongArtistPosition = Remap(
            static_cast<float>(
                getEasingFunction(EaseInOutBack)(timeSinceStart / SongNameDuration)
            ),
            0,
            1.0,
            35,
            -SongArtistWidth
        );
    }
    if (curTime > TheSongTime.GetStartTime() + 8
        && curTime < TheSongTime.GetStartTime() + 8 + SongNameDuration) {
        double timeSinceStart = GetTime() - (TheSongTime.GetStartTime() + 8);
        SongExtrasAlpha = static_cast<unsigned char>(Remap(
            static_cast<float>(
                getEasingFunction(EaseOutCirc)(timeSinceStart / SongNameDuration)
            ),
            0,
            1.0,
            255,
            0
        ));
        SongBackgroundWidth = Remap(
            static_cast<float>(
                getEasingFunction(EaseInOutCirc)(timeSinceStart / SongNameDuration)
            ),
            0,
            1.0,
            SongNameBackgroundWidth,
            0
        );

        SongExtrasPosition = Remap(
            static_cast<float>(
                getEasingFunction(EaseInOutBack)(timeSinceStart / SongNameDuration)
            ),
            0,
            1.0,
            35,
            -SongExtrasWidth
        );
    }
    if (curTime < TheSongTime.GetStartTime() + 7.75 + SongNameDuration) {
        DrawRectangleGradientH(
            0,
            u.hpct(0.19f),
            1.25 * SongBackgroundWidth,
            u.hinpct(0.02f + MediumHeader + SmallHeader + SmallHeader),
            Color { 0, 0, 0, 128 },
            Color { 0, 0, 0, 0 }
        );
        DrawTextEx(
            assets.rubikBoldItalic,
            TheSongList.curSong->title.c_str(),
            { SongNamePosition, u.hpct(0.2f) },
            u.hinpct(MediumHeader),
            0,
            Color { 255, 255, 255, SongNameAlpha }
        );
        DrawTextEx(
            assets.rubikItalic,
            SongArtistString.c_str(),
            { SongArtistPosition, u.hpct(0.2f + MediumHeader) },
            u.hinpct(SmallHeader),
            0,
            Color { 200, 200, 200, SongArtistAlpha }
        );
        DrawTextEx(
            assets.rubikItalic,
            TheSongList.curSong->charters[0].c_str(),
            { SongExtrasPosition, u.hpct(0.2f + MediumHeader + SmallHeader) },
            u.hinpct(SmallHeader),
            0,
            Color { 200, 200, 200, SongExtrasAlpha }
        );
    }

    int songLength;
    if (TheSongList.curSong->end == 0)
        songLength = static_cast<int>(TheAudioManager.GetMusicTimeLength());
    else
        songLength = static_cast<int>(TheSongList.curSong->end);

    GuiSetStyle(PROGRESSBAR, BORDER_WIDTH, 0);

    GuiSetStyle(DEFAULT, TEXT_SIZE, static_cast<int>(u.hinpct(0.03f)));
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetFont(assets.rubik);

    float floatSongLength = TheAudioManager.GetMusicTimePlayed();



    if (ThePlayerManager.BandStats->Paused) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color { 0, 0, 0, 80 });
        encOS::DrawTopOvershell(0.2f);
        SET_LARGE_BUTTON_STYLE();
        float Left = u.wpct(0.02f);
        float Width = u.winpct(0.2f);
        float Height = u.hinpct(0.08f);
        float Top = u.hpct(0.3f);
        float Spacing = u.hinpct(0.09f);
        Rectangle ResumeBox = { Left, Top, Width, Height };
        Rectangle RestartBox = { Left, Top + Spacing, Width, Height };
        Rectangle QuitBox = { Left, Top + (Spacing * 2), Width, Height };

        if (GuiButton(ResumeBox, "Resume")) {
            if (TheSongTime.IsInResumeGracePeriod()) {
                TheSongTime.ExtendGracePeriod();
            } else {
                TheSongTime.Resume();
            }
            TheAudioManager.unpauseStreams();
            
            ThePlayerManager.BandStats->Paused = false;
            for (int playerNum = 0; playerNum < ThePlayerManager.PlayersActive;
                 playerNum++) {
                ThePlayerManager.GetActivePlayer(playerNum).stats->Paused = false;
            }
        }
        if (GuiButton(RestartBox, "Restart")) {
            TheGameRenderer.ResetFadeState();
            
            if (TheGameRenderer.backgroundVideo.IsLoaded()) {
                TheGameRenderer.backgroundVideo.Stop();
                TheGameRenderer.backgroundVideo.Unload();
                
                if (TheGameSettings.VideoBackgrounds) {
                    std::filesystem::path videoPath = TheSongList.curSong->songInfoPath.parent_path() / "video.mp4";
                    if (TheGameRenderer.backgroundVideo.Load(videoPath)) {
                        if (TheSongList.curSong->videoEndTime > 0) {
                            TheGameRenderer.backgroundVideo.SetEndTime(TheSongList.curSong->videoEndTime);
                        }
                        
                        if (TheSongList.curSong->videoStartTime > 0) {
                            if (TheSongList.curSong->videoEndTime > 0) {
                                TheGameRenderer.backgroundVideo.PlayWithDelayAndEndTime(
                                    TheSongList.curSong->videoStartTime, 
                                    TheSongList.curSong->videoEndTime
                                );
                            } else {
                                TheGameRenderer.backgroundVideo.PlayWithDelay(TheSongList.curSong->videoStartTime);
                            }
                        } else {
                            TheGameRenderer.backgroundVideo.Play();
                        }
                    }
                }
            }
            TheSongTime.Reset();
            for (int player = 0; player < ThePlayerManager.PlayersActive; player++) {
                TheSongList.curSong
                    ->parts[ThePlayerManager.GetActivePlayer(player).Instrument]
                    ->charts[ThePlayerManager.GetActivePlayer(player).Difficulty]
                    .restartNotes();
            }

            TheGameRenderer.highwayInAnimation = false;
            TheGameRenderer.highwayInEndAnim = false;
            TheGameRenderer.songPlaying = false;
            TheGameRenderer.Restart = true;
            delete ThePlayerManager.BandStats;
            ThePlayerManager.BandStats = new BandGameplayStats;
            for (int playerNum = 0; playerNum < ThePlayerManager.PlayersActive;
                 playerNum++) {
                delete ThePlayerManager.GetActivePlayer(playerNum).stats;
                ThePlayerManager.GetActivePlayer(playerNum).stats =
                    new PlayerGameplayStats(
                        ThePlayerManager.GetActivePlayer(playerNum).Difficulty,
                        ThePlayerManager.GetActivePlayer(playerNum).Instrument
                    );
            }
            ThePlayerManager.BandStats->ResetBandGameplayStats();
            ThePlayerManager.BandStats->Paused = false;
        }
        if (GuiButton(QuitBox, "Back to Music Library")) {
            for (int playerNum = 0; playerNum < ThePlayerManager.PlayersActive; playerNum++) {
                Player& player = ThePlayerManager.GetActivePlayer(playerNum);
                if (player.stats) {
                    player.stats->Quit = true;
                }
            }
            CleanupAndSwitchToResults();
            SETDEFAULTSTYLE();
            return;
        }
        SETDEFAULTSTYLE();

        DrawTextEx(
            assets.rubikBoldItalic,
            "PAUSED",
            { u.wpct(0.02f), u.hpct(0.05f) },
            u.hinpct(0.1f),
            0,
            WHITE
        );

        float SongFontSize = u.hinpct(0.03f);

        float TitleHeight =
            MeasureTextEx(
                assets.rubikBoldItalic, TheSongList.curSong->title.c_str(), SongFontSize, 0
            )
                .y;
        float TitleWidth =
            MeasureTextEx(
                assets.rubikBoldItalic, TheSongList.curSong->title.c_str(), SongFontSize, 0
            )
                .x;
        float ArtistHeight =
            MeasureTextEx(
                assets.rubikItalic, TheSongList.curSong->artist.c_str(), SongFontSize, 0
            )
                .y;
        float ArtistWidth =
            MeasureTextEx(
                assets.rubikItalic, TheSongList.curSong->artist.c_str(), SongFontSize, 0
            )
                .x;
        if (!ThePlayerManager.BandStats->Multiplayer) {
            const char *instDiffText = TextFormat(
                "%s %s",
                diffList[ThePlayerManager.GetActivePlayer(0).Difficulty].c_str(),
                songPartsList[ThePlayerManager.GetActivePlayer(0).Instrument].c_str()
            );
            float InstDiffHeight =
                MeasureTextEx(assets.rubikBold, instDiffText, SongFontSize, 0).y;
            float InstDiffWidth =
                MeasureTextEx(assets.rubikBold, instDiffText, SongFontSize, 0).x;
            Vector2 SongInstDiffBox = { u.RightSide - InstDiffWidth - u.winpct(0.01f),
                                        u.hpct(0.1f) + (ArtistHeight / 2)
                                            + (InstDiffHeight * 0.1f) };
            DrawTextEx(
                assets.rubikBold, instDiffText, SongInstDiffBox, SongFontSize, 0, WHITE
            );
        }

        Vector2 SongTitleBox = { u.RightSide - TitleWidth - u.winpct(0.01f),
                                 u.hpct(0.1f) - (ArtistHeight / 2)
                                     - (TitleHeight * 1.1f) };
        Vector2 SongArtistBox = { u.RightSide - ArtistWidth - u.winpct(0.01f),
                                  u.hpct(0.1f) - (ArtistHeight / 2) };

        DrawTextEx(
            assets.rubikBoldItalic,
            TheSongList.curSong->title.c_str(),
            SongTitleBox,
            SongFontSize,
            0,
            WHITE
        );
        DrawTextEx(
            assets.rubikItalic,
            TheSongList.curSong->artist.c_str(),
            SongArtistBox,
            SongFontSize,
            0,
            WHITE
        );

        DrawOvershell();
    }

    GameMenu::DrawFPS(u.LeftSide, u.hpct(0.0025f) + u.hinpct(0.025f));
    GameMenu::DrawVersion();

    if (!ThePlayerManager.BandStats->Multiplayer
        && ThePlayerManager.GetActivePlayer(0).stats->Health <= 0) {
        CleanupAndSwitchToResults();
    }

    extern Encore::Settings TheGameSettings;
    if (ThePlayerManager.PlayersActive && !TheGameSettings.HideHitWindow) {
        DrawRectangle(
            u.wpct(0.5f) - (u.winpct(0.12f) / 2),
            u.hpct(0.02f) - u.winpct(0.01f),
            u.winpct(0.12f),
            u.winpct(0.065f),
            DARKGRAY
        );
        for (int fretBox = 0;
             fretBox < ThePlayerManager.GetActivePlayer(0).stats->HeldFrets.size();
             fretBox++) {
            float leftInputBoxSize = (5 * u.winpct(0.02f)) / 2;

            Color fretColor;
            if (TheGameSettings.ClassicNotesOnPad) {
                switch (fretBox) {
                default:
                    fretColor = BROWN;
                    break;
                case (0):
                    fretColor = GREEN;
                    break;
                case (1):
                    fretColor = RED;
                    break;
                case (2):
                    fretColor = YELLOW;
                    break;
                case (3):
                    fretColor = BLUE;
                    break;
                case (4):
                    fretColor = ORANGE;
                    break;
                }
            } else {
                fretColor = ThePlayerManager.GetActivePlayer(0).AccentColor;
            }

            DrawRectangle(
                u.wpct(0.5f) - leftInputBoxSize + (fretBox * u.winpct(0.02f)),
                u.hpct(0.02f),
                u.winpct(0.02f),
                u.winpct(0.02f),
                ThePlayerManager.GetActivePlayer(0).stats->HeldFrets[fretBox] ? fretColor
                                                                              : GRAY
            );
        }
        DrawRectangle(
            u.wpct(0.5f) - ((5 * u.winpct(0.02f)) / 2),
            u.hpct(0.02f) + u.winpct(0.025f),
            u.winpct(0.1f),
            u.winpct(0.01f),
            ThePlayerManager.GetActivePlayer(0).stats->UpStrum ? WHITE : GRAY
        );
        DrawRectangle(
            u.wpct(0.5f) - ((5 * u.winpct(0.02f)) / 2),
            u.hpct(0.02f) + u.winpct(0.035f),
            u.winpct(0.1f),
            u.winpct(0.01f),
            ThePlayerManager.GetActivePlayer(0).stats->DownStrum ? WHITE : GRAY
        );
    }
    
    if (TheGameSettings.ShowDebugTimers) {
        double trackTime = TheSongTime.GetSongTime();
        double videoTime = 0.0;
        if (TheGameRenderer.backgroundVideo.IsLoaded()) {
            videoTime = TheGameRenderer.backgroundVideo.GetCurrentPositionMs() / 1000.0;
        }
        
        float fontSize = u.hinpct(0.025f);
        float rightMargin = u.wpct(0.02f);
        float topMargin = u.hpct(0.15f);
        float lineHeight = fontSize * 1.3f;
        
        const char* trackTimeText = TextFormat("Track: %.3fs", trackTime);
        const char* videoTimeText = TextFormat("Video: %.3fs", videoTime);
        const char* diffText = TextFormat("Diff: %.3fs", trackTime - videoTime);
        
        Vector2 trackTimeSize = MeasureTextEx(assets.rubikBold, trackTimeText, fontSize, 0);
        Vector2 videoTimeSize = MeasureTextEx(assets.rubikBold, videoTimeText, fontSize, 0);
        Vector2 diffSize = MeasureTextEx(assets.rubikBold, diffText, fontSize, 0);
        
        float maxWidth = trackTimeSize.x;
        if (videoTimeSize.x > maxWidth) maxWidth = videoTimeSize.x;
        if (diffSize.x > maxWidth) maxWidth = diffSize.x;
        
        float boxWidth = maxWidth + u.winpct(0.02f);
        float boxHeight = lineHeight * 3 + u.hinpct(0.02f);
        float boxX = GetScreenWidth() - boxWidth - rightMargin;
        float boxY = topMargin;
        
        DrawRectangle(boxX, boxY, boxWidth, boxHeight, Color{0, 0, 0, 180});
        DrawRectangleLinesEx({boxX, boxY, boxWidth, boxHeight}, 2.0f, WHITE);
        
        float textX = boxX + u.winpct(0.01f);
        float textY = boxY + u.hinpct(0.01f);
        
        DrawTextEx(assets.rubikBold, trackTimeText, {textX, textY}, fontSize, 0, WHITE);
        textY += lineHeight;
        DrawTextEx(assets.rubikBold, videoTimeText, {textX, textY}, fontSize, 0, WHITE);
        textY += lineHeight;
        
        Color diffColor = WHITE;
        float diff = trackTime - videoTime;
        if (diff > 0.1f) diffColor = RED;
        else if (diff < -0.1f) diffColor = YELLOW;
        else diffColor = GREEN;
        
        DrawTextEx(assets.rubikBold, diffText, {textX, textY}, fontSize, 0, diffColor);
    }
}

void GameplayMenu::Load() {
    TheGameRenderer.ResetFadeState();
    
    for (int i = 0; i < 4; i++) {
        streakPopEffects[i].active = false;
        streakPopEffects[i].triggerTime = 0.0;
        streakPopEffects[i].lastCombo = 0;
    }
    
    highScoreEffect.triggered = false;
    highScoreEffect.triggerTime = 0.0;
    highScoreEffect.previousHighScore = 0;
    highScoreEffect.highScoreLoaded = false;
    
    if (ThePlayerManager.PlayersActive == 1) {
        Player &player = ThePlayerManager.GetActivePlayer(0);
        std::string songID = LeaderboardManager::GenerateSongID(
            TheSongList.curSong->title,
            TheSongList.curSong->artist
        );
        ScoreData highScore = LeaderboardManager::GetHighestScoreForInstrument(
            player.PlayerID,
            songID,
            static_cast<int>(player.Instrument)
        );
        highScoreEffect.previousHighScore = highScore.hasScore ? highScore.score : 0;
        highScoreEffect.highScoreLoaded = true;
    }
    
    TheSongList.curSong->LoadAlbumArt();
    
    if (TheGameSettings.VideoBackgrounds) {
        std::filesystem::path videoPath = TheSongList.curSong->songInfoPath.parent_path() / "video.mp4";
        if (TheGameRenderer.backgroundVideo.Load(videoPath)) {
            if (TheSongList.curSong->videoEndTime > 0) {
                TheGameRenderer.backgroundVideo.SetEndTime(TheSongList.curSong->videoEndTime);
            }
            
            if (TheSongList.curSong->videoStartTime > 0) {
                if (TheSongList.curSong->videoEndTime > 0) {
                    TheGameRenderer.backgroundVideo.PlayWithDelayAndEndTime(
                        TheSongList.curSong->videoStartTime, 
                        TheSongList.curSong->videoEndTime
                    );
                } else {
                    TheGameRenderer.backgroundVideo.PlayWithDelay(TheSongList.curSong->videoStartTime);
                }
            } else {
                TheGameRenderer.backgroundVideo.Play();
            }
        }
    }
    if (ThePlayerManager.PlayersActive > 1) {
        ThePlayerManager.BandStats->Multiplayer = true;
        for (int player = 0; player < ThePlayerManager.PlayersActive; player++) {
            ThePlayerManager.GetActivePlayer(player).stats->Multiplayer = true;
        }
    } else {
        ThePlayerManager.BandStats->Multiplayer = false;
        for (int player = 0; player < ThePlayerManager.PlayersActive; player++) {
            ThePlayerManager.GetActivePlayer(player).stats->Multiplayer = false;
        }
    }

    for (int i = 0; i < ThePlayerManager.PlayersActive; i++) {
        Player &player = ThePlayerManager.GetActivePlayer(i);
        player.stats->BaseScore = TheSongList.curSong->parts[player.Instrument]
                                      ->charts[player.Difficulty]
                                      .baseScore;
        if (i == 0) {
            ThePlayerManager.BandStats->BaseScore = player.stats->BaseScore;
        } else {
            ThePlayerManager.BandStats->BaseScore += player.stats->BaseScore;
        }
    }
}

