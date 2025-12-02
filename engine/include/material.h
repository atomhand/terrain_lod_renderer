#pragma once
#include "shader.h"
#include "light.h"

namespace Engine {
    struct Material {
    protected:
        glm::mat4 view;
    public:
        Shader shader;
        std::vector<Engine::Texture> textures;

        virtual void use() {
            shader.use();

            for(int i =0; i<textures.size(); i++) {
                glActiveTexture(GL_TEXTURE0 + i);
                textures[i].bind();
            }        
        }

        void setCamera(const Camera &camera) {
            // set camera
            view = camera.view();
            shader.setVec3("viewPos", camera.position());
            shader.setMat4("view", view);
            shader.setMat4("projection", camera.projection());

        }

        void setLight(PointLight& light, glm::mat4 view, int index) {
            if(index >= 4) {
                std::cout << "Trying to set " << index << " lights to material, only 4 are suported";
                return;
            }

            glm::vec4 pos = light.globalTransform * glm::vec4(0.f,0.f,0.f,1.f);
            shader.setVec3("lightPositions[" + std::to_string(index) + "]", glm::vec3(pos)/pos.w);
            shader.setVec3("lightColors[" + std::to_string(index) + "]", light.color);
        }

        void setLight(DirectionalLight& light, glm::mat4 view, int index) {
            if(index > 4) {
                std::cout << "Trying to set " << index << " lights to material, only 4 are suported";
                return;
            }

            shader.setVec3("lightDirections[" + std::to_string(index) + "]", light.direction);
            shader.setVec3("directionalLightColors[" + std::to_string(index) + "]", light.color);

            if(index == 0) {
                glActiveTexture(GL_TEXTURE0 + 5);
                light.depthMap().bind();
                shader.setMat4("directionLightMatrix", light.lightSpaceMatrix);
            }
        }

        // setCamera must be called first
        void setModel(const glm::mat4 &model) {
            // set model          
            shader.setMat4("model", model);
            // normal matrix
            // configured for world space lighting
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));                
            shader.setMat3("normalMatrix", normalMatrix);
        }

        void unbind() {
            for(int i =0; i<textures.size(); i++) {
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D,0);
            }            
            glUseProgram(0);
        }

        Material(Shader shader) : shader(shader) {};
    };

    struct PbrMaterial : public Material {
    public:
        glm::vec3 albedo = glm::vec3(1.0,1.0,1.0);
        float metallic = 0.f;
        float roughness = 0.5f;
        float ao = 1.f;

        void use() override {
            Material::use();
            shader.setVec3("mAlbedo",albedo);
            shader.setFloat("mMetallic",metallic);
            shader.setFloat("mRoughness",roughness);
            shader.setFloat("mAo",ao);
        }

        // inherit constructor
        using Material::Material;
    };
}