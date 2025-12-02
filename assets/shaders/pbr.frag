
#version 420

out vec4 outputColor;
in vec2 TexCoords;

#include "pbr_shared.glsl"

void main()
{
    vec3 N = normalize(Normal);

    vec3 color = CalculateLighting(N,mAlbedo,mAo,mRoughness,mMetallic);

    outputColor = vec4(ToneMap(color),1.0);
}