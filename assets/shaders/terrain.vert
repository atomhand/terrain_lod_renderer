#version 460
#inject
// Tom Kellett 2025

#include "shared/uniforms_shared.glsl"
#include "corepass/instancing_shared.glsl"

#define TERRAIN_VERTEX
#include "terrain_shared.glsl"

layout(binding=0) uniform sampler2DArray dataTex; // xyz normal, w height

#include "shared/curvature_shared.glsl"

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;

out vec2 erosionFactor;

void main()
{
    mat4 model;
    Vertex vert;
    uint materialInstanceId = GetModelVertex(model,vert);

    vec4 t = textureLod(dataTex, vec3(vert.position.xz,materialInstanceId), 0);
    WorldPos = vec3(model * vec4(vert.position.x, vert.position.y + t.w, vert.position.z, 1.0));
    Normal = t.xyz;


#ifdef SHADOW_PASS
    gl_Position = cullingVP * vec4(WorldPos.xyz,1.0);
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