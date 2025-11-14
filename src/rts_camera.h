#pragma once
#include "camera.h"

class RtsCamera : public Engine::Camera {
private:
    float zoom = 1.0;
    const float CAMERA_SPEED = 8.0;

    glm::vec3 target;

    void update_zoom(float scrollDelta) {
        zoom = glm::clamp(zoom-scrollDelta*0.1f, 0.0f, 1.0f);
    }

    void update_transform() {
        glm::vec3 pos = target + glm::vec3(0.0f,15.0f,-9.0f) * (zoom+0.1f);
        transform = glm::lookAt(pos, target, glm::vec3(0.0f,1.0f,0.0f));
    }
public:
    void update(World& world) {
        update_zoom(world.input.scrollDelta);
        update_transform();

        target += glm::vec3(-world.input.xAxisKeyDelta, 0.0, -world.input.yAxisKeyDelta)  * CAMERA_SPEED * world.input.deltaTime;
    }
};

