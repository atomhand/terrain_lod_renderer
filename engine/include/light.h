#pragma once
#include <memory>
#include <glm/glm.hpp>
#include "scenegraph.h"

namespace Engine {
    class PointLight : public SceneNode {
    private:
        struct LightData {

        };

        GLuint depthMapFBO;
        std::shared_ptr<LightData> data;
    public:
        glm::vec3 colour = glm::vec3(1.0,1.0,1.0);

        PointLight() {

        }
    };
}