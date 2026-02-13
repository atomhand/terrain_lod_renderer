#include "renderpasses.h"
#include "shader_shared.h"
#include "terrain_geometry.h"
#include "culling.h"
#include "light.h"
#include "water_material.h"
#include "terrain_material.h"
#include "profiler.h"

#include "gpu_render.h"
#include "gpu_mesh.h"

using Engine::ViewUniformData, Engine::LightUniformData, Engine::MiscUniformData, Engine::Profiler;

void RenderPasses::Init(World& world) {
    debugCameraEntity = world.registry.create();
    auto& debugCamera = world.registry.emplace<Camera>(debugCameraEntity);
    debugCamera.main = false;
    world.registry.emplace<Transform>(debugCameraEntity);

    // Mesh cache that stores all mesh data, needs to be intialised before we start loading any meshes
    // maybe a better place this could be initialised, review later
    world.registry.emplace<Engine::MeshCache>(world.registry.create());

    GpuRender::Init(world);

    // Make sure passCullingVpUniform is allocated on GPU with sufficient capacity
    // (because it can be set by glCopyBufferSubData, which does not resize)
    glm::mat4 dummy = glm::mat4(1.f);
    passCullingVpUniform.Set(&dummy);

    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);    
    glClearColor(0.f,0.f,0.,0.0f);
    glClearDepth(0.f);
}

void RenderPasses::RunAll(World& world, Engine::Application& app) {
    if(!world.input.enableRendering) {
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }

    Engine::Camera* cameraMain;
    Engine::Transform* cameraMainTransform;        
    auto cameraView = world.registry.view<Camera,Transform>();
    for(auto entity : cameraView) {
        auto [camera,transform] = cameraView.get(entity);
        if(camera.main) {
            cameraMain = &camera;
            cameraMainTransform = &transform;
            break;
        }
    }
    assert(cameraMain != nullptr);

    auto [debugCamera,debugCameraTransform] = world.registry.get<Camera,Transform>(debugCameraEntity);
    Engine::Camera& cullingCamera = world.input.debugMetaCam ? debugCamera : *cameraMain;
    Engine::Transform& cullingCameraTransform = world.input.debugMetaCam ? debugCameraTransform : *cameraMainTransform;

    if(!world.input.debugMetaCam ) {        
        world.registry.replace<Engine::Transform>(debugCameraEntity,*cameraMainTransform);
        auto& debugCamera = world.registry.replace<Engine::Camera>(debugCameraEntity,*cameraMain);
        debugCamera.main = false;
    }

    bool skipDeferred = world.input.previewTriangleDensity || world.input.wireFrame;

    RetrieveData(world,app,cullingCamera,cullingCameraTransform);    

    if(skipDeferred) {
        hdr.BindHdrFramebuffer(display_w,display_h);
        PrepareMain(world,*cameraMain,*cameraMainTransform);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        DrawOpaque(world,world.input.drawAABBs, *cameraMain, cullingCamera);
    } else {
        deferred.BindGBuffer(display_w,display_h);
        PrepareMain(world,*cameraMain,*cameraMainTransform);
        DrawOpaque(world,world.input.drawAABBs, *cameraMain, cullingCamera);    
        DrawShadowMaps(world, cullingCamera);

        hdr.BindHdrFramebuffer(display_w,display_h);
        deferred.LightingPass(world);
    }

    if(world.input.previewTriangleDensity || world.input.wireFrame) {
        hdr.ApplyHeatmapping(world);
    } else {
        DrawSkybox(world, *cameraMain);
        DrawTransparent(world,world.input.drawAABBs);
        if(world.input.previewCascades)                
            DrawDebugOverlays(world,*cameraMain);
        hdr.ApplyTonemapping(world);
    }

    DrawDebugQuad(world);
}


void RenderPasses::RetrieveData(World& world, Engine::Application& app, Engine::Camera& cameraMain, Engine::Transform& cameraMainTransform) {
    auto profileHandle = Profiler::StartCpu("RenderPasses::RetrieveData");
    auto gpuProfileHandle = Profiler::StartGpu("RenderPasses::RetrieveData");

    auto& gpuRender = world.GetSingle<Engine::GpuRender>();
    gpuRender.PrePrepare(world);

    WaterMaterial::PrepareMain(world, deferred.gBuffer.depthAttachment);

    gpuRender.PrepareGpuScene(world);

    glm::mat4 VP = cameraMain.projection * cameraMain.view;
    glm::vec3 cameraPos = cameraMainTransform.position();
    
    // Rendering
    app.getFramebufferSize(display_w,display_h);
}

