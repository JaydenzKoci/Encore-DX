//
// Created by Jaydenz on 10/2/2025.
//

#include "trackDownloader.h"

#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <thread>
#include <chrono>

#include "song/songlist.h"
#include "settings.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define CloseWindow CloseWindow_Win32
#define ShowCursor ShowCursor_Win32
#define LoadImage LoadImage_Win32
#define DrawText DrawText_Win32
#define DrawTextEx DrawTextEx_Win32
#include <windows.h>
#include <wininet.h>
#include <urlmon.h>
#undef CloseWindow
#undef ShowCursor
#undef LoadImage
#undef DrawText
#undef DrawTextEx
#undef GetObject
#undef NOGDI
#else
#include <cstdlib>
#endif

namespace fs = std::filesystem;

Encore::TrackDownloader TheTrackDownloader;

Encore::TrackDownloader::TrackDownloader() : tracksLoaded(false), isDownloading(false), 
    downloadProgress(0.0f), downloadStatusText("Ready"), completedDownloads(0), totalDownloads(0) {
}

Encore::TrackDownloader::~TrackDownloader() {
}

bool downloadFileContent(const std::string& url, std::string& content) {
#ifdef _WIN32
    HINTERNET hInternet = InternetOpenA("Encore", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return false;
    
    HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return false;
    }
    
    char buffer[4096];
    DWORD bytesRead;
    content.clear();
    
    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        content.append(buffer, bytesRead);
    }
    
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
    return true;
#else
    std::string command = "curl -s \"" + url + "\"";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return false;
    
    char buffer[4096];
    content.clear();
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        content += buffer;
    }
    
    pclose(pipe);
    return !content.empty();
#endif
}

bool Encore::TrackDownloader::LoadTrackList() {
    if (tracksLoaded) return true;
    
    std::string jsonContent;
    const std::string url = "https://raw.githubusercontent.com/Encore-Developers/EncoreCustoms/refs/heads/main/data/tracks.json";
    
    if (!downloadFileContent(url, jsonContent)) {
        std::cerr << "Failed to download tracks.json" << std::endl;
        return false;
    }
    
    try {
        nlohmann::json tracksJson = nlohmann::json::parse(jsonContent);
        tracks.clear();
        
        for (auto it = tracksJson.begin(); it != tracksJson.end(); ++it) {
            const auto& track = it.value();
            if (track.contains("title") && track.contains("artist") && track.contains("download")) {
                TrackInfo info;
                info.key = it.key();
                info.title = track["title"];
                info.artist = track["artist"];
                info.downloadUrl = track["download"];
                info.filesize = track.contains("filesize") ? track["filesize"] : "Unknown";
                info.cover = track.contains("cover") ? track["cover"] : "";
                if (track.contains("releaseYear")) {
                    if (track["releaseYear"].is_string()) {
                        info.releaseYear = track["releaseYear"];
                    } else if (track["releaseYear"].is_number()) {
                        info.releaseYear = std::to_string(track["releaseYear"].get<int>());
                    } else {
                        info.releaseYear = "Unknown";
                    }
                } else {
                    info.releaseYear = "Unknown";
                }
                
                info.duration = track.contains("duration") ? track["duration"] : "Unknown";
                info.verification = track.contains("verification") ? track["verification"] : "Unknown";
                info.charter = track.contains("charter") ? track["charter"] : "Unknown";
                info.genre = track.contains("genre") ? track["genre"] : "Unknown";
                info.format = track.contains("format") ? track["format"] : "Unknown";

                if (track.contains("bpm")) {
                    if (track["bpm"].is_string()) {
                        info.bpm = track["bpm"];
                    } else if (track["bpm"].is_number()) {
                        info.bpm = std::to_string(track["bpm"].get<int>());
                    } else {
                        info.bpm = "Unknown";
                    }
                } else {
                    info.bpm = "Unknown";
                }
                
                info.trackKey = track.contains("key") ? track["key"] : "Unknown";
                info.loadingPhrase = track.contains("loading_phrase") ? track["loading_phrase"] : "";
                info.isDownloaded = IsTrackDownloaded(info.key);
                
                tracks.push_back(info);
            }
        }
        
        tracksLoaded = true;
        std::cout << "Loaded " << tracks.size() << " tracks from EncoreCustoms" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse tracks.json: " << e.what() << std::endl;
        return false;
    }
}

std::vector<Encore::TrackInfo> Encore::TrackDownloader::GetAllTracks() {
    if (!LoadTrackList()) {
        return {};
    }
    return tracks;
}

std::string Encore::TrackDownloader::getSongsPath() {
    fs::path currentPath = fs::current_path();
    fs::path songsPath = currentPath / "songs";

    if (!fs::exists(songsPath)) {
        fs::create_directories(songsPath);
    }
    
    return songsPath.string();
}

std::string Encore::TrackDownloader::GetSongsFolder() {
    return getSongsPath();
}
bool Encore::TrackDownloader::IsTrackDownloaded(const std::string& trackKey) {
    std::string songsPath = getSongsPath();

    fs::path trackPath = fs::path(songsPath) / trackKey;

    if (fs::exists(trackPath) && fs::is_directory(trackPath)) {
        return true;
    }

    try {
        for (const auto& entry : fs::directory_iterator(songsPath)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().stem().string();
                if (filename.find(trackKey) == 0) {
                    return true;
                }
            }
        }
    } catch (const std::exception& e) {
        return false;
    }
    
    return false;
}

