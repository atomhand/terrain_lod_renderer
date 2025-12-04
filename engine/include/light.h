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
#include "shadow_map.h"

// Shadow map code partially adapted from https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
// Modified to use
// -- Opengl shadow sampler instead of standard texture sampler
// -- Adaptive frustum calculation

namespace Engine {
    class RenderItem;

    class PointLight : public SceneNode {
    public:
        glm::vec3 color = glm::vec3(1.0,1.0,1.0);
    };

    class DirectionalLight : public SceneNode {
    public:
        DirectionalShadowMap shadowMap;
        glm::vec3 color = glm::vec3(1.0,1.0,1.0);
        glm::vec3 direction = glm::normalize(glm::vec3(4.0,-2.0,4.0));

        glm::mat4 lightView;
        glm::mat4 lightProjection;

        // Build the matrix which defines the projection from light space
        // This should be called once per frame, 
        std::vector<size_t> MakeLightSpaceMatrix(Camera& camera, std::vector<RenderItem*> &shadowReceivers, std::vector<RenderItem*> &shadowCasters, std::vector<AABB> &terrainAABBs, std::vector<bool> terrainCullingResults);

        glm::mat4 lightSpaceMatrix;

        DirectionalLight() : shadowMap(DirectionalShadowMap(4096)) {
        }
    };
}