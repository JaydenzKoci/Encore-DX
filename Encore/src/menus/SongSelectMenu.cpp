//
// Created by marie on 16/11/2024.
//

#include "raylib.h"
#include "SongSelectMenu.h"


#include "MenuManager.h"
#include "gameMenu.h"
#include "raygui.h"
#include "settings.h"
#include "uiUnits.h"
#include "gameplay/gameplayRenderer.h"
#include "song/audio.h"
#include "song/songlist.h"
#include "song/scoring.h"
#include "assets.h"
#include "leaderboard/leaderboard.h"
#include "users/playerManager.h"
#include <filesystem>
#include <string>
#include <set>
#include <vector>

float EaseInOutQuad(float t) {
    t = t < 0.5f ? 2.0f * t * t : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) / 2.0f;
    return t;
}

SortType currentSortValue = SortType::Title;
Color AccentColor = {255, 0, 255, 255};
SongSelectMenu::~SongSelectMenu() {
    Unload();
}

void SongSelectMenu::Load() {
    if (TheSongList.songs.empty()) {
        TraceLog(LOG_ERROR, "Cannot load SongSelectMenu: No songs loaded!");
        TheMenuManager.SwitchScreen(MAIN_MENU);
        return;
    }

    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
        TraceLog(LOG_INFO, "Initialized audio device");
    }
    previewStartTime = 0.0;
    phaseStartTime = 0.0;
    currentPreviewVolume = 0.0f;
    previewState = PreviewState::FadeIn;
    animatingSongID = -1;
    prevAnimatingSongID = -1;
    pendingSongID = -1;
    selectionTime = 0.0;
    songTextMetrics.clear();

    if (TheSongList.curSong && !TheSongList.curSong->AlbumArtLoaded) {
        try {
            TheSongList.curSong->LoadAlbumArt();
            SetTextureWrap(TheSongList.curSong->albumArtBlur, TEXTURE_WRAP_REPEAT);
            SetTextureFilter(TheSongList.curSong->albumArtBlur, TEXTURE_FILTER_ANISOTROPIC_16X);
            TheSongList.curSong->AlbumArtLoaded = true;
            TraceLog(LOG_DEBUG, "Loaded album art for %s", TheSongList.curSong->title.c_str());
        } catch (const std::exception& e) {
            TraceLog(LOG_ERROR, "Failed to load album art for %s: %s", TheSongList.curSong->title.c_str(), e.what());
        }
    }

    if (TheSongList.curSong) {
        TheSongList.SongSelectOffset = TheSongList.curSong->songListPos - 5;
        if (TheSongList.SongSelectOffset < 1) TheSongList.SongSelectOffset = 1;
        if (!TheSongList.listMenuEntries.empty() && TheSongList.SongSelectOffset > TheSongList.listMenuEntries.size() - 10)
            TheSongList.SongSelectOffset = TheSongList.listMenuEntries.size() - 10;
        animatingSongID = TheSongList.curSong->songListPos - 1;
        animationStartTime = GetTime();
        try {
            ComputeSongTextMetrics(*TheSongList.curSong);
        } catch (const std::exception& e) {
            TraceLog(LOG_ERROR, "Failed to compute song text metrics: %s", e.what());
        }
    } else {
        Encore::EncoreLog(LOG_WARNING, "No current song selected for offset adjustment");
        TheSongList.SongSelectOffset = 1;
    }

    // TheGameRenderer.streamsLoaded = false;
    // TheGameRenderer.midiLoaded = false;
    if (!TheSongList.songs.empty()) {
        for (Song& song : TheSongList.songs) {
            try {
                if (!song.ini) {
                    song.LoadInfo(song.songInfoPath);
                } else {
                    song.LoadInfoINI(song.songInfoPath);
                }
                if (song.songID.empty()) {
                    song.songID = LeaderboardManager::GenerateSongID(song.title, song.artist);
                }
                ComputeSongTextMetrics(song);
            } catch (const std::exception& e) {
                TraceLog(LOG_ERROR, "Failed to load song info: %s", e.what());
            }
        }
    }
}

void SongSelectMenu::Unload() {
    if (!TheAudioManager.loadedStreams.empty()) {
        for (auto& stream : TheAudioManager.loadedStreams) {
            TheAudioManager.StopPlayback(stream.handle);
        }
        TheAudioManager.loadedStreams.clear();
    }
}

void SongSelectMenu::KeyboardInputCallback(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_UP) {
            if (TheSongList.curSong && TheSongList.curSong->songListPos > 0) {
                int currentPos = TheSongList.curSong->songListPos - 1;
                int newPos = currentPos - 1;
                
                while (newPos >= 0 && (TheSongList.listMenuEntries[newPos].isHeader || TheSongList.listMenuEntries[newPos].hiddenEntry)) {
                    newPos--;
                }
                
                if (newPos >= 0) {
                    prevAnimatingSongID = currentPos;
                    int songID = TheSongList.listMenuEntries[newPos].songListID;
                    TheSongList.curSong = &TheSongList.songs[songID];
                    animatingSongID = newPos;
                    animationStartTime = GetTime();
                    ComputeSongTextMetrics(*TheSongList.curSong);
                    
                    if (!TheAudioManager.loadedStreams.empty()) {
                        for (auto& stream : TheAudioManager.loadedStreams) {
                            TheAudioManager.StopPlayback(stream.handle);
                        }
                        TheAudioManager.loadedStreams.clear();
                        currentPreviewVolume = 0.0f;
                        previewState = PreviewState::FadeIn;
                    }
                    
                    if (!TheSongList.curSong->AlbumArtLoaded) {
                        try {
                            TheSongList.curSong->LoadAlbumArt();
                            TheSongList.curSong->AlbumArtLoaded = true;
                            SetTextureWrap(TheSongList.curSong->albumArtBlur, TEXTURE_WRAP_REPEAT);
                            SetTextureFilter(TheSongList.curSong->albumArtBlur, TEXTURE_FILTER_ANISOTROPIC_16X);
                        } catch (const std::exception& e) {
                            TraceLog(LOG_ERROR, "Failed to load album art: %s", e.what());
                        }
                    }
                    
                    pendingSongID = songID;
                    selectionTime = GetTime();
                }
            }
        } else if (key == GLFW_KEY_DOWN) {
            if (TheSongList.curSong) {
                int currentPos = TheSongList.curSong->songListPos - 1;
                int newPos = currentPos + 1;
                
                while (newPos < TheSongList.listMenuEntries.size() && (TheSongList.listMenuEntries[newPos].isHeader || TheSongList.listMenuEntries[newPos].hiddenEntry)) {
                    newPos++;
                }
                
                if (newPos < TheSongList.listMenuEntries.size()) {
                    prevAnimatingSongID = currentPos;
                    int songID = TheSongList.listMenuEntries[newPos].songListID;
                    TheSongList.curSong = &TheSongList.songs[songID];
                    animatingSongID = newPos;
                    animationStartTime = GetTime();
                    ComputeSongTextMetrics(*TheSongList.curSong);
                    
                    if (!TheAudioManager.loadedStreams.empty()) {
                        for (auto& stream : TheAudioManager.loadedStreams) {
                            TheAudioManager.StopPlayback(stream.handle);
                        }
                        TheAudioManager.loadedStreams.clear();
                        currentPreviewVolume = 0.0f;
                        previewState = PreviewState::FadeIn;
                    }
                    
                    if (!TheSongList.curSong->AlbumArtLoaded) {
                        try {
                            TheSongList.curSong->LoadAlbumArt();
                            TheSongList.curSong->AlbumArtLoaded = true;
                            SetTextureWrap(TheSongList.curSong->albumArtBlur, TEXTURE_WRAP_REPEAT);
                            SetTextureFilter(TheSongList.curSong->albumArtBlur, TEXTURE_FILTER_ANISOTROPIC_16X);
                        } catch (const std::exception& e) {
                            TraceLog(LOG_ERROR, "Failed to load album art: %s", e.what());
                        }
                    }
                    
                    pendingSongID = songID;
                    selectionTime = GetTime();
                }
            }
        }
    }
}
void SongSelectMenu::ControllerInputCallback(int joypadID, GLFWgamepadstate state) {}
std::string SecondsToTimeFormat(int seconds) {
    int minutes = seconds / 60;
    int remainingSeconds = seconds % 60;
    return TextFormat("%d:%02d", minutes, remainingSeconds);
}

