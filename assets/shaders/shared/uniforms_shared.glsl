#ifndef UNIFORMS_SHARED_GLSL
#define UNIFORMS_SHARED_GLSL

layout (std140, binding=0) uniform Matrices
{
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    mat4 inverseViewProjection;
    mat4 boundedInverseViewProjection; // vp without infinite far plane
    vec4 viewPos;
    vec4 lodViewPos; // typically the same as viewPos, different in debug modes
    vec2 screenDimensions;
    float nearPlane;
    float farPlane;
    float lodFovFactor;
    float lodControlParam;
};

layout (std140, binding=1) uniform LightUniformData {
    // The view matrix used to compute the fragment depth used to select light cascade
    // Typically it's the same as the main camera VP
    // It may be separated when the debug camera mode is active
    mat4 cascadeDeterminationView;

    vec4 lightDirection;
    vec4 lightColor;
    int cascadeCount;
};

layout (std140, binding=2) uniform MiscUniformData {
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


layout(std140, binding=3) uniform LightSpaceMatricesData {
    mat4 lightSpaceMatrices[16];
};

layout(std140, binding=4) uniform CascadePlaneDistancesData {
    vec4 cascadePlaneDistances[16];
};

layout (std140, binding=5) uniform PassCullingVPUniform {
    mat4 cullingVP;
};

layout (std140, binding=6) uniform PassRenderPassIdUniform {
    uint renderPassId;
    uint materialId;
};

#endif