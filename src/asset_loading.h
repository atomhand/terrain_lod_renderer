#pragma once

#include <string>
#include <glad/glad.h>
using namespace std;

class AssetLoading
{
public:
    static string readFile(const char *filePath);
};
