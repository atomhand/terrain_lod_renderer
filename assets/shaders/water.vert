#version 460
// Tom Kellett 2025

#include "shared/uniforms_shared.glsl"
#include "corepass/instancing_shared.glsl"

layout (location = 0) in vec3 aPos;

#include "shared/curvature_shared.glsl"

out vec3 WorldPos;
out vec4 ClipPos;

void main()
{
    WorldPos = vec3(GetModel() * vec4(aPos,1.0));

    ClipPos = projection * view * vec4(getCurvedPosition(WorldPos.xyz),1.0);

    gl_Position = ClipPos;
}