#include "leaderboard.h"
#include <fstream>
#include <algorithm>
#include <cctype>
#include "raylib.h"

std::string LeaderboardManager::SanitizeForID(const std::string& input) {
    std::string result;
    for (char c : input) {
        if (std::isalnum(c)) {
            result += std::tolower(c);
        }
    }
    return result;
}

std::string LeaderboardManager::GenerateSongID(const std::string& title, const std::string& artist) {
    std::string sanitizedTitle = SanitizeForID(title);
    std::string sanitizedArtist = SanitizeForID(artist);
    return sanitizedTitle + sanitizedArtist;
}

std::filesystem::path LeaderboardManager::GetLeaderboardPath() {
    std::filesystem::path appDir = GetApplicationDirectory();
    std::filesystem::path leaderboardPath = appDir / "leaderboard.json";
    return leaderboardPath;
}

void LeaderboardManager::SaveScore(const std::string& playerUUID, const std::string& songID,
                                   int score, int stars, int difficulty, int instrument,
                                   int perfectHits, int goodHits, int misses, bool goldStars) {
    std::filesystem::path leaderboardPath = GetLeaderboardPath();
    
    rapidjson::Document document;
    
    if (std::filesystem::exists(leaderboardPath)) {
        std::ifstream ifs(leaderboardPath);
        if (ifs.is_open()) {
            std::string jsonString((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            ifs.close();
            
            document.Parse(jsonString.c_str());
            if (document.HasParseError()) {
                TraceLog(LOG_WARNING, "Failed to parse leaderboard.json, creating new one");
                document.SetObject();
            }
        } else {
            document.SetObject();
        }
    } else {
        document.SetObject();
    }
    
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
    
    if (!document.HasMember(playerUUID.c_str())) {
        rapidjson::Value playerObj(rapidjson::kObjectType);
        document.AddMember(rapidjson::Value(playerUUID.c_str(), allocator).Move(), playerObj, allocator);
    }
    
    rapidjson::Value& playerObj = document[playerUUID.c_str()];
    
    if (!playerObj.HasMember(songID.c_str())) {
        rapidjson::Value songObj(rapidjson::kObjectType);
        playerObj.AddMember(rapidjson::Value(songID.c_str(), allocator).Move(), songObj, allocator);
    }
    
    rapidjson::Value& songObj = playerObj[songID.c_str()];
    
    rapidjson::Value scoreEntry(rapidjson::kObjectType);
    scoreEntry.AddMember("score", score, allocator);
    scoreEntry.AddMember("stars", stars, allocator);
    scoreEntry.AddMember("difficulty", difficulty, allocator);
    scoreEntry.AddMember("instrument", instrument, allocator);
    scoreEntry.AddMember("perfectHits", perfectHits, allocator);
    scoreEntry.AddMember("goodHits", goodHits, allocator);
    scoreEntry.AddMember("misses", misses, allocator);
    scoreEntry.AddMember("goldStars", goldStars, allocator);
    
    int totalNotes = perfectHits + goodHits + misses;
    float hitPercentage = totalNotes > 0 ? ((float)(perfectHits + goodHits) / (float)totalNotes) * 100.0f : 0.0f;
    scoreEntry.AddMember("hitPercentage", hitPercentage, allocator);
    
    std::string entryKey = std::to_string(difficulty) + "_" + std::to_string(instrument);
    
    bool shouldUpdate = true;
    if (songObj.HasMember(entryKey.c_str())) {
        int existingScore = songObj[entryKey.c_str()]["score"].GetInt();
        if (existingScore >= score) {
            shouldUpdate = false;
        }
    }
    
    if (shouldUpdate) {
        if (songObj.HasMember(entryKey.c_str())) {
            songObj.RemoveMember(entryKey.c_str());
        }
        songObj.AddMember(rapidjson::Value(entryKey.c_str(), allocator).Move(), scoreEntry, allocator);
        
        TraceLog(LOG_INFO, "Saved score to leaderboard: Player=%s, Song=%s, Score=%d, Stars=%d", 
                 playerUUID.c_str(), songID.c_str(), score, stars);
    }
    
    // Write back to file
    std::ofstream ofs(leaderboardPath);
    if (ofs.is_open()) {
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        document.Accept(writer);
        ofs << buffer.GetString();
        ofs.close();
    } else {
        TraceLog(LOG_ERROR, "Failed to write leaderboard.json");
    }
}

ScoreData LeaderboardManager::GetHighestScore(const std::string& playerUUID, const std::string& songID) {
    ScoreData result;
    result.hasScore = false;
    
    std::filesystem::path leaderboardPath = GetLeaderboardPath();
    
    if (!std::filesystem::exists(leaderboardPath)) {
        return result;
    }
    
    std::ifstream ifs(leaderboardPath);
    if (!ifs.is_open()) {
        return result;
    }
    
    std::string jsonString((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();
    
    rapidjson::Document document;
    document.Parse(jsonString.c_str());
    
    if (document.HasParseError() || !document.IsObject()) {
        return result;
    }
    
    if (!document.HasMember(playerUUID.c_str())) {
        return result;
    }
    
    const rapidjson::Value& playerObj = document[playerUUID.c_str()];
    
    if (!playerObj.HasMember(songID.c_str())) {
        return result;
    }
    
    const rapidjson::Value& songObj = playerObj[songID.c_str()];
    
    int highestScore = 0;
    for (auto& entry : songObj.GetObject()) {
        if (entry.value.IsObject() && entry.value.HasMember("score")) {
            int entryScore = entry.value["score"].GetInt();
            if (entryScore > highestScore) {
                highestScore = entryScore;
                result.score = entryScore;
                result.stars = entry.value.HasMember("stars") ? entry.value["stars"].GetInt() : 0;
                result.difficulty = entry.value.HasMember("difficulty") ? entry.value["difficulty"].GetInt() : 0;
                result.instrument = entry.value.HasMember("instrument") ? entry.value["instrument"].GetInt() : 0;
                result.perfectHits = entry.value.HasMember("perfectHits") ? entry.value["perfectHits"].GetInt() : 0;
                result.goodHits = entry.value.HasMember("goodHits") ? entry.value["goodHits"].GetInt() : 0;
                result.misses = entry.value.HasMember("misses") ? entry.value["misses"].GetInt() : 0;
                result.goldStars = entry.value.HasMember("goldStars") ? entry.value["goldStars"].GetBool() : false;
                result.hitPercentage = entry.value.HasMember("hitPercentage") ? entry.value["hitPercentage"].GetFloat() : 0.0f;
                
                if (result.hitPercentage == 0.0f && (result.perfectHits > 0 || result.goodHits > 0 || result.misses > 0)) {
                    int totalNotes = result.perfectHits + result.goodHits + result.misses;
                    result.hitPercentage = totalNotes > 0 ? ((float)(result.perfectHits + result.goodHits) / (float)totalNotes) * 100.0f : 0.0f;
                }
                
                result.hasScore = true;
            }
        }
    }
    
    return result;
}

ScoreData LeaderboardManager::GetHighestScoreForInstrument(const std::string& playerUUID, const std::string& songID, int instrument) {
    ScoreData result;
    result.hasScore = false;
    
    std::filesystem::path leaderboardPath = GetLeaderboardPath();
    
    if (!std::filesystem::exists(leaderboardPath)) {
        return result;
    }
    
    std::ifstream ifs(leaderboardPath);
    if (!ifs.is_open()) {
        return result;
    }
    
    std::string jsonString((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();
    
    rapidjson::Document document;
    document.Parse(jsonString.c_str());
    
    if (document.HasParseError() || !document.IsObject()) {
        return result;
    }
    
    if (!document.HasMember(playerUUID.c_str())) {
        return result;
    }
    
    const rapidjson::Value& playerObj = document[playerUUID.c_str()];
    
    if (!playerObj.HasMember(songID.c_str())) {
        return result;
    }
    
    const rapidjson::Value& songObj = playerObj[songID.c_str()];
    
    int highestScore = 0;
    for (auto& entry : songObj.GetObject()) {
        if (entry.value.IsObject() && entry.value.HasMember("score") && entry.value.HasMember("instrument")) {
            int entryInstrument = entry.value["instrument"].GetInt();
            if (entryInstrument != instrument) continue;
            
            int entryScore = entry.value["score"].GetInt();
            if (entryScore > highestScore) {
                highestScore = entryScore;
                result.score = entryScore;
                result.stars = entry.value.HasMember("stars") ? entry.value["stars"].GetInt() : 0;
                result.difficulty = entry.value.HasMember("difficulty") ? entry.value["difficulty"].GetInt() : 0;
                result.instrument = entryInstrument;
                result.perfectHits = entry.value.HasMember("perfectHits") ? entry.value["perfectHits"].GetInt() : 0;
                result.goodHits = entry.value.HasMember("goodHits") ? entry.value["goodHits"].GetInt() : 0;
                result.misses = entry.value.HasMember("misses") ? entry.value["misses"].GetInt() : 0;
                result.goldStars = entry.value.HasMember("goldStars") ? entry.value["goldStars"].GetBool() : false;
                result.hitPercentage = entry.value.HasMember("hitPercentage") ? entry.value["hitPercentage"].GetFloat() : 0.0f;
                
                if (result.hitPercentage == 0.0f && (result.perfectHits > 0 || result.goodHits > 0 || result.misses > 0)) {
                    int totalNotes = result.perfectHits + result.goodHits + result.misses;
                    result.hitPercentage = totalNotes > 0 ? ((float)(result.perfectHits + result.goodHits) / (float)totalNotes) * 100.0f : 0.0f;
                }
                
                result.hasScore = true;
            }
        }
    }
    
    return result;
}
