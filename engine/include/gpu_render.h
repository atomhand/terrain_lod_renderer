#pragma once

#include <unordered_set>
#include <memory>

#include "world.h"
#include "culling.h"
#include "compute_shader.h"
#include "shader.h"
#include "storage_buffer.h"
#include "uniform_buffer.h"
#include "gpu_sort.h"
#include "gpu_filter.h"
#include "shader_shared.h"
#include "gpu_mesh.h"

namespace Engine {
    class GpuRender;
    enum class RenderPassId : uint32_t {
        OPAQUE,
        POST_OPAQUE,
        TRANSPARENT,
        SHADOW
    };

    struct DebugRenderUtil {
    public:
        Shader wireframeShader = Shader("shaders/basic_instanced.vert","shaders/primitive/wireframe.frag","shaders/primitive/triangle_density.geom");
        Shader triangleDensityShader = Shader("shaders/basic_instanced.vert","shaders/primitive/basic.frag","shaders/primitive/triangle_density.geom");

        uint32_t wireframeIdPos;
        uint32_t densityIdPos;

        void BindTriangleDensity(uint32_t materialId) {
            triangleDensityShader.use();
        }
        void BindWireframe(uint32_t materialId) {
            wireframeShader.use();
        }
    };

    struct CullingFilter {
    private:
        GpuFilter filter = GpuFilter("shaders/corepass/culling.cs");
        GpuFilter shadowFilter = GpuFilter("shaders/corepass/shadow_culling.cs");
    public:
        void Cull(GpuRender& gpuRender, RenderPassId pass, StorageBuffer& input, StorageBuffer& output, StorageBuffer& inputCount, StorageBuffer& outputCount);
    };

    class MaterialRenderPass {
    public:
        virtual bool SupportsWireframe() { return true; }
        virtual bool SupportsTriangleDensity() { return true; }

        // called once per frame, opportunity to fill buffers
        virtual void Prepare(World& world) {};

        virtual void RenderWireframe(World& world, uint32_t offset, uint32_t count, RenderPassId pass) {
            glMultiDrawElementsIndirect(GL_TRIANGLES,  GL_UNSIGNED_INT, (void*)(offset*sizeof(DrawElementsIndirectCommand)), count, 0);
        }
        
        virtual void RenderTriangleDensity(World& world, uint32_t offset, uint32_t count, RenderPassId pass) {
            glMultiDrawElementsIndirect(GL_TRIANGLES,  GL_UNSIGNED_INT, (void*)(offset*sizeof(DrawElementsIndirectCommand)), count, 0);
        }

        virtual void Render(World& world, uint32_t offset, uint32_t count, RenderPassId pass) {            
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
        uint32_t materialInstanceId;
        glm::vec4 aabbMax;

        RenderItemData(glm::mat4& model, AABB& aabb, uint32_t materialInstanceId) :
            model(model), aabbMin(aabb.min), materialInstanceId(materialInstanceId), aabbMax(aabb.max,1.0) {}
    };

    struct GpuMaterialInstance {
        uint32_t materialId;
        uint32_t meshId;
        // id within the material's internal buffer, used to access material specific per-instance attributes
        // Can be 0 if the material doesn't care
        uint32_t materialInstanceId;
    };

    // Same struct used on CPU and GPU side
    struct MaterialHeader {
        uint32_t id;
        uint32_t drawBufferOffset;
        uint32_t drawCount;
        uint32_t drawKeyOffset; // not set on  CPU side

        uint32_t renderPassesMask;        
        uint32_t shadowMaterialRedirect = 0xffffffff;

        void SetRenderPass(RenderPassId pass, bool x = true) {
            uint32_t position = static_cast<uint32_t>(pass);
            // clear bit and then set it
            renderPassesMask = (renderPassesMask & ~((uint32_t)1 << position) | (uint32_t(x) << position));
        }

        bool IsRenderPassEnabled(RenderPassId pass) {
            uint32_t position = static_cast<uint32_t>(pass);
            return (renderPassesMask >> position) & uint32_t(1);
        }

        bool operator < (MaterialHeader const& rhs) {
            return this->id < rhs.id;
        }
    };

