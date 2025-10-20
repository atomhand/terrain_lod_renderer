#ifndef DEFERRED_SHARED_GLSL
#define DEFERRED_SHARED_GLSL

#include "shared/uniforms_shared.glsl"

layout(location = 0) out vec3 gNormal;
layout(location = 1) out vec3 gAlbedo;
layout(location = 2) out vec3 gArm;

void WriteGBuffer(vec3 worldPos, vec3 normal, vec3 albedo, float ao, float roughness, float metallic) {
    //gPosition = worldPos - viewPos.xyz;
    gNormal = normal;
    gAlbedo = albedo;
    gArm = vec3(ao,roughness,metallic);
}

#endif