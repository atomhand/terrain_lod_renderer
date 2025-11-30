#pragma once

#include <string>
#include <filesystem>

namespace Engine {
    class AssetHelper
    {
    public:
        static std::string readFile(const char *filePath);

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
