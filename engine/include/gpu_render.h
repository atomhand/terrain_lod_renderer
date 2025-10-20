#pragma once

#include "world.h"
#include "culling.h"
#include "compute_shader.h"
#include "shader.h"
#include "storage_buffer.h"

namespace Engine {    
    class GpuRender {
        struct MaterialHeader {
            glm::mat4 model;
            AABB aabb;
            int id;
        };

        struct GpuMaterial {
            int materialId;
        };
        int numMaterials;
        std::vector<MaterialHeader> materialHeaderData;
        StorageBuffer materialHeaderBuffer;

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

                gpuRender.materialHeaderData.push_back(MaterialHeader { transform.global, aabb, material.materialId});
            }


        }

        static void Cull(Engine::World& world) {

        }


    };
}