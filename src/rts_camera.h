#pragma once
#include "camera.h"

class RtsCamera : public Engine::Camera {
private:
    float zoom = 1.0;
    const float CAMERA_SPEED = 16.0;

    glm::vec3 target;

    void update_zoom(float scrollDelta) {
        zoom = glm::clamp(zoom-scrollDelta*0.1f, 0.0f, 1.0f);
    }

    void update_transform() {
        float scale = 2.f * (zoom+0.1f);

        glm::vec3 offset = glm::vec3(0.0f,15.0f,-9.0f) * scale;
        position = target + offset;
        transform = glm::lookAt(position, target, glm::vec3(0.0f,1.0f,0.0f));

        far = glm::length(offset) * 2.0;
    }
public:
    void update(Engine::World& world) {
        float scale = 2.f * (zoom+0.1f);
        target += glm::vec3(-world.input.xAxisKeyDelta, 0.0, -world.input.yAxisKeyDelta)  * scale * CAMERA_SPEED * world.input.deltaTime;

        update_zoom(world.input.scrollDelta);
        update_transform();
    }
};

