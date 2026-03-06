#pragma once
#include "world.h"
#include "camera.h"

namespace Engine {
    struct ViewUniformData {
        glm::mat4 view;
        glm::mat4 projection;
        glm::mat4 viewProjection;
        glm::mat4 inverseViewProjection;
        glm::mat4 boundedInverseViewProjection;
        glm::vec4 viewPos;
        glm::vec4 lodViewPos;
        glm::vec2 screenDimensions;
        float nearPlane;
        float farPlane;
        float lodFovFactor;
        float lodControlParam;
        float lodMorphs;

        ViewUniformData(Engine::Camera& camera, Transform& transform, glm::vec3 lodCameraViewPos, float lodControlParam, bool enableLodMorphs) : lodControlParam(lodControlParam) {
            view = camera.view;
            projection = camera.projection;
            boundedInverseViewProjection = camera.lightingInvVP;
            viewPos = glm::vec4(transform.position(),1.f);
            nearPlane = camera.near;
            farPlane = camera.far;
            lodViewPos = glm::vec4(lodCameraViewPos,1.0f);

            viewProjection = camera.VP;
            inverseViewProjection = glm::inverse(viewProjection);
            screenDimensions = glm::vec2(camera.width,camera.height);

            lodFovFactor = camera.lodFovFactor;
            lodMorphs = enableLodMorphs ? 1.f : 0.f;
        }
    };
    struct LightUniformData {
        glm::mat4 view;
        glm::vec4 lightDirection;
        glm::vec4 lightColor;
        int cascadeCount;
        float pssmFactor; // "parallel split shadow maps"
    };
    struct MiscUniformData {
        float heightFogFactor;
        float constantFogFactor;
        float heightFogTransitionStart;
        float heightFogTransitionDuration;
        float previewCascades;
        float previewNormalsMode;
        float enableStochasticBlending;
        float applyExposure;
        float displacementScale;
        float time;
    };

    // GPU DRIVEN RENDER

    struct DrawElementsIndirectCommand {
        unsigned int  count;
        unsigned int  instanceCount;
        unsigned int  firstIndex;
        unsigned int  baseVertex;
        unsigned int  baseInstance;
    };

    struct PassRenderPassIdUniform{
        uint32_t renderPassId;
        uint32_t materialId;
        uint32_t subPassId;

        PassRenderPassIdUniform(uint32_t renderPassId, uint32_t materialId, uint32_t subPassId) : renderPassId(renderPassId), materialId(materialId), subPassId(subPassId) {};
    };

    struct PassCullingVPUniform {
        glm::mat4 cullingVP;
        glm::mat4 cullingView;
        glm::vec4 frustum; // right, top, near, far

        PassCullingVPUniform(Camera& camera) {
            cullingVP = camera.VP;
            cullingView = camera.view;

            float tan_fov = std::tan(0.5f * camera.fov);

            glm::mat4 invProj= inverse(camera.projection);
            glm::vec4 x_near_h = invProj * glm::vec4(1.0,0.0,1.0,1.0);
            glm::vec4 y_near_h = invProj * glm::vec4(0.0,1.0,1.0,1.0);
            glm::vec4 z_near_h = invProj * glm::vec4(0.0,0.0,1.0,1.0);
            glm::vec4 z_far_h = invProj * glm::vec4(0.0,0.0,0.0,1.0);

            frustum = glm::vec4(
                x_near_h.x / x_near_h.w,//camera.aspect_ratio * camera.near * tan_fov, //x_near
                y_near_h.y / y_near_h.w, // y_near
                -camera.near, // near
                -camera.far // far
            );
        }
    };

    struct DebugUniformData {
        float debugWireframe;
        float debugTriangleDensity;

        DebugUniformData(World& world) : debugWireframe(world.input.wireFrame ? 1.f : 0.f), debugTriangleDensity(world.input.previewTriangleDensity ? 1.f : 0.f) {

        }
    };

    struct TerrainNoiseUniformData {
        float noisePeriod  = 30000.0;
        float noiseScale  = 4000.0;
        float noiseFoothillsFreq = 1.f;
        float noiseFoothillsScale = 0.25f;
        float noiseMountainFreq = 0.4f;
        float noiseMountainScale = 10.f;
        int noiseMountainExponent = 2;
        int noiseFoothillOctaves = 8;//8;
        int noiseMountainOctaves = 10;//10;
    };
}