#version 460
#inject
// Tom Kellett 2025

#include "shared/uniforms_shared.glsl"
#include "corepass/instancing_shared.glsl"

#define TERRAIN_VERTEX
#include "terrain_shared.glsl"

layout(binding=0) uniform sampler2DArray dataTex; // xyz normal, w height

struct InstanceData {
    vec4 uvs[4];
};

layout(binding = 6, std430) readonly buffer instancingSsbo {
    InstanceData instanceData[];
};

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;
out vec3 debugColor;

out vec2 erosionFactor;

float TargetLodDepth(vec3 position, float geometricError) {
    float d = distance(lodViewPos.xyz,position);
    float screenSpaceErrorEstimate = (2.f * geometricError / d) * lodFovFactor;

    // assumption: Geometric error approximately halves with each higher LoD level
    // maybe it would be possible to actually measure this factor and create a better empirical heuristic?
    float lodDiff = log2(screenSpaceErrorEstimate / lodControlParam);
    
    return max(0,1.0 - lodDiff);
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
vec3 RemapUv(vec2 uv, InstanceData instanceHeader, int lodBias) {
    vec4 uvScale = instanceHeader.uvs[clamp(lodBias,0,3)];

    uv = uvScale.xy + uv*uvScale.z;

    // params to map (unmorphed) vertices to exact texels
    vec2 texel = vec2(1.0) / vec2(textureSize(dataTex,0).xy);
    vec2 dimension = vec2(textureSize(dataTex,0).xy);
    vec2 scale = (dimension-1)/dimension;

    float layer = uvScale.w;

    return vec3((uv + texel * 0.5f) * scale,layer);
}

vec3 GetBaseVertPos(vec3 inPosition, InstanceData instanceHeader, mat4 model) {
    float baseHeight = textureLod(dataTex, RemapUv(inPosition.xz,instanceHeader,0), 0).w;
    return (model * vec4(inPosition.x,baseHeight,inPosition.z,1.0f)).xyz;
}

void main()
{
    mat4 model;
    Vertex vert;
    uint materialInstanceId = GetModelVertex(model,vert);

    InstanceData instanceHeader = instanceData[materialInstanceId];

    vec3 initialVertPos = GetBaseVertPos(vert.position,instanceHeader,model);
    // dimensions of a node can be derived from the texture dimensions
    // (minor convenience, saves binding a uniforms)
    vec2 nodeDimension = (textureSize(dataTex,0).xy-1.0) / 2.0;
    float edgeLength = model[0][0] / nodeDimension.x * 1.73;
    float k = TargetLodDepth(initialVertPos,edgeLength);

    // vertexes on chunk edges should snap to a whole number LoD
    // This is effective at preventing cracks in practice
    if(min(vert.position.x,vert.position.z) <= 0.001f || max(vert.position.x,vert.position.z) >= 0.999f) {
        k = round(k);
    }

    vec2 p = morphVertex(vert.position.xz,nodeDimension,k);

    vec2 texel = vec2(1.0) / vec2(textureSize(dataTex,0).xy);

    vec4 t1 = textureLod(dataTex, RemapUv(p,instanceHeader,int(floor(k))), 0);
    vec4 t2 = textureLod(dataTex, RemapUv(p,instanceHeader,int(ceil(k))), 0);
    vec4 t = mix(t2,t1, 1.0-fract(k));
    WorldPos = vec3(model * vec4(p.x, t.w, p.y, 1.0));
    Normal = vec3(t.x,sqrt(1.0-t.x*t.x-t.y*t.y),t.y);
    //Normal = normalize(t.xyz);
#ifdef TERRAIN_HEATMAP
    debugColor = vec3(k,k,k) / 2.0f;
#endif

    vec3 displacedPos = WorldPos + Normal * t.z * displacementScale;
#ifdef SHADOW_PASS
    gl_Position = cullingVP * vec4(displacedPos.xyz,1.0);
#else
    gl_Position = projection * view * vec4(displacedPos,1.0);
#endif
}