// Draw to a special debug camera with a different
// Not part of the regular render
void RenderPasses::DrawDebugOverlays(World& world, Engine::Camera& cameraToDebug, bool drawCameraFrustum) {
    auto profileHandle = Profiler::StartCpu("RenderPasses::DrawDebugOverlays");
    auto gpuProfileHandle = Profiler::StartGpu("RenderPasses::DrawDebugOverlays");

    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_CULL_FACE);

    glDepthMask(GL_FALSE);
    debugWireframeMaterial.use();
    if(drawCameraFrustum) {
        debugWireframeMaterial.SetColor(glm::vec3(1.0,0.0,0.0));
        debugWireframeMaterial.SetModel(cameraToDebug.invCamera);
        Engine::DrawUtil::DrawCubeNdc();
    }
    auto lightView = world.registry.view<Engine::DirectionalLight>();
    for(auto entity : lightView) {
        auto& light = lightView.get<Engine::DirectionalLight>(entity);

        std::vector lightSpaceMatrices = light.ReadbackLightMatrices();

        glm::vec3 rgb = glm::vec3(1,0.5,0);
        for(auto lsm : lightSpaceMatrices) {                
            glm::mat4 invLsm = glm::inverse(lsm);
            debugWireframeMaterial.SetColor(rgb);
            debugWireframeMaterial.SetModel(invLsm);
            Engine::DrawUtil::DrawCubeNdc();

            rgb = glm::vec3(rgb.z,rgb.x,rgb.y);
        }
    }
    glDepthMask(GL_TRUE);

    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Draw to the shadow map depth buffers
void RenderPasses::DrawShadowMaps(World& world, Engine::Camera& cameraMain) {
    auto profileHandle = Profiler::StartCpu("RenderPasses::DrawShadowMaps");
    auto gpuProfileHandle = Profiler::StartGpu("RenderPasses::DrawShadowMaps");

    // directional lights
    auto& sun = world.GetSingle<Engine::DirectionalLight>();

    sun.shadowMap.PrepareFramebuffer();

    if(!world.input.drawShadows()) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    auto lightUniformData = LightUniformData {
        cameraMain.view,
        glm::vec4(sun.direction,1.f),
        glm::vec4(sun.color,1.f),

    };
    lightUniformData.cascadeCount = world.input.drawShadows() ? sun.NumCascades : 0;
    lightUniforms.Set(&lightUniformData);
    lightUniforms.BindBase(1);

    if(!world.input.debugMetaCam ) {
        sun.MakeLightSpaceMatrices(world, cameraMain, deferred.gBuffer.depthAttachment, lightUniforms);
        sun.BindUniforms();
    }

    glEnable(GL_DEPTH_CLAMP);

    // No face culling for shadows right now, because it doesn't work with my 
    // non-manifold terrain mesh
    //glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
    for(int i =0; i<sun.NumCascades; i++) {
        sun.shadowMap.PrepareFramebufferLayer(i);
        sun.BindCascadeToCullingVPUniform(passCullingVpUniform, i);
        
        auto& gpuRender = world.GetSingle<Engine::GpuRender>();
        gpuRender.ExecutePass(world, Engine::RenderPassId::SHADOW);
    }

    glDisable(GL_DEPTH_CLAMP);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
}

