#ifndef COORDINATE_SHARED_GLSL
#define COORDINATE_SHARED_GLSL

#include "shared/uniforms_shared.glsl"

vec3 UvToNdc(vec3 uv) {
    return vec3(uv.xy * 2.0 - 1.0,uv.z);
}

vec3 NdcToUv(vec3 ndc) {
    return vec3(ndc.xy * 0.5 + 0.5,ndc.z);
}

// https://therealmjp.github.io/posts/reconstructing-position-from-depth/
vec3 WorldPosFromDepth(vec2 uv, float depth) {
    vec3 ndc = UvToNdc(vec3(uv,depth));
    vec4 wPos = inverseViewProjection * vec4(ndc,1.0);
    return wPos.xyz / wPos.w;
}

float LinearizeDepth(float depth)
{
    return nearPlane/depth;
}

#endif