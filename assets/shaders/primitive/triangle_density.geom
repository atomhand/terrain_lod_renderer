#version 420

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

#include "shared/uniforms_shared.glsl"
#include "shared/coordinate_shared.glsl"

uniform mat4 model;

#ifdef TERRAIN_HEATMAP
in vec3 debugColor[];
#endif

out vec3 wireframeDist;
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

    for(int i =0; i<3; i++) {
        gl_Position = DepthInv(gl_in[i].gl_Position);
#ifdef TERRAIN_HEATMAP
        fColor = mix(col,debugColor[i],debugWireframe);
#else
        fColor = vec3(col);
#endif
        wireframeDist = vec3(0.f,0.f,0.f);
        wireframeDist[i] = 1.f;
        EmitVertex();
    }

    EndPrimitive();
}