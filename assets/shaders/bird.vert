#version 460
#inject
// TBN snippet from: https://learnopengl.com/Advanced-Lighting/Normal-Mapping

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;

#include "shared/uniforms_shared.glsl"
#include "corepass/instancing_shared.glsl"

layout(binding = 6, std430) readonly buffer instancingSsbo {
    float animOffsets[];
};

void main()
{
    mat4 model;
    Vertex vert;
    uint materialInstanceId = GetModelVertex(model,vert);

    float animOffset = animOffsets[materialInstanceId];

    float animTime = time + animOffset;

    // Very basic wing flapping by vertex diplsacement
    float shift = 0.75 * max(0.,abs(vert.position.x)-0.4) * (sin(animTime*3.0)*2.0 - 1.0);
    float d = length(vert.position);
    vec3 pos = normalize(vert.position + vec3(0,shift,0)) * d;

#ifdef SHADOW_PASS
    gl_Position = cullingVP * model * vec4(pos,1.0);
#else
    mat3 normalMatrix = mat3(transpose(inverse(model)));

    TexCoords = vert.uv;//
    WorldPos = vec3(model * vec4(pos, 1.0));
    Normal = normalMatrix * vert.normal;

    gl_Position = projection * view * vec4(WorldPos.xyz,1.0);
#endif
}