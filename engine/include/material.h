// Tom Kellett 2025
#pragma once
#include "shader.h"

namespace Engine {
    struct Material {
    protected:
        int modelLocation = -2;
        int timeLocation = -2;
        int normalMatrixLocation = -2;
        int colorLocation = -2;
    public:
        Shader shader;
        std::vector<Engine::Texture> textures;
        std::vector<Engine::Texture2DArray> texturearrays;

        virtual void use() {
            shader.use();

            int offset = GL_TEXTURE0;

            for(int i =0; i<textures.size(); i++) {
                glActiveTexture(offset++);
                textures[i].bind();
            }
            for(int i=0; i<texturearrays.size(); i++) {
                glActiveTexture(offset++);
                texturearrays[i].bind();
            }
        }        

        // Make sure to bind the shader first
        void SetModel(glm::mat4 &model) {
            if(modelLocation == -2) {
                modelLocation = glGetUniformLocation(shader.programId(), "model");
            }
            glUniformMatrix4fv(modelLocation,1, GL_FALSE, &model[0][0]);
        }

        // Make sure to bind the shader first
        void SetModelAndNormalMatrix(glm::mat4 &model) {
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));  
            if(normalMatrixLocation == -2) {
                normalMatrixLocation = glGetUniformLocation(shader.programId(), "normalMatrix");
            }
            glUniformMatrix3fv(normalMatrixLocation,1, GL_FALSE, &normalMatrix[0][0]);

            SetModel(model);
        }

        // Make sure to bind the shader first
        void SetColor(glm::vec3 color) {
            if(colorLocation == -2) {
                colorLocation = glGetUniformLocation(shader.programId(), "color");
            }
            glUniform3fv(colorLocation, 1, &color[0]);
        }

        // Make sure to bind the shader first
        void SetTime(float time) {
            if(timeLocation == -2) {
                timeLocation = glGetUniformLocation(shader.programId(), "time");
            }
            glUniform1f(timeLocation, time);
        }

        void unbind() {
            int offset = GL_TEXTURE0;
            for(int i =0; i<textures.size(); i++) {
                glActiveTexture(offset++);
                glBindTexture(GL_TEXTURE_2D,0);
            }
            for(int i=0; i<texturearrays.size(); i++) {
                glActiveTexture(offset++);
                glBindTexture(GL_TEXTURE_2D,0);
            }

            glUseProgram(0);
        }

        Material(Shader shader) : shader(shader) {};
    };

    struct PbrMaterial : public Material {
        int albedoLocation;
        int metallicLocation;
        int roughnessLocation;
        int aoLocation;
    public:
        glm::vec3 albedo = glm::vec3(1.0,1.0,1.0);
        float metallic = 0.f;
        float roughness = 0.5f;
        float ao = 1.f;

        void use() override {
            Material::use();
            glUniform3fv(albedoLocation,1,&albedo[0]);
            glUniform1f(metallicLocation,metallic);
            glUniform1f(roughnessLocation,roughness);
            glUniform1f(aoLocation,ao);
        }

        PbrMaterial(Shader shader) : Material{shader} {
            albedoLocation = glGetUniformLocation(shader.programId(), "mAlbedo");
            metallicLocation = glGetUniformLocation(shader.programId(), "mMetallic");
            roughnessLocation = glGetUniformLocation(shader.programId(), "mRoughness");
            aoLocation = glGetUniformLocation(shader.programId(), "mAo");
        };
    };
}