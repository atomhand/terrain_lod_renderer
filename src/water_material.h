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

struct WaterMaterial {
public:
    struct Cache {
    public:
        size_t capacity;
        Engine::Mesh mesh;
        Engine::Shader shader;
        Engine::Shader depthOnlyShader;
        Engine::Shader shadowShader;
        Engine::StorageBuffer storage;
        std::vector<glm::mat4> transforms;

        int timeOffset;

        std::vector<Engine::Texture> textures;

        Cache(size_t capacity, Engine::Mesh mesh) : 
            capacity(capacity),
            mesh(mesh),
            shader(Engine::Shader("shaders/water.vert", "shaders/water_pbr.frag")),
            depthOnlyShader(Engine::Shader("shaders/water.vert","shaders/shadow.frag")),
            shadowShader(Engine::Shader("shaders/water.vert","shaders/shadow.frag","shaders/shadow_cascade.geom")),
            storage(capacity * sizeof(glm::mat4)) {
            transforms.reserve(capacity);

            timeOffset = glGetUniformLocation(shader.programId(), "time");

            textures.push_back(Engine::Texture("textures/waterN1.jpg"));
            textures.push_back(Engine::Texture("textures/waterN2.jpg"));
        }
    };
    bool enabled = true;

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
        cache.storage.Set((void*)cache.transforms.data(), cache.transforms.size()*sizeof(glm::mat4), 0);

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