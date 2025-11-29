#pragma once
#include <glm/glm.hpp>
#include "scenegraph.h"

namespace Engine {
    struct  Input {
    public:
        glm::vec2 mousePos;
        float scrollDelta;

        float yAxisKeyDelta;
        float xAxisKeyDelta;

        float animSpeed = 3.0;

        float deltaTime;
    };

    class World {
    public:
        Input input;
        SceneGraph scenegraph;
    };
}