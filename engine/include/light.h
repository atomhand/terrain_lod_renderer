#pragma once
#include <memory>
#include <limits>
#include <glm/glm.hpp>
#include "scenegraph.h"
#include "camera.h"
#include "world.h"

// Shadow map code partially adapted from https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
// Modified to use
// -- Opengl shadow sampler instead of standard texture sampler
// -- Adaptive frustum calculation

namespace Engine {
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
        static const unsigned int SHADOWMAP_WIDTH = 1024, SHADOWMAP_HEIGHT = 1024;
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
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);

                // fragments outside the shadow map are assumed not to be occluded
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

        void MakeLightSpaceMatrix(Camera& camera) {
            glm::vec3 frustumCorners[8];
            glm::vec3 frustumCenter = camera.FrustumCorners(frustumCorners);

            // Light is looking towards the center of the frustum
            glm::mat4 lightView = glm::lookAt(  frustumCenter-direction,
                                                frustumCenter,
                                                glm::vec3(0.f,1.f,0.f));

            // Light projection is chosen to tightly fit around the corners
            // of the view frustum

            // Transform view frustum corners into the light's coordinate system
            for(int i=0; i<8; i++)
                frustumCorners[i] = lightView * glm::vec4(frustumCorners[i],1.0);

            float xMin, xMax, yMin, yMax, zMin, zMax;
            xMin = yMin = zMin = std::numeric_limits<float>::max();
            xMax = yMax = zMax = std::numeric_limits<float>::min();

            for(int i =0; i<8; i++) {
                glm::vec3 p = frustumCorners[i];
                xMin = std::min(xMin,p.x);
                yMin = std::min(yMin,p.y);
                zMin = std::min(zMin,p.z);
                
                xMax = std::max(xMax,p.x);
                yMax = std::max(yMax,p.y);
                zMax = std::max(zMax,p.z);
            }

            // add margin
            // These constants are a hack
            zMax += 150.f;
            zMin -= 50.f;

            glm::mat4 lightProjection = glm::ortho(xMin,xMax,yMin,yMax,zMin,zMax);
            
            lightSpaceMatrix = lightProjection * lightView;
        }
    public:
        glm::vec3 color = glm::vec3(1.0,1.0,1.0);
        glm::vec3 direction = glm::normalize(glm::vec3(4.0,-2.0,4.0));

        Texture depthMap() { return data-> depthMap; }

        void Update(World& world) override {
            auto cameras = world.scenegraph.Filter<Camera>();
            MakeLightSpaceMatrix(*cameras[0]);
        }

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