#pragma once

#include "world.h"
#include "culling.h"
#include "compute_shader.h"
#include "shader.h"
#include "storage_buffer.h"

namespace Engine {    
    class GpuRender {
        struct RenderItemData {
            glm::mat4 model;
            AABB aabb;
            uint16_t id;
        };

        struct GpuMaterial {
            uint16_t materialId;
        };
        int numMaterials;
        std::vector<RenderItemData> renderItemData;
        std::vector<unsigned int> materialKeys;
        StorageBuffer materialHeaderBuffer;

        StorageBuffer inputKeysBuffer;
        StorageBuffer sortedKeysBuffer;

        ComputeShader cullingShader;

        int RegisterMaterial() {
            return numMaterials++;
        }
        
        static void UpdateGpuBuffers(Engine::World& world) {
            auto gpuRender = world.GetSingle<GpuRender>();
            auto view = world.registry.view<Transform,AABB,GpuMaterial>();
            gpuRender.materialHeaderData.clear();

            for(auto entity : view) {
                auto [transform,aabb,material] = view.get(entity);

                gpuRender.materialKeys.push_back((material.materialId << 16) | );
                gpuRender.renderItemData.push_back(RenderItemData { transform.global, aabb });
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