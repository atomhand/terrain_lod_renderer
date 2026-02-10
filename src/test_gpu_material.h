#pragma once

#include "world.h"
#include "gpu_render.h"
#include "gpu_mesh.h"
#include <memory>
#include "shapes.h"
#include "culling.h"

using Engine::GpuRender, Engine::World, Engine::MaterialHeader, Engine::MaterialRenderComponent, Engine::MaterialRenderPass, Engine::MeshCache, Engine::MeshHeader;

struct TestGpuMaterial{
private:
    class TestGpuMaterialManager {
    public:
        std::vector<const char*> defs = {"#define VERTEX_NORMAL","#define VERTEX_UV"};
        Engine::Shader shader = Engine::Shader("shaders/pbr.vert","shaders/pbr.frag", defs);
        Engine::Shader shadowShader = Engine::Shader("shaders/gpu_shadow.vert","shaders/shadow.frag", defs);

        uint32_t idLocation;
        uint32_t shadowIdLocation;

        TestGpuMaterialManager() {            
            idLocation = glGetUniformLocation(shader.programId(), "materialId");
            shadowIdLocation = glGetUniformLocation(shadowShader.programId(), "materialId");
        }
    };

    class TestRenderPass : MaterialRenderPass {
        void Render(World& world, uint32_t drawOffset, uint32_t drawCount, Engine::RenderPassId pass) override {
            auto view = world.registry.view<TestGpuMaterialManager,MaterialHeader>();
            for(auto entity : view) {
                auto [manager,header] = view.get(entity);

                if(pass == Engine::RenderPassId::SHADOW) {                  
                    manager.shadowShader.use();
                    glUniform1i(manager.shadowIdLocation,header.id);
                    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(drawOffset*sizeof(Engine::DrawElementsIndirectCommand)), drawCount, 0);
                } else {
                    manager.shader.use();
                    glUniform1i(manager.idLocation,header.id);
                    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(drawOffset*sizeof(Engine::DrawElementsIndirectCommand)), drawCount, 0);
                }
            }
        }
    };
public:
    static void Setup(Engine::World& world) {
        auto& gpuRender = world.GetSingle<GpuRender>();
        auto& meshCache = world.GetSingle<MeshCache>();
        auto headerEntity = world.registry.create();

        world.registry.emplace<MaterialRenderComponent>(headerEntity, (MaterialRenderPass*)new TestRenderPass());
        world.registry.emplace<TestGpuMaterialManager>(headerEntity);

        auto& header = gpuRender.RegisterMaterial(world,headerEntity);
        header.SetRenderPass(Engine::RenderPassId::OPAQUE);
        header.SetRenderPass(Engine::RenderPassId::SHADOW);

        // sphere
        auto sphere = Sphere(16,16);
        uint32_t sphereId = meshCache.RegisterMesh(sphere);

        
        for(int i = 0; i<1000; i++) {
            auto entity = world.registry.create();

            world.registry.emplace<Engine::CullingResult>(entity);
            world.registry.emplace<Engine::AABB>(entity, sphere.aabb);
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

            world.registry.emplace<Engine::CullingResult>(entity);
            world.registry.emplace<Engine::AABB>(entity, cube.aabb);
            auto& instance = world.registry.emplace<Engine::GpuMaterialInstance>(entity);
            instance.materialId = header.id;
            instance.meshId = cubeId;

            auto& transform = world.registry.emplace<Engine::Transform>(entity);
            transform.global = glm::translate(glm::mat4(1.0), glm::vec3(100.0,i*64.0,0.0)) * glm::scale(glm::mat4(1.0),glm::vec3(32.0,32.0,32.0));
        }
    }
};