#pragma once
#include <string>
#include <filesystem>

namespace Encore {
    namespace FileDialog {
        // Opens a file dialog to select a video file
        // Returns empty string if cancelled or no file selected
        std::string OpenVideoFile();
        
        // Check if a file exists and is a valid video file
        bool IsValidVideoFile(const std::filesystem::path& path);
    }
}