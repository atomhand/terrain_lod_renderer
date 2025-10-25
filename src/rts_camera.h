#pragma once
#include "camera.h"

class RtsCamera : public Engine::Camera {
private:
    float zoom = 1.0;


public:
    void update_zoom(float scroll_delta) {
        zoom = glm::clamp(zoom-scroll_delta*0.1f, 0.0f, 1.0f);
    }

    void update_transform() {        
        glm::vec3 target = glm::vec3(0.0f,0.0f,0.0f);
        glm::vec3 pos = target + glm::vec3(0.0f,5.0f,-3.0f) * (zoom+0.1f);
        transform = glm::lookAt(pos, target, glm::vec3(0.0f,1.0f,0.0f));
    }
};