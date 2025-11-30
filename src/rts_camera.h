#pragma once
#include "camera.h"

class RtsCameraController : public Engine::SceneNode {
private:
    float zoom = 1.0;
    const float CAMERA_SPEED = 16.0;

    glm::vec3 target;

    void update_zoom(float scrollDelta) {
        zoom = glm::clamp(zoom-scrollDelta*0.1f, 0.0f, 1.0f);
    }

    void update_transforms() {
        localTransform = glm::translate(glm::mat4(1.), target);

        float scale = 64.f * (zoom+0.1f);
        m_camera->localTransform = glm::translate(glm::mat4(1.), glm::vec3(0.,0.,scale));
        m_camera->far = scale * 2.0;
    }

    Engine::SceneNode* cameraPivot;
    Engine::Camera* m_camera;
public:
    void OnEnter(Engine::SceneGraph& sceneGraph) override {
        cameraPivot = new Engine::SceneNode();
        cameraPivot->localTransform = glm::rotate(glm::mat4(1.), glm::radians(-45.f), glm::vec3(1.,0.,0.));
        sceneGraph.SetParent(cameraPivot,this);

        m_camera = new Engine::Camera();
        sceneGraph.SetParent(m_camera,cameraPivot);
    }

    void Update(Engine::World& world) override {
        float scale = 2.f * (zoom+0.1f);
        target += glm::vec3(world.input.xAxisKeyDelta, 0.0, world.input.yAxisKeyDelta)  * scale * CAMERA_SPEED * world.input.deltaTime;

        update_zoom(world.input.scrollDelta);
        update_transforms();
    }
};

