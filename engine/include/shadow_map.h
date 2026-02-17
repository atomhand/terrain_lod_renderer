// Tom Kellett 2025
// partly adapted from https://learnopengl.com/Guest-Articles/2021/CSM
#pragma once
#include <memory>
#include <iostream>
#include <glad/gl.h>
#include "shader.h"
#include "material.h"

namespace Engine {
    class DirectionalShadowCascadeMap {
    private:
        struct DirectionalShadowCascadeMapData {
            DirectionalShadowCascadeMapData & operator=(const DirectionalShadowCascadeMapData&) = delete;
            DirectionalShadowCascadeMapData(const DirectionalShadowCascadeMapData&) = delete;

            std::vector<GLuint> layerFBO;
            GLuint depthMaps; // texture object
            const unsigned int width;
            const unsigned int height;

            const unsigned int NUM_CASCADES;

            DirectionalShadowCascadeMapData(unsigned int width, unsigned int height, uint32_t numCascades) : width(width), height(height), NUM_CASCADES(numCascades) {             
                glGenTextures(1, &depthMaps);

                // Set up depth map texture

                glBindTexture(GL_TEXTURE_2D_ARRAY, depthMaps);
                glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, width, height, NUM_CASCADES, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_GEQUAL);

                // fragments outside the shadow map are assumed not to be occluded
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                float white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
                glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, white);

                layerFBO.resize(NUM_CASCADES);
                glGenFramebuffers(NUM_CASCADES, layerFBO.data());
                for(int i =0; i<NUM_CASCADES; i++) {
                    glBindFramebuffer(GL_FRAMEBUFFER, layerFBO[i]);
                    glNamedFramebufferTextureLayer( layerFBO[i], GL_DEPTH_ATTACHMENT, depthMaps, 0, i);
                    glNamedFramebufferDrawBuffer( layerFBO[i], GL_NONE);
                    glNamedFramebufferReadBuffer( layerFBO[i], GL_NONE);
                    
                    int status = glCheckNamedFramebufferStatus(layerFBO[i],GL_FRAMEBUFFER);
                    if (status != GL_FRAMEBUFFER_COMPLETE)
                    {
                        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!";
                        throw 0;
                    }
                }

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }

            ~DirectionalShadowCascadeMapData() {
                glDeleteFramebuffers(NUM_CASCADES,layerFBO.data());
                glDeleteTextures(1,&depthMaps);
            }
        };

        std::shared_ptr<DirectionalShadowCascadeMapData> data;
    public:
        uint32_t NumCascades() {
            return data->NUM_CASCADES;
        }
        
        GLuint depthMaps() {
            return data->depthMaps;
        }

        void BindDepthMap() {
            glBindTexture(GL_TEXTURE_2D_ARRAY,data->depthMaps);
        }

        DirectionalShadowCascadeMap(unsigned int width, uint32_t numCascades) {
            data = std::make_shared<DirectionalShadowCascadeMapData>(width,width,numCascades);
        }
        
        void PrepareFramebufferLayer(uint32_t layer) {
            assert(layer < data->NUM_CASCADES);
            glViewport(0, 0, data->width, data->height);
            glBindFramebuffer(GL_FRAMEBUFFER, data->layerFBO[layer]);
            glClear(GL_DEPTH_BUFFER_BIT);
        }
    };

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
                glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GEQUAL);

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