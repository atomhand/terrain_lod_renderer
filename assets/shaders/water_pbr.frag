#version 420

out vec4 outputColor;
//in vec2 TexCoords;

#include "pbr_shared.glsl"

layout(binding=0) uniform sampler2D normalMap1;
layout(binding=1) uniform sampler2D normalMap2;

uniform float time;

void main()
{
    // For a wave/ripple effect I combine 2 scrolling normal maps
    vec2 uv = WorldPos.xz / 16.0;
    float anim = (time*0.25) * 0.5;
    vec3 n1 = texture(normalMap1,uv + vec2(0.25,0.25)*anim).xzy * 2.0 - 1.0;
    vec3 n2 = texture(normalMap2,uv + vec2(-0.25,0.25)*anim).xzy * 2.0 - 1.0;

    vec3 N = normalize(n1 + n2);
    
    vec3 color = CalculateLighting(N,matAlbedo);
    
    outputColor = vec4(ToneMap(color),1.0);
}