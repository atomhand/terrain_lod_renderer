#version 460
#inject
// Tom Kellett 2025

#include "shared/uniforms_shared.glsl"

// material parameters
uniform vec3 mAlbedo;
uniform float mMetallic;
uniform float mRoughness;
uniform float mAo;

#include "shared/deferred_shared.glsl"

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

layout(binding=0) uniform sampler2D albedoTex;

void main()
{
    vec3 N = normalize(Normal);
    vec3 albedo = mAlbedo * texture(albedoTex,TexCoords).xyz;    
    WriteGBuffer(WorldPos, N, albedo, mAo, mRoughness, mMetallic);
}