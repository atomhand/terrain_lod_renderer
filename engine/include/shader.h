#pragma once

#include <glad/glad.h>
#include <string>
#include "camera.h"
#include "texture.h"

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
        void setVec4(const std::string &name, glm::vec4 value) const {            
            glUniform4f(glGetUniformLocation(programID, name.c_str()), value.x, value.y, value.z, value.w); 
        };
        void setMat4(const std::string &name, const glm::mat4 &mat) const
        {
            glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }
        void setMat3(const std::string &name, const glm::mat3 &mat) const
        {
            glUniformMatrix3fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }

        void setCamera(const Camera &camera);
    };
}