// Created by marie on 02/10/2024.
//

#ifndef SETTINGS_H
#define SETTINGS_H
#include "GLFW/glfw3.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

namespace nlohmann {
    template <>
    struct adl_serializer<std::filesystem::path> {
        static void to_json(json& j, const std::filesystem::path& p) {
            j = p.string();
        }
        static void from_json(const json& j, std::filesystem::path& p) {
            p = std::filesystem::path(j.get<std::string>());
        }
    };
}

#define SETTINGS_OPTIONS                                                                 \
    OPTION(float, avMainVolume, 0.25f)                                                    \
    OPTION(float, avActiveInstrumentVolume, 0.75f)                                       \
    OPTION(float, avInactiveInstrumentVolume, 0.5f)                                      \
    OPTION(float, avSoundEffectVolume, 0.5f)                                             \
    OPTION(float, avMuteVolume, 0.15f)                                                   \
    OPTION(float, avMenuMusicVolume, 0.15f)                                              \
    OPTION(float, avBackingTrackVolume, 0.5f)                                           \
    OPTION(bool, Fullscreen, false)                                                      \
    OPTION(int, AudioOffset, 0)                                                          \
    OPTION(bool, DiscordRichPresence, true)                                              \
    OPTION(int, Framerate, 60)                                                           \
    OPTION(bool, VerticalSync, true)                                                     \
    OPTION(bool, BackgroundBeatFlash, true)                                              \
    OPTION(bool, BackgroundTint, true)                                                   \
    OPTION(bool, HideHitWindow, false)                                                     \
    OPTION(bool, ShowHealthBar, true)                                                    \
    OPTION(bool, HideFPSCounter, false)                                                  \
    OPTION(bool, HideVersionInfo, false)                                                 \
    OPTION(int, HUDPosition, 0)                                                          \
    OPTION(bool, TrackFading, true)                                                  \
    OPTION(bool, VideoBackgrounds, true)                                                \
    OPTION(int, VideoBackgroundResolution, 0)                                           \
    OPTION(int, BackgroundFade, 50)                                                     \
    OPTION(bool, ClassicNotesOnPad, false)                                              \
    OPTION(bool, ShowDebugTimers, false)                                                \
    OPTION(int, InputOffset, 0)                                                         \
    OPTION(bool, MirrorMode, false)                                                     \
    OPTION(int, TrackSpeed, 4)                                                          \
    OPTION(float, HighwayLengthMult, 1.0f)                                              \
    OPTION(bool, MissHighwayColor, false)                                               \
    OPTION(int, ControllerType, 0)                                                      \
    OPTION(bool, ScrollingSongText, true)                                               \
    OPTION(bool, CompactScoreDisplay, false)                                            \
    OPTION(int, ScoreInstrumentFilter, 4)                                               \
    OPTION(bool, ShowInstrumentIcon, true)                                              \
    OPTION(int, InstrumentIconPosition, 1)
namespace Encore {
    inline void WriteJsonFile(const std::filesystem::path &FileToWrite, const nlohmann::json &JSONobject) {
        std::ofstream o(FileToWrite, std::ios::out | std::ios::trunc);
        o << JSONobject.dump(2, ' ', false, nlohmann::detail::error_handler_t::strict);
        o.close();
    }
    class Settings {
    public:
#define OPTION(type, value, default) type value = default;
        SETTINGS_OPTIONS
#undef OPTION
        std::vector<std::filesystem::path> SongPaths;
        std::vector<bool> EnabledSongPaths;
        
        std::vector<std::filesystem::path> GetEnabledSongPaths() const {
            std::vector<std::filesystem::path> enabled;
            for (size_t i = 0; i < SongPaths.size(); i++) {
                if (i < EnabledSongPaths.size() && EnabledSongPaths[i]) {
                    enabled.push_back(SongPaths[i]);
                } else if (i >= EnabledSongPaths.size()) {
                    enabled.push_back(SongPaths[i]);
                }
            }
            return enabled;
        }
        
        std::vector<int> Keybinds4K = {68, 70, 74, 75};
        std::vector<int> Keybinds5K = {68, 70, 74, 75, 76};
        std::vector<int> Keybinds4KAlt = {-2, -2, -2, -2};
        std::vector<int> Keybinds5KAlt = {-2, -2, -2, -2, -2};
        int KeybindStrumUp = 265;
        int KeybindStrumDown = 264;
        int KeybindOverdrive = 340;
        int KeybindOverdriveAlt = -2;
        int KeybindPause = 256;
        
        std::vector<int> Controller4K = {14, 12, 2, 1};
        std::vector<int> Controller5K = {14, 12, 2, 3, 1};
        std::vector<int> Controller4KAxisDirection = {0, 0, 0, 0};
        std::vector<int> Controller5KAxisDirection = {0, 0, 0, 0, 0};
        int ControllerOverdrive = -6;
        int ControllerOverdriveAxisDirection = 1;
        int ControllerPause = 7;
        int ControllerPauseAxisDirection = 0;
        
        std::vector<float> TrackSpeedOptions = {0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f};
        
        void SaveToFile(const std::string& filename) const;
        void LoadFromFile(const std::string& filename);
        void SaveIfChanged(const std::string& filename);
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
        Settings,
        avMainVolume,
        avActiveInstrumentVolume,
        avInactiveInstrumentVolume,
        avSoundEffectVolume,
        avMuteVolume,
        avMenuMusicVolume,
        avBackingTrackVolume,
        Fullscreen,
        Framerate,
        VerticalSync,
        AudioOffset,
        DiscordRichPresence,
        SongPaths,
        EnabledSongPaths,
        BackgroundBeatFlash,
        BackgroundTint,
        HideHitWindow,
        ShowHealthBar,
        HideFPSCounter,
        HideVersionInfo,
        HUDPosition,
        TrackFading,
        VideoBackgrounds,
        VideoBackgroundResolution,
        BackgroundFade,
        ClassicNotesOnPad,
        ShowDebugTimers,
        InputOffset,
        MirrorMode,
        TrackSpeed,
        HighwayLengthMult,
        MissHighwayColor,
        ControllerType,
        ScrollingSongText,
        CompactScoreDisplay,
        ScoreInstrumentFilter,
        ShowInstrumentIcon,
        InstrumentIconPosition,
        Keybinds4K,
        Keybinds5K,
        Keybinds4KAlt,
        Keybinds5KAlt,
        KeybindStrumUp,
        KeybindStrumDown,
        KeybindOverdrive,
        KeybindOverdriveAlt,
        KeybindPause,
        Controller4K,
        Controller5K,
        Controller4KAxisDirection,
        Controller5KAxisDirection,
        ControllerOverdrive,
        ControllerOverdriveAxisDirection,
        ControllerPause,
        ControllerPauseAxisDirection,
        TrackSpeedOptions
    );

    class SettingsInit {
        std::filesystem::path directory;
        void ReadSettings();
        void CreateSettings();
        void MigrateSettings();
        void LegacyMigrateSettings();
        void ConvertOldSettingsToNew();
        void MergeWithDefaults();
    public:
        void InitSettings(std::filesystem::path directory);
        bool ConvertSettingsIfNeeded(std::filesystem::path directory);
        static bool ConvertOldSettingsToNewFormat(std::filesystem::path directory);
        std::filesystem::path GetSettingsDirectory() const { return directory; }
        std::string GetSettingsFilePath() const { return (directory / "settings.json").string(); }
    };
}

extern Encore::Settings TheGameSettings;
extern Encore::SettingsInit TheSettingsInitializer;

#endif // SETTINGS_H