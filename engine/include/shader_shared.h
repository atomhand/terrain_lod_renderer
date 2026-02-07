#pragma once
#include "world.h"
#include "camera.h"

namespace Engine {
    struct ViewUniformData {
        glm::mat4 view;
        glm::mat4 projection;
        glm::mat4 viewProjection;
        glm::mat4 inverseViewProjection;
        glm::vec4 viewPos;
        float nearPlane;
        float farPlane;
        glm::vec2 screenDimensions;

        ViewUniformData(Engine::Camera& camera, Transform& transform) {
            view = camera.view;
            projection = camera.projection;
            viewPos = glm::vec4(transform.position(),1.f);
            nearPlane = camera.near;
            farPlane = camera.far;

            viewProjection = projection * view;
            inverseViewProjection = glm::inverse(viewProjection);
            screenDimensions = glm::vec2(camera.width,camera.height);
        }
    };
    struct LightUniformData {
        glm::mat4 view;
        glm::vec4 lightDirection;
        glm::vec4 lightColor;
        glm::mat4 lightSpaceMatrices[16];
        glm::vec4 cascadePlaneDistances[16];
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