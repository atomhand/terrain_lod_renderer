#pragma once
#include <vector>
#include "shader.h"
#include "mesh.h"
#include "texture.h"
#include "scenegraph.h"

class RenderItem : public Engine::SceneNode {
public:
    Engine::Shader shader;
    std::vector<Engine::Texture> textures;
    glm::vec4 colour;

    bool transparent;

    float shininess = 8.0;

    Engine::Mesh mesh;
};

// group instances sharing the same mesh and shader for more efficient rendering
// (TODO - implement instanced rendering)
struct MaterialRenderGroup
{
public:
    Engine::Shader shader;
    std::vector<Engine::Texture> textures;
    std::vector<glm::mat4x4> transforms;
    std::vector<glm::vec4> colours;

    bool transparent;

    float shininess = 8.0;

    Engine::Mesh mesh;

    MaterialRenderGroup(Engine::Shader shader, Engine::Mesh mesh) : shader(shader), mesh(mesh) {

    };
};