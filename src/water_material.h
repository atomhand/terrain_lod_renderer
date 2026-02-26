#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glad/gl.h>
#include "world.h"
#include "shader.h"
#include "culling.h"
#include "storage_buffer.h"
#include "light.h"

#include "gpu_render.h"
#include "gpu_mesh.h"
#include "shader_shared.h"

using Engine::World, Engine::Transform, Engine::GpuRender, Engine::MaterialHeader, Engine::MaterialRenderComponent, Engine::MaterialImplementation, Engine::DrawElementsIndirectCommand, Engine::MeshCache, Engine::GpuMeshBuilder;

struct WaterMaterial {
    struct Cache;

    static inline std::vector<glm::uvec2> keys;
    static inline std::vector<uint32_t> drawBaseInstance;
    static inline std::vector<DrawElementsIndirectCommand> drawCommands;
    static inline std::vector<Engine::MaterialHeader> materialHeaders;

    class WaterMaterialImplementation : public MaterialImplementation {
    public:
        Engine::Shader shader = (Engine::Shader("shaders/water.vert", "shaders/water_pbr.frag"));
        Engine::Shader depthOnlyShader = (Engine::Shader("shaders/water.vert","shaders/shadow.frag"));

        void Render(World& world, uint32_t drawOffset, uint32_t drawCount, Engine::RenderPassId pass, std::unordered_map<Engine::RenderPassId,Engine::Shader>& defaultShaders) override {
            if(pass == Engine::RenderPassId::DIAGNOSTIC) {
                MaterialImplementation::Render(world,drawOffset,drawCount,pass,defaultShaders);
                return;
            }

            auto cacheView = world.registry.view<Cache,MaterialHeader>();
            auto [cache,header] = cacheView.get(cacheView.front());

            shader.use();
            
            int texOffset = GL_TEXTURE0;
            glActiveTexture(texOffset++);
            cache.depthTarget.bind();

            for(int i =0; i<cache.textures.size(); i++) {
                glActiveTexture(texOffset++);
                cache.textures[i].bind();
            }

            // draw colour
            glDepthMask(GL_FALSE);
            glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(drawOffset*sizeof(DrawElementsIndirectCommand)), header.id*sizeof(uint32_t), drawCount, 0);

            // draw depth
            glDepthMask(GL_TRUE);
            depthOnlyShader.use();
            glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT,(void*)(drawOffset*sizeof(DrawElementsIndirectCommand)), header.id*sizeof(uint32_t), drawCount, 0);
        }

        WaterMaterialImplementation() : MaterialImplementation("WaterMaterial") {

        };
    };


    static void SetupMesh(GpuMeshBuilder& meshBuilder, float scale, int chunk_size) {
        auto& verts = meshBuilder.verts;
        auto& indices = meshBuilder.indices;

        verts.clear();
        indices.clear();

        int cw = chunk_size;
        assert(cw >= 2);

        // no skirts
        int vw = cw+1;
        int iz;
        for(iz=0; iz<vw; iz++) {
            for(int ix=0; ix<vw; ix++) {
                glm::vec3 pos = glm::vec3(ix / float(cw),0.f,iz / float(cw));
                verts.push_back(pos);
                //normals.push_back(glm::vec3(0,1,0));
            }
        }

        for(iz=0; iz<vw-1; iz++) {            
            for(int ix=0; ix<vw-1; ix++) {
                GLuint i00 = ix + iz*(vw);
                GLuint i10 = (ix+1) + iz*(vw);
                GLuint i01 = ix + (iz+1)*(vw);
                GLuint i11 = (ix+1) + (iz+1)*(vw);

                indices.push_back(i00);
                indices.push_back(i01);
                indices.push_back(i11);

                indices.push_back(i00);
                indices.push_back(i11);
                indices.push_back(i10);
            }
        }

        meshBuilder.aabb = Engine::AABB(glm::vec3(-0.25,-1.5,-0.25),glm::vec3(1.25,1.5,1.25));
    }

public:
    struct Cache {
    public:
        uint32_t meshId;
        
        size_t capacity;

        Engine::Texture depthTarget;

        std::vector<Engine::Texture> textures;

        Cache(uint32_t meshId) :
            meshId(meshId) {
            textures.push_back(Engine::Texture("textures/waterN1.jpg"));
            textures.push_back(Engine::Texture("textures/waterN2.jpg"));
        }

        entt::entity CreateWaterItem(World& world, glm::vec3 pos, glm::vec3 extent) {
            auto cacheView = world.registry.view<Cache,MaterialHeader>();
            auto [cache,header] = cacheView.get(cacheView.front());

            auto entity = world.registry.create();
            world.registry.emplace<WaterMaterial>(entity);
            auto& transform = world.registry.emplace<Transform>(entity);

            transform.global = glm::translate(glm::mat4(1.), pos) * glm::scale(glm::mat4(1.), glm::vec3(extent.x,256.f,extent.z));
            world.registry.emplace<Engine::AABB>(entity, glm::vec3(-0.2,-0.5,-0.2), glm::vec3(1.2,0.5,1.2));

            auto& instance = world.registry.emplace<Engine::GpuMaterialInstance>(entity);
            instance.materialId = header.id;
            instance.meshId = cache.meshId;

            return entity;
        }
    };

    static void Setup(Engine::World& world, float scale, int chunk_size) {
        GpuMeshBuilder builder;
        SetupMesh(builder, scale, chunk_size/4);

        auto& meshCache = world.GetSingle<MeshCache>();
        uint32_t meshId = meshCache.RegisterMesh(builder);

        auto& gpuRender = world.GetSingle<GpuRender>();
        auto headerEntity = world.registry.create();
        world.registry.emplace<Cache>(headerEntity, meshId);
        
        auto& materialHeader = gpuRender.RegisterMaterial<WaterMaterialImplementation>(world, headerEntity);
        materialHeader.SetRenderPass(Engine::RenderPassId::POST_OPAQUE);
        materialHeader.SetRenderPass(Engine::RenderPassId::DIAGNOSTIC);
    }

    static void PrepareMain(Engine::World& world, Engine::Texture depthTexture) {
        auto cacheView = world.registry.view<WaterMaterial::Cache,MaterialHeader>();
        auto [cache,header] = cacheView.get(cacheView.front());

        cache.depthTarget = depthTexture;
    }
};