bool Encore::TrackDownloader::downloadFile(const std::string& url, const std::string& outputPath, std::function<void(float)> progressCallback) {
#ifdef _WIN32
    if (progressCallback) {
        HRESULT hr = URLDownloadToFileA(NULL, url.c_str(), outputPath.c_str(), 0, NULL);
        if (progressCallback) progressCallback(1.0f);
        return SUCCEEDED(hr);
    } else {
        HRESULT hr = URLDownloadToFileA(NULL, url.c_str(), outputPath.c_str(), 0, NULL);
        return SUCCEEDED(hr);
    }
#else
    std::string command = "curl -L \"" + url + "\" -o \"" + outputPath + "\"";
    int result = system(command.c_str());
    if (progressCallback) progressCallback(1.0f);
    return result == 0;
#endif
}

bool Encore::TrackDownloader::extractZip(const std::string& zipPath, const std::string& extractPath) {
#ifdef _WIN32
    std::string command = "powershell -Command \"Expand-Archive -Path '" + zipPath + "' -DestinationPath '" + extractPath + "' -Force\"";
    int result = system(command.c_str());
    return result == 0;
#else
    std::string command = "unzip -o \"" + zipPath + "\" -d \"" + extractPath + "\"";
    int result = system(command.c_str());
    return result == 0;
#endif
}

bool Encore::TrackDownloader::DownloadTrack(const std::string& trackKey, std::function<void(float)> progressCallback) {
    if (!LoadTrackList()) {
        return false;
    }

    auto it = std::find_if(tracks.begin(), tracks.end(), 
        [&trackKey](const TrackInfo& track) { return track.key == trackKey; });
    
    if (it == tracks.end()) {
        std::cerr << "Track not found: " << trackKey << std::endl;
        return false;
    }
    
    if (it->isDownloaded) {
        std::cout << "Track already downloaded: " << it->title << std::endl;
        if (progressCallback) progressCallback(1.0f);
        return true;
    }
    
    std::string songsPath = getSongsPath();
    std::string zipPath = (fs::path(songsPath) / (trackKey + ".zip")).string();
    
    std::cout << "Downloading: " << it->title << " by " << it->artist << std::endl;

    if (!downloadFile(it->downloadUrl, zipPath, progressCallback)) {
        std::cerr << "Failed to download: " << it->title << std::endl;
        return false;
    }

    if (!extractZip(zipPath, songsPath)) {
        std::cerr << "Failed to extract: " << it->title << std::endl;
        return false;
    }

    try {
        fs::remove(zipPath);
    } catch (const std::exception& e) {
        std::cerr << "Warning: Could not remove zip file: " << e.what() << std::endl;
    }

    it->isDownloaded = true;
    
    std::cout << "Successfully downloaded: " << it->title << std::endl;
    return true;
}

void Encore::TrackDownloader::DownloadAllTracks(std::function<void(const std::string&, float)> progressCallback) {
    if (!LoadTrackList()) {
        return;
    }
    
    int totalTracks = tracks.size();
    int currentTrack = 0;
    
    for (auto& track : tracks) {
        if (!track.isDownloaded) {
            if (progressCallback) {
                progressCallback(track.title, (float)currentTrack / totalTracks);
            }
            
            DownloadTrack(track.key, nullptr);
        }
        currentTrack++;
    }
    
    if (progressCallback) {
        progressCallback("Complete", 1.0f);
    }
}

void Encore::TrackDownloader::StartBackgroundDownload() {
    if (isDownloading) return;
    
    if (!LoadTrackList()) {
        downloadStatusText = "Failed to load track list";
        return;
    }

    int tracksToDownload = 0;
    for (const auto& track : tracks) {
        if (!track.isDownloaded) {
            tracksToDownload++;
        }
    }
    
    if (tracksToDownload == 0) {
        downloadStatusText = "All tracks already downloaded";
        return;
    }
    
    isDownloading = true;
    downloadProgress = 0.0f;
    completedDownloads = 0;
    totalDownloads = tracksToDownload;
    downloadStatusText = "Starting download...";

    std::thread([this]() {
        int currentTrack = 0;
        
        for (auto& track : tracks) {
            if (!track.isDownloaded) {
                downloadStatusText = "Downloading: " + track.title;

                bool success = DownloadTrack(track.key, [this, currentTrack](float trackProgress) {
                    downloadProgress = (float(completedDownloads) + trackProgress) / float(totalDownloads);
                });
                
                if (success) {
                    completedDownloads++;
                    track.isDownloaded = true;
                }
                
                currentTrack++;
                downloadProgress = float(completedDownloads) / float(totalDownloads);
            }
        }

        downloadStatusText = "Downloading covers...";
        std::string songsPath = getSongsPath();
        for (const auto& track : tracks) {
            if (!track.cover.empty() && track.isDownloaded) {
                fs::path trackPath = fs::path(songsPath) / track.key;
                if (fs::exists(trackPath) && fs::is_directory(trackPath)) {
                    fs::path coverPath = trackPath / "album.jpg";
                    if (!fs::exists(coverPath)) {
                        downloadFile(track.cover, coverPath.string(), nullptr);
                    }
                }
            }
        }

        downloadStatusText = "Scanning songs...";
        auto enabledPaths = TheGameSettings.GetEnabledSongPaths();
        if (enabledPaths.empty()) enabledPaths = TheGameSettings.SongPaths;
        if (!enabledPaths.empty()) {
            TheSongList.ScanSongs(enabledPaths);
        }

        isDownloading = false;
        downloadProgress = 1.0f;
        downloadStatusText = "Download complete!";

        std::this_thread::sleep_for(std::chrono::seconds(3));
        if (!isDownloading) {
            downloadStatusText = "Ready";
            downloadProgress = 0.0f;
            completedDownloads = 0;
            totalDownloads = 0;
        }
    }).detach();
}