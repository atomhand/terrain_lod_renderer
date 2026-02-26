// Tom Kellett 2025
// reference https://learnopengl.com/Getting-started/Camera
// --> Code to convert pitch and yaw to a direction is from the OpenGL tut
// everything else original mine
#pragma once
#include <algorithm>
#include <glm/gtc/random.hpp>
#include "world.h"
#include "terrain.h"
#include "bird_material.h"
#include "profiler.h"

using Engine::Transform, Engine::Camera, Engine::World;

// Instantiates a flock of birds which fly around randomly
// When they get too far from the main camera they'll teleport to a new position a bit closer to it
class BirdFlockManager {
private:
    const static inline float ACTIVE_RADIUS = 5000.f;
    const static inline int UPDATE_INTERVAL = 50;
    const static inline int NUM_BIRDS = 10000;

    static inline glm::mat4 birdModelTransform;

    class Bird {        
        static inline const float SPEED = 16.0;
        float yaw;
        
        glm::vec3 birdFront = glm::vec3(0.0f, 0.0f, -1.0f);
        static inline const  glm::vec3 birdUp    = glm::vec3(0.0f, 1.0f,  0.0f); 

        float current_speed = 1.0;
        float steer = 0.f;
    public:
        glm::vec3 pos = glm::vec3(0.0f,32.f,0.0f);
        glm::vec3 direction = glm::vec3(1.f,0.f,0.f);
        glm::vec3 targetDirection = glm::vec3(1.f,0.f,0.f);
        int ticksSinceTrajectoryUpdate = 0;

        // Test the terrain in a given direction
        // Returns a higher score the more different the terrain altitude (+some clearance) is from the bird's current altitude
        // (high score is less preferred)
        float sampleDirection(float iYaw, Terrain& terrain) {
            glm::vec3 dir = glm::vec3(cos(glm::radians(iYaw)),
                    0.f,
                    sin(glm::radians(iYaw)));

            float targetY = pos.y;

            glm::vec3 target = pos+dir*16.0f;
            float terrainHeight = std::max(0.0f,terrain.Height(target.x,target.z));
            targetY = terrainHeight + 96.0f;

            return (targetY-pos.y)*(targetY-pos.y) + 1e-6;
        }

        void UpdateTrajectory(Transform& transform, float deltaTime, Terrain& terrain) {
            ticksSinceTrajectoryUpdate = 0;
            // steering behaviour
            // birds will (loosely) prefer to steer towards terrains which let them maintain a similar
            // altitude to their current one

            float w1 = sampleDirection(yaw-15,terrain);
            float w2 = sampleDirection(yaw+15,terrain);

            float t = w1+w2;
            w1 /= t;
            w2 /= t;
            float steer = w1 * 15. + w2 * (-15.);
            yaw += steer * deltaTime;

            //direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            //direction.y = sin(glm::radians(pitch));
            //direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            targetDirection.x = cos(glm::radians(yaw));
            targetDirection.z = sin(glm::radians(yaw));
            targetDirection.y = -0.1f;

            // Determine whether its necessary to climb
            glm::vec3 target;
            for(int i =0; i<4; i++) {
                target = pos+targetDirection* (8.0f * i);
                float terrainHeight = std::max(0.0f,terrain.Height(target.x,target.z));
                target.y = std::max(target.y,terrainHeight + 96.0f - i * 12.0f);
            }

            // Slow down when climbing, speed up when diving
            targetDirection = glm::normalize(target-pos);
        }

        void UpdateMotion(Transform& transform, float deltaTime, Terrain& terrain) {
            direction = glm::normalize(glm::mix(direction,targetDirection, ticksSinceTrajectoryUpdate/float(UPDATE_INTERVAL)));

            float speed_target = std::max(0.1f,1.0f - direction.y);
            if(current_speed < speed_target)
                current_speed = std::min(current_speed+0.25f * deltaTime, speed_target);
            else            
                current_speed = std::max(current_speed-1.5f * deltaTime, speed_target);
            // Apply movement and calculate transform
            pos += SPEED * current_speed * deltaTime * direction;
            transform.global = glm::inverse(glm::lookAt(pos,pos+direction,birdUp)) * birdModelTransform;
        }

