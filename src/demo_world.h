#pragma once
#include "world.h"
#include "rts_camera.h"
#include "windmill.h"
#include "duck.h"
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "material_render_group.h"
#include "mesh.h"
#include "texture.h"

// Container for the demo data so it can easily be passed between parts of the application
// As a future extension, the World class will have a dynamic registry (ECS pattern)
// removing the need to manually implement the stored data types in this container
class DemoWorld : public World {
public:

    RtsCamera camera;
    Windmill windmill;
    Duck duck;
    glm::vec4 lightDir = glm::vec4(glm::normalize(glm::vec3(1.0f,0.8f,0.0f)), 2.0f); // light intensity packed into W

    Engine::Texture* testTex;
    
    std::vector<MaterialRenderGroup> material_render_groups;

    std::vector<Engine::Mesh*> meshes;

    DemoWorld() {
    }

    ~DemoWorld() {
        for(auto mesh : meshes) {
            delete mesh;
        }
    }
};