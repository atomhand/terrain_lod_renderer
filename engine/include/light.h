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
        glm::vec3 color = glm::vec3(1.0,1.0,1.0);
    };

    class DirectionalLight : public SceneNode {
        public:
            glm::vec3 color = glm::vec3(1.0,1.0,1.0);
            glm::vec3 direction = glm::normalize(glm::vec3(4.0,-2.0,4.0));
    };
}