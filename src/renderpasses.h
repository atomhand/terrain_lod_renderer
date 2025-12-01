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

    Engine::Camera* cameraMain;
    Engine::Camera* debugCamera;

    Engine::Shader debugShader = Engine::Shader("shaders/fullscreen_quad.vert","shaders/fullscreen_quad.frag");
    Engine::Shader debugWireframeShader = Engine::Shader("shaders/basic.vert","shaders/basic.frag");

    Engine::Texture debugCameraOutput;
    GLuint debugCameraFBO;
    const int DEBUG_WIDTH = 1024;
    const int DEBUG_HEIGHT = 768;

    // Retrieve the world data used during rendering and cache
    // it for efficient access during the remaining passes
    void RetrieveData(DemoWorld& world) {
        cameraMain = world.cameraMain();

        pointLights.clear();
        directionalLights.clear();
        opaqueItems.clear();
        transparentItems.clear();
        shadowCasters.clear();

        auto nodes = world.scenegraph.AllNodes();
        glm::mat4 VP = cameraMain->projection() * cameraMain->view();

        for(auto node : nodes) {
            if(RenderItem* item= dynamic_cast<RenderItem*>(node); item != nullptr) {
                if(item->casts_shadow())
                shadowCasters.push_back(item);

                if(item->enableCulling) {
                    glm::mat4 MVP = VP * item->globalTransform;
                    bool frustumTest = Engine::FrustumAABBTest(MVP, item->mesh.aabb);
                    if(!frustumTest)
                        continue;
                }

                switch(item->renderPass) {
                    case RenderPass::OPAQUE:
                        opaqueItems.push_back(item);
                        break;
                    case RenderPass::TRANSPARENT:
                        transparentItems.push_back(item);
                        break;
                }
            }
            else if(Engine::DirectionalLight* t= dynamic_cast<Engine::DirectionalLight*>(node); t != nullptr) {
                directionalLights.push_back(t);
            }  else if(Engine::PointLight* t= dynamic_cast<Engine::PointLight*>(node); t != nullptr) {
                pointLights.push_back(t);
            }
        }
    }

    void DrawDebug(DemoWorld& world) {        
        glBindFramebuffer(GL_FRAMEBUFFER, debugCameraFBO);
        glViewport(0, 0, DEBUG_WIDTH, DEBUG_HEIGHT);
        glClearColor(0.1f,0.1f,0.25f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto temp = cameraMain;
        cameraMain =debugCamera;
        cameraMain->setFramebufferSize(DEBUG_WIDTH,DEBUG_HEIGHT);
        DrawOpaque(world);
        DrawTransparent(world);
        cameraMain= temp;

        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
        //glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        debugWireframeShader.use();
        if(true) {
            debugWireframeShader.setMat4("view", debugCamera->view());
            debugWireframeShader.setMat4("projection", debugCamera->projection());
        } else {
            auto sun = directionalLights[0];            
            debugWireframeShader.setMat4("view", sun->lightView);
            debugWireframeShader.setMat4("projection", glm::ortho(-32.,32.,-32.,32.,0.1,256.));
        }

        // draw AABBS for items that survived culling
        for(auto item : opaqueItems) {
            auto aabb = item->mesh.aabb;
            glm::vec3 extent = aabb.max - aabb.min;
            glm::vec3 offset = aabb.min + (extent / 2.f);
            glm::mat4 aabbT = item->globalTransform* glm::translate(glm::mat4(1.0),offset) * glm::scale(glm::mat4(1.0), extent/2.f);
            
            debugWireframeShader.setMat4("model", aabbT);
            Engine::DrawUtil::DrawCube();
        }
        for(auto item : transparentItems) {
            auto aabb = item->mesh.aabb;
            glm::vec3 extent = aabb.max - aabb.min;
            glm::vec3 offset = aabb.min + (extent / 2.f);
            glm::mat4 aabbT = item->globalTransform* glm::translate(glm::mat4(1.0),offset) * glm::scale(glm::mat4(1.0), extent/2.f);
            
            debugWireframeShader.setMat4("model", aabbT);
            Engine::DrawUtil::DrawCube();
        }

        glm::mat4 invCamera = glm::inverse(cameraMain->projection() * cameraMain->view());
        debugWireframeShader.setVec3("color", glm::vec3(1.0,0.0,0.0));
        debugWireframeShader.setMat4("model", invCamera);
        Engine::DrawUtil::DrawCube();
        
        for(auto light : directionalLights) {
            glm::mat4 invCamera = glm::inverse(light->lightSpaceMatrix);
            debugWireframeShader.setVec3("color", glm::vec3(1.0,1.1,0.0));
            debugWireframeShader.setMat4("model", invCamera);
            Engine::DrawUtil::DrawCube();
        }
        
        glEnable(GL_DEPTH_TEST);
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void DrawShadowMaps(DemoWorld& world) {
        // No face culling for shadows right now, because it doesn't work with my 
        // non-manifold terrain mesh
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        std::vector<RenderItem*> shadowReceivers;
        for(auto item : opaqueItems) {
            shadowReceivers.push_back(item);
        }
        for(auto item : transparentItems) {
            shadowReceivers.push_back(item);
        }

        world.shadowShader.use();
        for(auto light : directionalLights) {
            auto culledShadowCasters = light->MakeLightSpaceMatrix(*cameraMain, shadowReceivers, shadowCasters);

            light->PrepareRenderShadowmap();
            world.shadowShader.setMat4("lightSpaceMatrix", light->lightSpaceMatrix);

            // Draw meshes
            for(size_t id : culledShadowCasters) {
                RenderItem* item = shadowCasters[id];               
                world.shadowShader.setMat4("model", item->globalTransform);

                Engine::Mesh& mesh =item->mesh;            
                glBindVertexArray(mesh.vao());
                glDrawElements(GL_TRIANGLES, mesh.count(), GL_UNSIGNED_INT, nullptr);
                glBindVertexArray(0);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        glUseProgram(0);
    }

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
                glBindTexture(GL_TEXTURE_2D,directionalLights[0]->depthMap().textureObject());
            }
        }

        Engine::DrawUtil::DrawQuad();
        
        glBindTexture(GL_TEXTURE_2D,0);
        glUseProgram(0);
    }

    void DrawOpaque(DemoWorld& world) {
        if(world.input.wireFrame)
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
        
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDisable(GL_BLEND);

        glm::mat4 view = cameraMain->view();

        for(RenderItem* item : opaqueItems) {            
            // bind and configure material
            item->material->use();
            item->material->setCamera(*cameraMain);
            item->material->setModel(item->globalTransform);
            item->material->shader.setFloat("time",world.time);
            for(int i =0; i<pointLights.size() && i < 4; i++) {
                item->material->setLight(*pointLights[i], view, i);
            }
            for(int i =0; i<directionalLights.size() && i < 4; i++) {
                item->material->setLight(*directionalLights[i], view, i);
            }

            world.skybox.Bind(6);

            // bind and draw mesh
            Engine::Mesh& mesh =item->mesh;            
            glBindVertexArray(mesh.vao());
            glDrawElements(GL_TRIANGLES, mesh.count(), GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);

            // bind
            item->material->unbind();
        }
        
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        world.skybox.DrawSkybox(cameraMain->view(),cameraMain->projection());
        glDepthMask(GL_TRUE);
    }

    void DrawTransparent(DemoWorld& world) {
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glm::mat4 view = cameraMain->view();

        for(RenderItem* item : transparentItems) {
            // bind and configure material
            item->material->use();
            item->material->setCamera(*cameraMain);
            item->material->setModel(item->globalTransform);
            item->material->shader.setFloat("time",world.time);
            for(int i =0; i<pointLights.size() && i < 4; i++) {
                item->material->setLight(*pointLights[i], view, i);
            }
            for(int i =0; i<directionalLights.size() && i < 4; i++) {
                item->material->setLight(*directionalLights[i], view, i);
            }

            // bind and draw mesh
            Engine::Mesh& mesh =item->mesh;            
            glBindVertexArray(mesh.vao());
            glDrawElements(GL_TRIANGLES, mesh.count(), GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);

            // bind
            item->material->unbind();
        }
    }
};