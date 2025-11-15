#pragma once

#include "application.h"
#include "ui.h"
#include <glad/glad.h>

namespace RenderPasses {
    void clear(const Engine::Application& app, Ui& ui) {
        // Rendering
        int display_w, display_h;
        app.getFramebufferSize(display_w,display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(ui.clear_color.x * ui.clear_color.w, ui.clear_color.y * ui.clear_color.w, ui.clear_color.z * ui.clear_color.w, ui.clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    
};