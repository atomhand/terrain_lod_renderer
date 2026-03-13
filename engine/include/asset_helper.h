// Tom Kellett 2025
#pragma once

#include <string>
#include <filesystem>

namespace Engine {
    class AssetHelper
    {
    public:
        static std::string readFile(const char *filePath);

        static void CreateDirectories() {            
            std::filesystem::path path = std::filesystem::current_path();

            path += "/compressed_assets/";
            std::filesystem::create_directory(path);
            
            path += "/textures/";
            std::filesystem::create_directory(path);

            path = std::filesystem::current_path();
            path += "/screenshots/";
            std::filesystem::create_directory(path);
        }

        static std::filesystem::path screenshotPath(const char* localPath) {
            std::filesystem::path path = std::filesystem::current_path();
            path += "/screenshots/";
            path += localPath;
            return path;
        }

        static std::filesystem::path compressedAssetPath(const char *localPath) {
            std::filesystem::path path = std::filesystem::current_path();
            path += "/compressed_assets/";
            path += localPath;
            return path;
        }

        // Given a path relative to the assets directory, returns an absolute path
        // Return format is const char* because that's used by dependencies like stb
        static std::filesystem::path assetPath(const char *localPath) {
            std::filesystem::path path = std::filesystem::current_path();
            path += "/assets/";
            path += localPath;
            return path;
        }
    };
}
