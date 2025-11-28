#pragma once
#include <vector>
#include "shader.h"
#include "mesh.h"
#include "texture.h"

// group instances sharing the same mesh and shader for more efficient rendering
// (TODO - implement instanced rendering)
struct MaterialRenderGroup
{
    //MaterialRenderGroup & operator=(const MaterialRenderGroup&) = delete;
    //MaterialRenderGroup(const MaterialRenderGroup&) = delete;
public:
    Engine::Shader shader;
    std::vector<Engine::Texture> textures;
    std::vector<glm::mat4x4> transforms;
    std::vector<glm::vec4> colours;

    float shininess = 8.0;

    Engine::Mesh* mesh;

    MaterialRenderGroup(Engine::Shader shader, Engine::Mesh* mesh) : shader(shader), mesh(mesh) {

    };

    ~MaterialRenderGroup() {
    }
};