    class GpuRender {
private:
    static void GenericPassBindings(GpuRender& gpuRender, MeshCache& meshCache) {
        gpuRender.passCulledKeysBuffer.BindBase(0);
        gpuRender.materialHeadersBuffer.BindBase(1);
        gpuRender.drawBaseInstanceBuffer.BindBase(2);
        gpuRender.renderItemBuffer.BindBase(3);
        meshCache.attributesBuffer.BindBase(4);
        gpuRender.meshHeadersBuffer.BindBase(5);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, gpuRender.drawCmdsBuffer.object());
        glBindBuffer(GL_PARAMETER_BUFFER, gpuRender.materialDrawCount.object());
        glBindVertexArray(meshCache.vao);
    }
public:
        GpuSort gpuSorter;
        DebugRenderUtil debugRenderUtil;

        GpuFilter baseInstanceFilter = GpuFilter("shaders/corepass/draw_base_instance.cs");
        CullingFilter cullingFilter;

        std::vector<RenderItemData> renderItemData;
        std::vector<glm::uvec2> materialKeys;
        std::vector<MaterialHeader> materialHeaders;

        // Render item keys
        StorageBuffer inputKeysBuffer = StorageBuffer(0);
        StorageBuffer passCulledKeysBuffer = StorageBuffer(0);

        // Render item associated data
        StorageBuffer renderItemBuffer = StorageBuffer(0);

        // Dispatch management buffers
        StorageBuffer indirectDispatchParamsBuffer = StorageBuffer(sizeof(unsigned int)*3);

        StorageBuffer drawCounterBuffer = StorageBuffer(4, 0);
        StorageBuffer keyCounterBuffer = StorageBuffer(4);
        StorageBuffer culledKeyCounterBuffer = StorageBuffer(4, 0);

        // Per-draw data
        StorageBuffer drawBaseInstanceBuffer = StorageBuffer(0, 0);
        StorageBuffer drawCmdsBuffer = StorageBuffer(0, 0);
        StorageBuffer materialDrawCount = StorageBuffer(0);

        // Material and mesh headers
        StorageBuffer materialHeadersBuffer = StorageBuffer(0);
        StorageBuffer meshHeadersBuffer = StorageBuffer(0);

        UniformBuffer passIdUniform;

        //ComputeShader cullingShader;

        //ComputeShader indirectGroupsFromCounterShader = ComputeShader("shaders/corepass/indirect_params_from_counter.cs");
        ComputeShader emitDrawCommandsShader = ComputeShader("shaders/corepass/emit_draw_commands.cs");

        uint32_t numDraws = 0;
        uint32_t numRenderItems = 0;

        uint32_t numMaterials = 0;

        static uint32_t PackKey(uint32_t materialId, uint32_t meshId) {
            assert(materialId <= 0x3fffu);
            assert(meshId <= 0x3ffffu);
            return (materialId << 18) | meshId;
        }

        MaterialHeader& RegisterMaterial(Engine::World& world, entt::entity entity) {
            auto& header = world.registry.emplace<MaterialHeader>(entity);
            header.id = numMaterials++;
            return header;
        }

        static void Init(World& world) {
            auto entity = world.registry.create();
            auto& gpuRender = world.registry.emplace<GpuRender>(entity);
        }

        static void PrePrepare(World& world) {
            auto& gpuRender = world.GetSingle<GpuRender>();
            //gpuRender.RegisterNewMaterials(world);
        }

        std::vector<std::unordered_set<uint32_t>> materialMeshPairs;

        void GatherRenderItems(World& world) {
            auto& meshCache = world.GetSingle<MeshCache>();

            materialMeshPairs.resize(numMaterials);
            for(auto& set : materialMeshPairs) {
                set.clear();
            }

            // Gather render items
            materialKeys.clear();
            renderItemData.clear();
            auto itemsView = world.registry.view<Transform,GpuMaterialInstance,Engine::CullingResult>();
            for(auto entity : itemsView) {
                auto [transform,material,cc] = itemsView.get(entity);

                Engine::AABB* aabbPtr = world.registry.try_get<AABB>(entity);

                Engine::AABB aabb;
                if(aabbPtr != nullptr) {
                    aabb = *aabbPtr;
                } else {
                    auto& mesh = meshCache.meshHeaders[material.meshId];
                    aabb = AABB(mesh.aabbMin,mesh.aabbMax);
                }

                // Key 
                materialKeys.push_back(glm::uvec2(PackKey(material.materialId, material.meshId), renderItemData.size()));
                renderItemData.emplace_back(transform.global,aabb,material.materialInstanceId);

                materialMeshPairs[material.materialId].insert(material.meshId);
            };
            numRenderItems = materialKeys.size();
            inputKeysBuffer.Set<glm::uvec2>(materialKeys.data(), materialKeys.size(), 0, true);
            passCulledKeysBuffer.SmartResizeBytes(materialKeys.size() * sizeof(glm::uvec2));
            renderItemBuffer.Set<RenderItemData>(renderItemData.data(), renderItemData.size(), 0, true);
        }

