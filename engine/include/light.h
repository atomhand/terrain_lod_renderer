#pragma once
#include <memory>
#include <limits>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <texture.h>
#include "scenegraph.h"
#include "camera.h"
#include "world.h"
#include "culling.h"

// Shadow map code partially adapted from https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
// Modified to use
// -- Opengl shadow sampler instead of standard texture sampler
// -- Adaptive frustum calculation

namespace Engine {
    class RenderItem;

    class PointLight : public SceneNode {
    private:
        struct LightData {

        };

        GLuint depthMapFBO;
        std::shared_ptr<LightData> data;
    public:
        glm::vec3 color = glm::vec3(1.0,1.0,1.0);
    };

    class DirectionalLight : public SceneNode {
    private:
        static const unsigned int SHADOWMAP_WIDTH = 2048, SHADOWMAP_HEIGHT = 2048;
        struct Data {
            GLuint depthMapFBO;
            Texture depthMap;

            Data() {
                glGenFramebuffers(1,&depthMapFBO);

                auto depthMapObject = depthMap.textureObject();

                // Set up depth map texture
                glBindTexture(GL_TEXTURE_2D, depthMapObject);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_HALF_FLOAT, NULL);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

                // fragments outside the shadow map are assumed not to be occluded
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                float white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
                glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, white); 

                // Set up framebuffer
                glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapObject, 0);
                glDrawBuffer(GL_NONE);
                glReadBuffer(GL_NONE);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
        };

        std::shared_ptr<Data> data;

    public:
        glm::vec3 color = glm::vec3(1.0,1.0,1.0);
        glm::vec3 direction = glm::normalize(glm::vec3(4.0,-2.0,4.0));

        Texture depthMap() { return data-> depthMap; }

        glm::mat4 lightView;
        glm::mat4 lightProjection;

        // Build the light space matrix
        // This should be called once per frame, 
        std::vector<size_t> MakeLightSpaceMatrix(Camera& camera, std::vector<RenderItem*> &shadowReceivers, std::vector<RenderItem*> &shadowCasters);

        void PrepareRenderShadowmap() {
            glViewport(0, 0, SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, data->depthMapFBO);
            glClear(GL_DEPTH_BUFFER_BIT);
        }

        glm::mat4 lightSpaceMatrix;

        DirectionalLight() {
            data = std::make_shared<Data>();
        }
    };
}