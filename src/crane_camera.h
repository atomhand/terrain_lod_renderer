#pragma once
#include <algorithm>
#include "camera.h"
#include "world.h"
#include "terrain.h"
#include "crane.h"

class CraneCamera : public Engine::SceneNode {
private:
    const float CAMERA_SPEED = 256.0;
    float pitch;
    float yaw;

    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f); 
    glm::vec3 pos = glm::vec3(0.0f,32.f,0.0f);

    float current_speed = 1.0;

    void CalcFlyCameraTransform(glm::vec2 keyDelta, float deltaTime, Terrain* terrain) {
        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(direction);

        glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront,cameraUp));

        glm::vec3 target = pos+cameraFront*8.0f;
        float targetY = target.y;
        if(terrain != nullptr) {
            for(int i =0; i<16; i++) {
                target = pos+cameraFront* (2.0f * i);
                float terrainHeight = std::max(0.0f,terrain->Height(target.x,target.z));
                targetY = std::max(targetY,terrainHeight + 64.0f - i * 2.0f);
            }
            
            target.y = targetY;
            keyDelta.y = std::max(0.5f,keyDelta.y);
            keyDelta = normalize(keyDelta);
        }

        glm::vec3 dir = (normalize(target-pos) * keyDelta.y + cameraRight * keyDelta.x);
        float climb_speed_factor = 1.0 - dir.y;
        if(current_speed < climb_speed_factor)
            current_speed = std::min(current_speed+0.25f * deltaTime, climb_speed_factor);
        else            
            current_speed = std::max(current_speed-1.5f * deltaTime, climb_speed_factor);
        pos += CAMERA_SPEED * current_speed * deltaTime * dir;

        m_camera->far = far;
        crane->localTransform = glm::inverse(glm::lookAt(pos,target,cameraUp));
        m_camera->localTransform = glm::inverse(glm::lookAt(pos-cameraFront*16.0f,pos,cameraUp));
    }
    Engine::Camera* m_camera;
    Crane* crane;
public:
    float far = 4096.0;

    void OnEnter(Engine::SceneGraph& sceneGraph) override {
        crane = new Crane();
        sceneGraph.SetParent(crane,this);

        m_camera = new Engine::Camera();
        m_camera->main = true;
        //m_camera->localTransform = glm::translate(glm::mat4(1.),glm::vec3(0.,0.,16.));
        sceneGraph.SetParent(m_camera,this);
        CalcFlyCameraTransform(glm::vec2(0.),0.0f,nullptr);

    }

    void Update(Engine::World& world) override {

        if(world.input.flyCamera) {
            const float sensitivity = 0.05;
            yaw += world.input.mousePosDelta.x * sensitivity;
            pitch -= world.input.mousePosDelta.y * sensitivity;
            pitch = std::clamp(pitch,-89.f,89.f);
        } else {
            //m_camera->main = false;
        }
        Terrain* terrain = world.scenegraph.First<Terrain>();
        CalcFlyCameraTransform(world.input.keyAxisDelta, world.input.deltaTime,terrain);
}
};