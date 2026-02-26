#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glad/gl.h>
#include "world.h"
#include "shader.h"
#include "culling.h"
#include "storage_buffer.h"
#include "light.h"

#include "gpu_mesh.h"
#include "gpu_render.h"

using Engine::World, Engine::Transform, Engine::GpuRender, Engine::MaterialHeader, Engine::MaterialRenderComponent, Engine::MaterialImplementation, Engine::DrawElementsIndirectCommand, Engine::MeshCache, Engine::GpuMeshBuilder;

struct TerrainMaterial {
private:
    class TerrainMaterialImplementation : public Engine::InstancedMaterialImplementation<TerrainMaterial> {
    public:
        void Bind(World& world) override {
            InstancedMaterialImplementation::Bind(world);
            auto& cache = world.GetSingle<Cache>();
            cache.BindTextures();
        }

        TerrainMaterialImplementation() : InstancedMaterialImplementation("TerrainMaterial") {           
            std::vector<const char*> wireframeDef = { "#define TERRAIN_HEATMAP"};
            auto shadow_defines = std::vector<const char*>{ "#define SHADOW_PASS"};
            
            MaterialImplementation::passShaders = {
                { Engine::RenderPassId::OPAQUE, Engine::Shader("shaders/terrain.vert","shaders/terrain_pbr.frag") },
                { Engine::RenderPassId::SHADOW, Engine::Shader("shaders/terrain.vert","shaders/shadow.frag", shadow_defines) },
                { Engine::RenderPassId::DIAGNOSTIC, Engine::Shader("shaders/terrain.vert","shaders/primitive/wireframe.frag","shaders/primitive/triangle_density.geom",wireframeDef) },
            };
        };
    };

    static void MakeTerrainMesh(GpuMeshBuilder& meshBuilder, int chunk_size) {
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

        /*
        auto& verts = meshBuilder.verts;
        auto& indices = meshBuilder.indices;

        verts.clear();
        indices.clear();

        int cw = chunk_size;
        assert(cw >= 2);

        int vw = cw+3;

        int iz;
        for(iz=0; iz<vw; iz++) {
            for(int ix=0; ix<vw; ix++) {
                int x = std::clamp((ix-1),0,cw);
                int z = std::clamp((iz-1),0,cw);

                glm::vec3 pos = glm::vec3(x / float(cw),0.f,z / float(cw));
                if(ix == 0 || iz == 0 || ix == vw-1 || iz == vw-1) {
                    pos.y -= 64.f;
                }

                verts.push_back(pos);
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

        meshBuilder.aabb = Engine::AABB(glm::vec3(0.,-1.,0.),glm::vec3(1.0,1.0,1.0));
        */
    }
public:
    struct Cache {
    public:
        const size_t capacity;
        uint32_t meshId;
        Engine::Texture2DArray terrainDataTex;
        std::vector<Engine::Texture2DArray> texturearrays;

        void BindTextures(bool bindDataTex = true) {            
            int offset = GL_TEXTURE0;
            if(bindDataTex) {
                glActiveTexture(offset);
                terrainDataTex.bind();
            }
            offset++;

            for(int i =0; i<texturearrays.size(); i++) {
                glActiveTexture(offset++);
                texturearrays[i].bind();
            }
        }

        Cache(size_t capacity, int chunkSize, uint32_t meshId) : 
            capacity(capacity),
            meshId(meshId)
        {
            terrainDataTex.Configure(1, GL_RGBA32F, chunkSize*2+1, chunkSize*2+1, capacity, GL_LINEAR, GL_CLAMP_TO_EDGE);

            texturearrays.push_back(Engine::Texture2DArray::Import({
                "textures/grass/diff_4k.jpg",
                "textures/cliff2/diff_4k.jpg",
                "textures/sand/diff_4k.jpg",
                "textures/snow/diff_4k.jpg"
            },
                "textures/TERRAIN_SPLAT_DIFFUSE"));
            texturearrays.push_back(Engine::Texture2DArray::Import({
                "textures/grass/norm_4k.png",
                "textures/cliff2/norm_4k.jpg",
                "textures/sand/norm_4k.png",
                "textures/snow/norm_4k.jpg"
            },
                "textures/TERRAIN_SPLAT_NORMAL"));
            texturearrays.push_back(Engine::Texture2DArray::Import({
                "textures/grass/arm_4k.jpg",
                "textures/cliff2/arm_4k.jpg",
                "textures/sand/arm_4k.jpg",
                "textures/snow/arm_4k.jpg"
            },
                "textures/TERRAIN_SPLAT_ARM"));
            texturearrays.push_back(Engine::Texture2DArray::Import({
                "textures/grass/disp_4k.png",
                "textures/cliff2/disp_4k.jpg",
                "textures/sand/disp_4k.png",
                "textures/snow/disp_4k.jpg"
            },
                "textures/TERRAIN_SPLAT_DISPLACEMENT"));
        }
    };
    
    glm::vec4 uvs[4];

    TerrainMaterial() {}

    static void Setup(Engine::World& world, size_t capacity, float scale, int chunk_size) {       
        GpuMeshBuilder builder;
        MakeTerrainMesh(builder, chunk_size);

        auto& meshCache = world.GetSingle<MeshCache>();
        uint32_t meshId = meshCache.RegisterMesh(builder);

        auto headerEntity = world.registry.create();
        world.registry.emplace<TerrainMaterial::Cache>(headerEntity, capacity, chunk_size, meshId);
        
        auto& gpuRender = world.GetSingle<GpuRender>();
        auto& materialHeader = gpuRender.RegisterMaterial<TerrainMaterialImplementation>(world, headerEntity);
        materialHeader.SetRenderPass(Engine::RenderPassId::OPAQUE);
        materialHeader.SetRenderPass(Engine::RenderPassId::SHADOW);
        materialHeader.SetRenderPass(Engine::RenderPassId::DIAGNOSTIC);
    }
};