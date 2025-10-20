#version 460
// Tom Kellett 2025

#include "shared/uniforms_shared.glsl"

layout (location = 0) in vec3 aPos;

layout(binding = 0, std430) readonly buffer ssbo1 {
    mat4 modelMatrices[];
};

#include "shared/curvature_shared.glsl"

out vec3 WorldPos;
out vec4 ClipPos;

void main()
{
    WorldPos = vec3(modelMatrices[gl_InstanceID] * vec4(aPos,1.0));

    ClipPos = projection * view * vec4(getCurvedPosition(WorldPos.xyz),1.0);

    gl_Position = ClipPos;
}