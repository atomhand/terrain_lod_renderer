#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "demo_world.h"
#include "application.h"
#include "light.h"



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

        glDisable(GL_BLEND);

        for(RenderItem* item : items) {
            if(item->renderPass != RenderPass::OPAQUE)
                continue;

            item->shader.use();
            item->shader.setCamera(world.camera);

            for(int i =0; i<pointLights.size() && i < 4; i++) {
                glm::vec4 pos = view * pointLights[i]->globalTransform * glm::vec4(0.f,0.f,0.f,1.f);
                item->shader.setVec3("lightPositions[" + std::to_string(i) + "]", glm::vec3(pos)/pos.w);
                item->shader.setVec3("lightColours[" + std::to_string(i) + "]", pointLights[i]->colour);
            }

            item->shader.setFloat("shininess", item->shininess);

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

    static void transparentRenderPass(DemoWorld& world, Engine::Application &app) {
        glEnable(GL_DEPTH_TEST);  
        glm::mat4 view = world.camera.get_view();

        std::vector<RenderItem*> items = world.scenegraph.Filter<RenderItem>();
        std::vector<Engine::PointLight*> pointLights = world.scenegraph.Filter<Engine::PointLight>();
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

        for(RenderItem* item : items) {
            if(item->renderPass != RenderPass::TRANSPARENT)
                continue;

            item->shader.use();
            item->shader.setCamera(world.camera);

            for(int i =0; i<pointLights.size() && i < 4; i++) {
                glm::vec4 pos = view * pointLights[i]->globalTransform * glm::vec4(0.f,0.f,0.f,1.f);
                item->shader.setVec3("lightPositions[" + std::to_string(i) + "]", glm::vec3(pos)/pos.w);
                item->shader.setVec3("lightColours[" + std::to_string(i) + "]", pointLights[i]->colour);
            }

            item->shader.setFloat("shininess", item->shininess);
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