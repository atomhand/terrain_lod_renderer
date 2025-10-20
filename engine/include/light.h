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

            Cascade(float near, float far, Camera& camera, glm::vec3 direction) : near(near), far(far), frustumCenter(camera.CascadeFrustumCenter(near,far)), direction(direction) {
                view = glm::lookAt(frustumCenter,
                                    frustumCenter+direction,
                                glm::vec3(0.f,1.f,0.f));
            };

            void updateProjection() {
                projection = Perspective::reverse_z(Perspective::normalize_unit_range(glm::ortho(min.x,max.x,min.y,max.y,min.z,max.z)));
            }
        };
    public:
        DirectionalShadowCascadeMap shadowMap;
        glm::vec3 color = glm::vec3(1.0,1.0,1.0);
        glm::vec3 direction = glm::normalize(glm::vec3(4.0,-2.0,4.0));

        glm::mat4 lightView;
        glm::mat4 lightProjection;

        
        std::vector<float> cascadeLevels;

        unsigned int NumCascades() {
            return cascadeLevels.size();
        }

        // Activate the shadow (depth map) shader ready for drawing
        Material& UseShadowMaterial() {
            return shadowMap.UseShadowMaterial();
        }

        // Build the world-to-light-space matrices for all cascades
        void MakeLightSpaceMatrices(World& world, Camera& camera);

        std::vector<glm::mat4> lightSpaceMatrices;

        DirectionalLight() : shadowMap(DirectionalShadowCascadeMap(2048)) {
        }
    };
}