        static void PrepareGpuScene(World& world) {
            auto renderComponentView = world.registry.view<MaterialHeader,MaterialRenderComponent>();
            for(auto entity : renderComponentView) {
                auto [materialHeader,materialRenderComponent] = renderComponentView.get<MaterialHeader,MaterialRenderComponent>(entity);
                materialRenderComponent.renderPass->Prepare(world);
            }

            auto& gpuRender = world.GetSingle<GpuRender>();
            auto& meshCache = world.GetSingle<MeshCache>();
            meshCache.FlushStagingBuffer();

            gpuRender.GatherRenderItems(world);

            gpuRender.materialHeaders.clear();
            // TODO - Counting unique draw cmds per material
            world.registry.sort<MaterialHeader>([](const MaterialHeader &lhs, const MaterialHeader &rhs) { return lhs.id < rhs.id ; });
            uint32_t currentDrawCmdOffset = 0;
            auto materialView = world.registry.view<MaterialHeader>();
            for(auto entity : materialView) {
                auto& materialHeader = materialView.get<MaterialHeader>(entity);

                materialHeader.drawCount = gpuRender.materialMeshPairs[materialHeader.id].size();

                materialHeader.drawBufferOffset = currentDrawCmdOffset;
                currentDrawCmdOffset += materialHeader.drawCount;
                gpuRender.materialHeaders.push_back(materialHeader);
            }
            gpuRender.numDraws = currentDrawCmdOffset;

            gpuRender.drawCmdsBuffer.SmartResizeBytes(gpuRender.numDraws * sizeof(DrawElementsIndirectCommand));
            gpuRender.materialDrawCount.SmartResizeBytes(gpuRender.numMaterials*sizeof(uint32_t));
            
            // Set gpu side buffer
            gpuRender.materialHeadersBuffer.Set<MaterialHeader>(gpuRender.materialHeaders.data(), gpuRender.materialHeaders.size(), 0, true);
            gpuRender.meshHeadersBuffer.Set<MeshHeader>(meshCache.meshHeaders.data(), meshCache.meshHeaders.size(), 0, true);

            // EmitDrawCommands needs total draw count as a uniform (maybe use a uniform buffer and share it between shaders?)
            gpuRender.emitDrawCommandsShader.use();
            gpuRender.emitDrawCommandsShader.setInt("numDraws", gpuRender.numDraws);

            // Partially set up draw commands

            gpuRender.drawBaseInstanceBuffer.SmartResizeBytes(gpuRender.numDraws*sizeof(unsigned int));


            //   Sort keys
            //auto gpuSorter = world.GetSingle<GpuSort>();
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            // can  use passCulledKeys as scratch buffer because it's not holding anything important right now
            // Potential improvement - replace this with a global pool for temp buffers
            gpuRender.gpuSorter.SortInPlacePaired(gpuRender.inputKeysBuffer, gpuRender.passCulledKeysBuffer, gpuRender.numRenderItems);
            gpuRender.keyCounterBuffer.Set<uint32_t>(&gpuRender.numRenderItems, 1);
        }
        
