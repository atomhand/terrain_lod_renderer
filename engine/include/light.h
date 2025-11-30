#pragma once
#include <memory>
#include <limits>
#include <glm/glm.hpp>
#include "scenegraph.h"
#include "camera.h"

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
                glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

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
    public:
        glm::vec3 color = glm::vec3(1.0,1.0,1.0);
        glm::vec3 direction = glm::normalize(glm::vec3(4.0,-2.0,4.0));

        Texture depthMap() { return data-> depthMap; }

        void PrepareRenderShadowmap() {
            glViewport(0, 0, SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, data->depthMapFBO);
            glClear(GL_DEPTH_BUFFER_BIT);
        }

        glm::mat4 cachedLightSpaceMatrix;

        glm::mat4 LightSpaceMatrix(Camera& camera) {
            glm::vec3 frustumCorners[8];
            glm::vec3 frustumCenter = camera.FrustumCorners(frustumCorners);

            glm::mat4 lightView = glm::lookAt(  frustumCenter-direction,
                                                frustumCenter,
                                                glm::vec3(0.f,1.f,0.f));

            // Transform frustum corners into the light's coordinate system
            for(int i=0; i<8; i++)
                frustumCorners[i] =  glm::vec4(frustumCorners[i],1.0);

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

            float margin = 100.f;
            zMax += margin;
            zMin -= margin;
            //zMin = std::max(0.1f,zMin);

            glm::mat4 lightProjection = glm::ortho(xMin,xMax,yMin,yMax,zMin,zMax);
            
            cachedLightSpaceMatrix = lightProjection * lightView;
            return cachedLightSpaceMatrix;
        }

        DirectionalLight() {
            data = std::make_shared<Data>();
        }
    };
}