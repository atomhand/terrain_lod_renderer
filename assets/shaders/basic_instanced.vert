#version 460
#inject

#include "shared/uniforms_shared.glsl"
#include "corepass/instancing_shared.glsl"

out vec3 WorldPos;

void main()
{
    mat4 model;
    Vertex vert;
    GetModelVertex(model,vert);
    
    WorldPos = vec3(model * vec4(vert.position, 1.0));

    gl_Position = projection * view * vec4(WorldPos.xyz,1.0);
}