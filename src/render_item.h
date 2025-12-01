#pragma once
#include <vector>
#include <memory>
#include "material.h"
#include "mesh.h"
#include "texture.h"
#include "scenegraph.h"

enum RenderPass { OPAQUE, TRANSPARENT };

class RenderItem : public Engine::SceneNode {
public:
    std::shared_ptr<Engine::Material> material;
    RenderPass renderPass;

    Engine::Mesh mesh;

    bool shadowEnabled = true;
    bool enableCulling = true;

    bool casts_shadow() {
        return shadowEnabled && renderPass == RenderPass::OPAQUE;
    }
};