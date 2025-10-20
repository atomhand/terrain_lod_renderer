// Tom Kellett 2025
#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp> 
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <iostream>

#include "world.h"
#include "perspective.h"


#include "imgui.h"

namespace Engine
{
    struct Camera {
    public:
        float width = 1024.f;
        float height = 768.f;
        float fov = 45.0f;
        float near= 0.1f;
        // The camera far plane is used for culling
        // The camera projection doesn't actually have a finite far plane
        float far = 100.f;
        bool main = false;

        // does not control the camera position, just caches it
        glm::vec3 position;

        glm::mat4 projection;
        glm::mat4 view;
        glm::mat4 VP;
        glm::mat4 invCamera;

        // Distance to nearest and furthest items that passed culling
        float nearestItem;
        float furthestItem;

        void setViewport(int width, int height) {
            this->width = float(width);
            this->height = float(height);
        }

        glm::mat4 makeProjection(float nearOverride, float farOverride) const {
            return Perspective::infinitePerspectiveFovReverseZLH_ZO(glm::radians(fov), width, height, nearOverride);
            //return Perspective::reverse_z(Perspective::normalize_unit_range(glm::perspective(glm::radians(fov), width/height, nearOverride, farOverride)));
        }

        glm::mat4 makeProjection() const {
            return makeProjection(near,far);
        }

        glm::mat4 makeView(const Transform& transform) const {
            return glm::inverse(transform.global);
        }

        // Return the center of the subsection of the camera frustum bounded on the view-space z
        // axis by the near and far overrides
        glm::vec3 CascadeFrustumCenter(float nearOverride, float farOverride) {

            glm::mat4 proj = Perspective::reverse_z(Perspective::normalize_unit_range(glm::perspective(glm::radians(fov), width/height, nearOverride, farOverride)));

            glm::mat4 invCamera = glm::inverse(proj * view);

            // returns frustum center
            glm::vec4 hc = invCamera * glm::vec4(0.,0.,0.5,1.);
            return glm::vec3(hc) / hc.w;
        }

        // Return the corners of the camera frustum projected into world space
        glm::vec3 FrustumCorners(glm::vec3 (&corners)[8]) {
            glm::vec3 cube[8] {
                glm::vec3(-1.0,-1.0,-1.0), //000
                glm::vec3(-1.0,-1.0,1.0), //001
                glm::vec3(-1.0,1.0,-1.0), //010
                glm::vec3(-1.0,1.0,1.0), //011
                glm::vec3(1.0,-1.0,-1.0), //100
                glm::vec3(1.0,-1.0,1.0), //101
                glm::vec3(1.0,1.0,-1.0), //110
                glm::vec3(1.0,1.0,1.0), //111
            };

            for(int i =0; i<8; i++) {
                glm::vec4 h = invCamera * glm::vec4(cube[i], 1.0);
                corners[i] = glm::vec3(h) / h.w;
            }

            // returns frustum center
            glm::vec4 hc = invCamera * glm::vec4(0.,0.,0.,1.);
            return glm::vec3(hc) / hc.w;
        }
    };

    static void UpdateCameraSystem(World& world) {
        auto view = world.registry.view<Camera,const Transform>();

        for(auto entity : view) {
            auto [camera,transform] = view.get<Camera,const Transform>(entity);

            camera.position = transform.global * glm::vec4(0.,0.,0.,1.);
            camera.projection = camera.makeProjection();
            camera.view = camera.makeView(transform);
            camera.VP = camera.projection * camera.view;
            camera.invCamera = glm::inverse(camera.VP);
        }
    }
}