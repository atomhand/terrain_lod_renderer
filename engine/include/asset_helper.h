#pragma once

#include <string>

namespace Engine {
    class AssetHelper
    {
    public:
        static std::string readFile(const char *filePath);
    };
}
