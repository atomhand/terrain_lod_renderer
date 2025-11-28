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

    static void materialRenderPass(DemoWorld& world, Engine::Application &app) {
        glEnable(GL_DEPTH_TEST);  
        glm::mat4 view = world.camera.get_view();
        for(MaterialRenderGroup& group : world.material_render_groups) {
            group.shader.use();
            group.shader.setCamera(world.camera);
            group.shader.setVec4("lightdir", glm::vec4(glm::normalize(glm::mat3(view) * glm::vec3(world.lightDir)),world.lightDir.w));
            group.shader.setFloat("shininess", group.shininess);

            if(group.transparent) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

            } else {
                glDisable(GL_BLEND);
            }

            for(int i =0; i<group.textures.size(); i++) {
                glActiveTexture(GL_TEXTURE0 + i);
                group.textures[i].bind();
            }

            for(int i=0; i<group.transforms.size(); i++) {
                auto transform = group.transforms[i];
                
                Engine::Mesh& mesh =group.mesh;
                
                group.shader.setMat4("model", transform);

                if(i < group.colours.size()) {
                    auto colour = group.colours[i];
                    group.shader.setVec4("colour", colour);
                }

                glm::mat3 normalmatrix = glm::transpose(glm::inverse(glm::mat3(view * transform)));                
                group.shader.setMat3("normalmatrix", normalmatrix);
                
                glBindVertexArray(mesh.vao());
                glDrawElements(GL_TRIANGLES, mesh.count(), GL_UNSIGNED_INT, nullptr);
                glBindVertexArray(0);
            }
            
            for(int i =0; i<group.textures.size(); i++) {
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D,0);
            }

            
            glUseProgram(0);
        }
    }
};