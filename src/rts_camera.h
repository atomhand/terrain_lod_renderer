#pragma once
#include "camera.h"
#include "world.h"
#include "application.h"


struct RtsCamera {
private:
    float zoom = 1.0;

public:
    void update_zoom(float scroll_delta) {
        zoom = glm::clamp(zoom-scroll_delta*0.1f, 0.0f, 1.0f);
    }

    glm::mat4 getTransform() {        
        glm::vec3 target = glm::vec3(0.0f,0.0f,0.0f);
        glm::vec3 pos = target + glm::vec3(0.0f,5.0f,-3.0f) * (zoom+0.1f);
        return glm::lookAt(pos, target, glm::vec3(0.0f,1.0f,0.0f));
    }
};

void UpdateCameraSystem(World& world, Engine::Application& app) {
    auto view = world.registry.view<Engine::Camera,RtsCamera>();

    for(auto entity : view) {
        auto &camera = view.get<Engine::Camera>(entity);
        auto &rts_camera = view.get<RtsCamera>(entity);
        
        rts_camera.update_zoom(world.input.scrollDelta);
        camera.transform = rts_camera.getTransform();
    }
}