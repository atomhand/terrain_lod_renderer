#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "demo_world.h"
#include "application.h"
#include "light.h"
#include "render_item.h"
#include "culling.h"
#include "render_item.h"

using Engine::RenderItem;
using Engine::RenderPass;

class Terrain;
// 
class RenderPasses {
public:
    void Init(DemoWorld& world) {
        debugCamera = new Engine::Camera();
        debugCamera -> localTransform = glm::rotate(glm::mat4(1.), glm::radians(-45.f), glm::vec3(0.,1.,0.)) * glm::rotate(glm::mat4(1.), glm::radians(-90.f), glm::vec3(1.,0.,0.)) * glm::translate(glm::mat4(1.), glm::vec3(0.,0.,256.));
        debugCamera->far = 600.f;
        debugCamera->setFramebufferSize(DEBUG_WIDTH,DEBUG_HEIGHT);
        world.scenegraph.SetParent(debugCamera,world.scenegraph.root);

        glGenFramebuffers(1,&debugCameraFBO);

        auto outputTexObject = debugCameraOutput.textureObject();

        // Set up render texture
        glBindTexture(GL_TEXTURE_2D, outputTexObject);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, DEBUG_WIDTH, DEBUG_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        // set up depth buffer
        glGenRenderbuffers(1, &debugDepthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, debugDepthBuffer); 
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, DEBUG_WIDTH, DEBUG_HEIGHT);  
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        // Set up framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, debugCameraFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexObject, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, debugDepthBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    // Run all render passes
    void RunAll(DemoWorld& world, Engine::Application& app) {
        RetrieveData(world);
        DrawShadowMaps(world);
        if(world.input.testQuad != 0) {
            DrawDebug(world);
            PrepareMain(world,app);
            DrawDebugQuad(world);
        }
        else {
            PrepareMain(world,app);
            DrawOpaque(world);
            DrawTransparent(world);            
        }
    }
private:    
    GLuint debugDepthBuffer;

    std::vector<Engine::PointLight*> pointLights;
    std::vector<Engine::DirectionalLight*> directionalLights;

    std::vector<RenderItem*> opaqueItems;
    std::vector<RenderItem*> transparentItems;
    std::vector<RenderItem*> shadowCasters;
    std::vector<RenderItem*> shadowReceivers;
    std::vector<bool> terrainCullingResults;

    Engine::Camera* cameraMain;
    Engine::Camera* debugCamera;

    Engine::Shader debugShader = Engine::Shader("shaders/fullscreen_quad.vert","shaders/fullscreen_quad.frag");
    Engine::Shader debugWireframeShader = Engine::Shader("shaders/basic.vert","shaders/basic.frag");

    Engine::Texture debugCameraOutput;
    GLuint debugCameraFBO;
    const int DEBUG_WIDTH = 1024;
    const int DEBUG_HEIGHT = 768;

    Terrain* terrain;

    // Retrieve the world data used during rendering and cache
    // it for efficient access during the remaining passes
    void RetrieveData(DemoWorld& world);

    void DrawDebug(DemoWorld& world);

    void DrawShadowMaps(DemoWorld& world);

    void PrepareMain(DemoWorld& world, Engine::Application &app) {
            // Rendering
        int display_w, display_h;
        app.getFramebufferSize(display_w,display_h);
        cameraMain->setFramebufferSize(display_w,display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f,0.1f,0.25f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void DrawDebugQuad(DemoWorld& world) {
        debugShader.use();
        
        glActiveTexture(GL_TEXTURE0);
        if(world.input.testQuad == 1) {
            glBindTexture(GL_TEXTURE_2D,debugCameraOutput.textureObject());
        } else {
            if(directionalLights.size() > 0) {
                glBindTexture(GL_TEXTURE_2D,directionalLights[0]->shadowMap.depthMap());
            } else {
                glBindTexture(GL_TEXTURE_2D,0);
            }
        }

        Engine::DrawUtil::DrawQuad();
        
        glBindTexture(GL_TEXTURE_2D,0);
        glUseProgram(0);
    }

    void DrawOpaque(DemoWorld& world);

    void DrawTransparent(DemoWorld& world);
};