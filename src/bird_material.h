#pragma once

#include "world.h"
#include "gpu_render.h"
#include "gpu_mesh.h"
#include <memory>
#include "shapes.h"
#include "culling.h"
#include "texture.h"

using Engine::GpuRender, Engine::World, Engine::MaterialHeader, Engine::MaterialRenderComponent, Engine::MaterialImplementation, Engine::MeshCache, Engine::MeshHeader, Engine::GpuMaterialInstance;

struct BirdMaterial {
private:
    float animationOffset;

    class BirdMaterialImplementation : public Engine::InstancedMaterialImplementation<BirdMaterial> {
    private:
        bool doneInitialSetup = false;

        glm::vec3 albedo = glm::vec3(1.0,1.0,1.0);
        float metallic = 0.f;
        float roughness = 0.5f;
        float ao = 1.f;

        Engine::Texture texture = Engine::Texture("textures/BirdUVTexture.jpeg");
    public:
        void Prepare(World& world) override {
            if(!doneInitialSetup) {
                Engine::Shader& shader = MaterialImplementation::passShaders[Engine::RenderPassId::OPAQUE];
                uint32_t albedoLocation = glGetUniformLocation(shader.programId(), "mAlbedo");
                uint32_t metallicLocation = glGetUniformLocation(shader.programId(), "mMetallic");
                uint32_t roughnessLocation = glGetUniformLocation(shader.programId(), "mRoughness");
                uint32_t aoLocation = glGetUniformLocation(shader.programId(), "mAo");

                shader.use();
                glUniform3fv(albedoLocation,1,&albedo[0]);
                glUniform1f(metallicLocation,metallic);
                glUniform1f(roughnessLocation,roughness);
                glUniform1f(aoLocation,ao);

                doneInitialSetup = true;
            }
            
            InstancedMaterialImplementation::Prepare(world);
        }

        void Bind(World& world) override {
            InstancedMaterialImplementation::Bind(world);
            glActiveTexture(GL_TEXTURE0);
            texture.bind();
        }

        BirdMaterialImplementation() : InstancedMaterialImplementation("BirdMaterial") {
            std::vector<const char*> defs = {"#define VERTEX_NORMAL","#define VERTEX_UV"};
            std::vector<const char*> shadow_defs = {"#define SHADOW_PASS"};

            MaterialImplementation::passShaders = {
                { Engine::RenderPassId::OPAQUE, Engine::Shader("shaders/bird.vert","shaders/bird.frag", defs) },
                { Engine::RenderPassId::SHADOW, Engine::Shader("shaders/bird.vert","shaders/shadow.frag", shadow_defs) },
                { Engine::RenderPassId::DIAGNOSTIC, Engine::Shader("shaders/bird.vert","shaders/primitive/wireframe.frag","shaders/primitive/triangle_density.geom", shadow_defs) },
            };
        }
    };
public:
    static void Setup(Engine::World& world) {
        auto& gpuRender = world.GetSingle<GpuRender>();
        auto& meshCache = world.GetSingle<MeshCache>();
        auto headerEntity = world.registry.create();

        auto& header = gpuRender.RegisterMaterial<BirdMaterialImplementation>(world,headerEntity);
        header.SetRenderPass(Engine::RenderPassId::OPAQUE);
        header.SetRenderPass(Engine::RenderPassId::SHADOW);
        header.SetRenderPass(Engine::RenderPassId::DIAGNOSTIC);
    }

    static void InitBirdItem(World& world, entt::entity entity, uint32_t meshId, float animOffset) {
        auto cacheView = world.registry.view<MaterialRenderComponent,MaterialHeader>();
        auto [renderComponent,header] = cacheView.get(cacheView.front());

        auto& mat = world.registry.emplace<GpuMaterialInstance>(entity);
        mat.materialId = header.id;
        mat.meshId = meshId;

        auto& bird = world.registry.emplace<BirdMaterial>(entity);
        bird.animationOffset = animOffset;
    }
};