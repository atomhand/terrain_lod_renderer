// Tom Kellett 2025
#pragma once
#include <vector>
#include <memory>
#include "mesh.h"
#include "culling.h"
#include "material.h"

namespace Engine
{
    // namespace name

    enum RenderPass { OPAQUE, TRANSPARENT };

    // Temp
    struct OpaqueRenderTag {};
    struct TransparentRenderTag{};

    struct RenderItem {
    public:
        std::shared_ptr<Material> material;
        RenderPass renderPass;

        bool enabled = true;

        float lodOffset;

        // offsets time uniform that is passed to the shader
        float animationPhaseOffset;
        bool prepass = true;

        Mesh mesh;

        size_t activeLod = 0;

        bool ShouldDraw() {
            return enabled && mesh.IsValid();
        }

        void SetMesh(Mesh mesh) {
            this->mesh = mesh;
        }

        void updateActiveLod(float distance, float distancePerLod) {
            float distanceFactor = std::max(1.0f,std::abs(distance) / distancePerLod);
            size_t targetLod = std::floor(std::log2(distanceFactor) + lodOffset);
            mesh.SetLod(std::min(targetLod, mesh.NumLods()-1));
        }

        bool shadowCastingEnabled = true;
        bool enableCulling = true;

        bool CastsShadow() {
            return shadowCastingEnabled && renderPass == RenderPass::OPAQUE;
        }

        RenderItem(Mesh mesh) : mesh(mesh) {}

        RenderItem() {}
    }; 
}