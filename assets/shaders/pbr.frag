
#version 420
// Tom Kellett 2025

#include "shared/uniforms_shared.glsl"

#include "shared/deferred_shared.glsl"

in vec2 TexCoords;

uniform mat4 view;

in vec3 WorldPos;
in vec3 Normal;

// material parameters
uniform vec3 mAlbedo;
uniform float mMetallic;
uniform float mRoughness;
uniform float mAo;

void main()
{
    vec3 N = normalize(Normal);
    WriteGBuffer(WorldPos,N,mAlbedo,mAo,mRoughness,mMetallic);
}