//
// Created by marie on 20/10/2024.
//

#include "playerManager.h"
#include "settings.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

PlayerManager::PlayerManager() = default;
PlayerManager::~PlayerManager() = default;

void PlayerManager::LoadPlayerList() {
    try {
        std::ifstream f(PlayerListSaveFile);
        if (!f.good()) {
            TraceLog(LOG_WARNING, "players.json not found or could not be opened. A new one will be created when a player is made.");
            return;
        }
        json PlayerListJson = json::parse(f);
        TraceLog(LOG_INFO, "Loading player list");
        for (auto &jsonObject : PlayerListJson.items()) {
            Player newPlayer;
            newPlayer.playerJsonObjectName = jsonObject.key();
            newPlayer.Name = jsonObject.value().at("name").get<std::string>();
            newPlayer.PlayerID = jsonObject.value().at("UUID").get<std::string>();

#define SETTING_ACTION(type, name, key)                                                  \
newPlayer.name = jsonObject.value().at(key).get<type>();
            PLAYER_JSON_SETTINGS;
#undef SETTING_ACTION

            if (!jsonObject.value()["accentColor"].is_null()) {
                int r, g, b;
                r = jsonObject.value()["accentColor"].at("r").get<int>();
                g = jsonObject.value()["accentColor"].at("g").get<int>();
                b = jsonObject.value()["accentColor"].at("b").get<int>();
                newPlayer.AccentColor = Color { static_cast<unsigned char>(r),
                                                static_cast<unsigned char>(g),
                                                static_cast<unsigned char>(b),
                                                255 };
            } else
                newPlayer.AccentColor = { 255, 0, 255, 255 };

            TraceLog(LOG_INFO, ("Successfully loaded player " + newPlayer.Name).c_str());
            if (newPlayer.PlayerID == "3" || newPlayer.PlayerID == "6"
                || newPlayer.PlayerID == "1") {
                // FOR GOOD MEASURE SO PEOPLE DONT HAVE TO ASK
                Encore::EncoreLog(
                    LOG_ERROR, "WE'RE DELETING YOUR PLAYER FILE. MAKE YOUR OWN PLAYERS."
                );
                Encore::EncoreLog(
                    LOG_ERROR, "WE'RE DELETING YOUR PLAYER FILE. MAKE YOUR OWN PLAYERS."
                );
                Encore::EncoreLog(
                    LOG_ERROR, "WE'RE DELETING YOUR PLAYER FILE. MAKE YOUR OWN PLAYERS."
                );
                Encore::EncoreLog(
                    LOG_ERROR, "WE'RE DELETING YOUR PLAYER FILE. MAKE YOUR OWN PLAYERS."
                );
                Encore::EncoreLog(
                    LOG_ERROR, "WE'RE DELETING YOUR PLAYER FILE. MAKE YOUR OWN PLAYERS."
                );

                remove(PlayerListSaveFile);
            } else {
                PlayerList.push_back(std::move(newPlayer));
            }
        };
    } catch (const std::exception &e) {
        Encore::EncoreLog(
            LOG_ERROR, TextFormat("Failed to load players. Reason: %s", e.what())
        );
    }
}; // make player, load player stuff to PlayerList

void PlayerManager::SavePlayerList() {
    for (int i = 0; i < PlayerList.size(); i++) {
        SaveSpecificPlayer(i);
    }
}; // ough this is gonna be complicated

void PlayerManager::SaveSpecificPlayer(const int slot) {
    json PlayerListJson;
    if (exists(PlayerListSaveFile)) {
        std::ifstream f(PlayerListSaveFile);
        PlayerListJson = json::parse(f);
        f.close();
    }
    Player &player = GetActivePlayer(slot);
    if (!PlayerListJson.contains(player.playerJsonObjectName)) {
        PlayerListJson[player.playerJsonObjectName] = {
            { "name", player.Name },
            { "UUID", player.PlayerID },
#define SETTING_ACTION(type, name, key) { key, player.name },
            PLAYER_JSON_SETTINGS
#undef SETTING_ACTION
            { "accentColor",
              { { "r", player.AccentColor.r },
                { "g", player.AccentColor.g },
                { "b", player.AccentColor.b } } }
        };
    } else {
        PlayerListJson.at(player.playerJsonObjectName)["name"] = player.Name;
        PlayerListJson.at(player.playerJsonObjectName)["UUID"] = player.PlayerID;

#define SETTING_ACTION(type, name, key)                                                  \
    PlayerListJson.at(player.playerJsonObjectName)[key] = player.name;
        PLAYER_JSON_SETTINGS;
#undef SETTING_ACTION

        PlayerListJson.at(player.playerJsonObjectName)["accentColor"]["r"] =
            player.AccentColor.r;
        PlayerListJson.at(player.playerJsonObjectName)["accentColor"]["g"] =
            player.AccentColor.g;
        PlayerListJson.at(player.playerJsonObjectName)["accentColor"]["b"] =
            player.AccentColor.b;
    }

    Encore::WriteJsonFile(PlayerListSaveFile, PlayerListJson);
}

void PlayerManager::CreatePlayer(const std::string &name) {
    // Create the new player object
    Player newPlayer;
    newPlayer.Name = name;
    newPlayer.playerJsonObjectName = name; // Use the name as the JSON key for simplicity

    // Add the new player to the in-memory list
    PlayerList.push_back(std::move(newPlayer));

    // Get a reference to the player we just added to the vector
    Player& playerToSave = PlayerList.back();

    // Load existing players.json or create a new json object
    json PlayerListJson = json::object();
    if (exists(PlayerListSaveFile)) {
        std::ifstream f(PlayerListSaveFile);
        // Check if the file is not empty before trying to parse
        if (f.peek() != std::ifstream::traits_type::eof()) {
            try {
                PlayerListJson = json::parse(f);
            } catch (const json::parse_error& e) {
                TraceLog(LOG_ERROR, "Failed to parse players.json: %s. A new file will be created.", e.what());
                PlayerListJson = json::object(); // Reset to empty object on parse failure
            }
        }
        f.close();
    }

    // Add or update the player in the json object
    PlayerListJson[playerToSave.playerJsonObjectName] = {
        { "name", playerToSave.Name },
        { "UUID", playerToSave.PlayerID },
#define SETTING_ACTION(type, name, key) { key, playerToSave.name },
        PLAYER_JSON_SETTINGS
#undef SETTING_ACTION
        { "accentColor",
          { { "r", playerToSave.AccentColor.r },
            { "g", playerToSave.AccentColor.g },
            { "b", playerToSave.AccentColor.b } } }
    };

    // Save the updated json object back to the file
    Encore::WriteJsonFile(PlayerListSaveFile, PlayerListJson);
    TraceLog(LOG_INFO, "Created and saved new player: %s", name.c_str());
}

void PlayerManager::DeletePlayer(const Player &PlayerToDelete) {

}; // remove player, reload playerlist
void PlayerManager::RenamePlayer(const Player &PlayerToRename) {

}; // rename player