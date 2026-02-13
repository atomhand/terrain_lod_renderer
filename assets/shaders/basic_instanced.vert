#version 460
#inject
// Tom Kellett 2025

#include "shared/uniforms_shared.glsl"
#include "corepass/instancing_shared.glsl"
#include "shared/curvature_shared.glsl"

out vec3 WorldPos;

void main()
{
    mat4 model;
    Vertex vert;
    GetModelVertex(model,vert);
    
    WorldPos = vec3(model * vec4(vert.position, 1.0));

    gl_Position = projection * view * vec4(getCurvedPosition(WorldPos.xyz),1.0);
}