// Tom Kellett 2025
#pragma once
#include <algorithm>
#include "camera.h"
#include "world.h"

using Engine::World, Engine::Camera, Engine::Transform;

// Fly camera behaviour from 
class FlyCamera {
private:
    static const inline float CAMERA_SPEED { 100.f };
    float pitch;
    float yaw;
    glm::vec3 pos;
    float fallSpeed = 0.0;

    static glm::vec3 makeDirection(float yaw, float pitch) {        
        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        return direction;
    }
public:
    float far = 4096.0;
    
    static void Setup(World& world, glm::vec3 camStartPos, float farPlane) {
        auto entity = world.registry.create();

        auto& camera = world.registry.emplace<Camera>(entity);
        camera.main = true;
        camera.far = farPlane;

        world.registry.emplace<Transform>(entity);
        auto& fly = world.registry.emplace<FlyCamera>(entity);
        fly.pos = camStartPos;
    }
    
    // If noClip is disabled (press T) you will be ground-bound
    // Fly camera direction/transform construction is from https://learnopengl.com/Getting-started/Camera 
    static void Update(World& world) {
        if(!world.input.flyCamera)
            return;

        glm::vec2 keyDelta = world.input.keyAxisDelta;
        bool noClip = world.input.noClip;
        float deltaTime = world.input.deltaTime;

        auto view = world.registry.view<Camera,Transform,FlyCamera>();

        auto terrain_view = world.registry.view<Terrain>();
        Terrain* terrain = terrain_view.front() == entt::null ? nullptr : &terrain_view.get<Terrain>(terrain_view.front());

        for(auto entity : view) {
            auto [camera,transform,flyCamera] = view.get(entity);
            
            // Mouse control
            const float sensitivity = 0.05;
            flyCamera.yaw += world.input.mousePosDelta.x * sensitivity;
            flyCamera.pitch -= world.input.mousePosDelta.y * sensitivity;
            flyCamera.pitch = std::clamp(flyCamera.pitch,-89.f,89.f);

            // Update transform
            
            glm::vec3 direction = flyCamera.makeDirection(flyCamera.yaw,flyCamera.pitch);
            
            glm::vec3 cameraFront = glm::normalize(direction);
            glm::vec3 cameraUp = glm::vec3(0.f,1.f,0.f);
            glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront,cameraUp));

            float clearance = flyCamera.pos.y;

            if(!noClip && terrain != nullptr) {
                float terrainHeight = std::max(0.5f,terrain->Height(flyCamera.pos.x,flyCamera.pos.z)+6.0f);
                if(flyCamera.pos.y > terrainHeight) {
                    flyCamera.fallSpeed += 9.8 * deltaTime * 2.0;
                    flyCamera.pos.y -= flyCamera.fallSpeed * deltaTime;
                } else {
                    flyCamera.fallSpeed = 0.f;
                }
                flyCamera.pos.y = std::max(flyCamera.pos.y,terrainHeight);
                clearance = flyCamera.pos.y - terrainHeight - 1.f;
            }
            float clearanceFactor = std::clamp(clearance / 10000.f,0.f,1.f);

            float spd = noClip ? CAMERA_SPEED* (1.0f + clearanceFactor * 200.f) : CAMERA_SPEED;
            flyCamera.pos += deltaTime * spd * (cameraFront * keyDelta.y + cameraRight * keyDelta.x);

            camera.near = std::lerp(1.f, 320.f, clearanceFactor);
            //camera.far = far;
            transform.global = glm::inverse(glm::lookAt(flyCamera.pos,flyCamera.pos+cameraFront,cameraUp));
        }
    }
};