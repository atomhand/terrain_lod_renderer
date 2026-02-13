#pragma once

#include "world.h"
#include "gpu_render.h"
#include "gpu_mesh.h"
#include <memory>
#include "shapes.h"
#include "culling.h"
#include "texture.h"

using Engine::GpuRender, Engine::World, Engine::MaterialHeader, Engine::MaterialRenderComponent, Engine::MaterialRenderPass, Engine::MeshCache, Engine::MeshHeader, Engine::GpuMaterialInstance;

struct BirdMaterial {
private:
    float animationOffset;

    class BirdMaterialManager {
    public:
        uint32_t topIdx = 0;

        glm::vec3 albedo = glm::vec3(1.0,1.0,1.0);
        float metallic = 0.f;
        float roughness = 0.5f;
        float ao = 1.f;

        GLuint albedoLocation;
        GLuint metallicLocation;
        GLuint roughnessLocation;
        GLuint aoLocation;

        std::vector<BirdMaterial> instanceData;
        Engine::StorageBuffer instanceDataBuffer = Engine::StorageBuffer(0);

        std::vector<const char*> defs = {"#define VERTEX_NORMAL","#define VERTEX_UV"};
        Engine::Shader shader = Engine::Shader("shaders/bird.vert","shaders/bird.frag", defs);


        std::vector<const char*> shadow_defs = {"#define SHADOW_PASS"};
        Engine::Shader wireframeShader = Engine::Shader("shaders/bird.vert","shaders/primitive/wireframe.frag","shaders/primitive/triangle_density.geom", shadow_defs);
        Engine::Shader triangleDensityShader = Engine::Shader("shaders/bird.vert","shaders/primitive/basic.frag","shaders/primitive/triangle_density.geom", shadow_defs);

        Engine::Shader shadowShader = Engine::Shader("shaders/bird.vert","shaders/shadow.frag", shadow_defs);

        Engine::Texture texture = Engine::Texture("textures/BirdUVTexture.jpeg");

        BirdMaterialManager() {
            albedoLocation = glGetUniformLocation(shader.programId(), "mAlbedo");
            metallicLocation = glGetUniformLocation(shader.programId(), "mMetallic");
            roughnessLocation = glGetUniformLocation(shader.programId(), "mRoughness");
            aoLocation = glGetUniformLocation(shader.programId(), "mAo");
        }
    };

    class BirdMaterialRenderPass : MaterialRenderPass {
        void Prepare(World& world) override {
            auto& manager = world.GetSingle<BirdMaterialManager>();

            auto view = world.registry.view<BirdMaterial,GpuMaterialInstance>();
            for(auto entity : view) {
                auto [birdMat,gpuInstance] = view.get(entity);

                if(manager.instanceData.size() <= gpuInstance.materialInstanceId) {
                    manager.instanceData.resize(gpuInstance.materialInstanceId+1);
                }
                manager.instanceData[gpuInstance.materialInstanceId] = birdMat;
            }

            manager.instanceDataBuffer.Set<BirdMaterial>(manager.instanceData.data(), manager.instanceData.size(), 0, true);
        }

        void RenderWireframe(World& world, uint32_t drawOffset, uint32_t drawCount, Engine::RenderPassId pass) override {
            auto view = world.registry.view<BirdMaterialManager,MaterialHeader>();
            for(auto entity : view) {
                auto [manager,header] = view.get(entity);
                manager.wireframeShader.use();
                manager.instanceDataBuffer.BindBase(6);
                glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(drawOffset*sizeof(DrawElementsIndirectCommand)), header.id*sizeof(uint32_t), drawCount, 0);
            }
        }

        void RenderTriangleDensity(World& world, uint32_t drawOffset, uint32_t drawCount, Engine::RenderPassId pass) override {
            auto view = world.registry.view<BirdMaterialManager,MaterialHeader>();
            for(auto entity : view) {
                auto [manager,header] = view.get(entity);
                manager.triangleDensityShader.use();
                manager.instanceDataBuffer.BindBase(6);
                glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(drawOffset*sizeof(DrawElementsIndirectCommand)), header.id*sizeof(uint32_t), drawCount, 0);
            }
        }

        void Render(World& world, uint32_t drawOffset, uint32_t drawCount, Engine::RenderPassId pass) override {
            auto view = world.registry.view<BirdMaterialManager,MaterialHeader>();
            for(auto entity : view) {
                auto [manager,header] = view.get(entity);

                if(pass == Engine::RenderPassId::SHADOW) {                  
                    manager.shadowShader.use();
                    manager.instanceDataBuffer.BindBase(6);

                    glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(drawOffset*sizeof(DrawElementsIndirectCommand)), header.id*sizeof(uint32_t), drawCount, 0);
                } else {
                    manager.shader.use();
                    manager.instanceDataBuffer.BindBase(6);
                    glActiveTexture(GL_TEXTURE0);
                    manager.texture.bind();

                    // Need to work out how to avoid needing to bind these
                    glUniform3fv(manager.albedoLocation,1,&manager.albedo[0]);
                    glUniform1f(manager.metallicLocation,manager.metallic);
                    glUniform1f(manager.roughnessLocation,manager.roughness);
                    glUniform1f(manager.aoLocation,manager.ao);

                    glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(drawOffset*sizeof(DrawElementsIndirectCommand)), header.id*sizeof(uint32_t), drawCount, 0);
                }
            }
        }
    };
public:
    static void Setup(Engine::World& world) {
        auto& gpuRender = world.GetSingle<GpuRender>();
        auto& meshCache = world.GetSingle<MeshCache>();
        auto headerEntity = world.registry.create();

        world.registry.emplace<MaterialRenderComponent>(headerEntity, (MaterialRenderPass*)new BirdMaterialRenderPass());
        world.registry.emplace<BirdMaterialManager>(headerEntity);

        auto& header = gpuRender.RegisterMaterial(world,headerEntity);
        header.SetRenderPass(Engine::RenderPassId::OPAQUE);
        header.SetRenderPass(Engine::RenderPassId::SHADOW);
    }

    static void InitBirdItem(World& world, entt::entity entity, uint32_t meshId, float animOffset) {
        auto cacheView = world.registry.view<BirdMaterialManager,MaterialHeader>();
        auto [manager,header] = cacheView.get(cacheView.front());

        auto& mat = world.registry.emplace<GpuMaterialInstance>(entity);
        mat.materialId = header.id;
        mat.materialInstanceId = manager.topIdx++;
        mat.meshId = meshId;

        auto& bird = world.registry.emplace<BirdMaterial>(entity);
        bird.animationOffset = animOffset;
    }
};