#pragma once

#include "world.h"
#include "gpu_render.h"
#include <memory>

using Engine::GpuRender, Engine::World, Engine::MaterialHeader, Engine::MaterialRenderComponent, Engine::MaterialRenderPass;

class TestGpuMaterialManager {
    struct TestGpuMaterial{};

    class TestRenderPass : MaterialRenderPass {
        void Render(GpuRender& gpuRender, uint32_t offset, uint32_t count, uint8_t pass) override {

        }
    };

    int meshId;
public:
    static void Setup(Engine::World& world) {
        auto& gpuRender = world.GetSingle<GpuRender>();
        auto entity = world.registry.create();

        world.registry.emplace<MaterialRenderComponent>(entity, std::make_unique<TestRenderPass>());

        auto& testGpuMaterial = world.registry.emplace<TestGpuMaterialManager>(entity);
        testGpuMaterial.meshId = gpuRender.RegisterMesh(6);
    }


};