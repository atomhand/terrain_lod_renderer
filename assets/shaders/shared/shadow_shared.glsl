#ifndef SHADOW_SHARED_GLSL
#define SHADOW_SHARED_GLSL

// TOm Kellett 2025 
// Shadow code is not a direct copy, but was written with reference to
// learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
// https://learnopengl.com/Guest-Articles/2021/CSM
// https://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/

layout(binding=5) uniform sampler2DArrayShadow shadowMap;

#include "shared/uniforms_shared.glsl"
#include "shared/hash_shared.glsl"

// Slightly modified from https://learnopengl.com/Guest-Articles/2021/CSM
int GetCascadeLayer(vec3 pos) {
    vec4 fragPosViewSpace = cascadeDeterminationView * vec4(pos,1.0);
    float depthValue = -fragPosViewSpace.z;

    int layer = cascadeCount-1;
    for(int i =0; i<cascadeCount; i++) {
        if(depthValue < cascadePlaneDistances[i].x) {
            layer = i;
            break;
        }
    }
    return layer;
}

// For debugging
vec3 getCascadeColor(int cascade) {
    vec3 c = vec3(1,0.5,0);
    for(int i =0; i<cascade; i++) {
        c = c.zxy;
    }
    return c;
}

// returns 0.0 if fully occluded
float ShadowAtSurface(vec3 L, vec3 wPos) {
    if(cascadeCount == 0) {
        return 1.f;
    }
    int layer = GetCascadeLayer(wPos);

    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(wPos, 1.0);
    vec3 ndc = fragPosLightSpace.xyz/fragPosLightSpace.w;
    vec3 uv = NdcToUv(ndc);
    float fragDepth =  uv.z;

    if(uv != clamp(uv, vec3(-1.f,-1.f,0.f),vec3(1.0f)))
        return 1.;

    // TODO - reivist dynamic bias Bias calculation
    float bias = 0.01;// max(0.005 * (1.0 - dot(geometryNormal, L)), 0.0005);
    // Apply bias
    // don't let the bias increase depth over 1
    fragDepth = min(1.0-1e-6,fragDepth+bias);

    // Stratified Poisson sampling

    // The idea and poisson kernel are from here
    // https://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/

    // Compared to the reference, I improved the visual result by using a shadowSampler to benefit
    // from hardware texture filtering and using a better hash to eliminate artefacts in the
    // noise pattern

    vec2 poissonDisk[16] = vec2[]( 
        vec2( -0.94201624, -0.39906216 ), 
        vec2( 0.94558609, -0.76890725 ), 
        vec2( -0.094184101, -0.92938870 ), 
        vec2( 0.34495938, 0.29387760 ), 
        vec2( -0.91588581, 0.45771432 ), 
        vec2( -0.81544232, -0.87912464 ), 
        vec2( -0.38277543, 0.27676845 ), 
        vec2( 0.97484398, 0.75648379 ), 
        vec2( 0.44323325, -0.97511554 ), 
        vec2( 0.53742981, -0.47373420 ), 
        vec2( -0.26496911, -0.41893023 ), 
        vec2( 0.79197514, 0.19090188 ), 
        vec2( -0.24188840, 0.99706507 ), 
        vec2( -0.81409955, 0.91437590 ), 
        vec2( 0.19984126, 0.78641367 ), 
        vec2( 0.14383161, -0.14100790 ) 
    );

    vec3 texelSize = 1.0 / textureSize(shadowMap, 0);
    float inverseOcclusion = 0.0;
    for(int i =0; i<4; i++) {
        int index = int(16.f*hash14(vec4(wPos.xyz,i)));
        inverseOcclusion += 0.25 * texture(shadowMap, vec4(uv.xy + poissonDisk[index] * texelSize.xy,layer,fragDepth)).r;
    }

    return inverseOcclusion;
}

// Shadow for non-surface point (such as in a volume)
// returns 1.0 if fully occluded
float ShadowAtPos(vec3 wPos) {
    if(cascadeCount == 0) {
        return 1.f;
    }
    
    int layer = GetCascadeLayer(wPos);

    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(wPos, 1.0);
    vec3 ndc = fragPosLightSpace.xyz/fragPosLightSpace.w;
    vec3 uv = NdcToUv(ndc);

    float fragDepth =  uv.z;

    float bias = 0.01;

    if(uv != clamp(uv, vec3(-1.f,-1.f,0.f),vec3(1.0f)))
        return 1.;

    return texture(shadowMap, vec4(uv.xy,layer,fragDepth+bias)).r;
}

#endif