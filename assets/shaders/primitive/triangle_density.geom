// Tom Kellett 2025
#version 420

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

#include "shared/uniforms_shared.glsl"
#include "shared/curvature_shared.glsl"
#include "shared/coordinate_shared.glsl"

uniform mat4 model;

out vec3 fColor;

vec4 DepthInv(vec4 input) {
    return input;
}

void main()
{
    vec3 aNdc = gl_in[0].gl_Position.xyz / gl_in[0].gl_Position.w;
    vec3 bNdc = gl_in[1].gl_Position.xyz / gl_in[1].gl_Position.w;
    vec3 cNdc = gl_in[2].gl_Position.xyz / gl_in[2].gl_Position.w;

    vec2 a = NdcToUv(aNdc).xy * screenDimensions;
    vec2 b = NdcToUv(bNdc).xy * screenDimensions;
    vec2 c = NdcToUv(cNdc).xy * screenDimensions;

    // shoelace formula
    float area = 0.5 * ((a.x-c.x)*(b.y-a.y) - (a.x-b.x)*(c.y-a.y));
    float col = 1.0 / area;

    gl_Position = DepthInv(gl_in[0].gl_Position);
    fColor = vec3(col);
    EmitVertex();

    gl_Position = DepthInv(gl_in[1].gl_Position);
    fColor = vec3(col);
    EmitVertex();

    gl_Position = DepthInv(gl_in[2].gl_Position);
    fColor = vec3(col);
    EmitVertex();

    EndPrimitive();
}