// Prepare framebuffer for the main pass
void RenderPasses::PrepareMain(World& world, Engine::Camera& camera, Engine::Transform& cameraTransform) {
    auto profileHandle = Profiler::StartCpu("RenderPasses::PrepareMain");
    auto gpuProfileHandle = Profiler::StartGpu("RenderPasses::PrepareMain");

    camera.setViewport(display_w,display_h);
    glViewport(0, 0, display_w, display_h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ViewUniformData viewUniformData(camera,cameraTransform);
    viewUniforms.Set(&viewUniformData);
    viewUniforms.BindBase(0);

    float fogFactor = world.input.drawFog ? std::lerp(0.000001f, 0.001f,world.input.fogStrength) : 0.f;

    auto miscUniformData = MiscUniformData {
        fogFactor * world.input.heightFogFactor,
        fogFactor * world.input.constantFogFactor,
        world.input.heightFogTransitionStart,
        world.input.heightFogTransitionDuration,
        world.input.fakeCurvature  ? 1.0f : 0.0f,
        world.input.previewCascades ? 1.0f : 0.0f,
        float(world.input.previewNormalsMode),
        float(world.input.stochasticBlending),
        float(world.input.applyAutoExposure),
        world.input.displacementScale,
        world.shaderAnimTime
    };
    miscUniforms.Set(&miscUniformData);
    miscUniforms.BindBase(2);
}

// To draw the debug camera output to the screen (press Q)
void RenderPasses::DrawDebugQuad(World& world) {
    auto profileHandle = Profiler::StartCpu("RenderPasses::DrawDebugQuad");
    auto gpuProfileHandle = Profiler::StartGpu("RenderPasses::DrawDebugQuad");

    if(world.input.shadowTestingMode == 1) {
        auto& sun = world.GetSingle<Engine::DirectionalLight>();
        
        debugCascadeMaterial.use();
        glActiveTexture(GL_TEXTURE0);
        sun.shadowMap.BindDepthMap();
        // turn off depth comparison so we can read the shadowmap with an ordinary sampler
        // (otherwise, it still works but GL considers it undefined behaviour and pushes a warning)
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_NONE );
        Engine::DrawUtil::DrawQuad();            
        // turn depth comparison back on
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glBindTexture(GL_TEXTURE_2D_ARRAY,0);
        glUseProgram(0);
    }        
}

// Draw opaque renderitems
void RenderPasses::DrawOpaque(World& world, bool drawAABB, Engine::Camera& camera, Engine::Camera& cullingCamera) {
    auto profileHandle = Profiler::StartCpu("RenderPasses::DrawOpaque");
    auto gpuProfileHandle = Profiler::StartGpu("RenderPasses::DrawOpaque");

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);
    glDisable(GL_BLEND);
    
    /*
    Water main pass can't write depth because it needs to read from the depth attachment
    So do 2 separate passes
    - Gbuffer write (reads depth)
    - Depth only (writes depth)
    (depth value is needed for correct position reconstruction in deferred pass)

    An alternative would be shift water into the transparent pass and do its lighting like
    a 
    (the advantage being it could then read the Gbuffer as well)
    */

    //TerrainMaterial::DrawMain(world);

    passCullingVpUniform.Set(&cullingCamera.VP);
    passCullingVpUniform.BindBase(5);
    
    auto& gpuRender = world.GetSingle<Engine::GpuRender>();
    gpuRender.ExecutePass(world, Engine::RenderPassId::OPAQUE);
    gpuRender.ExecutePass(world, Engine::RenderPassId::POST_OPAQUE);

    /*
    if(!world.input.previewTriangleDensity) {
        glDepthMask(GL_FALSE);
        WaterMaterial::DrawMain(world, deferred.gBuffer.depthAttachment);
        glDepthMask(GL_TRUE);
        WaterMaterial::DrawDepth(world);
    }
    */

    if(drawAABB) {
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);

        debugWireframeMaterial.use();
        debugWireframeMaterial.SetColor(glm::vec3(100.,100.,100.));

        auto aabbView = world.registry.view<Transform,Engine::CullingResult,Engine::AABB>();
        for(auto entity : aabbView) {
            auto [transform,cullingResult,aabb] = aabbView.get(entity);
            if(!cullingResult.viewResult) continue;
            
            glm::vec3 extent = aabb.max - aabb.min;
            assert(extent.x >= 0.f && extent.y >= 0.f && extent.z >= 0.f);
            glm::vec3 offset = aabb.min + (extent / 2.f);
            glm::mat4 aabbT = transform.global * glm::translate(glm::mat4(1.0),offset) * glm::scale(glm::mat4(1.0), extent/2.f);
            
            debugWireframeMaterial.SetModel(aabbT);
            Engine::DrawUtil::DrawCube();
        }

        glDepthMask(GL_TRUE);
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_CULL_FACE);
    }
}

// Draw transparent renderitems
void RenderPasses::DrawTransparent(World& world, bool drawAABB) {
    auto profileHandle = Profiler::StartCpu("RenderPasses::DrawTransparent");
    auto gpuProfileHandle = Profiler::StartGpu("RenderPasses::DrawTransparent");

    auto& sun = world.GetSingle<Engine::DirectionalLight>();

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);        
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    auto& gpuRender = world.GetSingle<Engine::GpuRender>();
    gpuRender.ExecutePass(world, Engine::RenderPassId::TRANSPARENT);
    glDisable(GL_BLEND);
}