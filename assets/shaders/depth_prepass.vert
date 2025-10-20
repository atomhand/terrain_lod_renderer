#version 420
// Tom Kellett 2025

#include "shared/uniforms_shared.glsl"

layout (location = 0) in vec3 aPos;

#include "shared/curvature_shared.glsl"

uniform mat4 model;

void main()
{
    vec3 WorldPos = vec3(model * vec4(aPos, 1.0));

    gl_Position = projection * view * vec4(getCurvedPosition(WorldPos.xyz),1.0);
}