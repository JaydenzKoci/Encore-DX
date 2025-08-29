#pragma once

#include <vector>
#include <filesystem>
#include <unordered_map>
#include <string>
namespace Encore {
    class AudioManager {
    public:

        struct AudioStream {
            unsigned int handle = 0;
            int instrument = 0;
        };
        std::vector<AudioStream> loadedStreams; // Loaded audio streams

        // Initialize the audio manager
        static bool Init();

        // Load and manage audio streams
        void loadStreams(std::vector<std::pair<std::string, int> > &paths);
        void unloadStreams();
        void pauseStreams() const;
        void playStreams() const;
        void restartStreams() const;
        void seekStreams(double time) const;
        // Audio stream information
        double GetMusicTimePlayed() const;
        [[nodiscard]] double GetMusicTimeLength() const;

        // --- NEW DIAGNOSTIC FUNCTION ---
        // Get the current playback status of a stream handle as a string.
        std::string GetStreamStatus(unsigned int handle) const;

        // Audio stream control
        static void UpdateMusicStream(unsigned int handle);
        void unpauseStreams() const;
        static void SetAudioStreamVolume(unsigned int handle, float volume);
        static void SetAudioStreamPosition(unsigned int handle, double time);
        static void BeginPlayback(unsigned int handle);
        static void StopPlayback(unsigned int handle);

        // Load and play samples
        void loadSample(const std::string &path, const std::string &name);
        void playSample(const std::string &name, float volume);
        void unloadSample(const std::string &name);

    private:
        std::unordered_map<std::string, unsigned int> samples; // Loaded audio samples
    };
}
extern Encore::AudioManager TheAudioManager;