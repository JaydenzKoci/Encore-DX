#include "file_dialog.h"
#include <raylib.h>
#include <algorithm>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#elif defined(__linux__)
#include <cstdlib>
#include <cstdio>
#include <memory>
#elif defined(__APPLE__)
#include <cstdlib>
#include <cstdio>
#include <memory>
#endif

namespace Encore {
    namespace FileDialog {
        
        std::string OpenVideoFile() {
#ifdef _WIN32
            OPENFILENAMEA ofn;
            char szFile[260] = {0};
            
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = "Video Files\0*.mp4;*.avi;*.mov;*.mkv;*.wmv;*.flv;*.webm\0All Files\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
            
            if (GetOpenFileNameA(&ofn)) {
                return std::string(szFile);
            }
            return "";
            
#elif defined(__linux__)
            // Use zenity on Linux if available
            std::string command = "zenity --file-selection --file-filter='Video files | *.mp4 *.avi *.mov *.mkv *.wmv *.flv *.webm' --title='Select Video File' 2>/dev/null";
            
            std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
            if (!pipe) {
                TraceLog(LOG_WARNING, "Could not open file dialog. Please install zenity for file browser support.");
                return "";
            }
            
            char buffer[1024];
            std::string result;
            while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
                result += buffer;
            }
            
            // Remove trailing newline
            if (!result.empty() && result.back() == '\n') {
                result.pop_back();
            }
            
            return result;
            
#elif defined(__APPLE__)
            // Use osascript on macOS
            std::string command = "osascript -e 'POSIX path of (choose file with prompt \"Select Video File\" of type {\"public.movie\", \"public.video\"})' 2>/dev/null";
            
            std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
            if (!pipe) {
                TraceLog(LOG_WARNING, "Could not open file dialog.");
                return "";
            }
            
            char buffer[1024];
            std::string result;
            while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
                result += buffer;
            }
            
            // Remove trailing newline
            if (!result.empty() && result.back() == '\n') {
                result.pop_back();
            }
            
            return result;
#else
            TraceLog(LOG_WARNING, "File dialog not supported on this platform.");
            return "";
#endif
        }
        
        bool IsValidVideoFile(const std::filesystem::path& path) {
            if (!std::filesystem::exists(path)) {
                return false;
            }
            
            std::string extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            
            std::vector<std::string> validExtensions = {
                ".mp4", ".avi", ".mov", ".mkv", ".wmv", ".flv", ".webm", ".m4v", ".3gp", ".ogv"
            };
            
            return std::find(validExtensions.begin(), validExtensions.end(), extension) != validExtensions.end();
        }
    }
}