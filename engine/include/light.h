// Tom Kellett 2025

// references (not copied) https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
// and https://learnopengl.com/Guest-Articles/2021/CSM
//
// Compared to the learnopengl Cascades implementation, mine is quite different (possibly worse)
// because I came up with my own algorithm to calculate the frustum/projection for each cascade

#pragma once
#include <memory>
#include <limits>
#include <glad/gl.h>
#include <glm/glm.hpp>

#include <texture.h>
#include "camera.h"
#include "world.h"
#include "culling.h"
#include "shadow_map.h"
#include "material.h"
#include "perspective.h"

#include "storage_buffer.h"
#include "compute_shader.h"
#include "uniform_buffer.h"

namespace Engine {
    class RenderItem;

    struct ShadowCaster{};
    struct SurvivedLightCullingTag{};

    struct DirectionalLight {
    private:
        struct Cascade {
            glm::vec3 min = glm::vec3(9999999.f);
            glm::vec3 max = glm::vec3(-9999999.f);

            glm::mat4 view;
            float near;
            float far;
            glm::vec3 frustumCenter;
            glm::vec3 direction;
            glm::mat4 projection;

            static glm::vec3 center(glm::mat4 inverseVp, float near, float far) {
                glm::dvec4 frustumCorners[8] = {
                    glm::vec4(-1.0,    -1.0,   0.0,1.0),
                    glm::vec4(1.0,     -1.0,   0.0,1.0),
                    glm::vec4(-1.0,    1.0,    0.0,1.0),
                    glm::vec4(1.0,     1.0,    0.0,1.0),
                    
                    glm::vec4(-1.0,    -1.0,   1.0,1.0),
                    glm::vec4(1.0,     -1.0,   1.0,1.0),
                    glm::vec4(-1.0,    1.0,    1.0,1.0),
                    glm::vec4(1.0,     1.0,    1.0,1.0),
                };

                // get view frustum corners
                for(int i=0; i<8; i++) {
                    frustumCorners[i] = inverseVp * frustumCorners[i];
                    frustumCorners[i] /= frustumCorners[i].w;
                }

                // get cascade frustum
                for(int i =0; i<4; i++) {
                    glm::vec4 ro = frustumCorners[i];
                    glm::vec4 rd = normalize(frustumCorners[i+4]-frustumCorners[i]);

                    frustumCorners[i] = ro + near * rd;
                    frustumCorners[i+4] = ro + far * rd;
                }

                glm::vec4 centroid = glm::vec4(0.0);
                for(int i=0; i<8; i++) {
                    centroid += frustumCorners[i];
                }
                centroid /= 8.0;
                return glm::vec3(centroid);
            }

            Cascade(float near, float far, Camera& camera, glm::vec3 direction) : near(near), far(far), frustumCenter(center(camera.lightingInvVP,near,far)), direction(direction) {
                view = glm::lookAt(frustumCenter,
                                    frustumCenter+direction,
                                glm::vec3(0.f,1.f,0.f));
            };

            void updateProjection() {
                projection = Perspective::reverse_z(Perspective::normalize_unit_range(glm::ortho(min.x,max.x,min.y,max.y,min.z,max.z)));
            }
        };

        StorageBuffer lightSpaceMatricesBuffer = StorageBuffer(16 * sizeof(glm::mat4));
        StorageBuffer lightViewMatricesBuffer = StorageBuffer(16 * sizeof(glm::mat4));
        StorageBuffer lightFrustumPlanesBuffer = StorageBuffer(16 * 6 * sizeof(float));
        StorageBuffer depthAnalysisOutput = StorageBuffer(2 * sizeof(uint32_t));
        StorageBuffer cascadeAnalysisOutput = StorageBuffer(16 * 6 * sizeof(uint32_t));

        // vec4 format because UBO requires std140 alignment
        StorageBuffer cascadePlaneDistancesBuffer = StorageBuffer(16 * sizeof(glm::vec4));

        ComputeShader depthAnalysisKernel = ComputeShader("shaders/corepass/sdsm_depth_reduction.cs");
        ComputeShader chooseMatricesKernel = ComputeShader("shaders/corepass/sdsm_light_view.cs");
        ComputeShader cascadeAnalysisKernel = ComputeShader("shaders/corepass/sdsm_cascade_reduction.cs");
        ComputeShader  finishMatricesKernel = ComputeShader("shaders/corepass/sdsm_light_proj.cs");

    public:
        DirectionalShadowCascadeMap shadowMap;
        glm::vec3 color = glm::vec3(1.0,1.0,1.0);
        glm::vec3 direction = glm::normalize(glm::vec3(4.0,-2.0,4.0));

        uint32_t NumCascades;

        // Build the world-to-light-space matrices for all cascades
        void MakeLightSpaceMatrices(World& world, Camera& camera, Texture& depthBuffer, UniformBuffer& lightUniforms);

        std::vector<glm::mat4> DebugLightCullingFrusta() {
            std::vector<float> frustumVals;

            std::vector<glm::mat4> debugFrusta;

            debugFrusta.resize(NumCascades);
            frustumVals.resize(NumCascades * 6);
            glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
            lightFrustumPlanesBuffer.Readback<float>(frustumVals.data(), NumCascades*6, 0);
            lightViewMatricesBuffer.Readback<glm::mat4>(debugFrusta.data(), NumCascades, 0);

            if(ImGui::Begin("Light frustum debug")) {
                for(int i =0; i<NumCascades; i++ ) {
                    ImGui::Separator();
                    ImGui::Text("Cascade %i", i);
                    ImGui::Text("l %f", frustumVals[i*6+0]);
                    ImGui::Text("r %f", frustumVals[i*6+1]);
                    ImGui::Text("b %f", frustumVals[i*6+2]);
                    ImGui::Text("t %f", frustumVals[i*6+3]);
                    ImGui::Text("near %f", frustumVals[i*6+4]);
                    ImGui::Text("far %f", frustumVals[i*6+5]);
                }
            }

            ImGui::End();
            return debugFrusta;
        }

        void BindUniforms() {
            glBindBufferBase(GL_UNIFORM_BUFFER, 3, lightSpaceMatricesBuffer.object());
            glBindBufferBase(GL_UNIFORM_BUFFER, 4, cascadePlaneDistancesBuffer.object());

            lightViewMatricesBuffer.BindBase(8);
            lightFrustumPlanesBuffer.BindBase(9);
        }

        void BindCascadeToCullingVPUniform(UniformBuffer& targetBuffer, uint32_t cascade) {
            glCopyNamedBufferSubData(lightSpaceMatricesBuffer.object(), targetBuffer.object(), cascade*sizeof(glm::mat4), 0, sizeof(glm::mat4));
            targetBuffer.BindBase(5);
            //glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
        }

        std::vector<glm::mat4> ReadbackLightMatrices() {
            std::vector<glm::mat4> ret;
            ret.resize(NumCascades);
            glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
            lightSpaceMatricesBuffer.Readback<glm::mat4>(ret.data(), NumCascades, 0);
            return ret;
        }

        DirectionalLight(uint32_t NumCascades) : NumCascades(NumCascades), shadowMap(DirectionalShadowCascadeMap(2048,NumCascades)) {
        }
    };
}