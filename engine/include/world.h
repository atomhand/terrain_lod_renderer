#pragma once
#include <glm/glm.hpp>
#include "scenegraph.h"
#include "camera.h"

namespace Engine {
    struct  Input {
    public:
        glm::vec2 mousePos;
        glm::vec2 mousePosDelta;
        float scrollDelta;

        glm::vec2 keyAxisDelta;

        float animSpeed = 3.0;

        float deltaTime;

        bool flyCamera = false;
        bool wireFrame = false;
        int testQuad = 0;
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
            for(auto camera : cameras) {
                if(camera->main)
                    return camera;
            }
            return cameras[0];
        }
    };
}