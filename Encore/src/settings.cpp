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

    void Settings::SaveIfChanged(const std::string& filename) {
        static nlohmann::json lastSavedState;
        nlohmann::json currentJson = *this;
        
        if (currentJson != lastSavedState) {
            SaveToFile(filename);
            lastSavedState = currentJson;
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
            
            nlohmann::json currentDefaults = *this;
            currentDefaults.update(j);
            currentDefaults.get_to(*this);
            
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
        bool oldSettingsFileExists = exists(directory / "settings-old.json");

        if (settingsFileExists) {
            this->ReadSettings();
            this->MergeWithDefaults();
        } else {
            TraceLog(LOG_INFO, "No settings file found, creating default settings.json");
            if (TheGameSettings.SongPaths.empty()) {
                TheGameSettings.SongPaths = {directory / "Songs"};
            }
            this->CreateSettings();
        }

        if (oldSettingsFileExists) {
            TraceLog(LOG_INFO, "Found settings-old.json, migrating settings and removing old file");
            this->LegacyMigrateSettings();
            
            try {
                std::filesystem::remove(directory / "settings-old.json");
                TraceLog(LOG_INFO, "Removed settings-old.json after successful migration");
            } catch (const std::exception& e) {
                TraceLog(LOG_WARNING, "Failed to remove settings-old.json: %s", e.what());
            }
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

    void SettingsInit::MergeWithDefaults() {
        bool needsSave = false;
        
        if (TheGameSettings.SongPaths.empty()) {
            TheGameSettings.SongPaths = {directory / "Songs"};
            needsSave = true;
            TraceLog(LOG_INFO, "Added default SongPaths to existing settings");
        }
        
        if (needsSave) {
            TheGameSettings.SaveIfChanged((directory / "settings.json").string());
            TraceLog(LOG_INFO, "Updated settings.json with missing default values");
        }
    }

    void SettingsInit::LegacyMigrateSettings() {
        SettingsOld& settingsMain = SettingsOld::getInstance();
        settingsMain.setDirectory(directory);
        
        if (exists(directory / "settings-old.json")) {
            settingsMain.loadOldSettings(directory / "settings-old.json");
            TraceLog(LOG_INFO, "Loaded settings-old.json for migration");
        }
        
        if (exists(directory / "keybinds.json")) {
            TraceLog(LOG_INFO, "Found keybinds.json, loading directly");
            try {
                std::ifstream keybindsFile(directory / "keybinds.json");
                if (keybindsFile.is_open()) {
                    nlohmann::json keybindsJson;
                    keybindsFile >> keybindsJson;
                    keybindsFile.close();
                    
                    if (keybindsJson.contains("keybinds")) {
                        auto kb = keybindsJson["keybinds"];
                        if (kb.contains("4k") && kb["4k"].is_array() && kb["4k"].size() == 4) {
                            TheGameSettings.Keybinds4K = kb["4k"].get<std::vector<int>>();
                        }
                        if (kb.contains("5k") && kb["5k"].is_array() && kb["5k"].size() == 5) {
                            TheGameSettings.Keybinds5K = kb["5k"].get<std::vector<int>>();
                        }
                        if (kb.contains("4kAlt") && kb["4kAlt"].is_array() && kb["4kAlt"].size() == 4) {
                            TheGameSettings.Keybinds4KAlt = kb["4kAlt"].get<std::vector<int>>();
                        }
                        if (kb.contains("5kAlt") && kb["5kAlt"].is_array() && kb["5kAlt"].size() == 5) {
                            TheGameSettings.Keybinds5KAlt = kb["5kAlt"].get<std::vector<int>>();
                        }
                        if (kb.contains("overdrive")) TheGameSettings.KeybindOverdrive = kb["overdrive"].get<int>();
                        if (kb.contains("overdriveAlt")) TheGameSettings.KeybindOverdriveAlt = kb["overdriveAlt"].get<int>();
                        if (kb.contains("pause")) TheGameSettings.KeybindPause = kb["pause"].get<int>();
                        if (kb.contains("strumUp")) TheGameSettings.KeybindStrumUp = kb["strumUp"].get<int>();
                        if (kb.contains("strumDown")) TheGameSettings.KeybindStrumDown = kb["strumDown"].get<int>();
                    }
                    
                    if (keybindsJson.contains("avOffset")) TheGameSettings.AudioOffset = keybindsJson["avOffset"].get<int>();
                    if (keybindsJson.contains("inputOffset")) TheGameSettings.InputOffset = keybindsJson["inputOffset"].get<int>();
                }
            } catch (const std::exception& e) {
                TraceLog(LOG_ERROR, "Failed to load keybinds.json: %s", e.what());
            }
        }
        
        if (exists(directory / "settings-old.json")) {
            TheGameSettings.SongPaths = settingsMain.songPaths;
            TheGameSettings.avMainVolume = settingsMain.MainVolume;
            TheGameSettings.avActiveInstrumentVolume = settingsMain.PlayerVolume;
            TheGameSettings.avInactiveInstrumentVolume = settingsMain.BandVolume;
            TheGameSettings.avSoundEffectVolume = settingsMain.SFXVolume;
            TheGameSettings.avMuteVolume = settingsMain.MissVolume;
            TheGameSettings.avMenuMusicVolume = settingsMain.MenuVolume;
            TheGameSettings.Fullscreen = settingsMain.fullscreen;
            TheGameSettings.AudioOffset = settingsMain.avOffsetMS;
            TheGameSettings.InputOffset = settingsMain.inputOffsetMS;
            TheGameSettings.MirrorMode = settingsMain.mirrorMode;
            TheGameSettings.TrackSpeed = settingsMain.trackSpeed;
            TheGameSettings.HighwayLengthMult = settingsMain.highwayLengthMult;
            TheGameSettings.MissHighwayColor = settingsMain.missHighwayColor;
            TheGameSettings.ControllerType = settingsMain.controllerType;
            
            TheGameSettings.Keybinds4K = settingsMain.keybinds4K;
            TheGameSettings.Keybinds5K = settingsMain.keybinds5K;
            TheGameSettings.Keybinds4KAlt = settingsMain.keybinds4KAlt;
            TheGameSettings.Keybinds5KAlt = settingsMain.keybinds5KAlt;
            TheGameSettings.KeybindStrumUp = settingsMain.keybindStrumUp;
            TheGameSettings.KeybindStrumDown = settingsMain.keybindStrumDown;
            TheGameSettings.KeybindOverdrive = settingsMain.keybindOverdrive;
            TheGameSettings.KeybindOverdriveAlt = settingsMain.keybindOverdriveAlt;
            TheGameSettings.KeybindPause = settingsMain.keybindPause;
            TheGameSettings.Controller4K = settingsMain.controller4K;
            TheGameSettings.Controller5K = settingsMain.controller5K;
            TheGameSettings.Controller4KAxisDirection = settingsMain.controller4KAxisDirection;
            TheGameSettings.Controller5KAxisDirection = settingsMain.controller5KAxisDirection;
            TheGameSettings.ControllerOverdrive = settingsMain.controllerOverdrive;
            TheGameSettings.ControllerOverdriveAxisDirection = settingsMain.controllerOverdriveAxisDirection;
            TheGameSettings.ControllerPause = settingsMain.controllerPause;
            TheGameSettings.ControllerPauseAxisDirection = settingsMain.controllerPauseAxisDirection;
            
            TheGameSettings.TrackSpeedOptions = settingsMain.trackSpeedOptions;
        }
        
        TraceLog(LOG_INFO, "Successfully migrated settings to new format");
        TheGameSettings.SaveToFile((directory / "settings.json").string());
        
        try {
            if (exists(directory / "keybinds.json")) {
                std::filesystem::remove(directory / "keybinds.json");
                TraceLog(LOG_INFO, "Removed keybinds.json after migration");
            }
        } catch (const std::exception& e) {
            TraceLog(LOG_WARNING, "Failed to remove keybinds.json: %s", e.what());
        }
    }

    bool SettingsInit::ConvertSettingsIfNeeded(std::filesystem::path directory) {
        this->directory = directory;
        bool oldSettingsExists = exists(directory / "settings-old.json");
        bool newSettingsExists = exists(directory / "settings.json");
        
        if (oldSettingsExists) {
            TraceLog(LOG_INFO, "Converting settings-old.json to settings.json format");
            this->ConvertOldSettingsToNew();
            return true;
        }
        
        return false;
    }

    void SettingsInit::ConvertOldSettingsToNew() {
        SettingsOld& settingsMain = SettingsOld::getInstance();
        settingsMain.setDirectory(directory);
        
        if (exists(directory / "settings-old.json")) {
            settingsMain.loadOldSettings(directory / "settings-old.json");
            
            TheGameSettings.SongPaths = settingsMain.songPaths;
            TheGameSettings.avMainVolume = settingsMain.MainVolume;
            TheGameSettings.avActiveInstrumentVolume = settingsMain.PlayerVolume;
            TheGameSettings.avInactiveInstrumentVolume = settingsMain.BandVolume;
            TheGameSettings.avSoundEffectVolume = settingsMain.SFXVolume;
            TheGameSettings.avMuteVolume = settingsMain.MissVolume;
            TheGameSettings.avMenuMusicVolume = settingsMain.MenuVolume;
            TheGameSettings.Fullscreen = settingsMain.fullscreen;
            TheGameSettings.AudioOffset = settingsMain.avOffsetMS;
            TheGameSettings.InputOffset = settingsMain.inputOffsetMS;
            TheGameSettings.MirrorMode = settingsMain.mirrorMode;
            TheGameSettings.TrackSpeed = settingsMain.trackSpeed;
            TheGameSettings.HighwayLengthMult = settingsMain.highwayLengthMult;
            TheGameSettings.MissHighwayColor = settingsMain.missHighwayColor;
            TheGameSettings.ControllerType = settingsMain.controllerType;
            TheGameSettings.Keybinds4K = settingsMain.keybinds4K;
            TheGameSettings.Keybinds5K = settingsMain.keybinds5K;
            TheGameSettings.Keybinds4KAlt = settingsMain.keybinds4KAlt;
            TheGameSettings.Keybinds5KAlt = settingsMain.keybinds5KAlt;
            TheGameSettings.KeybindStrumUp = settingsMain.keybindStrumUp;
            TheGameSettings.KeybindStrumDown = settingsMain.keybindStrumDown;
            TheGameSettings.KeybindOverdrive = settingsMain.keybindOverdrive;
            TheGameSettings.KeybindOverdriveAlt = settingsMain.keybindOverdriveAlt;
            TheGameSettings.KeybindPause = settingsMain.keybindPause;
            
            TheGameSettings.Controller4K = settingsMain.controller4K;
            TheGameSettings.Controller5K = settingsMain.controller5K;
            TheGameSettings.Controller4KAxisDirection = settingsMain.controller4KAxisDirection;
            TheGameSettings.Controller5KAxisDirection = settingsMain.controller5KAxisDirection;
            TheGameSettings.ControllerOverdrive = settingsMain.controllerOverdrive;
            TheGameSettings.ControllerOverdriveAxisDirection = settingsMain.controllerOverdriveAxisDirection;
            TheGameSettings.ControllerPause = settingsMain.controllerPause;
            TheGameSettings.ControllerPauseAxisDirection = settingsMain.controllerPauseAxisDirection;
            
            TheGameSettings.TrackSpeedOptions = settingsMain.trackSpeedOptions;
            
            this->CreateSettings();
            
            TraceLog(LOG_INFO, "Successfully converted settings-old.json to settings.json");
            TraceLog(LOG_INFO, "The game will now use settings.json for all configuration");
        }
    }


    bool SettingsInit::ConvertOldSettingsToNewFormat(std::filesystem::path directory) {
        if (!exists(directory / "settings-old.json")) {
            TraceLog(LOG_INFO, "No settings-old.json found, nothing to convert");
            return false;
        }

        SettingsInit converter;
        converter.directory = directory;
        
        try {
            converter.ConvertOldSettingsToNew();
            

            std::filesystem::remove(directory / "settings-old.json");
            TraceLog(LOG_INFO, "Settings conversion completed successfully and old file removed");
            return true;
        } catch (const std::exception& e) {
            TraceLog(LOG_ERROR, "Settings conversion failed: %s", e.what());
            return false;
        }
    }
}
