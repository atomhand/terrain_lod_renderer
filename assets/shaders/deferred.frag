#version 420

#include "shared/coordinate_shared.glsl"
#include "shared/uniforms_shared.glsl"
#include "shared/pbr_shared.glsl"

out vec4 FragColor;
  
in vec2 TexCoords;

layout(binding=0) uniform sampler2D gNormal;
layout(binding=1) uniform sampler2D gAlbedo;
layout(binding=2) uniform sampler2D gArm;
layout(binding=3) uniform sampler2D gDepth;

// https://therealmjp.github.io/posts/reconstructing-position-from-depth/
vec3 WorldPosFromDepth(vec2 uv, float depth) {
    vec3 ndc = UvToNdc(vec3(uv,depth));
    vec4 wPos = inverseViewProjection * vec4(ndc,1.0);
    return wPos.xyz / wPos.w;
}


void main()
{
    vec3 position = WorldPosFromDepth(TexCoords, texture(gDepth, TexCoords).x);
    vec3 normal = texture(gNormal, TexCoords).xyz;
    vec3 albedo = texture(gAlbedo, TexCoords).xyz;
    vec3 arm = texture(gArm, TexCoords).xyz;

    FragColor = vec4(CalculateLighting(position, normal, albedo, arm.x, arm.y, arm.z),1.0);
}