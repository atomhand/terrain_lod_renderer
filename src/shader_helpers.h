#pragma once

#include <string>
#include <glad/glad.h>
using namespace std;

class ShaderHelpers
{
private:
    static GLuint BuildShader(GLenum eShaderType, const string &shaderText);
    static string readFile(const char *filePath);
    static GLuint BuildShaderProgram(string vertShaderStr, string fragShaderStr);
public:
    static GLuint LoadShader(const char *vertex_path, const char *fragment_path);
};
