#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp> 
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective

namespace Engine
{
    class Camera {
    protected:
        glm::mat4x4 transform;

        float fov = 45.0f;

        float width = 1024.f;
        float height = 768.f;

        float near= 0.1f;
        float far = 100.f;  
    public:
        glm::mat4 get_proj() const {
            return glm::perspective(glm::radians(fov), width/height, near, far);
        }

        glm::mat4 get_view() const {
            return transform;
        }
    };
}