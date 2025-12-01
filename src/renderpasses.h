#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "demo_world.h"
#include "application.h"
#include "light.h"
#include "render_item.h"

// 
class RenderPasses {    
public:
    // Run all render passes
    void RunAll(DemoWorld& world, Engine::Application& app) {
        RetrieveData(world);
        DrawShadowMaps(world);
        PrepareMain(world,app);
        DrawOpaque(world);
        DrawTransparent(world);
    }
private:
    std::vector<RenderItem*> renderItems;
    std::vector<Engine::PointLight*> pointLights;
    std::vector<Engine::DirectionalLight*> directionalLights;

    std::vector<RenderItem*> opaqueItems;
    std::vector<RenderItem*> transparentItems;
    std::vector<RenderItem*> shadowCasters;

    Engine::Camera* cameraMain;

    // Retrieve the world data used during rendering and cache
    // it for efficient access during the remaining passes
    void RetrieveData(DemoWorld& world) {
        cameraMain = world.cameraMain();

        renderItems = world.scenegraph.Filter<RenderItem>();
        pointLights = world.scenegraph.Filter<Engine::PointLight>();
        directionalLights = world.scenegraph.Filter<Engine::DirectionalLight>();

        opaqueItems.clear();
        transparentItems.clear();
        shadowCasters.clear();

        for(RenderItem* item : renderItems) {
            if(item->casts_shadow())
                shadowCasters.push_back(item);

            switch(item->renderPass) {
                case RenderPass::OPAQUE:
                    opaqueItems.push_back(item);
                    break;
                case RenderPass::TRANSPARENT:
                    transparentItems.push_back(item);
                    break;
            }
        }
    }

    void DrawShadowMaps(DemoWorld& world) {
        // No face culling for shadows right now, because it doesn't work with my 
        // non-manifold terrain mesh
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        world.shadowShader.use();
        for(auto light : directionalLights) {
            light->PrepareRenderShadowmap();
            world.shadowShader.setMat4("lightSpaceMatrix", light->lightSpaceMatrix);

            // Draw meshes
            for(RenderItem* item : shadowCasters) {                
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