        // Reset to a random position and facing, on the edge of a circle around the camera
        void Randomise(glm::vec3 cameraPos, float radius, Terrain& terrain, bool initialPlacement = false) {
            glm::vec2 posxz = glm::vec2(cameraPos.x,cameraPos.z) + (initialPlacement ? glm::diskRand(radius) : glm::circularRand( radius));
            pos.x = posxz.x;
            pos.z = posxz.y;
            
            float terrainHeight = std::max(0.0f,terrain.Height(pos.x,pos.z));        
            pos.y = terrainHeight+32.f + glm::gaussRand(0.f,16.f);
            
            yaw = glm::linearRand(0.f,360.f);
        }
    };

    // Make 1 bird
    static Bird& MakeBird(Engine::World &world, uint32_t meshId, int i) {
        auto bird_entity = world.registry.create();

        auto& bird = world.registry.emplace<Bird>(bird_entity);
        auto& transform = world.registry.emplace<Transform>(bird_entity);

        world.registry.emplace<Engine::RenderEnabledMarker>(bird_entity);

        bird.ticksSinceTrajectoryUpdate = i % UPDATE_INTERVAL;

        float animationPhaseOffset = glm::linearRand(0.f,1.f);
        BirdMaterial::InitBirdItem(world, bird_entity, meshId, animationPhaseOffset);
        return bird;
    }

    std::vector<Bird*> birds;
public:
    // On entering the scenegraph
    static void Setup(World& world) {
        BirdMaterial::Setup(world);

        auto& meshCache = world.GetSingle<Engine::MeshCache>();

        uint32_t birdMeshId = Engine::AssimpWrapper::ImportMesh(meshCache, "models/Bird_Asset.fbx")[0];
        auto& birdMeshHeader = meshCache.meshHeaders[birdMeshId];

        float width = birdMeshHeader.aabbMax.x - birdMeshHeader.aabbMin.x;
        float scale = 9.0 / width;
        birdModelTransform = glm::rotate(glm::mat4(1.0f), glm::radians(180.f), glm::vec3(0.f,1.f,0.f)) * glm::scale(glm::mat4(1.0f), glm::vec3(scale));

        Terrain& terrain = world.GetSingle<Terrain>();
        for(int i =0; i<NUM_BIRDS;i++) {
            auto& bird = MakeBird(world, birdMeshId, i);
            bird.Randomise(glm::vec3(0.f), ACTIVE_RADIUS,terrain,true);
        }
    }

    // Update bird positions (and rerandomise them if they are too far from the main camera)
    static void Update(Engine::World& world) {
        auto profileHandle = Engine::Profiler::StartCpu("BirdFlock::Update");
        Engine::Camera* cameraMain;
        Engine::Transform* cameraMainTransform;
        auto cameraView = world.registry.view<Camera,Transform>();
        for(auto entity : cameraView) {
            auto [camera,transform] = cameraView.get(entity);
            if(camera.main) {
                cameraMain = &camera;
                cameraMainTransform = &transform;
                break;
            }
        }
        
        Terrain& terrain = world.GetSingle<Terrain>();
        glm::vec3 cameraPos = cameraMainTransform->position();

        auto birdView = world.registry.view<Bird,Transform>();

        for(auto entity : birdView) {
            auto [bird,transform] = birdView.get(entity);

            bird.ticksSinceTrajectoryUpdate += 1;

            if(bird.ticksSinceTrajectoryUpdate == UPDATE_INTERVAL) {
                glm::vec2 birdxz = glm::vec2(bird.pos.x,bird.pos.z);
                glm::vec2 camxz = glm::vec2(cameraPos.x,cameraPos.z);
                float rad = std::max(cameraPos.y,ACTIVE_RADIUS);
                if(glm::distance(birdxz,camxz) > rad * 1.5f)
                    bird.Randomise(cameraPos, ACTIVE_RADIUS,terrain);
                    
                bird.UpdateTrajectory(transform,world.input.deltaTime*world.input.animSpeed*10.f,terrain);
            }
            bird.UpdateMotion(transform,world.input.deltaTime*world.input.animSpeed,terrain);
        }
    }
};