#pragma once
#include "world.h"
#include "rts_camera.h"
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "render_item.h"
#include "mesh.h"
#include "texture.h"

// Container for the demo data so it can easily be passed between parts of the application
// As a future extension, the World class will have a dynamic registry (ECS pattern)
// removing the need to manually implement the stored data types in this container
class DemoWorld : public Engine::World {
public:

    RtsCamera camera;
    glm::vec4 lightPos = glm::vec4(glm::normalize(glm::vec3(1.0f,0.8f,0.0f)), 2.0f); // light intensity packed into W

    Engine::Texture* testTex;
};