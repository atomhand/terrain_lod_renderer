#pragma once

#include <string>
using namespace std;

namespace Engine {
    class AssetHelper
    {
    public:
        static string readFile(const char *filePath);
    };
}
