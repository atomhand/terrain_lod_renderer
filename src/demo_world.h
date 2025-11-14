#pragma once
#include "world.h"
#include "mesh.h"
#include "rts_camera.h"
#include "windmill.h"
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

struct MaterialRenderGroup
{    
    Engine::Shader shader;
    std::vector<glm::mat4x4> transforms;
    std::vector<glm::vec4> colours;

    Mesh mesh;

    MaterialRenderGroup(Engine::Shader shader, Mesh mesh) : shader(shader), mesh(mesh) {

    };
};

// Container for the demo data so it can easily be passed between parts of the application
// As a future extension, the World class will have a dynamic registry (ECS pattern)
// removing the need to manually implement the stored data types in this container
class DemoWorld : public World {
public:
    RtsCamera camera;
    Windmill windmill;
    
    std::vector<MaterialRenderGroup> material_render_groups;
};