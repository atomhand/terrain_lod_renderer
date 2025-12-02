#pragma once
#include <algorithm>
#include "camera.h"
#include "world.h"

class FlyCamera : public Engine::SceneNode {
private:
    const float CAMERA_SPEED = 16.0;
    float pitch;
    float yaw;
    bool flyCamera = true;

    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f); 

    glm::vec3 pos = glm::vec3(0.0f,32.f,0.0f);

    void CalcFlyCameraTransform(glm::vec2 keyDelta) {
        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(direction);

        glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront,cameraUp));

        pos += CAMERA_SPEED * (cameraFront * keyDelta.y + cameraRight * keyDelta.x);

        m_camera->far = 256.0;
        m_camera->localTransform = glm::inverse(glm::lookAt(pos,pos+cameraFront,cameraUp));
    }
    Engine::Camera* m_camera;
public:
    void OnEnter(Engine::SceneGraph& sceneGraph) override {
        m_camera = new Engine::Camera();
        m_camera->main = false;
        sceneGraph.SetParent(m_camera,this);
    }

    void Update(Engine::World& world) override {
        if(world.input.flyCamera) {
            m_camera->main = true;
            const float sensitivity = 0.05;
            yaw += world.input.mousePosDelta.x * sensitivity;
            pitch -= world.input.mousePosDelta.y * sensitivity;
            pitch = std::clamp(pitch,-89.f,89.f);
            CalcFlyCameraTransform(world.input.keyAxisDelta * world.input.deltaTime);
        } else {
            m_camera->main = false;
        }
    }
};