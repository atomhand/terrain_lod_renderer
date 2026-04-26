#version 420

#include "shared/uniforms_shared.glsl"

layout (location = 0) in vec3 aPos;

uniform mat4 model;

void main()
{
    vec3 WorldPos = vec3(model * vec4(aPos, 1.0));

    gl_Position = projection * view * vec4(WorldPos.xyz,1.0);
}