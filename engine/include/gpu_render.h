#pragma once

#include "world.h"
#include "culling.h"
#include "compute_shader.h"
#include "shader.h"
#include "storage_buffer.h"
#include "gpu_sort.h"
#include "gpu_filter.h"
#include "shader_shared.h"
#include <memory>
#include "gpu_mesh.h"

namespace Engine {
    class GpuRender;

    class MaterialRenderPass {
    public:
        virtual void Render(World& world, uint32_t offset, uint32_t count, uint8_t pass) {            
            // Uniforms that should always be set
            // - Material header index
            // Buffers
            // - DrawBaseInstance
            // - FilteredKeys

            // Bind material specific data

            glMultiDrawElementsIndirect(GL_TRIANGLES,  GL_UNSIGNED_INT, (void*)(offset*sizeof(DrawElementsIndirectCommand)), count, 0);
        }
    };

    struct MaterialRenderComponent {
    public:
        std::unique_ptr<MaterialRenderPass> renderPass;

        MaterialRenderComponent(MaterialRenderPass* renderPass) : renderPass(renderPass) {};
    };

    struct RenderItemData {
        glm::mat4 model;
        glm::vec3 aabbMin;
        float pad;
        glm::vec3 aabbMax;
        float pad2;
    };

    struct GpuMaterialInstance {
        uint32_t materialId;
        uint32_t meshId;
        //uint16_t idInBatch;
    };

    // Same struct used on CPU and GPU side
    struct MaterialHeader {
        uint32_t id;
        uint32_t drawBufferOffset;
        uint32_t drawCount;
        uint32_t drawKeyOffset; // not set on  CPU side

        bool operator < (MaterialHeader const& rhs) {
            return this->id < rhs.id;
        }
    };

    class GpuRender {
private:
public:
        GpuSort gpuSorter;

        GpuFilter baseInstanceFilter = GpuFilter("shaders/corepass/draw_base_instance.cs");

        std::vector<RenderItemData> renderItemData;
        std::vector<glm::uvec2> materialKeys;
        std::vector<MaterialHeader> materialHeaders;

        std::vector<entt::entity> materialEntities;

        // Render item keys
        StorageBuffer inputKeysBuffer = StorageBuffer(0);
        StorageBuffer passCulledKeysBuffer = StorageBuffer(0);

        // Render item associated data
        StorageBuffer renderItemBuffer = StorageBuffer(0);

        // Dispatch management buffers
        StorageBuffer indirectDispatchParamsBuffer = StorageBuffer(sizeof(unsigned int)*3);

        StorageBuffer drawCounterBuffer = StorageBuffer(4);
        StorageBuffer keyCounterBuffer = StorageBuffer(4);

        // Per-draw data
        StorageBuffer drawBaseInstanceBuffer = StorageBuffer(0);
        StorageBuffer drawCmdsBuffer = StorageBuffer(0);

        // Material and mesh headers
        StorageBuffer materialHeadersBuffer = StorageBuffer(0);
        StorageBuffer meshHeadersBuffer = StorageBuffer(0);

        //ComputeShader cullingShader;

        ComputeShader indirectGroupsFromCounterShader = ComputeShader("shaders/corepass/indirect_params_from_counter.cs");
        ComputeShader emitDrawCommandsShader = ComputeShader("shaders/corepass/emit_draw_commands.cs");

        uint32_t numDraws = 0;
        uint32_t numRenderItems = 0;

        static unsigned int PackKey(uint32_t materialId, uint32_t meshId) {
            assert(materialId <= 0x3fffu);
            assert(meshId <= 0x3ffffu);
            return (materialId << 18) | meshId;
        }

        void RegisterNewMaterials(Engine::World& world) {
            
            auto materialHeadersView = world.registry.view<MaterialRenderComponent>(entt::exclude<MaterialHeader>);
            for(auto entity : materialHeadersView) {
                auto& header = world.registry.emplace<MaterialHeader>(entity);
                header.id = materialHeaders.size();

                materialHeaders.push_back(header);
                materialEntities.push_back(entity);
            }
        }

        static void Init(Engine::World& world) {
            auto entity = world.registry.create();
            auto& gpuRender = world.registry.emplace<GpuRender>(entity);
        }

        static void PrePrepare(Engine::World& world) {
            auto& gpuRender = world.GetSingle<GpuRender>();
            gpuRender.RegisterNewMaterials(world);
        }

