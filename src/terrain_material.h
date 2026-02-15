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

using Engine::World, Engine::Transform, Engine::GpuRender, Engine::MaterialHeader, Engine::MaterialRenderComponent, Engine::MaterialRenderPass, Engine::DrawElementsIndirectCommand, Engine::MeshCache, Engine::GpuMeshBuilder;

struct TerrainMaterial {
private:
    class TerrainRenderPass : MaterialRenderPass {
        void RenderTriangleDensity(World& world, uint32_t drawOffset, uint32_t drawCount, Engine::RenderPassId pass) override {
            auto cacheView = world.registry.view<Cache,MaterialHeader>();
            auto [cache,header] = cacheView.get(cacheView.front());

            cache.triangleDensityShader.use();
            cache.BindTextures();
            glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(drawOffset*sizeof(DrawElementsIndirectCommand)), header.id*sizeof(uint32_t), drawCount, 0);
        }

        void RenderWireframe(World& world, uint32_t drawOffset, uint32_t drawCount, Engine::RenderPassId pass) override {
            auto cacheView = world.registry.view<Cache,MaterialHeader>();
            auto [cache,header] = cacheView.get(cacheView.front());

            cache.wireframeShader.use();
            cache.BindTextures();
            glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(drawOffset*sizeof(DrawElementsIndirectCommand)), header.id*sizeof(uint32_t), drawCount, 0);
        }

        void Render(World& world, uint32_t drawOffset, uint32_t drawCount, Engine::RenderPassId pass) override {
            auto cacheView = world.registry.view<Cache,MaterialHeader>();
            auto [cache,header] = cacheView.get(cacheView.front());

            if(pass == Engine::RenderPassId::SHADOW) {                  
                cache.shadowShader.use();
                cache.BindTextures();
                glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(drawOffset*sizeof(DrawElementsIndirectCommand)), header.id*sizeof(uint32_t), drawCount, 0);
            } else {
                cache.shader.use();
                cache.BindTextures();
                glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(drawOffset*sizeof(DrawElementsIndirectCommand)), header.id*sizeof(uint32_t), drawCount, 0);
            }
        }
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
        size_t capacity;
        uint32_t meshId;
        Engine::Shader shader;
        Engine::Shader depthOnlyShader;
        Engine::Shader wireframeShader;
        Engine::Shader shadowShader;
        
        Engine::Shader triangleDensityShader;
        //Engine::Shader wireframeShader;

        Engine::Texture2DArray terrainDataTex;

        int timeOffset;

        std::vector<Engine::Texture2DArray> texturearrays;

        entt::entity CreateTerrainItem(Engine::World& world, glm::vec3 nodePos, glm::vec3 extent, Engine::AABB& aabb, uint32_t topIdx) {
            auto cacheView = world.registry.view<Cache,MaterialHeader>();
            auto [cache,header] = cacheView.get(cacheView.front());

            auto entity = world.registry.create();
            auto& terrainMat = world.registry.emplace<TerrainMaterial>(entity, topIdx);

            auto& transform = world.registry.emplace<Engine::Transform>(entity);
            transform.global = glm::translate(glm::mat4(1.), nodePos) * glm::scale(glm::mat4(1.), glm::vec3(extent.x,1.f,extent.z));

            world.registry.emplace<Engine::AABB>(entity, aabb);

            auto& instance = world.registry.emplace<Engine::GpuMaterialInstance>(entity);
            instance.materialId = header.id;
            instance.meshId = cache.meshId;
            instance.materialInstanceId = topIdx;

            return entity;
        }

        void BindTextures() {            
            int offset = GL_TEXTURE0;
            glActiveTexture(offset++);
            terrainDataTex.bind();

            for(int i =0; i<texturearrays.size(); i++) {
                glActiveTexture(offset++);
                texturearrays[i].bind();
            }
        }

        Cache(size_t capacity, int chunkSize, uint32_t meshId) : 
            capacity(capacity),
            meshId(meshId),
            shader(Engine::Shader("shaders/terrain.vert", "shaders/terrain_pbr.frag")),
            depthOnlyShader(Engine::Shader("shaders/terrain.vert","shaders/shadow.frag")),
            triangleDensityShader(Engine::Shader("shaders/terrain.vert","shaders/primitive/basic.frag","shaders/primitive/triangle_density.geom"))
            //wireframeShader(Engine::Shader("shaders/terrain.vert","shaders/wireframe.frag","shaders/wireframe.geom")),
        {
            std::vector<const char*> wireframeDef = { "#define TERRAIN_HEATMAP"};
            wireframeShader = Engine::Shader("shaders/terrain.vert","shaders/primitive/wireframe.frag","shaders/primitive/triangle_density.geom",wireframeDef);

            auto defines = std::vector<const char*>{ "#define SHADOW_PASS"};
            shadowShader = Engine::Shader("shaders/terrain.vert","shaders/shadow.frag", defines);

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
    
    const int index;
    bool enabled;

    TerrainMaterial(int index) : index(index) {}

    static void Setup(Engine::World& world, size_t capacity, float scale, int chunk_size) {       
        GpuMeshBuilder builder;
        MakeTerrainMesh(builder, chunk_size);

        auto& meshCache = world.GetSingle<MeshCache>();
        uint32_t meshId = meshCache.RegisterMesh(builder);

        auto headerEntity = world.registry.create();
        world.registry.emplace<TerrainMaterial::Cache>(headerEntity, capacity, chunk_size, meshId);
        
        auto& gpuRender = world.GetSingle<GpuRender>();
        auto& materialHeader = gpuRender.RegisterMaterial(world, headerEntity);
        materialHeader.SetRenderPass(Engine::RenderPassId::OPAQUE);
        materialHeader.SetRenderPass(Engine::RenderPassId::SHADOW);

        world.registry.emplace<MaterialRenderComponent>(headerEntity, (MaterialRenderPass*)new TerrainRenderPass());
    }
};