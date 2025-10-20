#pragma once

#include <memory>
#include "shader_cache.h"
#include "glad/gl.h"

namespace Engine {
    class ComputeShader {
    private:
        struct ShaderId {
            GLuint programId;

            void Retrieve(std::span<const char*> pushDefines, const char* path) {                           
                programId = ShaderCache::GetComputeProgram(path, pushDefines);
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

        ComputeShader() {};

        // Load a shader program with the given shader paths (specified relative to the Assets folder)
        // optional paths are left null to indicate that shader type is not part of the program
        // Shader loading code adapted from https://learnopengl.com/Getting-started/Shaders (but somewhat rewritten)
        ComputeShader(const char* path, std::span<const char*> pushDefines = std::span<const char*>()) {            
            data = std::make_shared<ShaderId>();
            data->Retrieve(pushDefines, path);
        };
        
        void use() {
            glUseProgram(programId());
        }

        void Dispatch(GLuint numGroupsX, GLuint numGroupsY, GLuint numGroupsZ) {
            use();
            glDispatchCompute(numGroupsX,numGroupsY,numGroupsZ);
            glUseProgram(0);
        }
        
        void setBool(const std::string &name, bool value) const {
            glUniform1i(glGetUniformLocation(programId(), name.c_str()), (int)value); 
        }

        void setInt(const std::string &name, int value) const {
            glUniform1i(glGetUniformLocation(programId(), name.c_str()), value); 
        }

        void setFloat(const std::string &name, float value) const {    
            glUniform1f(glGetUniformLocation(programId(), name.c_str()), value); 
        }

        void setVec2(const std::string &name, glm::vec2 value) const { 
            glUniform2fv(glGetUniformLocation(programId(), name.c_str()), 1, &value[0]); 
        }

        void setVec3(const std::string &name, glm::vec3 value) const { 
            glUniform3fv(glGetUniformLocation(programId(), name.c_str()), 1, &value[0]); 
        }

        void setVec4(const std::string &name, glm::vec4 value) const {            
            glUniform4fv(glGetUniformLocation(programId(), name.c_str()), 1, &value[0]); 
        }

        void setMat4(const std::string &name, const glm::mat4 &mat) const {
            glUniformMatrix4fv(glGetUniformLocation(programId(), name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }

        void setMat3(const std::string &name, const glm::mat3 &mat) const {
            glUniformMatrix3fv(glGetUniformLocation(programId(), name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }
    };
}