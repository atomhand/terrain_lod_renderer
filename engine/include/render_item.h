#pragma once
#include <vector>
#include <memory>
#include "material.h"
#include "mesh.h"
#include "culling.h"
#include "scenegraph.h"

namespace Engine
{
    // namespace name

    enum RenderPass { OPAQUE, TRANSPARENT };

    class RenderItem : public SceneNode {
    public:
        std::shared_ptr<Material> material;
        RenderPass renderPass;

        Mesh mesh;

        bool shadowEnabled = true;
        bool enableCulling = true;

        AABB aabb() { return mesh.aabb; }

        bool casts_shadow() {
            return shadowEnabled && renderPass == RenderPass::OPAQUE;
        }
    }; 
}