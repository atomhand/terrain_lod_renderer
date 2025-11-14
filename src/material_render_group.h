#pragma once
#include <vector>
#include "shader.h"
#include "mesh.h"

// group instances sharing the same mesh and shader for more efficient rendering
// (TODO - implement instanced rendering)
struct MaterialRenderGroup
{    
    Engine::Shader shader;
    std::vector<glm::mat4x4> transforms;
    std::vector<glm::vec4> colours;

    float shininess = 8.0;

    Mesh mesh;

    MaterialRenderGroup(Engine::Shader shader, Mesh mesh) : shader(shader), mesh(mesh) {

    };
};