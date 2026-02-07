#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glad/gl.h>
#include "mesh.h"
#include "world.h"
#include "shader.h"
#include "culling.h"
#include "storage_buffer.h"
#include "light.h"

#include "gpu_render.h"
#include "shader_shared.h"

using Engine::World, Engine::Mesh, Engine::Transform, Engine::GpuRender, Engine::MaterialHeader, Engine::MaterialRenderComponent, Engine::MaterialRenderPass, Engine::DrawElementsIndirectCommand;

struct WaterMaterial {
    struct Cache;

    class WaterRenderPass : MaterialRenderPass {
        void Render(World& world, uint32_t drawOffset, uint32_t drawCount, uint8_t pass) override {
            auto& cache = world.GetSingle<Cache>();
            auto& gpuRender = world.GetSingle<GpuRender>();
            // Uniforms that should always be set
            // - Material header index
            // Buffers
            // - DrawBaseInstance
            // - FilteredKeys

            gpuRender.inputKeysBuffer.BindBase(0);
            gpuRender.materialHeadersBuffer.BindBase(1);
            gpuRender.drawBaseInstanceBuffer.BindBase(2);
            gpuRender.renderItemBuffer.BindBase(3);

            // Bind material specific datacache.shader.use();
            cache.shader.use();
            glUniform1f(cache.timeOffset,world.shaderAnimTime);
            glUniform1i(cache.idOffset,cache.materialId);
            
            int texOffset = GL_TEXTURE0;
            glActiveTexture(texOffset++);
            cache.depthTarget.bind();

            for(int i =0; i<cache.textures.size(); i++) {
                glActiveTexture(texOffset++);
                cache.textures[i].bind();
            }

            glBindVertexArray(cache.mesh.vao());

            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, gpuRender.drawCmdsBuffer.object());

            // draw colour
            glDepthMask(GL_FALSE);
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(drawOffset*sizeof(DrawElementsIndirectCommand)), drawCount, 0);

            // draw depth
            glDepthMask(GL_TRUE);
            cache.depthOnlyShader.use();
            glUniform1i(cache.depthOnlyIdOffset,cache.materialId);
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(drawOffset*sizeof(DrawElementsIndirectCommand)), drawCount, 0);

            glBindVertexArray(0);
        }
    };


public:
    struct Cache {
    public:
        int meshId;
        int materialId = -1;
        
        size_t capacity;
        Engine::Mesh mesh;
        Engine::Shader shader;
        Engine::Shader depthOnlyShader;
        Engine::Shader shadowShader;
        Engine::StorageBuffer storage;
        std::vector<glm::mat4> transforms;

        Engine::Texture depthTarget;

        int timeOffset;
        int idOffset;
        int depthOnlyIdOffset;

        std::vector<Engine::Texture> textures;

        Cache(size_t capacity, Engine::Mesh mesh, int meshId) :
            meshId(meshId), 
            capacity(capacity),
            mesh(mesh),
            shader(Engine::Shader("shaders/water.vert", "shaders/water_pbr.frag")),
            depthOnlyShader(Engine::Shader("shaders/water.vert","shaders/shadow.frag")),
            shadowShader(Engine::Shader("shaders/water.vert","shaders/shadow.frag","shaders/shadow_cascade.geom")),
            storage(capacity * sizeof(glm::mat4)) {
            transforms.reserve(capacity);

            timeOffset = glGetUniformLocation(shader.programId(), "time");
            idOffset = glGetUniformLocation(shader.programId(), "materialId");
            depthOnlyIdOffset = glGetUniformLocation(depthOnlyShader.programId(), "materialId");

            textures.push_back(Engine::Texture("textures/waterN1.jpg"));
            textures.push_back(Engine::Texture("textures/waterN2.jpg"));
        }

        entt::entity CreateWaterItem(World& world, glm::vec3 pos, glm::vec3 extent) {
            if(materialId < 0) return entt::null;

            auto water_entity = world.registry.create();
            auto& waterItem = world.registry.emplace<WaterMaterial>(water_entity);
            auto& transform = world.registry.emplace<Transform>(water_entity);

            auto& instance = world.registry.emplace<Engine::GpuMaterialInstance>(water_entity);
            instance.materialId = materialId;
            instance.meshId = meshId;

            transform.global = glm::translate(glm::mat4(1.), pos) * glm::scale(glm::mat4(1.), glm::vec3(extent.x,256.f,extent.z));
            world.registry.emplace<Engine::AABB>(water_entity, glm::vec3(-0.2,-0.5,-0.2), glm::vec3(1.2,0.5,1.2));
            return water_entity;
        }
    };

    static void Setup(Engine::World& world, size_t capacity, Engine::Mesh mesh) {
        auto& gpuRender = world.GetSingle<GpuRender>();
        auto headerEntity = world.registry.create();
        world.registry.emplace<Cache>(headerEntity, capacity, mesh, gpuRender.RegisterMesh(mesh.GetVertCount(0)));

        world.registry.emplace<MaterialRenderComponent>(headerEntity, (MaterialRenderPass*)new WaterRenderPass());
    }

    bool enabled = true;

    static void PrepareMain(Engine::World& world, Engine::Texture depthTexture) {

        auto cacheView = world.registry.view<WaterMaterial::Cache,MaterialHeader>();
        auto [cache,header] = cacheView.get(cacheView.front());

        cache.depthTarget = depthTexture;
        cache.materialId = header.id;
        header.drawCount = 1;
    }

    static void DrawMain(Engine::World& world, Engine::Texture depthTexture) {
        auto& cache = world.GetSingle<WaterMaterial::Cache>();

        // Prepare transforms        
        cache.transforms.clear();
        auto view = world.registry.view<WaterMaterial,Transform,Engine::CullingResult>();
        for(auto entity : view) {
            auto [mat,transform,cullingResult] = view.get(entity);

            if(mat.enabled && cullingResult.viewResult) {
                cache.transforms.push_back(transform.global);
            }
        }

        // There should never be more water chunks than the SSBO can support
        assert(cache.transforms.size() <= cache.capacity);
        cache.storage.SetBytes((void*)cache.transforms.data(), cache.transforms.size()*sizeof(glm::mat4), 0);

        // Bind shader

        cache.shader.use();
        glUniform1f(cache.timeOffset,world.shaderAnimTime);
        
        int offset = GL_TEXTURE0;
        glActiveTexture(offset++);
        depthTexture.bind();

        for(int i =0; i<cache.textures.size(); i++) {
            glActiveTexture(offset++);
            cache.textures[i].bind();
        }

        cache.storage.BindBase(0);

        // Draw
        cache.shader.use();
        cache.mesh.DrawInstanced(cache.transforms.size());
    }

    static void DrawDepth(Engine::World& world) {
        auto& cache = world.GetSingle<WaterMaterial::Cache>();

        // Draw
        cache.depthOnlyShader.use();
        cache.mesh.DrawInstanced(cache.transforms.size());
    }
    
    // NOTE - water doesnt actually cast shadow..
    static void DrawShadow(Engine::World& world) {
        auto& cache = world.GetSingle<WaterMaterial::Cache>();

        // Prepare transforms
        cache.transforms.clear();
        auto view = world.registry.view<WaterMaterial,Transform,Engine::SurvivedLightCullingTag>();
        for(auto entity : view) {
            auto [mat,transform] = view.get(entity);

            if(mat.enabled) {
                cache.transforms.push_back(transform.global);
            }
        }

        // Draw
        cache.storage.BindBase(0);
        cache.shadowShader.use();
        cache.mesh.DrawInstanced(cache.transforms.size());
    }
};