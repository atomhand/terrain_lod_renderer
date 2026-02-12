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

#include "imgui.h"

using Engine::World, Engine::Mesh, Engine::Transform, Engine::GpuRender, Engine::MaterialHeader, Engine::MaterialRenderComponent, Engine::MaterialRenderPass, Engine::DrawElementsIndirectCommand, Engine::MeshCache, Engine::GpuMeshBuilder;

struct WaterMaterial {
    struct Cache;

    static inline std::vector<glm::uvec2> keys;
    static inline std::vector<uint32_t> drawBaseInstance;
    static inline std::vector<DrawElementsIndirectCommand> drawCommands;
    static inline std::vector<Engine::MaterialHeader> materialHeaders;

    static void DebugUi(World& world) {
        auto& gpuRender = world.GetSingle<GpuRender>();
        auto& meshCache = world.GetSingle<MeshCache>();
        
        uint32_t keysSize = gpuRender.numRenderItems;
        keys.resize(keysSize);
        gpuRender.inputKeysBuffer.Readback<glm::uvec2>(keys.data(), keysSize, 0);

        uint32_t drawsSize = gpuRender.numDraws;
        drawCommands.resize(drawsSize);
        gpuRender.drawCmdsBuffer.Readback<DrawElementsIndirectCommand>(drawCommands.data(), drawsSize, 0);

        drawBaseInstance.resize(drawsSize);
        gpuRender.drawBaseInstanceBuffer.Readback<uint32_t>(drawBaseInstance.data(), drawsSize, 0);

        materialHeaders.resize(gpuRender.numMaterials);
        gpuRender.materialHeadersBuffer.Readback<MaterialHeader>(materialHeaders.data(), materialHeaders.size(), 0);

        uint32_t numFilteredDraws;
        gpuRender.drawCounterBuffer.Readback<uint32_t>(&numFilteredDraws,1,0);

        if(ImGui::Begin("WaterMaterial tester")) {
            if(ImGui::CollapsingHeader("Overview")) {
                ImGui::Text("Num meshes: %u", meshCache.meshHeaders.size());
                ImGui::Text("Num indices: %u", meshCache.indexHead);
                ImGui::Text("Num attribute values: %u", meshCache.attributesHead);
            }


            if(ImGui::CollapsingHeader("Keys")) {
                if(ImGui::BeginTable("valuesTable", 3)) {
                    ImGui::TableSetupColumn("i", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("inputkey", ImGuiTableColumnFlags_WidthStretch);       
                    ImGui::TableSetupColumn("outputkey", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("i");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("keyIn");
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("keyOut");

                    int n = std::min(gpuRender.numRenderItems, (uint32_t)5000);
                    for(int i =0; i<n; i++) {               
                        ImGui::TableNextRow();               
                        ImGui::TableSetColumnIndex(0);                
                        ImGui::Text("%u", i);

                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%u (%u)", gpuRender.materialKeys[i].x, gpuRender.materialKeys[i].y);

                        ImGui::TableSetColumnIndex(2);                
                        ImGui::Text("%u (%u)", keys[i].x, keys[i].y);
                    }

                    ImGui::EndTable();
                }
            }

            if(ImGui::CollapsingHeader("MaterialHeaders")) {
                for(int i =0; i<materialHeaders.size(); i++) {
                    ImGui::Text("ID %u", materialHeaders[i].id);
                    ImGui::Text("DrawBufferOffset %u", materialHeaders[i].drawBufferOffset);
                    ImGui::Text("DrawCount %u", materialHeaders[i].drawCount);
                    ImGui::Text("filteredDrawBufferOffset %u", materialHeaders[i].drawKeyOffset);
                }
            }

            if(ImGui::CollapsingHeader("MeshHeaders")) {
                for(auto& header : meshCache.meshHeaders) {
                    ImGui::Text("ID %u", header.id);

                    ImGui::Text("Count %u", header.count);
                    ImGui::Text("firstIndex %u", header.firstIndex);
                    ImGui::Text("baseVertex %u", header.baseVertex);
                    ImGui::Text("Stride %u", header.stride);

                    ImGui::Text("AABB min (%f,%f,%f)", header.aabbMin.x,header.aabbMin.y,header.aabbMin.z);
                    ImGui::Text("AABB max (%f,%f,%f)", header.aabbMax.x,header.aabbMax.y,header.aabbMax.z);
                }
            }

            if(ImGui::CollapsingHeader("DrawBaseInstance")) {
                
                ImGui::Text("numDraws %u", gpuRender.numDraws);
                ImGui::Text("numFilteredDraws %u", numFilteredDraws);
                ImGui::Separator();

                for(int i =0; i<drawBaseInstance.size(); i++) {
                    ImGui::Text("[%i] %u", i, drawBaseInstance[i]);
                }
            }

            if(ImGui::CollapsingHeader("DrawCmds")) {
                for(int i =0; i<drawCommands.size(); i++) {
                    ImGui::Text("Draw Command");
                    ImGui::Text("vertex count % u", drawCommands[i].count);
                    ImGui::Text("instanceCount % u", drawCommands[i].instanceCount);
                    ImGui::Text("firstIndex % u", drawCommands[i].firstIndex);
                    ImGui::Text("baseVertex % u", drawCommands[i].baseVertex);
                    ImGui::Text("baseInstance % u", drawCommands[i].baseInstance);
                }
            }
        }

        ImGui::End();
    }

    class WaterRenderPass : MaterialRenderPass {
        void Render(World& world, uint32_t drawOffset, uint32_t drawCount, Engine::RenderPassId pass) override {
            auto cacheView = world.registry.view<Cache,MaterialHeader>();
            auto [cache,header] = cacheView.get(cacheView.front());

            /*
            if(pass == Engine::RenderPassId::OPAQUE) {                
                DebugUi(world);
            }
            */

            // Bind material specific datacache.shader.use();
            cache.shader.use();
            glUniform1f(cache.timeOffset,world.shaderAnimTime);
            glUniform1i(cache.idOffset,header.id);
            
            int texOffset = GL_TEXTURE0;
            glActiveTexture(texOffset++);
            cache.depthTarget.bind();

            for(int i =0; i<cache.textures.size(); i++) {
                glActiveTexture(texOffset++);
                cache.textures[i].bind();
            }

            // draw colour
            glDepthMask(GL_FALSE);
            glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(drawOffset*sizeof(DrawElementsIndirectCommand)), header.drawBufferOffset*sizeof(uint32_t), drawCount, 0);

            // draw depth
            glDepthMask(GL_TRUE);
            cache.depthOnlyShader.use();
            glUniform1i(cache.depthOnlyIdOffset,header.id);
            glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT,(void*)(drawOffset*sizeof(DrawElementsIndirectCommand)), header.drawBufferOffset*sizeof(uint32_t), drawCount, 0);
        }
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

        Cache(size_t capacity, uint32_t meshId) :
            meshId(meshId), 
            capacity(capacity),
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
            auto cacheView = world.registry.view<Cache,MaterialHeader>();
            auto [cache,header] = cacheView.get(cacheView.front());

            auto entity = world.registry.create();
            auto& waterItem = world.registry.emplace<WaterMaterial>(entity);
            auto& transform = world.registry.emplace<Transform>(entity);

            transform.global = glm::translate(glm::mat4(1.), pos) * glm::scale(glm::mat4(1.), glm::vec3(extent.x,256.f,extent.z));
            world.registry.emplace<Engine::AABB>(entity, glm::vec3(-0.2,-0.5,-0.2), glm::vec3(1.2,0.5,1.2));

            auto& instance = world.registry.emplace<Engine::GpuMaterialInstance>(entity);
            instance.materialId = header.id;
            instance.meshId = cache.meshId;

            return entity;
        }
    };

    static void Setup(Engine::World& world, size_t capacity, float scale, int chunk_size) {
        GpuMeshBuilder builder;
        SetupMesh(builder, scale, chunk_size);

        auto& meshCache = world.GetSingle<MeshCache>();
        uint32_t meshId = meshCache.RegisterMesh(builder);

        auto& gpuRender = world.GetSingle<GpuRender>();
        auto headerEntity = world.registry.create();
        world.registry.emplace<Cache>(headerEntity, capacity, meshId);
        
        auto& materialHeader = gpuRender.RegisterMaterial(world, headerEntity);
        materialHeader.SetRenderPass(Engine::RenderPassId::OPAQUE);

        world.registry.emplace<MaterialRenderComponent>(headerEntity, (MaterialRenderPass*)new WaterRenderPass());
    }

    bool enabled = true;

    static void PrepareMain(Engine::World& world, Engine::Texture depthTexture) {
        auto cacheView = world.registry.view<WaterMaterial::Cache,MaterialHeader>();
        auto [cache,header] = cacheView.get(cacheView.front());

        cache.depthTarget = depthTexture;
    }
};