        static void ExecutePass(Engine::World& world, RenderPassId passId, uint32_t subPassIndex) {
            auto& gpuRender = world.GetSingle<GpuRender>();
            PassRenderPassIdUniform passIdUniformData(static_cast<uint32_t>(passId),0,subPassIndex);
            gpuRender.passIdUniform.Set(&passIdUniformData);
            gpuRender.passIdUniform.BindBase(6);

            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            gpuRender.cullingFilter.Cull(gpuRender, passId, gpuRender.inputKeysBuffer, gpuRender.passCulledKeysBuffer, gpuRender.keyCounterBuffer, gpuRender.culledKeyCounterBuffer);
            
            // TODO: CULL according to the VP and the pass id
            // write a filtered buffer to passCulledKeysBuffer
            // (skipping culling for initial implementation)

            // Clear draw commands
            glClearNamedBufferData(gpuRender.drawCmdsBuffer.object(), GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
            glClearNamedBufferData(gpuRender.materialDrawCount.object(), GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);

            // TODO filter should support an indirect dispatch
            gpuRender.materialHeadersBuffer.BindBase(6);
            gpuRender.baseInstanceFilter.Filter(gpuRender.passCulledKeysBuffer, gpuRender.drawBaseInstanceBuffer, gpuRender.culledKeyCounterBuffer, gpuRender.drawCounterBuffer);

            // Emit draw commands
            gpuRender.passCulledKeysBuffer.BindBase(0);
            gpuRender.culledKeyCounterBuffer.BindBase(1);
            gpuRender.materialHeadersBuffer.BindBase(2);
            gpuRender.drawBaseInstanceBuffer.BindBase(3);
            gpuRender.drawCmdsBuffer.BindBase(4);
            gpuRender.meshHeadersBuffer.BindBase(5);
            gpuRender.drawCounterBuffer.BindBase(6);
            gpuRender.materialDrawCount.BindBase(7);
            gpuRender.emitDrawCommandsShader.use();
            // depends on previous kernel output so barrier is required
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            glDispatchCompute((gpuRender.numDraws+255)/256,1,1);

            auto& meshCache = world.GetSingle<MeshCache>();
            GenericPassBindings(gpuRender,meshCache);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ELEMENT_ARRAY_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
            // Iterate materials, bind and draw
            auto materialView = world.registry.view<MaterialHeader,MaterialRenderComponent>();

            if(world.input.wireFrame) {
                if(passId == RenderPassId::OPAQUE || passId == RenderPassId::POST_OPAQUE) {                    
                    for(auto entity : materialView) {
                        auto [materialHeader,materialRenderComponent] = materialView.get<MaterialHeader,MaterialRenderComponent>(entity);
                        if(materialHeader.IsRenderPassEnabled(passId) && materialRenderComponent.renderPass->SupportsWireframe()) {
                            passIdUniformData.materialId = materialHeader.id;
                            gpuRender.passIdUniform.Set(&passIdUniformData);
                            gpuRender.passIdUniform.BindBase(6);

                            gpuRender.debugRenderUtil.BindWireframe(materialHeader.id);
                            materialRenderComponent.renderPass->RenderWireframe(world, materialHeader.drawBufferOffset, materialHeader.drawCount, passId);
                        }
                    }
                }
            } else if(world.input.previewTriangleDensity) {
                if(passId == RenderPassId::OPAQUE || passId == RenderPassId::POST_OPAQUE) {                    
                    for(auto entity : materialView) {
                        auto [materialHeader,materialRenderComponent] = materialView.get<MaterialHeader,MaterialRenderComponent>(entity);
                        if(materialHeader.IsRenderPassEnabled(passId) && materialRenderComponent.renderPass->SupportsTriangleDensity()) {
                            passIdUniformData.materialId = materialHeader.id;
                            gpuRender.passIdUniform.Set(&passIdUniformData);
                            gpuRender.passIdUniform.BindBase(6);

                            gpuRender.debugRenderUtil.BindTriangleDensity(materialHeader.id);
                            materialRenderComponent.renderPass->RenderTriangleDensity(world, materialHeader.drawBufferOffset, materialHeader.drawCount, passId);
                        }
                    }
                }
            } else {
                for(auto entity : materialView) {
                    auto [materialHeader,materialRenderComponent] = materialView.get<MaterialHeader,MaterialRenderComponent>(entity);
                    if(materialHeader.IsRenderPassEnabled(passId)) {
                        passIdUniformData.materialId = materialHeader.id;
                        gpuRender.passIdUniform.Set(&passIdUniformData);
                        gpuRender.passIdUniform.BindBase(6);

                        materialRenderComponent.renderPass->Render(world, materialHeader.drawBufferOffset, materialHeader.drawCount, passId);
                    }
                }
            }

            glBindVertexArray(0);
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