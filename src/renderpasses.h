#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "demo_world.h"
#include "application.h"
#include "light.h"
#include "render_item.h"


class RenderPasses {
public:
    static void preRender(DemoWorld& world, Engine::Application &app) {
            // Rendering
        int display_w, display_h;
        app.getFramebufferSize(display_w,display_h);
        world.camera.setFramebufferSize(display_w,display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f,0.1f,0.25f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    static void opaqueRenderPass(DemoWorld& world, Engine::Application &app) {
        glEnable(GL_DEPTH_TEST);  
        glm::mat4 view = world.camera.get_view();

        std::vector<RenderItem*> items = world.scenegraph.Filter<RenderItem>();
        std::vector<Engine::PointLight*> pointLights = world.scenegraph.Filter<Engine::PointLight>();
        std::vector<Engine::DirectionalLight*> directionalLights = world.scenegraph.Filter<Engine::DirectionalLight>();

        glDisable(GL_BLEND);

        for(RenderItem* item : items) {
            if(item->renderPass != RenderPass::OPAQUE)
                continue;
            
            // bind and configure material
            item->material->use();
            item->material->setCamera(world.camera);
            item->material->setModel(item->globalTransform);
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

    static void transparentRenderPass(DemoWorld& world, Engine::Application &app) {
        glEnable(GL_DEPTH_TEST);  
        glm::mat4 view = world.camera.get_view();

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
            item->material->setCamera(world.camera);
            item->material->setModel(item->globalTransform);
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