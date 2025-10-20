// Tom Kellett 2025
#version 420
layout (location = 0) in vec3 aPos;

#include "shared/uniforms_shared.glsl"
#include "shared/curvature_shared.glsl"


uniform vec3 color;

out vec3 fColor;
out vec3 WorldPos;

uniform mat4 model;

void main()
{
    WorldPos = (model * vec4(aPos,1.0)).xyz;
    gl_Position = projection * view * vec4(getCurvedPosition(WorldPos),1.0);
    fColor = color;
    //gl_Position =  projection * view * model * vec4(aPos, 1.0);
}