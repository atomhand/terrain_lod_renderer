#pragma once

#include <string>
#include <glad/glad.h>
using namespace std;

namespace Engine {
    class AssetLoading
    {
    public:
        static string readFile(const char *filePath);
    };
}
