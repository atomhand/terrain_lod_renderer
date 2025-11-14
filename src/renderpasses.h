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
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f,0.1f,0.25f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    static void materialRenderPass(DemoWorld& world, Engine::Application &app) {
        glEnable(GL_DEPTH_TEST);  
        glm::mat4 view = world.camera.get_view();
        for(MaterialRenderGroup group : world.material_render_groups) {
            group.shader.setCamera(world.camera);
            group.shader.use();

            for(int i=0; i<group.transforms.size(); i++) {
                auto transform = group.transforms[i];
                
                Mesh& mesh =group.mesh;
                
                group.shader.setMat4("model", transform);
                group.shader.setVec4("lightpos", glm::vec4(16.0f,16.0f,0.0f, 1.0f));

                if(i < group.colours.size()) {
                    auto colour = group.colours[i];
                    group.shader.setVec4("colour", colour);
                }

                glm::mat3 normalmatrix = glm::transpose(glm::inverse(glm::mat3(view * transform)));                
                group.shader.setMat3("normalmatrix", normalmatrix);
                
                glBindVertexArray(mesh.vao);
                glDrawElements(GL_TRIANGLES, mesh.count(), GL_UNSIGNED_INT, nullptr);
                glBindVertexArray(0);
            }

            for(auto transform : group.transforms) {
                Mesh& mesh =group.mesh;
                
                group.shader.setMat4("model", transform);
                group.shader.setVec4("lightpos", glm::vec4(16.0f,16.0f,0.0f, 1.0f));

                glm::mat3 normalmatrix = glm::transpose(glm::inverse(glm::mat3(view * transform)));                
                group.shader.setMat3("normalmatrix", normalmatrix);
                
                glBindVertexArray(mesh.vao);
                glDrawElements(GL_TRIANGLES, mesh.count(), GL_UNSIGNED_INT, nullptr);
                glBindVertexArray(0);
            }
        }
    }
};