#pragma once
#include <glm/glm.hpp>
#include "scenegraph.h"
#include "camera.h"

namespace Engine {
    struct  Input {
    public:
        glm::vec2 mousePos;
        float scrollDelta;

        float yAxisKeyDelta;
        float xAxisKeyDelta;

        float animSpeed = 3.0;

        float deltaTime;

        bool wireFrame = false;
        bool testQuad = false;
    };

    class World {
    public:
        Input input;
        SceneGraph scenegraph;
        float time;

        float animDeltaTime() {
            return input.deltaTime * input.animSpeed * 0.2f;
        }

        Camera* cameraMain() {
            auto cameras = scenegraph.Filter<Camera>();
            assert(cameras.size() > 0); // Can't be missing a main camera
            return cameras[0];
        }
    };
}