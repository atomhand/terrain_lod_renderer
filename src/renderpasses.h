#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "demo_world.h"
#include "application.h"
#include "light.h"
#include "render_item.h"


class RenderPasses {
public:
    static void DrawShadowMaps(DemoWorld& world, Engine::Application &app) {        
        std::vector<RenderItem*> items = world.scenegraph.Filter<RenderItem>();
        std::vector<Engine::DirectionalLight*> directionalLights = world.scenegraph.Filter<Engine::DirectionalLight>();

        // No face culling for shadows right now, because it doesn't work with my 
        // non-manifold terrain mesh
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        world.shadowShader.use();
        for(auto light : directionalLights) {
            light->PrepareRenderShadowmap();
            world.shadowShader.setMat4("lightSpaceMatrix", light->lightSpaceMatrix);

            // Draw meshes
            for(RenderItem* item : items) {
                if(!item->casts_shadow())
                    continue;
                
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

    static void PrepareMain(DemoWorld& world, Engine::Application &app) {
            // Rendering
        int display_w, display_h;
        app.getFramebufferSize(display_w,display_h);
        world.cameraMain().setFramebufferSize(display_w,display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f,0.1f,0.25f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    static void DrawOpaque(DemoWorld& world, Engine::Application &app) {
        if(world.input.wireFrame)
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
        
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        // using LEQUAL depth test lets us use a simple trick to draw the skybox
        glDepthFunc(GL_LEQUAL);
        glm::mat4 view = world.cameraMain().view();

        std::vector<RenderItem*> items = world.scenegraph.Filter<RenderItem>();
        std::vector<Engine::PointLight*> pointLights = world.scenegraph.Filter<Engine::PointLight>();
        std::vector<Engine::DirectionalLight*> directionalLights = world.scenegraph.Filter<Engine::DirectionalLight>();

        glDisable(GL_BLEND);

        for(RenderItem* item : items) {
            if(item->renderPass != RenderPass::OPAQUE)
                continue;
            
            // bind and configure material
            item->material->use();
            item->material->setCamera(world.cameraMain());
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
        world.skybox.DrawSkybox(world.cameraMain().view(),world.cameraMain().projection());
        glDepthMask(GL_TRUE);
    }

    static void DrawTransparent(DemoWorld& world, Engine::Application &app) {
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);  
        glm::mat4 view = world.cameraMain().view();

        std::vector<RenderItem*> items = world.scenegraph.Filter<RenderItem>();
        std::vector<Engine::PointLight*> pointLights = world.scenegraph.Filter<Engine::PointLight>();
        std::vector<Engine::DirectionalLight*> directionalLights = world.scenegraph.Filter<Engine::DirectionalLight>();
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

        for(RenderItem* item : items) {
            if(item->renderPass != RenderPass::TRANSPARENT)
                continue;

            // bind and configure material
            item->material->use();
            item->material->setCamera(world.cameraMain());
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