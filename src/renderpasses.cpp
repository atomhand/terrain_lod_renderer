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

using Engine::RenderItem, Engine::ViewUniformData, Engine::LightUniformData, Engine::MiscUniformData, Engine::Profiler;

void RenderPasses::Init(World& world) {
    debugCameraEntity = world.registry.create();
    auto& debugCamera = world.registry.emplace<Camera>(debugCameraEntity);
    debugCamera.main = false;
    world.registry.emplace<Transform>(debugCameraEntity);

    // Mesh cache that stores all mesh data, needs to be intialised before we start loading any meshes
    // maybe a better place this could be initialised, review later
    world.registry.emplace<Engine::MeshCache>(world.registry.create());

    GpuRender::Init(world);

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

    bool skipDeferred = world.input.previewTriangleDensity;

    RetrieveData(world,app,cullingCamera,cullingCameraTransform);        
    DrawShadowMaps(world, cullingCamera);

    if(skipDeferred) {
        hdr.BindHdrFramebuffer(display_w,display_h);
        PrepareMain(world,*cameraMain,*cameraMainTransform);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        DrawOpaque(world,world.input.drawAABBs, *cameraMain, cullingCamera);
    } else {
        deferred.BindGBuffer(display_w,display_h);
        PrepareMain(world,*cameraMain,*cameraMainTransform);
        DrawOpaque(world,world.input.drawAABBs, *cameraMain, cullingCamera);

        hdr.BindHdrFramebuffer(display_w,display_h);
        deferred.LightingPass(world);
    }

    if(world.input.previewTriangleDensity) {
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

    auto& gpuRender = world.GetSingle<Engine::GpuRender>();
    gpuRender.PrePrepare(world);

    WaterMaterial::PrepareMain(world, deferred.gBuffer.depthAttachment);

    gpuRender.PrepareGpuScene(world);



    glm::mat4 VP = cameraMain.projection * cameraMain.view;
    glm::vec3 cameraPos = cameraMainTransform.position();
    
    cameraMain.nearestItem = cameraMain.far;
    cameraMain.furthestItem = cameraMain.near;
    
    // Rendering
    app.getFramebufferSize(display_w,display_h);

    auto cullingView = world.registry.view<Engine::CullingResult,Engine::AABB,Transform>();
    for(auto entity : cullingView) {
        auto [cullingResult,aabb,transform] = cullingView.get(entity);
        
        // test the OOBB (model * AABB) against the frustum. Approximate.
        // The result also includes the nearest and furthest depth on the rotated AABB (roughly)
        cullingResult = Engine::FrustumAABBTest(transform.global,cameraMain.view,VP,cameraPos,aabb,cameraMain.far,world.input.fakeCurvature);

        cullingResult.viewResult |= !world.input.enableCulling;

        if(!cullingResult.viewResult) {
            // convert to model space so we can find nearest space on AABB with clamp()
            glm::vec3 modelSpaceCameraPos = inverse(transform.global) * glm::vec4(cameraPos,1.f);
            glm::vec3 nearestM = glm::clamp(modelSpaceCameraPos,aabb.min,aabb.max);
            // convert back to world space
            glm::vec3 nearestW = transform.global * glm::vec4(nearestM,1.f);
            float aabbDist =  glm::distance(nearestW,cameraPos);
            // force pass culling if the entity is sufficiently close to the camera
            cullingResult.viewResult |= aabbDist < glm::distance(aabb.min,aabb.max);
        }

        if(cullingResult.viewResult) {
            cameraMain.nearestItem = std::min(cameraMain.nearestItem,cullingResult.viewDepthMin);
            cameraMain.furthestItem = std::max(cameraMain.furthestItem,cullingResult.viewDepthMax);
        }
    }

    auto renderItemView = world.registry.view<RenderItem,Transform,Engine::CullingResult,Engine::AABB>();
    for(auto entity : renderItemView) {
        auto [renderItem,transform,cullingResult,aabb] = renderItemView.get(entity);

        if(!renderItem.ShouldDraw()) {
            cullingResult.viewResult = false;
            continue;
        }

        // Set which Lod to use based on distance from camera pos to the nearest point
        // in the render item's AABB
        // for simplicity the same Lod is used ror shadows and rendering
        if(cullingResult.viewResult) {
            renderItem.updateActiveLod(cullingResult.viewDepthMin, viewDistanceLevels[world.input.viewDistanceParam]);
        } else {
            // culled items are forced to lower LoD (for eg. shadow casting)
            renderItem.updateActiveLod(cullingResult.viewDepthMin * 2.f, viewDistanceLevels[world.input.viewDistanceParam]);
        }
    }
}

// Draw to a special debug camera with a different
// Not part of the regular render
void RenderPasses::DrawDebugOverlays(World& world, Engine::Camera& cameraToDebug, bool drawCameraFrustum) {
    auto profileHandle = Profiler::StartCpu("RenderPasses::DrawDebugOverlays");
    auto gpuProfileHandle = Profiler::StartGpu("RenderPasses::DrawDebugOverlays");

    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
    //glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    debugWireframeMaterial.use();
    if(drawCameraFrustum) {
        debugWireframeMaterial.SetColor(glm::vec3(1.0,0.0,0.0));
        debugWireframeMaterial.SetModel(cameraToDebug.invCamera);
        Engine::DrawUtil::DrawCubeNdc();
    }
    
    auto lightView = world.registry.view<Engine::DirectionalLight>();
    for(auto entity : lightView) {
        auto& light = lightView.get<Engine::DirectionalLight>(entity);
        glm::vec3 rgb = glm::vec3(1,0.5,0);
        for(auto lsm : light.lightSpaceMatrices) {                
            glm::mat4 invLsm = glm::inverse(lsm);
            debugWireframeMaterial.SetColor(rgb);
            debugWireframeMaterial.SetModel(invLsm);
            Engine::DrawUtil::DrawCubeNdc();

            rgb = glm::vec3(rgb.z,rgb.x,rgb.y);
        }
    }
    
    glEnable(GL_DEPTH_TEST);
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

    sun.MakeLightSpaceMatrices(world, cameraMain);

    auto lightUniformData = LightUniformData {
        cameraMain.view,
        glm::vec4(sun.direction,1.f),
        glm::vec4(sun.color,1.f),

    };

    for(int i =0; i<sun.lightSpaceMatrices.size(); i++) {
        lightUniformData.lightSpaceMatrices[i] = sun.lightSpaceMatrices[i];
    }

    lightUniformData.cascadeCount = world.input.drawShadows() ? sun.NumCascades() : 0;
    for(int i =0; i<sun.cascadeLevels.size(); i++) {
        lightUniformData.cascadePlaneDistances[i] = glm::vec4(sun.cascadeLevels[i],sun.cascadeLevels[i],sun.cascadeLevels[i],sun.cascadeLevels[i]);
    } 

    lightUniforms.Set(&lightUniformData);
    lightUniforms.BindBase(1);

    // No face culling for shadows right now, because it doesn't work with my 
    // non-manifold terrain mesh
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    auto& material = sun.UseShadowMaterial();
            
    // Draw meshes
    auto shadowCasterView = world.registry.view<RenderItem,Transform,Engine::SurvivedLightCullingTag>();
    for(auto entity : shadowCasterView) {
        auto [item,transform] = shadowCasterView.get(entity);
        if(!item.ShouldDraw()) continue;

        material.SetModel(transform.global);
        item.mesh.Draw();
    }
    //WaterMaterial::DrawShadow(world);
    TerrainMaterial::DrawShadow(world);

    for(int i =0; i<sun.lightSpaceMatrices.size(); i++) {
        sun.shadowMap.PrepareFramebufferLayer(i);

        auto passUniformData = Engine::PassUniformData {
            sun.lightSpaceMatrices[i],
            static_cast<uint32_t>(Engine::RenderPassId::SHADOW)
        };
        passUniforms.Set(&passUniformData);
        passUniforms.BindBase(3);
        
        auto& gpuRender = world.GetSingle<Engine::GpuRender>();
        gpuRender.PreparePass(world, Engine::RenderPassId::SHADOW);
    }

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
        world.input.displacementScale
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

    auto view = world.registry.view<RenderItem,Transform,Engine::CullingResult,Engine::AABB,Engine::OpaqueRenderTag>();

    // depth prepass
    if(world.input.prepass) {
        prepassMaterial.use();
        for(auto entity : view) {
            auto [item,transform,cullingResult,aabb] = view.get(entity);
            if(!item.prepass || !cullingResult.viewResult) continue;

            prepassMaterial.SetModel(transform.global);

            // bind and draw mesh
            item.mesh.Draw();
        }
    }

    if(world.input.previewTriangleDensity) {
        debugTriangleDensityMaterial.use();        
        for(auto entity : view) {
            auto [item,transform,cullingResult,aabb] = view.get(entity);
            if(!cullingResult.viewResult) continue;
            debugTriangleDensityMaterial.SetModel(transform.global);   
            item.mesh.Draw();
        }
    } else {
        for(auto entity : view) {
            auto [item,transform,cullingResult,aabb] = view.get(entity);
            if(!cullingResult.viewResult) continue;

            // bind and configure material
            item.material->use();
            
            item.material->SetModelAndNormalMatrix(transform.global);
            item.material->SetTime(world.shaderAnimTime+item.animationPhaseOffset);

            world.skybox.Bind(6);

            // bind and draw mesh
            item.mesh.Draw();

            // bind
            item.material->unbind();
        }
    }

    if(world.input.wireFrame) {
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);

        debugWireframeMaterial.use(); 
        debugWireframeMaterial.SetColor(glm::vec3(0.0,0.0,0.0));
        
        for(auto entity : view) {
            auto [item,transform,cullingResult,aabb] = view.get(entity);
            if(!cullingResult.viewResult) continue;
            debugWireframeMaterial.SetModel(transform.global);   
            item.mesh.Draw();
        }

        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
    }
    
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

    TerrainMaterial::DrawMain(world);

    auto passUniformData = Engine::PassUniformData {
        cullingCamera.VP,
        static_cast<uint32_t>(Engine::RenderPassId::OPAQUE)
    };
    passUniforms.Set(&passUniformData);
    passUniforms.BindBase(3);
    
    auto& gpuRender = world.GetSingle<Engine::GpuRender>();
    gpuRender.PreparePass(world, Engine::RenderPassId::OPAQUE);

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

    auto view = world.registry.view<RenderItem,Transform,Engine::CullingResult,Engine::AABB,Engine::TransparentRenderTag>();
    for(auto entity : view) {
        auto [item,transform,cullingResult,aabb] = view.get(entity);
        if(!cullingResult.viewResult) continue;

        // bind and configure material
        item.material->use();

        glActiveTexture(GL_TEXTURE0 + 5);
        sun.shadowMap.BindDepthMap();
        
        item.material->SetModelAndNormalMatrix(transform.global);
        item.material->SetTime(world.shaderAnimTime+item.animationPhaseOffset);

        world.skybox.Bind(6);

        // bind and draw mesh
        item.mesh.Draw();

        // bind
        item.material->unbind();

        if(drawAABB) {
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
            glDisable(GL_CULL_FACE);

            debugWireframeMaterial.use();
            glm::vec3 extent = aabb.max - aabb.min;
            glm::vec3 offset = aabb.min + (extent / 2.f);
            glm::mat4 aabbT = transform.global * glm::translate(glm::mat4(1.0),offset) * glm::scale(glm::mat4(1.0), extent/2.f);
            
            debugWireframeMaterial.SetModel(aabbT);
            Engine::DrawUtil::DrawCube();

            glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_CULL_FACE);
        }
    }
}