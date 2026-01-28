//
// Created by Jaydenz on 10/2/2025.
//

#ifndef TRACKDOWNLOADER_H
#define TRACKDOWNLOADER_H

#include <string>
#include <vector>
#include <functional>

namespace Encore {
    struct TrackInfo {
        std::string key;
        std::string title;
        std::string artist;
        std::string downloadUrl;
        std::string filesize;
        std::string cover;
        std::string releaseYear;
        std::string duration;
        std::string verification;
        std::string charter;
        std::string genre;
        std::string format;
        std::string bpm;
        std::string trackKey;
        std::string loadingPhrase;
        bool isDownloaded;
    };

    class TrackDownloader {
    public:
        TrackDownloader();
        ~TrackDownloader();

        bool LoadTrackList();

        std::vector<TrackInfo> GetAllTracks();

        bool DownloadTrack(const std::string& trackKey, std::function<void(float)> progressCallback = nullptr);

        void DownloadAllTracks(std::function<void(const std::string&, float)> progressCallback = nullptr);

        void StartBackgroundDownload();

        bool IsDownloading() const { return isDownloading; }

        float GetDownloadProgress() const { return downloadProgress; }

        std::string GetDownloadStatus() const { return downloadStatusText; }

        int GetCompletedCount() const { return completedDownloads; }

        int GetTotalCount() const { return totalDownloads; }

        bool IsTrackDownloaded(const std::string& trackKey);

        std::string GetSongsFolder();
        
    private:
        std::vector<TrackInfo> tracks;
        bool tracksLoaded;

        bool isDownloading;
        float downloadProgress;
        std::string downloadStatusText;
        int completedDownloads;
        int totalDownloads;

        bool downloadFile(const std::string& url, const std::string& outputPath, std::function<void(float)> progressCallback = nullptr);
        bool extractZip(const std::string& zipPath, const std::string& extractPath);
        std::string getSongsPath();
    };
}

extern Encore::TrackDownloader TheTrackDownloader;

#endif //TRACKDOWNLOADER_H