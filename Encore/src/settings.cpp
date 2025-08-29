//
// Created by marie on 02/10/2024.
//

#include "settings.h"
#include "settings-old.h"
#include <raylib.h>
#include <fstream>
#include <nlohmann/json.hpp>

namespace Encore {
    Settings TheGameSettings;

    void Settings::SaveToFile(const std::string& filename) const {
        try {
            nlohmann::json j = *this;
            std::ofstream file(filename);
            if (!file.is_open()) {
                TraceLog(LOG_ERROR, "Failed to open settings file for writing: %s", filename.c_str());
                return;
            }
            file << j.dump(4);
            file.close();
            TraceLog(LOG_INFO, "Settings saved to %s", filename.c_str());
        } catch (const std::exception& e) {
            TraceLog(LOG_ERROR, "Error saving settings to %s: %s", filename.c_str(), e.what());
        }
    }

    void Settings::LoadFromFile(const std::string& filename) {
        try {
            std::ifstream file(filename);
            if (!file.is_open()) {
                TraceLog(LOG_WARNING, "Settings file not found, using defaults: %s", filename.c_str());
                return;
            }
            nlohmann::json j;
            file >> j;
            file.close();
            j.get_to(*this);
            TraceLog(LOG_INFO, "Settings loaded from %s", filename.c_str());
        } catch (const std::exception& e) {
            TraceLog(LOG_ERROR, "Error loading settings from %s: %s", filename.c_str(), e.what());
        }
    }

    // SettingsInit methods
    void SettingsInit::InitSettings(std::filesystem::path directory) {
        this->directory = directory;
        SettingsOld& settingsMain = SettingsOld::getInstance();
        settingsMain.setDirectory(directory);

        bool settingsFileExists = exists(directory / "settings.json");

        if (settingsFileExists) {
            this->ReadSettings(); // Load existing modern settings
        }

        // If song paths are not loaded from modern settings, try legacy migration
        if (TheGameSettings.SongPaths.empty()) {
            this->LegacyMigrateSettings(); // This will load old settings and save to new settings
        }

        // After attempting to load/migrate, if SongPaths is STILL empty, set the default.
        // This typically happens on a fresh install.
        if (TheGameSettings.SongPaths.empty()) {
            TheGameSettings.SongPaths = {directory / "Songs"};
            TraceLog(LOG_INFO, "No song paths found, setting default: %s", (directory / "Songs").string().c_str());
            this->CreateSettings(); // Create settings.json with the default path
        }
    }

    void SettingsInit::ReadSettings() {
        nlohmann::json SettingsFile;
        std::ifstream f(directory / "settings.json");
        if (!f.is_open()) {
            TraceLog(LOG_ERROR, "Failed to open settings.json for reading in %s", directory.string().c_str());
            return;
        }
        SettingsFile = nlohmann::json::parse(f);
        f.close();
        Encore::from_json(SettingsFile, TheGameSettings);
    }

    void SettingsInit::CreateSettings() {
        nlohmann::json SettingsFile = TheGameSettings;
        Encore::WriteJsonFile(directory / "settings.json", SettingsFile);
    }

    void SettingsInit::LegacyMigrateSettings() {
        SettingsOld& settingsMain = SettingsOld::getInstance();
        settingsMain.setDirectory(directory);
        if (exists(directory / "keybinds.json")) {
            settingsMain.migrateKeybindsToOldSettings(
                directory / "keybinds.json", directory / "settings-old.json");
            TraceLog(LOG_INFO, "Migrated keybinds.json to settings-old.json");
        }
        if (exists(directory / "settings-old.json")) {
            settingsMain.loadOldSettings(directory / "settings-old.json");
            TraceLog(LOG_INFO, "Loaded settings-old.json for keybinds and offsets");
            TheGameSettings.SongPaths = settingsMain.songPaths;
            TheGameSettings.avMainVolume = settingsMain.MainVolume;
            TheGameSettings.avActiveInstrumentVolume = settingsMain.PlayerVolume;
            TheGameSettings.avInactiveInstrumentVolume = settingsMain.BandVolume;
            TheGameSettings.avSoundEffectVolume = settingsMain.SFXVolume;
            TheGameSettings.avMuteVolume = settingsMain.MissVolume;
            TheGameSettings.avMenuMusicVolume = settingsMain.MenuVolume;
            TheGameSettings.Fullscreen = settingsMain.fullscreen;
            TheGameSettings.AudioOffset = settingsMain.avOffsetMS;
            TheGameSettings.SaveToFile((directory / "settings.json").string());
            TraceLog(LOG_INFO, "Final SongPaths after migration:");
            for (const auto& path : TheGameSettings.SongPaths) {
                TraceLog(LOG_INFO, "  - %s", path.string().c_str());
            }
        }
    }
}
