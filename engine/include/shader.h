// Tom Kellett 2025
// Partially adapted https://learnopengl.com/Getting-started/Shaders
#pragma once
#include <string>
#include<memory>
#include <glad/gl.h>
#include <glm/ext/matrix_transform.hpp> 
#include <glm/ext/matrix_clip_space.hpp>
#include "texture.h"
#include "shader_cache.h"

namespace Engine
{
    class Shader
    {
    private:
        struct ShaderId {
            GLuint programId;

            void Retrieve(std::span<const char*> pushDefines, const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr) {                           
                programId = ShaderCache::GetProgram(vertexPath, fragmentPath, geometryPath, pushDefines);
            }

            ~ShaderId() {
                //glDeleteProgram(programId);
            }
        };

        std::shared_ptr<ShaderId> data;
    public:
        GLuint programId() const {
            assert(data);
            return data->programId;
        }

        Shader() {};

        // Load a shader program with the given shader paths (specified relative to the Assets folder)
        // optional paths are left null to indicate that shader type is not part of the program
        // Shader loading code adapted from https://learnopengl.com/Getting-started/Shaders (but somewhat rewritten)
        Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr, std::span<const char*> pushDefines = std::span<const char*>()) {            
            data = std::make_shared<ShaderId>();
            data->Retrieve(pushDefines, vertexPath, fragmentPath, geometryPath);
        };

        Shader(const char* vertexPath, const char* fragmentPath, std::span<const char*> pushDefines) {            
            data = std::make_shared<ShaderId>();
            data->Retrieve(pushDefines, vertexPath, fragmentPath, nullptr);
        };

        // use/bind the shader pprogram
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
    };
}