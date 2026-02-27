// Tom Kellett 2025
#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include "world.h"
#include "application.h"
#include "uniform_buffer.h"
#include "camera.h"
#include "material.h"
#include "compute_shader.h"
#include "hdr.h"
#include "deferred.h"

using Engine::World, Engine::Camera, Engine::Transform;
// 
class RenderPasses {
public:
    RenderPasses() {
        deferred.gBuffer.depthAttachment.internalFormat = GL_DEPTH_COMPONENT32F;
        hdr.hdrFramebuffer.depthAttachment = deferred.gBuffer.depthAttachment;
    }

    void Init(World& world);
    
    // Run all render passes
    void RunAll(World& world, Engine::Application& app);
private:
    int display_w, display_h;
    HDR hdr;
    Deferred deferred;

    Engine::UniformBuffer viewUniforms;
    Engine::UniformBuffer lightUniforms;
    Engine::UniformBuffer debugUniforms;
    Engine::UniformBuffer miscUniforms;
    Engine::UniformBuffer passCullingVpUniform;

    entt::entity debugCameraEntity;

    Engine::DirectDrawMaterial debugCascadeMaterial = Engine::DirectDrawMaterial(Engine::Shader("shaders/primitive/fullscreen_quad.vert","shaders/primitive/fullscreen_quad_texture2darray.frag"));
    Engine::DirectDrawMaterial debugWireframeMaterial = Engine::DirectDrawMaterial(Engine::Shader("shaders/primitive/basic.vert","shaders/primitive/basic.frag"));

    // Retrieve render data from the scene (since the scene queries are not very efficient
    // it's better to cache anything that will be used more than once)
    void RetrieveData(World& world, Engine::Application& app, Engine::Camera& cameraMain, Engine::Transform& cameraMainTransform);

    // Draw to a special debug camera with a different
    // Not part of the regular render
    void DrawDebugOverlays(World& world, Engine::Camera& cameraToDebug);

    // Draw to the shadow map depth buffers
    void DrawShadowMaps(World& world, Engine::Camera& cameraMain);

    // Prepare framebuffer for the main pass
    void PrepareMain(World& world, Engine::Camera& camera, Engine::Transform& cameraTransform, Engine::Transform& cullingCameraTransform);

    // To draw the debug camera output to the screen (press Q)
    void DrawDebugQuad(World& world);

    // Draw opaque renderitems
    void DrawOpaque(World& world, Engine::Camera& camera, Engine::Camera& cullingCamera);

    // Draw transparent renderitems
    void DrawTransparent(World& world);

    void DrawSkybox(World& world, Engine::Camera& camera) {
        auto profile = Engine::Profiler::StartCpu("RenderPasses::DrawSkybox");
        auto gpuProfileHandle = Engine::Profiler::StartGpu("RenderPasses::DrawSkybox");
        
        glDisable(GL_CULL_FACE);
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_GEQUAL);
        glDisable(GL_BLEND);

        glDepthMask(GL_FALSE);
        world.skybox.DrawSkybox(camera.view,camera.projection);
        glDepthMask(GL_TRUE);
    }
};