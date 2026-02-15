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

        ViewUniformData(Engine::Camera& camera, Transform& transform, glm::vec3 lodCameraViewPos, float lodControlParam) : lodControlParam(lodControlParam) {
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
        }
    };
    struct LightUniformData {
        glm::mat4 view;
        glm::vec4 lightDirection;
        glm::vec4 lightColor;
        int cascadeCount;
    };
    struct MiscUniformData {
        float heightFogFactor;
        float constantFogFactor;
        float heightFogTransitionStart;
        float heightFogTransitionDuration;
        float fakeCurvature;
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
}