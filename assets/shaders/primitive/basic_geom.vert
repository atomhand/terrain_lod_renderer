// Tom Kellett 2025
#version 420
layout (location = 0) in vec3 aPos;

#include "shared/uniforms_shared.glsl"

uniform mat4 model;

void main()
{
    vec3 WorldPos = (model * vec4(aPos,1.0)).xyz;
    gl_Position = projection * view * vec4(WorldPos,1.0);
}