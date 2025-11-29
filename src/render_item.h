#pragma once
#include <vector>
#include "shader.h"
#include "mesh.h"
#include "texture.h"
#include "scenegraph.h"

enum RenderPass { OPAQUE, TRANSPARENT };

class RenderItem : public Engine::SceneNode {
public:
    Engine::Shader shader;
    std::vector<Engine::Texture> textures;
    glm::vec4 colour;
    RenderPass renderPass;

    float shininess = 8.0;

    Engine::Mesh mesh;
};