        static void PrepareGpuScene(Engine::World& world) {      
            auto& gpuRender = world.GetSingle<GpuRender>();
            auto& meshCache = world.GetSingle<MeshCache>();
            meshCache.FlushStagingBuffer();

            // Gather material headers
            for(int i =0; i<gpuRender.materialEntities.size(); i++) {
                gpuRender.materialHeaders[i] = world.registry.get<MaterialHeader>(gpuRender.materialEntities[i]);
            }

            // TODO - Counting unique draw cmds per material
            uint32_t currentDrawCmdOffset = 0;
            for(auto& materialHeader : gpuRender.materialHeaders) {
                materialHeader.drawBufferOffset = currentDrawCmdOffset;
                currentDrawCmdOffset += materialHeader.drawCount;
            }
            gpuRender.numDraws = currentDrawCmdOffset;

            gpuRender.drawCmdsBuffer.SmartResizeBytes(gpuRender.numDraws * sizeof(DrawElementsIndirectCommand));
            

            // Set gpu side buffer
            gpuRender.materialHeadersBuffer.Set<MaterialHeader>(gpuRender.materialHeaders.data(), gpuRender.materialHeaders.size(), 0, true);
            gpuRender.meshHeadersBuffer.Set<MeshHeader>(meshCache.meshHeaders.data(), meshCache.meshHeaders.size(), 0, true);

            // EmitDrawCommands needs total draw count as a uniform (maybe use a uniform buffer and share it between shaders?)
            gpuRender.emitDrawCommandsShader.use();
            gpuRender.emitDrawCommandsShader.setInt("numDraws", gpuRender.numDraws);

            // Partially set up draw commands

            // Gather render items
            gpuRender.materialKeys.clear();
            gpuRender.renderItemData.clear();
            auto itemsView = world.registry.view<Transform,AABB,GpuMaterialInstance,Engine::CullingResult>();
            for(auto entity : itemsView) {
                auto [transform,aabb,material,cc] = itemsView.get(entity);

                // Key 
                gpuRender.materialKeys.push_back(glm::uvec2(PackKey(material.materialId, material.meshId), gpuRender.renderItemData.size()));
                //gpuRender.renderItemData.push_back(RenderItemData { transform.global, aabb});
                gpuRender.renderItemData.push_back(RenderItemData { transform.global});
            };
            gpuRender.numRenderItems = gpuRender.materialKeys.size();

            gpuRender.drawBaseInstanceBuffer.SmartResizeBytes(gpuRender.numDraws*sizeof(unsigned int));

            gpuRender.inputKeysBuffer.Set<glm::uvec2>(gpuRender.materialKeys.data(), gpuRender.materialKeys.size(), 0, true);
            gpuRender.passCulledKeysBuffer.SmartResizeBytes(gpuRender.materialKeys.size() * sizeof(glm::uvec2));
            gpuRender.renderItemBuffer.Set<RenderItemData>(gpuRender.renderItemData.data(), gpuRender.renderItemData.size(), 0, true);

            //   Sort keys
            //auto gpuSorter = world.GetSingle<GpuSort>();
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            // can  use passCulledKeys as scratch buffer because it's not holding anything important right now
            // Potential improvement - replace this with a global pool for temp buffers
            gpuRender.gpuSorter.SortInPlacePaired(gpuRender.inputKeysBuffer, gpuRender.passCulledKeysBuffer, gpuRender.numRenderItems);
        }
        
        static void PreparePass(Engine::World& world, Engine::Camera& camera, uint8_t passId) {
            auto& gpuRender = world.GetSingle<GpuRender>();
            
            // TODO: CULL according to the VP and the pass id
            // write a filtered buffer to passCulledKeysBuffer
            // (skipping culling for initial implementation)

            gpuRender.keyCounterBuffer.Set<uint32_t>(&gpuRender.numRenderItems, 1);

            // Clear draw commands
            glClearNamedBufferData(gpuRender.drawCmdsBuffer.object(), GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);

            // TODO filter should support an indirect dispatch
            gpuRender.materialHeadersBuffer.BindBase(5);
            gpuRender.baseInstanceFilter.Filter(gpuRender.inputKeysBuffer, gpuRender.drawBaseInstanceBuffer, gpuRender.numRenderItems, gpuRender.drawCounterBuffer);

            // Interpret counter to get indirect dispatch params
            // (assumes workgroup layout (256,1,1))

            /*
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            gpuRender.drawCounterBuffer.BindBase(0);
            gpuRender.indirectDispatchParamsBuffer.BindBase(1);
            gpuRender.indirectGroupsFromCounterShader.Dispatch(1,1,1);
            */

            gpuRender.inputKeysBuffer.BindBase(0);
            gpuRender.keyCounterBuffer.BindBase(1);
            gpuRender.materialHeadersBuffer.BindBase(2);
            gpuRender.drawBaseInstanceBuffer.BindBase(3);
            gpuRender.drawCmdsBuffer.BindBase(4);

            // Emit draw commands
            // counterBuffer, materialHeaderBuffer, drawBaseInstanceBuffer are already bound to correct positions

            gpuRender.meshHeadersBuffer.BindBase(5);
            gpuRender.drawCounterBuffer.BindBase(6);
            gpuRender.emitDrawCommandsShader.use();
            // depends on previous kernel output so barrier is required
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            //glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, gpuRender.indirectDispatchParamsBuffer.object());

            glDispatchCompute((gpuRender.numDraws+255)/256,1,1);

            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ELEMENT_ARRAY_BARRIER_BIT);
            // Iterate materials, bind and draw
            auto materialView = world.registry.view<MaterialHeader,MaterialRenderComponent>();
            for(auto entity : materialView) {
                auto [materialHeader,materialRenderComponent] = materialView.get<MaterialHeader,MaterialRenderComponent>(entity);
                materialRenderComponent.renderPass->Render(world, materialHeader.drawBufferOffset, materialHeader.drawCount, passId);
            }
        }

        // Overview

        // Culling uses a 32 bit key
        // most significant bit - Cull result
        // MSbits 2-16 - Instance Group ID (up to 32768 draws)
        // Bits 17-32 - Instance ID within group (up to 65536 objects per drawcall)

        // Process
        // 1. CPU side - Write to GPU
        //  - instance IDs
        //  - generic instance information (transform, AABB)
        //  - specialised instance information
        // - need to think about how 

        static void Cull(Engine::World& world) {

        }


    };
}