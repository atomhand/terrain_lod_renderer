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

using Engine::World, Engine::Mesh, Engine::Transform;

struct TerrainMaterial {
public:
    struct Cache {
    public:
        size_t capacity;
        Engine::Mesh mesh;
        Engine::Shader shader;
        Engine::Shader depthOnlyShader;
        Engine::Shader shadowShader;

        
        Engine::Shader triangleDensityShader;
        //Engine::Shader wireframeShader;

        Engine::StorageBuffer transformBuffer;
        Engine::StorageBuffer indexBuffer;
        std::vector<glm::mat4> transforms;
        std::vector<int> indices;

        Engine::Texture2DArray terrainDataTex;

        int timeOffset;

        std::vector<Engine::Texture2DArray> texturearrays;

        Cache(size_t capacity, int chunkSize, Engine::Mesh mesh) : 
            capacity(capacity),
            mesh(mesh),
            shader(Engine::Shader("shaders/terrain.vert", "shaders/terrain_pbr.frag")),
            depthOnlyShader(Engine::Shader("shaders/terrain.vert","shaders/shadow.frag")),
            triangleDensityShader(Engine::Shader("shaders/terrain.vert","shaders/primitive/basic.frag","shaders/primitive/triangle_density.geom")),
            //wireframeShader(Engine::Shader("shaders/terrain.vert","shaders/wireframe.frag","shaders/wireframe.geom")),
            transformBuffer(capacity * sizeof(glm::mat4)),
            indexBuffer(capacity * sizeof(int))
        {
            auto defines = std::vector<const char*>{ "#define SHADOW_PASS"};
            shadowShader = Engine::Shader("shaders/terrain.vert","shaders/shadow.frag","shaders/shadow_cascade.geom", defines);

            terrainDataTex.Configure(1, GL_RGBA32F, chunkSize, chunkSize, capacity, GL_LINEAR, GL_CLAMP_TO_EDGE);

            transforms.reserve(capacity);
            indices.reserve(capacity);

            timeOffset = glGetUniformLocation(shader.programId(), "time");

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

    static void DrawMain(Engine::World& world) {
        auto& cache = world.GetSingle<TerrainMaterial::Cache>();
        // Prepare transforms
        
        cache.transforms.clear();
        cache.indices.clear();
        auto view = world.registry.view<TerrainMaterial,Transform,Engine::CullingResult>();
        for(auto entity : view) {
            auto [mat,transform,cullingResult] = view.get(entity);

            if(mat.enabled && cullingResult.viewResult) {
                cache.transforms.push_back(transform.global);
                cache.indices.push_back(mat.index);
            }
        }

        // There should never be more chunks than the SSBO can support
        assert(cache.transforms.size() <= cache.capacity);
        cache.transformBuffer.SetBytes((void*)cache.transforms.data(), cache.transforms.size()*sizeof(glm::mat4), 0);
        cache.indexBuffer.SetBytes((void*)cache.indices.data(), cache.indices.size()*sizeof(int), 0);

        // Bind shader

        if(world.input.previewTriangleDensity) {
            cache.triangleDensityShader.use();
        } else {
            cache.shader.use();
        }
        
        int offset = GL_TEXTURE0;
        glActiveTexture(offset++);
        cache.terrainDataTex.bind();

        for(int i =0; i<cache.texturearrays.size(); i++) {
            glActiveTexture(offset++);
            cache.texturearrays[i].bind();
        }

        cache.transformBuffer.BindBase(0);
        cache.indexBuffer.BindBase(1);

        glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT| GL_BUFFER_UPDATE_BARRIER_BIT);

        // Draw
        cache.mesh.DrawInstanced(cache.transforms.size());
    }
    
    // NOTE - water doesnt actually cast shadow..
    static void DrawShadow(Engine::World& world) {
        auto& cache = world.GetSingle<TerrainMaterial::Cache>();

        // Prepare transforms
        cache.transforms.clear();
        cache.indices.clear();
        auto view = world.registry.view<TerrainMaterial,Transform,Engine::SurvivedLightCullingTag>();
        for(auto entity : view) {
            auto [mat,transform] = view.get(entity);

            if(mat.enabled) {
                cache.transforms.push_back(transform.global);
                cache.indices.push_back(mat.index);
            }
        }

        // There should never be more chunks than the SSBO can support
        assert(cache.transforms.size() <= cache.capacity);
        cache.transformBuffer.SetBytes((void*)cache.transforms.data(), cache.transforms.size()*sizeof(glm::mat4), 0);
        cache.indexBuffer.SetBytes((void*)cache.indices.data(), cache.indices.size()*sizeof(int), 0);

        // Draw
        cache.transformBuffer.BindBase(0);
        cache.indexBuffer.BindBase(1);
        cache.shadowShader.use();
        glActiveTexture(GL_TEXTURE0);
        cache.terrainDataTex.bind();
        cache.mesh.DrawInstanced(cache.transforms.size());
    }
};