#pragma once

#include "world.h"
#include "gpu_render.h"
#include "gpu_mesh.h"
#include <memory>
#include "shapes.h"
#include "culling.h"

using Engine::GpuRender, Engine::World, Engine::MaterialHeader, Engine::MaterialRenderComponent, Engine::MaterialImplementation, Engine::MeshCache, Engine::MeshHeader;

struct TestGpuMaterial{
private:
    class TestMaterialImplementation : public MaterialImplementation {
    public:
        TestMaterialImplementation() : MaterialImplementation("TestMaterial") {
            std::vector<const char*> defs = {"#define VERTEX_NORMAL","#define VERTEX_UV"};
            MaterialImplementation::passShaders = {
                { Engine::RenderPassId::OPAQUE, Engine::Shader("shaders/pbr.vert","shaders/pbr.frag", defs) },
                { Engine::RenderPassId::SHADOW, Engine::Shader("shaders/gpu_shadow.vert","shaders/shadow.frag", defs) },
            };
        };
    };
public:
    static void Setup(Engine::World& world) {
        auto& gpuRender = world.GetSingle<GpuRender>();
        auto& meshCache = world.GetSingle<MeshCache>();
        auto headerEntity = world.registry.create();

        auto& header = gpuRender.RegisterMaterial<TestMaterialImplementation>(world,headerEntity);
        header.SetRenderPass(Engine::RenderPassId::OPAQUE);
        header.SetRenderPass(Engine::RenderPassId::SHADOW);
        header.SetRenderPass(Engine::RenderPassId::DIAGNOSTIC);

        // sphere
        auto sphere = Sphere(16,16);
        uint32_t sphereId = meshCache.RegisterMesh(sphere);

        
        for(int i = 0; i<1000; i++) {
            auto entity = world.registry.create();

            world.registry.emplace<Engine::RenderEnabledMarker>(entity);
            auto& instance = world.registry.emplace<Engine::GpuMaterialInstance>(entity);
            instance.materialId = header.id;
            instance.meshId = sphereId;

            auto& transform = world.registry.emplace<Engine::Transform>(entity);
            transform.global = glm::translate(glm::mat4(1.0), glm::vec3(0.0,i*64.0,0.0)) * glm::scale(glm::mat4(1.0),glm::vec3(32.0,32.0,32.0));
        }
        
        auto cube = Cube();
        uint32_t cubeId = meshCache.RegisterMesh(cube);

        for(int i = 0; i<1000; i++) {
            auto entity = world.registry.create();

            world.registry.emplace<Engine::RenderEnabledMarker>(entity);
            auto& instance = world.registry.emplace<Engine::GpuMaterialInstance>(entity);
            instance.materialId = header.id;
            instance.meshId = cubeId;

            auto& transform = world.registry.emplace<Engine::Transform>(entity);
            transform.global = glm::translate(glm::mat4(1.0), glm::vec3(100.0,i*64.0,0.0)) * glm::scale(glm::mat4(1.0),glm::vec3(32.0,32.0,32.0));
        }
    }
};