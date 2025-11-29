#pragma once

#include <glad/glad.h>
#include <string>
#include "camera.h"
#include "texture.h"
#include<memory>

namespace Engine
{
    class Shader
    {
    private:
        struct ShaderId {
            GLuint programId;

            ShaderId() {                
                programId = glCreateProgram();
            }
            ~ShaderId() {
                glDeleteProgram(programId);
            }
        };

        std::shared_ptr<ShaderId> data;

        GLuint programId() const {
            if(!data)
                return 0;
            return data->programId;
        }
    public:
        Shader(const char* vertexPath, const char* fragmentPath);
        Shader() {};

        // use/active the shader
        void use();

        // set uniforms
        void setBool(const std::string &name, bool value) const;
        void setInt(const std::string &name, int value) const;
        void setFloat(const std::string &name, float value) const;
        void setVec3(const std::string &name, glm::vec3 value) const {            
            glUniform3fv(glGetUniformLocation(programId(), name.c_str()), 1, &value[0]); 
        };
        void setVec4(const std::string &name, glm::vec4 value) const {            
            glUniform4fv(glGetUniformLocation(programId(), name.c_str()), 1, &value[0]); 
        };
        void setMat4(const std::string &name, const glm::mat4 &mat) const
        {
            glUniformMatrix4fv(glGetUniformLocation(programId(), name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }
        void setMat3(const std::string &name, const glm::mat3 &mat) const
        {
            glUniformMatrix3fv(glGetUniformLocation(programId(), name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }

        void setCamera(const Camera &camera);
    };
}