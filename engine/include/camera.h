#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp> 
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <iostream>
#include "scenegraph.h"

namespace Engine
{
    class Camera : public SceneNode {
    protected:        
        float width = 1024.f;
        float height = 768.f;
    public:
        float fov = 45.0f;
        float near= 0.1f;
        float far = 100.f;

        void setFramebufferSize(int width, int height) {
            this->width = float(width);
            this->height = float(height);
        }

        glm::mat4 projection() const {
            return glm::perspective(glm::radians(fov), width/height, near, far);
        }

        glm::mat4 view() const {
            return glm::inverse(globalTransform);
        }

        glm::vec3 position() const {
            return view() * glm::vec4(0.,0.,0.,1.);
        }

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

            glm::mat4 invCamera = glm::inverse(projection() * view());

            for(int i =0; i<8; i++) {
                glm::vec4 h = invCamera * glm::vec4(cube[i], 1.0);
                corners[i] = glm::vec3(h) / h.w;
            }

            // returns frustum center
            glm::vec4 hc = invCamera * glm::vec4(0.,0.,0.,1.);
            return glm::vec3(hc) / hc.w;
        }
    };
}