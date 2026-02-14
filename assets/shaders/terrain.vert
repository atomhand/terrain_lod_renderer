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
out vec3 debugColor;

out vec2 erosionFactor;

float TargetLodDepth(vec3 position, float geometricError) {
    float d = distance(viewPos.xyz,position);
    float screenSpaceErrorEstimate = (2.f * geometricError / d) * lodFovFactor;

    // assumption: Geometric error approximately halves with each higher LoD level
    // maybe it would be possible to actually measure this factor and create a better empirical heuristic?
    float lodDiff = log2(screenSpaceErrorEstimate / lodControlParam);
    
    return 1.0 - lodDiff;
}

// Morph a uv towards a lower-LoD vertex
// morphK is the reduction in LoD steps

// TODO - shouldn't attempt to reduce the LoD below the minimum level
vec2 morphVertex(vec2 uv, vec2 nodeDimension, float morphK) {
    if(morphK <= 0.f) {
        return uv;
    }
    // Snap the uv/vertex K level LoDs down, where K 
    // is the whole part of morphK
    float kFloor = floor(morphK);
    vec2 scale = nodeDimension.xy / (1 << int(kFloor));
    vec2 fracPart = fract(uv*scale) / scale;
    if(kFloor > 0.f)
        uv -= fracPart;

    // For the fractional part of morphK, blend between
    // the snapped vertex and the next-lower LoD
    float kFract = morphK - kFloor;
    scale *= 0.5f;
    fracPart = fract(uv*scale)/scale;
    uv -= fracPart * kFract;

    return uv;
}

// Remap  UV in 0..1 range to target a specific subquadrant
// and such that (unmorphed) vertices correspond to exact texels
vec2 RemapUv(vec2 uv, uint materialInstanceId) {
    // target quadrant is encoded in materialInstanceId
    uint local = materialInstanceId % 4;

    // params to map (unmorphed) vertices to exact texels
    vec2 texel = vec2(1.0) / vec2(textureSize(dataTex,0).xy);
    vec2 dimension = vec2(textureSize(dataTex,0).xy);
    vec2 scale = (dimension-1)/dimension;

    return (uv + vec2(local>>1,local&1) + texel) * 0.5f * scale;
}

void main()
{
    mat4 model;
    Vertex vert;
    uint materialInstanceId = GetModelVertex(model,vert);

    uint texArrayIndex = materialInstanceId / 4;

    vec3 initialVertPos = vec3(model * vec4(vert.position.x,0.f,vert.position.z,1.0));
    // dimensions of a node can be derived from the texture dimensions
    // (minor convenience, saves binding a uniforms)
    vec2 nodeDimension = (textureSize(dataTex,0).xy-1.0) / 2.0;
    float edgeLength = model[0][0] / nodeDimension.x * 1.73;
    float k = TargetLodDepth(initialVertPos,edgeLength);
    vec2 p = morphVertex(vert.position.xz,nodeDimension,k);

    vec2 texel = vec2(1.0) / vec2(textureSize(dataTex,0).xy);

    vec4 t = textureLod(dataTex, vec3(RemapUv(p,materialInstanceId),texArrayIndex), 0);
    WorldPos = vec3(model * vec4(p.x, vert.position.y + t.w, p.y, 1.0));
    Normal = normalize(t.xyz);
#ifdef TERRAIN_HEATMAP
    debugColor = vec3(k,k,k) / 2.0f;
#endif

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