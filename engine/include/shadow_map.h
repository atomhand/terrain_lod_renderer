#pragma once
#include <memory>
#include <iostream>
#include <glad/glad.h>

namespace Engine {
    class DirectionalShadowMap {
    private:
        struct DirectionalShadowMapData {
            DirectionalShadowMapData & operator=(const DirectionalShadowMapData&) = delete;
            DirectionalShadowMapData(const DirectionalShadowMapData&) = delete;

            GLuint depthMapFBO;
            GLuint depthMap; // texture object
            const unsigned int width;
            const unsigned int height;

            DirectionalShadowMapData(unsigned int width, unsigned int height) : width(width), height(height) {
                glGenFramebuffers(1,&depthMapFBO);                
                glGenTextures(1, &depthMap);

                // Set up depth map texture
                glBindTexture(GL_TEXTURE_2D, depthMap);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_HALF_FLOAT, NULL);
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
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
                glDrawBuffer(GL_NONE);
                glReadBuffer(GL_NONE);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }

            ~DirectionalShadowMapData() {
                glDeleteFramebuffers(1,&depthMapFBO);
                glDeleteTextures(1,&depthMap);
            }
        };

        std::shared_ptr<DirectionalShadowMapData> data;
    public:
        GLuint depthMap() {
            return data->depthMap;
        }

        void BindDepthMap() {
            glBindTexture(GL_TEXTURE_2D,data->depthMap);
        }

        DirectionalShadowMap(unsigned int width) {
            data = std::make_shared<DirectionalShadowMapData>(width,width);
        }
        
        void PrepareFramebuffer() {
            glViewport(0, 0, data->width, data->height);
            glBindFramebuffer(GL_FRAMEBUFFER, data->depthMapFBO);
            glClear(GL_DEPTH_BUFFER_BIT);
        }
    };
}