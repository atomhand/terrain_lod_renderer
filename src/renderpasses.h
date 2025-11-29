#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "demo_world.h"
#include "application.h"

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

    static void itemsRenderPass(DemoWorld& world, Engine::Application &app) {
        glEnable(GL_DEPTH_TEST);  
        glm::mat4 view = world.camera.get_view();

        std::vector<RenderItem*> items = world.scenegraph.Filter<RenderItem>();

        for(RenderItem* item : items) {
            item->shader.use();
            item->shader.setCamera(world.camera);

            item->shader.setVec4("lightpos", view * glm::vec4(world.lightPos.x,world.lightPos.y,world.lightPos.z,1.0));
            item->shader.setFloat("shininess", item->shininess);

            if(item->transparent) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

            } else {
                glDisable(GL_BLEND);
            }

            for(int i =0; i<item->textures.size(); i++) {
                glActiveTexture(GL_TEXTURE0 + i);
                item->textures[i].bind();
            }            
            Engine::Mesh& mesh =item->mesh;
            auto transform = item->globalTransform;
            
            item->shader.setMat4("model", transform);
            item->shader.setVec4("colour", item->colour);

            glm::mat3 normalmatrix = glm::transpose(glm::inverse(glm::mat3(view * transform)));                
            item->shader.setMat3("normalmatrix", normalmatrix);
            
            glBindVertexArray(mesh.vao());
            glDrawElements(GL_TRIANGLES, mesh.count(), GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
            
            for(int i =0; i<item->textures.size(); i++) {
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D,0);
            }

            
            glUseProgram(0);
        }
    }
};