void SongSelectMenu::ComputeSongTextMetrics(Song& song) {
    Units u = Units::getInstance();
    Assets& assets = Assets::getInstance();

    TextMetrics metrics;
    const int songTitleWidth = (u.winpct(0.25f)) - 6;
    const int songArtistWidth = (u.winpct(0.25f)) - 19;

    // i tried........ - Jaydenz
    metrics.titleFontSize = u.hinpct(0.035f);
    metrics.titleTextWidth = MeasureTextEx(assets.rubikBold, song.title.c_str(), metrics.titleFontSize, 0).x;
    metrics.titleNeedsScroll = false;
    
    if (metrics.titleTextWidth > songTitleWidth) {
        if (TheGameSettings.ScrollingSongText) {
            metrics.titleNeedsScroll = true;
        } else {
            metrics.titleFontSize = (songTitleWidth / metrics.titleTextWidth) * u.hinpct(0.035f);
            const float minFontSize = u.hinpct(0.02f);
            if (metrics.titleFontSize < minFontSize) metrics.titleFontSize = minFontSize;
            metrics.titleTextWidth = MeasureTextEx(assets.rubikBold, song.title.c_str(), metrics.titleFontSize, 0).x;
        }
    }

    metrics.artistFontSize = u.hinpct(0.025f);
    metrics.artistTextWidth = MeasureTextEx(assets.josefinSansItalic, song.artist.c_str(), metrics.artistFontSize, 0).x;
    metrics.artistNeedsScroll = false;
    
    if (metrics.artistTextWidth > songArtistWidth) {
        if (TheGameSettings.ScrollingSongText) {
            metrics.artistNeedsScroll = true;
        } else {
            metrics.artistFontSize = (songArtistWidth / metrics.artistTextWidth) * u.hinpct(0.025f);
            const float minFontSize = u.hinpct(0.02f);
            if (metrics.artistFontSize < minFontSize) metrics.artistFontSize = minFontSize;
            metrics.artistTextWidth = MeasureTextEx(assets.josefinSansItalic, song.artist.c_str(), metrics.artistFontSize, 0).x;
        }
    }

    if (song.songListPos >= 0) {
        songTextMetrics[song.songListPos] = metrics;
    }
}

float SongSelectMenu::GetScrollOffset(int songID, float textWidth, float maxWidth, double currentTime, bool isTitle, bool isSelected) {
    if (!TheGameSettings.ScrollingSongText || !isSelected) return 0.0f;
    
    int scrollKey = songID * 2 + (isTitle ? 0 : 1);
    
    if (scrollStartTimes.find(scrollKey) == scrollStartTimes.end()) {
        scrollStartTimes[scrollKey] = currentTime;
        scrollDirections[scrollKey] = true;
    }
    
    float maxScroll = textWidth - maxWidth;
    if (maxScroll <= 0) return 0.0f;
    
    const double scrollDuration = 4.0;
    
    double elapsed = currentTime - scrollStartTimes[scrollKey];
    const double pauseDuration = 2.0;
    const double totalCycleDuration = scrollDuration + pauseDuration;
    
    if (elapsed >= totalCycleDuration) {
        scrollDirections[scrollKey] = !scrollDirections[scrollKey];
        scrollStartTimes[scrollKey] = currentTime;
        elapsed = 0.0;
    }
    
    if (elapsed < scrollDuration) {
        float t = (float)(elapsed / scrollDuration);
        
        if (scrollDirections[scrollKey]) {
            return -t * maxScroll;
        } else {
            return -(1.0f - t) * maxScroll;
        }
    } else {
        if (scrollDirections[scrollKey]) {
            return -maxScroll;
        } else {
            return 0.0f;
        }
    }
}

void SongSelectMenu::UpdatePreviewVolume(double currentTime) {
    float targetVolume = TheGameSettings.avMainVolume * TheGameSettings.avMenuMusicVolume;
    float t;
    static PreviewState lastState = previewState;

    if (previewState != lastState) {
        lastState = previewState;
    }

    if (TheAudioManager.loadedStreams.empty()) {
        currentPreviewVolume = 0.0f;
        return;
    }

    switch (previewState) {
        case PreviewState::FadeIn:
            t = (currentTime - phaseStartTime) / fadeDuration;
            if (t >= 1.0f) {
                currentPreviewVolume = targetVolume;
                previewState = PreviewState::Playing;
                phaseStartTime = currentTime;
            } else {
                t = EaseInOutQuad(t);
                currentPreviewVolume = t * targetVolume;
            }
            break;
        case PreviewState::Playing:
            currentPreviewVolume = targetVolume;
            if (currentTime - phaseStartTime >= previewPlayDuration) {
                previewState = PreviewState::FadeOut;
                phaseStartTime = currentTime;
            }
            break;
        case PreviewState::FadeOut:
            t = (currentTime - phaseStartTime) / fadeDuration;
            if (t >= 1.0f) {
                currentPreviewVolume = 0.0f;
                previewState = PreviewState::Pause;
                phaseStartTime = currentTime;
            } else {
                t = EaseInOutQuad(t);
                currentPreviewVolume = (1.0f - t) * targetVolume;
            }
            break;
        case PreviewState::Pause:
            currentPreviewVolume = 0.0f;
            if (currentTime - phaseStartTime >= pauseDuration) {
                previewState = PreviewState::FadeIn;
                phaseStartTime = currentTime;
                if (TheSongList.curSong && !TheAudioManager.loadedStreams.empty()) {
                    float previewStartTimeSec = TheSongList.curSong->previewStartTime / 1000.0f;
                    TheAudioManager.seekStreams(previewStartTimeSec);
                    TheAudioManager.playStreams();
                }
            }
            break;
    }

    for (int i = 0; i < TheAudioManager.loadedStreams.size(); i++) {
        float volume = currentPreviewVolume;
        if (i == PartVocals) volume = 0;
        TheAudioManager.SetAudioStreamVolume(TheAudioManager.loadedStreams[i].handle, volume);
    }
}

