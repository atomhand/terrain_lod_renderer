#version 420
// Tom Kellett 2025
// TBN snippet from: https://learnopengl.com/Advanced-Lighting/Normal-Mapping

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout(location = 3) in vec2 aTexCoords2;

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;

out vec3 testCol;

uniform float time;

#include "shared/uniforms_shared.glsl"

uniform mat4 model;
uniform mat3 normalMatrix;

#include "shared/curvature_shared.glsl"

void main()
{
    // Very basic wing flapping by vertex diplsacement
    float shift = 0.75 * max(0.,abs(aPos.x)-0.4) * (sin(time*3.0)*2.0 - 1.0);
    float d = length(aPos);
    vec3 pos = normalize(aPos + vec3(0,shift,0)) * d;


    TexCoords = aTexCoords;//
    WorldPos = vec3(model * vec4(pos, 1.0));
    Normal = normalMatrix * aNormal;


    gl_Position = projection * view * vec4(getCurvedPosition(WorldPos.xyz),1.0);
}