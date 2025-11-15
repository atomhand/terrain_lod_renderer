#pragma once

#include <glm/glm.hpp>
#include "application.h"

class Ui
{
private:
    bool show_demo_window;
    bool show_another_window;

public:
    glm::vec4 clear_color = glm::vec4(0.45f, 0.55f, 0.60f, 1.00f);

    void init(Engine::Application &window);
    void frame_update(Engine::Application &window);
};