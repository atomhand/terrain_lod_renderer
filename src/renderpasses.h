#pragma once
#include <glad/glad.h>
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
        glClear(GL_COLOR_BUFFER_BIT);
    }

    static void instancedRenderPasses(DemoWorld& world, Engine::Application &app) {
        for(InstancedRenderBatch group : world.instanced_render_batches) {
            group.shader.setCamera(world.camera);
            group.shader.use();
            for(auto transform : group.transforms) {
                group.shader.setMat4("transform", transform);
                glBindBuffer(GL_ARRAY_BUFFER, group.positionBufferObject);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

                glDrawArrays(GL_TRIANGLES, 0, group.num_indices);

                glDisableVertexAttribArray(0);
            }
        }
    }
};