#version 460
#inject
// Tom Kellett 2025

#include "shared/uniforms_shared.glsl"

#define TERRAIN_VERTEX
#include "terrain_shared.glsl"

layout (location = 0) in vec3 aPos;

layout(binding=0) uniform sampler2DArray dataTex; // xyz normal, w height

layout(binding = 0, std430) readonly buffer ssbo1 {
    mat4 modelMatrices[];
};

layout(binding = 1, std430) readonly buffer ssbo2 {
    int terrainIndex[];
};

#include "shared/curvature_shared.glsl"

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;

out vec2 erosionFactor;

void main()
{
    vec4 t = textureLod(dataTex, vec3(aPos.xz,terrainIndex[gl_InstanceID]), 0);
    WorldPos = vec3(modelMatrices[gl_InstanceID] * vec4(aPos.x, aPos.y + t.w, aPos.z, 1.0));
    Normal = t.xyz;


#ifdef SHADOW_PASS
    gl_Position = vec4(getCurvedPosition(WorldPos.xyz),1.0);
#else
    // Temp - disable
    erosionFactor = vec2(0.0);
    vec3 displacedPos = WorldPos.xyz;
    if(displacementScale > 0.f && distance(WorldPos.xyz,viewPos.xyz) < 2048.f) {
        // Triplanar displacement
        TriplanarSample X, Y, Z;
        GetTriplanarSamples(WorldPos, Normal, X,Y,Z, erosionFactor);
        vec3 triplanarWeights = TriplanarWeights(Normal, X.h, Y.h, Z.h, erosionFactor);

        // Triplanar blend weights    
        float h = X.h * triplanarWeights.x + Y.h * triplanarWeights.y + Z.h * triplanarWeights.z - 0.5;
        displacedPos += Normal * h * displacementScale;
    }

    gl_Position = projection * view * vec4(getCurvedPosition(displacedPos),1.0);
#endif
}