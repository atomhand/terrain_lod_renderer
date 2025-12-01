
#version 420

out vec4 outputColor;
in vec2 TexCoords;
in vec3 Normal;

#include "pbr_shared.glsl"

void main()
{
    vec3 N = normalize(Normal);

    vec3 color = CalculateLighting(N,matAlbedo);

    outputColor = vec4(ToneMap(color),1.0);
}