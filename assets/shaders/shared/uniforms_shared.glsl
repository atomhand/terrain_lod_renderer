#ifndef UNIFORMS_SHARED_GLSL
#define UNIFORMS_SHARED_GLSL

layout (std140, binding=0) uniform Matrices
{
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    mat4 inverseViewProjection;
    vec4 viewPos;
    float nearPlane;
    float farPlane;
    vec2 screenDimensions;
};

layout (std140, binding=1) uniform LightUniformData {
    // The view matrix used to compute the fragment depth used to select light cascade
    // Typically it's the same as the main camera VP
    // It may be separated when the debug camera mode is active
    mat4 cascadeDeterminationView;

    vec4 lightDirection;
    vec4 lightColor;
    mat4 lightSpaceMatrices[16];
    vec4 cascadePlaneDistances[16];
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
};

layout (std140, binding=3) uniform PassUniformData {
    mat4 cullingVP;
    uint renderPassId;
};

#endif