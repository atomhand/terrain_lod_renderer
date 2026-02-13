// Tom Kellett 2025
#pragma once
#include "shader.h"

namespace Engine {
    struct Material {
    protected:
        int modelLocation = -2;
        int normalMatrixLocation = -2;
        int colorLocation = -2;
    public:
        Shader shader;

        virtual void use() {
            shader.use();
        }        

        // Make sure to bind the shader first
        void SetModel(glm::mat4 &model) {
            if(modelLocation == -2) {
                modelLocation = glGetUniformLocation(shader.programId(), "model");
            }
            glUniformMatrix4fv(modelLocation,1, GL_FALSE, &model[0][0]);
        }

        // Make sure to bind the shader first
        void SetColor(glm::vec3 color) {
            if(colorLocation == -2) {
                colorLocation = glGetUniformLocation(shader.programId(), "color");
            }
            glUniform3fv(colorLocation, 1, &color[0]);
        }

        void unbind() {
            glUseProgram(0);
        }

        Material(Shader shader) : shader(shader) {};
    };
}