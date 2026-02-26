#version 460
#inject
// Tom Kellett 2025

#include "shared/uniforms_shared.glsl"
#include "corepass/instancing_shared.glsl"

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;

void main()
{
    mat4 model;
    Vertex vert;
    GetModelVertex(model,vert);

    // revisit
    mat3 normalMatrix = mat3(transpose(inverse(model)));

    TexCoords = vert.uv;
    WorldPos = vec3(model * vec4(vert.position, 1.0));
    Normal = normalMatrix * vert.normal;

    gl_Position = projection * view * vec4(WorldPos.xyz,1.0);
}