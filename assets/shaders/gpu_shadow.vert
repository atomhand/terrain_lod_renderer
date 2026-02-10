#version 460
#inject
// Tom Kellett 2025

#include "shared/uniforms_shared.glsl"
#include "corepass/instancing_shared.glsl"

void main()
{
    mat4 model;
    Vertex vert;
    GetModelVertex(model,vert);

    gl_Position = cullingVP * model * vec4(vert.position,1.0);
}