void SongSelectMenu::Draw() {
    Assets &assets = Assets::getInstance();
    Units u = Units::getInstance();

    double curTime = GetTime();
    // -5 -4 -3 -2 -1 0 1 2 3 4 5 6
    if (pendingSongID >= 0 && curTime - selectionTime >= 0.75) {
        if (pendingSongID < TheSongList.songs.size()) {
            try {
                TheAudioManager.loadStreams(TheSongList.songs[pendingSongID].stemsPath);
                float previewStartTimeSec = TheSongList.songs[pendingSongID].previewStartTime / 1000.0f;
                TheAudioManager.seekStreams(previewStartTimeSec);
                TheAudioManager.playStreams();
                for (int j = 0; j < TheAudioManager.loadedStreams.size(); j++) {
                    float volume = 0.0f;
                    if (j == PartVocals) volume = 0;
                    TheAudioManager.SetAudioStreamVolume(TheAudioManager.loadedStreams[j].handle, volume);
                }
                previewStartTime = curTime;
                phaseStartTime = curTime;
                currentPreviewVolume = 0.0f;
                previewState = PreviewState::FadeIn;
            } catch (const std::exception& e) {
                TraceLog(LOG_ERROR, "Failed to load preview audio for song %d: %s", pendingSongID, e.what());
            }
        }
        pendingSongID = -1;
    }

    UpdatePreviewVolume(curTime);
    Vector2 mouseWheel = GetMouseWheelMoveV();
    int lastIntChosen = (int)mouseWheel.y;
    // set to specified height

    // Update song select offset based on mouse wheel
    if (TheSongList.SongSelectOffset <= TheSongList.listMenuEntries.size() && TheSongList.SongSelectOffset >= 1
        && TheSongList.listMenuEntries.size() >= 10) {
        TheSongList.SongSelectOffset -= (int)mouseWheel.y;
    }

    // prevent going past top
    if (TheSongList.SongSelectOffset < 1)
        TheSongList.SongSelectOffset = 1;

    // prevent going past bottom
    if (TheSongList.SongSelectOffset >= TheSongList.listMenuEntries.size() - 10)
        TheSongList.SongSelectOffset = TheSongList.listMenuEntries.size() - 10;

    // todo(3drosalia): clean this shit up after changing it
    Song SongToDisplayInfo = TheSongList.curSong ? *TheSongList.curSong : Song();
    if (TheSongList.curSong) {
        if (TheSongList.curSong->ini)
            TheSongList.curSong->LoadInfoINI(TheSongList.curSong->songInfoPath);
        else
            TheSongList.curSong->LoadInfo(TheSongList.curSong->songInfoPath);
    }

    BeginDrawing();
    ClearBackground(DARKGRAY);
    if (TheSongList.curSong && TheSongList.curSong->AlbumArtLoaded) {
        BeginShaderMode(assets.bgShader);
        DrawAlbumArtBackgroundPro(TheSongList.curSong->albumArtBlur, {0, 0, (float)TheSongList.curSong->albumArtBlur.width, (float)TheSongList.curSong->albumArtBlur.height});
        EndShaderMode();
    } else {
    }

    DrawRectangle(0, 0, u.RightSide - u.LeftSide, (float)GetScreenHeight(), GetColor(0x00000080));
    BeginScissorMode(0, u.hpct(0.15f), u.RightSide - u.winpct(0.25f), u.hinpct(0.7f));
    GameMenu::DrawTopOvershell(0.208333f);
    EndScissorMode();
    encOS::DrawTopOvershell(0.15f);

    GameMenu::DrawVersion();
    int AlbumX = u.RightSide - u.winpct(0.25f);
    int AlbumY = u.hpct(0.075f);
    int AlbumHeight = u.winpct(0.25f);
    int AlbumOuter = u.hinpct(0.01f);
    int AlbumInner = u.hinpct(0.005f);

    DrawTextEx(
        assets.josefinSansItalic,
        TextFormat("Sorted by: %s", sortTypes[(int)currentSortValue].c_str()),
        { u.LeftSide, u.hinpct(0.165f) },
        u.hinpct(0.03f),
        0,
        WHITE
    );
    DrawTextEx(
        assets.josefinSansItalic,
        TextFormat("Songs loaded: %01i", TheSongList.songs.size()),
        { AlbumX - (AlbumOuter * 2) - MeasureTextEx(assets.josefinSansItalic, TextFormat("Songs loaded: %01i", TheSongList.songs.size()), u.hinpct(0.03f), 0).x, u.hinpct(0.165f) },
        u.hinpct(0.03f),
        0,
        WHITE
    );

    float baseSongEntryHeight = u.hinpct(0.058333f);
    float selectedSongHeightMultiplier = 1.5f;
    float selectedSongEntryHeight = baseSongEntryHeight * selectedSongHeightMultiplier;
    float cumulativeYOffset = u.hpct(0.266666f);

    float buttonY = GetScreenHeight() - u.hpct(0.1475f);
    float buttonHeight = u.hinpct(0.05f);
    float maxScissorHeight = buttonY - u.hpct(0.15f) - u.hinpct(0.01f);
    
    float scissorHeight = u.hinpct(0.75f);
    if (TheSongList.curSong && TheSongList.curSong->songListPos > 0) {
        float totalListHeight = 0.0f;
        int selectedSongIndex = TheSongList.curSong->songListPos - 1;
        int totalSongs = TheSongList.listMenuEntries.size();
        int songsFromEnd = totalSongs - selectedSongIndex - 1;
        
        for (int i = TheSongList.SongSelectOffset; i < TheSongList.listMenuEntries.size() && i < TheSongList.SongSelectOffset + 10; i++) {
            if (TheSongList.listMenuEntries[i].hiddenEntry) continue;
            bool isAnimating = (i == selectedSongIndex && i == animatingSongID) || (i == prevAnimatingSongID);
            totalListHeight += isAnimating ? selectedSongEntryHeight : (i == selectedSongIndex ? selectedSongEntryHeight : baseSongEntryHeight);
        }
        scissorHeight = u.hpct(0.266666f) + totalListHeight - u.hpct(0.15f);
        
        if (songsFromEnd < 6 && scissorHeight > maxScissorHeight) {
            float extraHeightNeeded = selectedSongEntryHeight - baseSongEntryHeight;
            scissorHeight = maxScissorHeight + extraHeightNeeded;
            if (scissorHeight > buttonY - u.hpct(0.15f)) {
                scissorHeight = buttonY - u.hpct(0.15f);
            }
        } else if (scissorHeight > maxScissorHeight) {
            scissorHeight = maxScissorHeight;
        }
    }

    BeginScissorMode(0, u.hpct(0.15f), u.RightSide - u.winpct(0.25f), scissorHeight);
    for (int i = TheSongList.SongSelectOffset; i < (int)TheSongList.listMenuEntries.size() && i < TheSongList.SongSelectOffset + 10; i++) {
        if ((int)TheSongList.listMenuEntries.size() == i) break;
        float currentEntryHeight = baseSongEntryHeight;
        bool isCurSong = TheSongList.curSong && i == TheSongList.curSong->songListPos - 1;
        bool isDeselecting = i == prevAnimatingSongID && !isCurSong;
        if (isCurSong && i == animatingSongID) {
            float t = (float)(curTime - animationStartTime) / animationDuration;
            if (t < 1.0f) {
                t = EaseInOutQuad(t);
                currentEntryHeight = baseSongEntryHeight + (selectedSongEntryHeight - baseSongEntryHeight) * t;
            } else {
                currentEntryHeight = selectedSongEntryHeight;
                animatingSongID = -1;
            }
        }
        else if (isDeselecting) {
            float t = (float)(curTime - animationStartTime) / animationDuration;
            if (t < 1.0f) {
                t = EaseInOutQuad(t);
                currentEntryHeight = selectedSongEntryHeight - (selectedSongEntryHeight - baseSongEntryHeight) * t;
            } else {
                currentEntryHeight = baseSongEntryHeight;
                prevAnimatingSongID = -1;
            }
        }
        else if (isCurSong) {
            currentEntryHeight = selectedSongEntryHeight;
        }

        if (TheSongList.listMenuEntries[i].isHeader) {
            float songXPos = u.LeftSide + u.winpct(0.005f) - 2;
            float songYPos = cumulativeYOffset;
            DrawRectangle(0, songYPos, (u.RightSide - u.winpct(0.25f)), baseSongEntryHeight, ColorBrightness(AccentColor, -0.75f));
            std::string headerText = TheSongList.listMenuEntries[i].headerChar;
            DrawTextEx(assets.rubikBold, headerText.c_str(), { songXPos, songYPos + u.hinpct(0.0125f) }, u.hinpct(0.035f), 0, WHITE);
            cumulativeYOffset += baseSongEntryHeight;
        } else if (!TheSongList.listMenuEntries[i].hiddenEntry) {
            Font& artistFont = isCurSong ? assets.josefinSansItalic : assets.josefinSansItalic;
            Song& songi = TheSongList.songs[TheSongList.listMenuEntries[i].songListID];
            int songID = TheSongList.listMenuEntries[i].songListID;

            float songXPos = u.LeftSide + u.winpct(0.005f) - 2;
            float songYPos = cumulativeYOffset;
            GuiSetStyle(BUTTON, BORDER_WIDTH, 0);
            GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, 0);
            if (isCurSong) {
                GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(ColorBrightness(AccentColor, -0.4)));
            }
            if (GuiButton(Rectangle{ 0, songYPos, (u.RightSide - u.winpct(0.25f)), currentEntryHeight }, "")) {
                prevAnimatingSongID = TheSongList.curSong ? TheSongList.curSong->songListPos - 1 : -1;
                TheSongList.curSong = &TheSongList.songs[songID];
                animatingSongID = i;
                animationStartTime = curTime;
                ComputeSongTextMetrics(*TheSongList.curSong);
                if (!TheAudioManager.loadedStreams.empty()) {
                    for (auto& stream : TheAudioManager.loadedStreams) {
                        TheAudioManager.StopPlayback(stream.handle);
                    }
                    TheAudioManager.loadedStreams.clear();
                    currentPreviewVolume = 0.0f;
                    previewState = PreviewState::FadeIn;
                }
                if (!TheSongList.songs[songID].ini) {
                    TheSongList.songs[songID].LoadInfo(TheSongList.songs[songID].songInfoPath);
                } else {
                    TheSongList.songs[songID].LoadInfoINI(TheSongList.songs[songID].songInfoPath);
                }
                if (!TheSongList.songs[songID].AlbumArtLoaded) {
                    try {
                        TheSongList.songs[songID].LoadAlbumArt();
                        TheSongList.songs[songID].AlbumArtLoaded = true;
                        SetTextureWrap(TheSongList.songs[songID].albumArtBlur, TEXTURE_WRAP_REPEAT);
                        SetTextureFilter(TheSongList.songs[songID].albumArtBlur, TEXTURE_FILTER_ANISOTROPIC_16X);
                        TraceLog(LOG_DEBUG, "Loaded album art for %s", TheSongList.songs[songID].title.c_str());
                    } catch (const std::exception& e) {
                        TraceLog(LOG_ERROR, "Failed to load album art for %s: %s", TheSongList.songs[songID].title.c_str(), e.what());
                    }
                }
                pendingSongID = songID;
                selectionTime = curTime;
            }
            GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, 0x181827FF);

            int songTitleWidth = (u.winpct(0.25f)) - 6;
            int songArtistWidth = (u.winpct(0.25f)) - 6;
            int songLengthWidth = (u.winpct(0.1f)) - 6;

            float titleFontSize = u.hinpct(0.035f);
            float artistFontSize = u.hinpct(0.025f);
            if (songTextMetrics.find(songi.songListPos) != songTextMetrics.end()) {
                titleFontSize = songTextMetrics[songi.songListPos].titleFontSize;
                artistFontSize = songTextMetrics[songi.songListPos].artistFontSize;
            } else {
                ComputeSongTextMetrics(songi);
                if (songTextMetrics.find(songi.songListPos) != songTextMetrics.end()) {
                    titleFontSize = songTextMetrics[songi.songListPos].titleFontSize;
                    artistFontSize = songTextMetrics[songi.songListPos].artistFontSize;
                }
            }

            float textXOffset = 10;

            auto LightText = Color{ 203, 203, 203, 255 };
            
            float titleScrollOffset = 0.0f;
            float artistScrollOffset = 0.0f;
            
            if (songTextMetrics.find(songi.songListPos) != songTextMetrics.end()) {
                if (songTextMetrics[songi.songListPos].titleNeedsScroll) {
                    titleScrollOffset = GetScrollOffset(songi.songListPos, songTextMetrics[songi.songListPos].titleTextWidth, songTitleWidth, curTime, true, isCurSong);
                }
                if (songTextMetrics[songi.songListPos].artistNeedsScroll) {
                    artistScrollOffset = GetScrollOffset(songi.songListPos, songTextMetrics[songi.songListPos].artistTextWidth, songArtistWidth, curTime, false, isCurSong);
                }
            }
            
            if (!isCurSong) {
                int titleScrollKey = songi.songListPos * 2;
                int artistScrollKey = songi.songListPos * 2 + 1;
                scrollStartTimes.erase(titleScrollKey);
                scrollStartTimes.erase(artistScrollKey);
                scrollDirections.erase(titleScrollKey);
                scrollDirections.erase(artistScrollKey);
            }
            
            if (!TheGameSettings.CompactScoreDisplay && isCurSong && ThePlayerManager.PlayersActive > 0) {
                Player &player = ThePlayerManager.GetActivePlayer(0);
                std::string songIDForList = LeaderboardManager::GenerateSongID(songi.title, songi.artist);
                ScoreData highScoreForList = LeaderboardManager::GetHighestScoreForInstrument(player.PlayerID, songIDForList, static_cast<int>(GetScoreInstrumentFilter()));
                
                float titleOnlyHeight = titleFontSize;
                float titleY = songYPos + (currentEntryHeight - titleOnlyHeight) / 4;
                
                BeginScissorMode(songXPos + textXOffset, songYPos, songTitleWidth, currentEntryHeight / 2);
                DrawTextEx(
                    assets.rubikBold,
                    songi.title.c_str(),
                    { songXPos + textXOffset + titleScrollOffset, titleY },
                    titleFontSize,
                    0,
                    WHITE
                );
                EndScissorMode();
                
                float scoreY = titleY + titleFontSize + u.hinpct(0.002f);
                float scoreFontSize = u.hinpct(0.025f);
                
                // Draw instrument icon to the left of the score
                float iconSize = u.hinpct(0.022f);
                int iconIndex = GetScoreInstrumentIconIndex();
                DrawTexturePro(
                    assets.InstIcons[iconIndex],
                    { 0, 0, (float)assets.InstIcons[iconIndex].width, (float)assets.InstIcons[iconIndex].height },
                    { songXPos + textXOffset, scoreY, iconSize, iconSize },
                    { 0, 0 },
                    0,
                    WHITE
                );
                
                float scoreStartX = songXPos + textXOffset + iconSize + u.winpct(0.005f);
                int displayScore = highScoreForList.hasScore ? highScoreForList.score : 0;
                std::string scoreText = GameMenu::scoreCommaFormatter(displayScore);
                float scoreTextWidth = MeasureTextEx(assets.rubikBold, scoreText.c_str(), scoreFontSize, 0).x;
                
                DrawTextEx(assets.rubikBold, scoreText.c_str(), { scoreStartX, scoreY }, scoreFontSize, 0, GetColor(0x00adffFF));
                
                float starScale = u.hinpct(0.02f);
                float starX = scoreStartX + scoreTextWidth + u.winpct(0.01f);
                
                if (highScoreForList.hasScore) {
                    std::string percentageText = TextFormat("%.0f%%", highScoreForList.hitPercentage);
                    Color percentageColor = (highScoreForList.hitPercentage >= 100.0f) ? GOLD : WHITE;
                    float percentageTextWidth = MeasureTextEx(assets.rubik, percentageText.c_str(), u.hinpct(0.02f), 0).x;
                    
                    float percentageX = scoreStartX + scoreTextWidth + u.winpct(0.01f);
                    DrawTextEx(assets.rubik, percentageText.c_str(), { percentageX, scoreY + u.hinpct(0.002f) }, u.hinpct(0.02f), 0, percentageColor);
                    
                    if (highScoreForList.hitPercentage >= 100.0f) {
                        float crownSize = u.hinpct(0.02f);
                        DrawTexturePro(
                            assets.crown,
                            { 0, 0, (float)assets.crown.width, (float)assets.crown.height },
                            { percentageX + percentageTextWidth + u.winpct(0.003f), scoreY, crownSize, crownSize },
                            { 0, 0 },
                            0,
                            WHITE
                        );
                    }
                    
                    starX = percentageX + percentageTextWidth + u.winpct(0.01f);
                    if (highScoreForList.hitPercentage >= 100.0f) {
                        starX += u.hinpct(0.025f);
                    }
                }
                
                for (int s = 0; s < 5; s++) {
                    DrawTexturePro(
                        assets.emptyStar,
                        { 0, 0, (float)assets.emptyStar.width, (float)assets.emptyStar.height },
                        { starX + (s * starScale), scoreY, starScale, starScale },
                        { 0, 0 },
                        0,
                        WHITE
                    );
                }
                if (highScoreForList.hasScore) {
                    for (int s = 0; s < highScoreForList.stars; s++) {
                        DrawTexturePro(
                            highScoreForList.goldStars ? assets.goldStar : assets.star,
                            { 0, 0, (float)(highScoreForList.goldStars ? assets.goldStar.width : assets.star.width), (float)(highScoreForList.goldStars ? assets.goldStar.height : assets.star.height) },
                            { starX + (s * starScale), scoreY, starScale, starScale },
                            { 0, 0 },
                            0,
                            WHITE
                        );
                    }
                }
            } else {
                BeginScissorMode(songXPos + textXOffset, songYPos, songTitleWidth, currentEntryHeight);
                DrawTextEx(
                    assets.rubikBold,
                    songi.title.c_str(),
                    { songXPos + textXOffset + titleScrollOffset, songYPos + (currentEntryHeight - titleFontSize) / 2 },
                    titleFontSize,
                    0,
                    isCurSong ? WHITE : LightText
                );
                EndScissorMode();
            }
            
            BeginScissorMode(songXPos + textXOffset + songTitleWidth + 25, songYPos, songArtistWidth - 13, currentEntryHeight);
            DrawTextEx(
                artistFont,
                songi.artist.c_str(),
                { songXPos + textXOffset + songTitleWidth + 25 + artistScrollOffset, songYPos + (currentEntryHeight - artistFontSize) / 2 },
                artistFontSize,
                0,
                isCurSong ? WHITE : LightText
            );
            EndScissorMode();
            
            DrawTextEx(
                assets.josefinSansItalic,
                SecondsToTimeFormat(songi.length).c_str(),
                { songXPos + textXOffset + songTitleWidth + songArtistWidth + 63, songYPos + (currentEntryHeight - u.hinpct(0.025f)) / 2 },
                u.hinpct(0.025f),
                0,
                isCurSong ? WHITE : LightText
            );

            cumulativeYOffset += currentEntryHeight;
        }
    }
    EndScissorMode();
    
    if (TheSongList.listMenuEntries.size() > 10) {
        float scrollbarX = u.RightSide - u.winpct(0.25f) - u.winpct(0.01f);
        float songsLoadedTextY = u.hinpct(0.165f);
        float songsLoadedTextHeight = u.hinpct(0.03f);
        float scrollbarY = songsLoadedTextY + songsLoadedTextHeight + u.hinpct(0.01f);
        float scrollbarWidth = u.winpct(0.012f);
        float scrollbarHeight = scissorHeight - (scrollbarY - u.hpct(0.15f));
        
        DrawRectangle(scrollbarX, scrollbarY, scrollbarWidth, scrollbarHeight, Color{255, 255, 255, 50});
        
        int totalEntries = TheSongList.listMenuEntries.size();
        float scrollPercentage = (float)TheSongList.SongSelectOffset / (float)(totalEntries - 10);
        if (scrollPercentage < 0) scrollPercentage = 0;
        if (scrollPercentage > 1) scrollPercentage = 1;
        
        float thumbHeight = (10.0f / totalEntries) * scrollbarHeight;
        if (thumbHeight < u.hinpct(0.03f)) thumbHeight = u.hinpct(0.03f);
        
        float thumbY = scrollbarY + scrollPercentage * (scrollbarHeight - thumbHeight);
        
        Vector2 mousePos = GetMousePosition();
        bool mouseOnScrollbar = mousePos.x >= scrollbarX && mousePos.x <= scrollbarX + scrollbarWidth &&
                                mousePos.y >= scrollbarY && mousePos.y <= scrollbarY + scrollbarHeight;
        
        if (mouseOnScrollbar && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            isDraggingScrollbar = true;
        }
        
        if (isDraggingScrollbar && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            float clickY = mousePos.y - scrollbarY;
            float newScrollPercentage = clickY / scrollbarHeight;
            if (newScrollPercentage < 0) newScrollPercentage = 0;
            if (newScrollPercentage > 1) newScrollPercentage = 1;
            
            int newOffset = (int)(newScrollPercentage * (totalEntries - 10)) + 1;
            if (newOffset < 1) newOffset = 1;
            if (newOffset > totalEntries - 10) newOffset = totalEntries - 10;
            TheSongList.SongSelectOffset = newOffset;
        }
        
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            isDraggingScrollbar = false;
        }
        
        Color thumbColor = mouseOnScrollbar ? Color{255, 255, 255, 255} : Color{255, 255, 255, 200};
        DrawRectangle(scrollbarX, thumbY, scrollbarWidth, thumbHeight, thumbColor);
    }
    
    if (isDraggingScrollbar && currentSortValue == SortType::Title) {
        DrawRectangle(0, 0, u.RightSide - u.winpct(0.25f), GetScreenHeight(), Color{0, 0, 0, 150});
        
        std::string currentLetter = "";
        int songIndex = TheSongList.SongSelectOffset;
        
        if (songIndex < TheSongList.listMenuEntries.size()) {
            if (TheSongList.listMenuEntries[songIndex].isHeader) {
                currentLetter = TheSongList.listMenuEntries[songIndex].headerChar;
            } else if (!TheSongList.listMenuEntries[songIndex].hiddenEntry) {
                int songID = TheSongList.listMenuEntries[songIndex].songListID;
                if (songID < TheSongList.songs.size() && !TheSongList.songs[songID].title.empty()) {
                    currentLetter = std::string(1, TheSongList.songs[songID].title[0]);
                }
            }
        }
        
        if (!currentLetter.empty()) {
            float letterSize = u.hinpct(0.4f);
            Vector2 textSize = MeasureTextEx(assets.rubikBold, currentLetter.c_str(), letterSize, 0);
            float songListWidth = u.RightSide - u.winpct(0.25f);
            float songListCenterX = u.LeftSide + (songListWidth - u.LeftSide) / 2;
            float letterX = songListCenterX - (textSize.x / 2);
            float letterY = (GetScreenHeight() / 2) - (textSize.y / 2);
            DrawTextEx(assets.rubikBold, currentLetter.c_str(), {letterX, letterY}, letterSize, 0, WHITE);
        }
    }
    
    DrawRectangle(AlbumX - AlbumOuter, AlbumY + AlbumHeight, AlbumHeight + AlbumOuter, AlbumHeight + u.hinpct(0.01f), WHITE);
    DrawRectangle(AlbumX - AlbumInner, AlbumY + AlbumHeight, AlbumHeight, u.hinpct(0.075f) + AlbumHeight, GetColor(0x181827FF));
    DrawRectangle(AlbumX - AlbumOuter, AlbumY - AlbumInner, AlbumHeight + AlbumOuter, AlbumHeight + AlbumOuter, WHITE);
    DrawRectangle(AlbumX - AlbumInner, AlbumY, AlbumHeight, AlbumHeight, BLACK);
    if (TheSongList.curSong && TheSongList.curSong->AlbumArtLoaded) {
        DrawTexturePro(
            TheSongList.curSong->albumArt,
            Rectangle{ 0, 0, (float)TheSongList.curSong->albumArt.width, (float)TheSongList.curSong->albumArt.height },
            Rectangle{ (float)AlbumX - AlbumInner, (float)AlbumY, (float)AlbumHeight, (float)AlbumHeight },
            { 0, 0 },
            0,
            WHITE
        );
    } else {
        DrawRectangle(AlbumX - AlbumInner, AlbumY, AlbumHeight, AlbumHeight, DARKGRAY);
    }
    if (TheSongList.SongSelectOffset > 0) {
        std::string SongTitleForCharThingyThatsTemporary = "";
        int songIndex = TheSongList.SongSelectOffset;
        if (TheSongList.listMenuEntries[songIndex].isHeader && songIndex > 0 && !TheSongList.listMenuEntries[songIndex - 1].isHeader) {
            songIndex = TheSongList.SongSelectOffset - 1;
        } else if (!TheSongList.listMenuEntries[songIndex].isHeader) {
            songIndex = TheSongList.SongSelectOffset;
        } else if (songIndex + 1 < TheSongList.listMenuEntries.size() && !TheSongList.listMenuEntries[songIndex + 1].isHeader) {
            songIndex = TheSongList.SongSelectOffset + 1;
        }

        if (songIndex < TheSongList.listMenuEntries.size() && !TheSongList.listMenuEntries[songIndex].isHeader) {
            switch (currentSortValue) {
                case SortType::Title:
                    SongTitleForCharThingyThatsTemporary = TheSongList.songs[TheSongList.listMenuEntries[songIndex].songListID].title.empty() ? "#" : std::string(1, TheSongList.songs[TheSongList.listMenuEntries[songIndex].songListID].title[0]);
                    break;
                case SortType::Artist:
                    SongTitleForCharThingyThatsTemporary = TheSongList.songs[TheSongList.listMenuEntries[songIndex].songListID].artist.empty() ? "#" : std::string(1, TheSongList.songs[TheSongList.listMenuEntries[songIndex].songListID].artist[0]);
                    break;
                case SortType::Source:
                    SongTitleForCharThingyThatsTemporary = TheSongList.songs[TheSongList.listMenuEntries[songIndex].songListID].source.empty() ? "Unknown" : TheSongList.songs[TheSongList.listMenuEntries[songIndex].songListID].source;
                    break;
                case SortType::Length:
                    SongTitleForCharThingyThatsTemporary = TheSongList.listMenuEntries[TheSongList.SongSelectOffset].headerChar;
                    break;
                case SortType::Year:
                    SongTitleForCharThingyThatsTemporary = TheSongList.songs[TheSongList.listMenuEntries[songIndex].songListID].releaseYear.empty() ? "Unknown Year" : TheSongList.songs[TheSongList.listMenuEntries[songIndex].songListID].releaseYear;
                    break;
                case SortType::Score:
                    SongTitleForCharThingyThatsTemporary = "By Score";
                    break;
                default:
                    SongTitleForCharThingyThatsTemporary = "";
                    break;
            }
        }
        if (SongTitleForCharThingyThatsTemporary.empty()) {
            SongTitleForCharThingyThatsTemporary = TheSongList.listMenuEntries[TheSongList.SongSelectOffset].headerChar;
        }
        DrawTextEx(assets.rubikBold, SongTitleForCharThingyThatsTemporary.c_str(), { u.LeftSide + 5, u.hpct(0.218333f) }, u.hinpct(0.035f), 0, WHITE);
    }

    float TextPlacementTB = u.hpct(0.05f);
    float TextPlacementLR = u.LeftSide;
    GameMenu::mhDrawText(assets.redHatDisplayBlack, "MUSIC LIBRARY", { TextPlacementLR, TextPlacementTB }, u.hinpct(0.125f), WHITE, assets.sdfShader, LEFT);

    std::string albumText = SongToDisplayInfo.album.empty() ? "No Album Listed" : SongToDisplayInfo.album;
    std::string yearText = SongToDisplayInfo.releaseYear.empty() ? "Unknown Year" : SongToDisplayInfo.releaseYear;
    std::string albumDisplayText = albumText + " | " + yearText;
    float albumTextHeight = MeasureTextEx(assets.rubikBold, albumDisplayText.c_str(), u.hinpct(0.035f), 0).y;
    float albumTextWidth = MeasureTextEx(assets.rubikBold, albumDisplayText.c_str(), u.hinpct(0.035f), 0).x;
    float albumNameTextCenter = u.RightSide - u.winpct(0.125f) - AlbumInner;
    float albumTTop = AlbumY + AlbumHeight + u.hinpct(0.011f);
    float albumNameFontSize = albumTextWidth <= u.winpct(0.25f) ? u.hinpct(0.035f) : u.winpct(0.23f) / (albumTextWidth / albumTextHeight);
    float albumNameLeft = albumNameTextCenter - (MeasureTextEx(assets.rubikBold, albumDisplayText.c_str(), albumNameFontSize, 0).x / 2);
    float albumNameTextTop = albumTextWidth <= u.winpct(0.25f) ? albumTTop : albumTTop + ((u.hinpct(0.035f) / 2) - (albumNameFontSize / 2));
    DrawTextEx(assets.rubikBold, albumDisplayText.c_str(), { albumNameLeft, albumNameTextTop }, albumNameFontSize, 0, WHITE);

    DrawLine(u.RightSide - AlbumHeight - AlbumOuter, AlbumY + AlbumHeight + AlbumOuter + (u.hinpct(0.04f)), u.RightSide, AlbumY + AlbumHeight + AlbumOuter + (u.hinpct(0.04f)), WHITE);

    if (TheSongList.curSong && ThePlayerManager.PlayersActive > 0) {
        Player &player = ThePlayerManager.GetActivePlayer(0);
        std::string songID = LeaderboardManager::GenerateSongID(
            TheSongList.curSong->title,
            TheSongList.curSong->artist
        );
        ScoreData highScore = LeaderboardManager::GetHighestScoreForInstrument(player.PlayerID, songID, static_cast<int>(GetScoreInstrumentFilter()));
        
        if (TheGameSettings.CompactScoreDisplay) {
            float scoreDisplayY = AlbumY + AlbumHeight + AlbumOuter + (u.hinpct(0.05f));
            float scoreDisplayX = u.RightSide - AlbumHeight + AlbumInner;
            
            float iconSize = u.hinpct(0.035f);
            int iconIndex = GetScoreInstrumentIconIndex();
            DrawTexturePro(
                assets.InstIcons[iconIndex],
                { 0, 0, (float)assets.InstIcons[iconIndex].width, (float)assets.InstIcons[iconIndex].height },
                { scoreDisplayX, scoreDisplayY, iconSize, iconSize },
                { 0, 0 },
                0,
                WHITE
            );
            
            float scoreStartX = scoreDisplayX + iconSize + u.winpct(0.008f);
            int displayScore = highScore.hasScore ? highScore.score : 0;
            std::string scoreText = GameMenu::scoreCommaFormatter(displayScore);
            float scoreFontSize = u.hinpct(0.04f);
            float scoreTextWidth = MeasureTextEx(assets.rubikBold, scoreText.c_str(), scoreFontSize, 0).x;
            
            DrawTextEx(assets.rubikBold, scoreText.c_str(), { scoreStartX, scoreDisplayY }, scoreFontSize, 0, GetColor(0x00adffFF));
            
            if (highScore.hasScore) {
                std::string percentageText = TextFormat("%.0f%%", highScore.hitPercentage);
                Color percentageColor = (highScore.hitPercentage >= 100.0f) ? GOLD : WHITE;
                float percentageTextWidth = MeasureTextEx(assets.rubik, percentageText.c_str(), u.hinpct(0.035f), 0).x;
                
                float percentageX = scoreStartX + scoreTextWidth + u.winpct(0.01f);
                DrawTextEx(assets.rubik, percentageText.c_str(), { percentageX, scoreDisplayY + u.hinpct(0.002f) }, u.hinpct(0.035f), 0, percentageColor);
                
                if (highScore.hitPercentage >= 100.0f) {
                    float crownSize = u.hinpct(0.035f);
                    DrawTexturePro(
                        assets.crown,
                        { 0, 0, (float)assets.crown.width, (float)assets.crown.height },
                        { percentageX + percentageTextWidth + u.winpct(0.005f), scoreDisplayY, crownSize, crownSize },
                        { 0, 0 },
                        0,
                        WHITE
                    );
                }
            }
            
            float starScale = u.hinpct(0.03f);
            float starY = scoreDisplayY + scoreFontSize + u.hinpct(0.005f);
            for (int i = 0; i < 5; i++) {
                DrawTexturePro(
                    assets.emptyStar,
                    { 0, 0, (float)assets.emptyStar.width, (float)assets.emptyStar.height },
                    { scoreDisplayX + (i * starScale), starY, starScale, starScale },
                    { 0, 0 },
                    0,
                    WHITE
                );
            }
            if (highScore.hasScore) {
                for (int i = 0; i < highScore.stars; i++) {
                    DrawTexturePro(
                        highScore.goldStars ? assets.goldStar : assets.star,
                        { 0, 0, (float)(highScore.goldStars ? assets.goldStar.width : assets.star.width), (float)(highScore.goldStars ? assets.goldStar.height : assets.star.height) },
                        { scoreDisplayX + (i * starScale), starY, starScale, starScale },
                        { 0, 0 },
                        0,
                        WHITE
                    );
                }
                
                float statsY = starY + starScale + u.hinpct(0.01f);
                float statsFontSize = u.hinpct(0.025f);
                std::string statsText = TextFormat("%d Perfects  %d Goods  %d Misses", highScore.perfectHits, highScore.goodHits, highScore.misses);
                DrawTextEx(assets.rubik, statsText.c_str(), { scoreDisplayX, statsY }, statsFontSize, 0, WHITE);
            }
        }
    }

    float DiffTop = TheGameSettings.CompactScoreDisplay ? AlbumY + AlbumHeight + AlbumOuter + (u.hinpct(0.2f)) : AlbumY + AlbumHeight + AlbumOuter + (u.hinpct(0.045f));
    float IconWidth = float(AlbumHeight - AlbumOuter) / 5.0f;
    int maxInstruments = TheGameSettings.CompactScoreDisplay ? 5 : 10;
    
    if (!TheGameSettings.CompactScoreDisplay) {
        GameMenu::mhDrawText(assets.rubikItalic, "Pad", { (u.RightSide - AlbumHeight + AlbumInner), DiffTop }, AlbumOuter * 3, WHITE, assets.sdfShader, LEFT);
        GameMenu::mhDrawText(assets.rubikItalic, "Classic", { (u.RightSide - AlbumHeight + AlbumInner), DiffTop + IconWidth + (AlbumOuter * 3) }, AlbumOuter * 3, WHITE, assets.sdfShader, LEFT);
    }
    
    for (int i = 0; i < maxInstruments; i++) {
        bool RowTwo = i < 5;
        int RowTwoInt = i - 5;
        float PosTopAddition = TheGameSettings.CompactScoreDisplay ? 0 : (RowTwo ? AlbumOuter * 3 : AlbumOuter * 6);
        float BoxTopPos = DiffTop + PosTopAddition + float(IconWidth * (TheGameSettings.CompactScoreDisplay ? 0 : (RowTwo ? 0 : 1)));
        float ResetToLeftPos = (float)(TheGameSettings.CompactScoreDisplay ? i : (RowTwo ? i : RowTwoInt));
        int asdasd = (float)(TheGameSettings.CompactScoreDisplay ? i : (RowTwo ? i : RowTwoInt));
        float IconLeftPos = (float)(u.RightSide - AlbumHeight) + IconWidth * ResetToLeftPos;
        Rectangle Placement = { IconLeftPos, BoxTopPos, IconWidth, IconWidth };
        Color TintColor = WHITE;
        if (SongToDisplayInfo.parts[i] && SongToDisplayInfo.parts[i]->diff == -1) TintColor = DARKGRAY;
        DrawTexturePro(assets.InstIcons[asdasd], { 0, 0, (float)assets.InstIcons[asdasd].width, (float)assets.InstIcons[asdasd].height }, Placement, { 0, 0 }, 0, TintColor);
        DrawTexturePro(assets.BaseRingTexture, { 0, 0, (float)assets.BaseRingTexture.width, (float)assets.BaseRingTexture.height }, Placement, { 0, 0 }, 0, ColorBrightness(WHITE, 2));
        if (SongToDisplayInfo.parts[i] && SongToDisplayInfo.parts[i]->diff > 0)
            DrawTexturePro(assets.YargRings[SongToDisplayInfo.parts[i]->diff - 1], { 0, 0, (float)assets.YargRings[SongToDisplayInfo.parts[i]->diff - 1].width, (float)assets.YargRings[SongToDisplayInfo.parts[i]->diff - 1].height }, Placement, { 0, 0 }, 0, WHITE);
    }

    GameMenu::DrawBottomOvershell();
    
    float buttonWidth = u.winpct(0.18f);
    float buttonGap = u.winpct(0.02f);
    
    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(ColorBrightness(AccentColor, -0.25)));
    GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    if (GuiButton(Rectangle{ u.LeftSide, buttonY + 2.5f, buttonWidth, buttonHeight - 2.5f }, "Play Song")) {
        if (TheSongList.curSong) {
            if (!TheSongList.curSong->ini) {
                TheSongList.curSong->LoadSong(TheSongList.curSong->songInfoPath);
            } else {
                TheSongList.curSong->LoadSongIni(TheSongList.curSong->songDir);
            }
            if (!TheAudioManager.loadedStreams.empty()) {
                for (auto& stream : TheAudioManager.loadedStreams) {
                    TheAudioManager.StopPlayback(stream.handle);
                }
                TheAudioManager.loadedStreams.clear();
            }
            TheMenuManager.SwitchScreen(READY_UP);
        }
    }
    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, 0x181827FF);
    GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);

    if (GuiButton(Rectangle{ u.LeftSide + (buttonWidth + buttonGap) * 2, buttonY + 2.5f, buttonWidth, buttonHeight - 2.5f }, "Sort")) {
        int selectedSongIndex = -1;
        if (TheSongList.curSong) {
            for (size_t i = 0; i < TheSongList.songs.size(); i++) {
                if (&TheSongList.songs[i] == TheSongList.curSong) {
                    selectedSongIndex = i;
                    break;
                }
            }
        }
        prevAnimatingSongID = TheSongList.curSong ? TheSongList.curSong->songListPos - 1 : -1;
        currentSortValue = NextSortType(currentSortValue);
        
        if (currentSortValue == SortType::Score) {
            Player &player = ThePlayerManager.GetActivePlayer(0);
            TheSongList.sortListByScore(static_cast<int>(GetScoreInstrumentFilter()), selectedSongIndex, player.PlayerID);
        } else {
            TheSongList.sortList(currentSortValue, selectedSongIndex);
        }
        
        if (selectedSongIndex >= 0 && selectedSongIndex < TheSongList.songs.size()) {
            TheSongList.curSong = &TheSongList.songs[selectedSongIndex];
            TheSongList.SongSelectOffset = TheSongList.curSong->songListPos - 5;
            if (TheSongList.SongSelectOffset < 1) TheSongList.SongSelectOffset = 1;
            if (TheSongList.SongSelectOffset > TheSongList.listMenuEntries.size() - 10)
                TheSongList.SongSelectOffset = TheSongList.listMenuEntries.size() - 10;
            if (!TheSongList.curSong->AlbumArtLoaded) {
                try {
                    TheSongList.curSong->LoadAlbumArt();
                    TheSongList.curSong->AlbumArtLoaded = true;
                    SetTextureWrap(TheSongList.curSong->albumArtBlur, TEXTURE_WRAP_REPEAT);
                    SetTextureFilter(TheSongList.curSong->albumArtBlur, TEXTURE_FILTER_ANISOTROPIC_16X);
                } catch (const std::exception& e) {
                }
            }
            if (!TheAudioManager.loadedStreams.empty()) {
                for (auto& stream : TheAudioManager.loadedStreams) {
                    TheAudioManager.StopPlayback(stream.handle);
                }
                TheAudioManager.loadedStreams.clear();
                currentPreviewVolume = 0.0f;
                previewState = PreviewState::FadeIn;
            }
            pendingSongID = selectedSongIndex;
            selectionTime = curTime;
            animatingSongID = TheSongList.curSong->songListPos - 1;
            animationStartTime = curTime;
            ComputeSongTextMetrics(*TheSongList.curSong);
        }
    }
    
    // Instrument filter button - cycles through Vocals -> Bass -> Lead -> Drums
    // Also re-sorts if currently sorted by Score
    if (GuiButton(Rectangle{ u.LeftSide + (buttonWidth + buttonGap) * 3, buttonY + 2.5f, buttonWidth, buttonHeight - 2.5f }, GetScoreInstrumentName())) {
        CycleScoreInstrumentFilter();
        
        // Re-sort by score if currently sorted by score
        if (currentSortValue == SortType::Score) {
            int selectedSongIndex = -1;
            if (TheSongList.curSong) {
                for (size_t i = 0; i < TheSongList.songs.size(); i++) {
                    if (&TheSongList.songs[i] == TheSongList.curSong) {
                        selectedSongIndex = i;
                        break;
                    }
                }
            }
            prevAnimatingSongID = TheSongList.curSong ? TheSongList.curSong->songListPos - 1 : -1;
            Player &player = ThePlayerManager.GetActivePlayer(0);
            TheSongList.sortListByScore(static_cast<int>(GetScoreInstrumentFilter()), selectedSongIndex, player.PlayerID);
            
            if (selectedSongIndex >= 0 && selectedSongIndex < TheSongList.songs.size()) {
                TheSongList.curSong = &TheSongList.songs[selectedSongIndex];
                TheSongList.SongSelectOffset = TheSongList.curSong->songListPos - 5;
                if (TheSongList.SongSelectOffset < 1) TheSongList.SongSelectOffset = 1;
                if (TheSongList.SongSelectOffset > TheSongList.listMenuEntries.size() - 10)
                    TheSongList.SongSelectOffset = TheSongList.listMenuEntries.size() - 10;
                animatingSongID = TheSongList.curSong->songListPos - 1;
                animationStartTime = curTime;
            }
        }
    }
    
    GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    if (GuiButton(Rectangle{ u.LeftSide + buttonWidth + buttonGap, buttonY + 2.5f, buttonWidth, buttonHeight - 2.5f }, "Back")) {
        if (!TheAudioManager.loadedStreams.empty()) {
            for (auto& stream : TheAudioManager.loadedStreams) {
                TheAudioManager.StopPlayback(stream.handle);
            }
            TheAudioManager.loadedStreams.clear();
        }
        TheMenuManager.SwitchScreen(MAIN_MENU);
    }
    DrawOvershell();
}
