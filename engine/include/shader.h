#pragma once

#include <glad/glad.h>
#include <string>

namespace Engine
{
    class Shader
    {
    public:
        GLuint programID;

        Shader(const char* vertexPath, const char* fragmentPath);

        // use/active the shader
        void use();

        // set uniforms
        void setBool(const std::string &name, bool value) const;
        void setInt(const std::string &name, int value) const;
        void setFloat(const std::string &name, float value) const;
    };
}