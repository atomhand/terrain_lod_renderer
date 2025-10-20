#ifndef TEXTURE_SHARED_GLSL
#define TEXTURE_SHARED_GLSL

#include "shared/hash_shared.glsl"

// https://discussions.unity.com/t/procedural-stochastic-texturing-prototype/732031/38
vec3 HeightBlend(vec3 inWeights, float h0, float h1, float h2, float contrast) {
    const float epsilon = 1.0f / 1024.0f;
    vec3 weights = vec3(inWeights.x * (h0+epsilon),
                        inWeights.y * (h1+epsilon),
                        inWeights.z * (h2+epsilon));
    
    float maxWeight = max(weights.x,max(weights.y,weights.z));
    float transition = contrast * maxWeight;
    float threshold = maxWeight - transition;
    float scale = 1.0f / transition;

    weights = clamp((weights-threshold)*scale, 0.f,1.f);
    weights /= (weights.x + weights.y + weights.z);
    return weights;
}

// Procedural Stochastic Textures by Tiling and Blending
void TriangleGrid(vec2 uv,
    out float w1, out float w2, out float w3,
    out ivec2 vertex1, out ivec2 vertex2, out ivec2 vertex3)
{
    // Scaling of the input
    uv *= 3.464; // 2 * sqrt(3)

    // Skew input space into simplex triangle grid
    const mat2 gridToSkewedGrid = mat2(1.0, 0.0, -0.57735027, 1.15470054);
    vec2 skewedCoord = gridToSkewedGrid * uv;

    // Compute local triangle vertex IDs and local barycentric coordinates
    ivec2 baseId = ivec2(floor(skewedCoord));
    vec3 temp = vec3(fract(skewedCoord), 0);
    temp.z = 1.0 - temp.x - temp.y;
    if(temp.z > 0.0) {
        w1 = temp.z;
        w2 = temp.y;
        w3 = temp.x;
        vertex1 = baseId;
        vertex2 = baseId + ivec2(0,1);
        vertex3 = baseId + ivec2(1,0);
    } else {
        w1 = -temp.z;
        w2 = 1.0 - temp.y;
        w3 = 1.0 - temp.x;
        vertex1 = baseId + ivec2(1,1);
        vertex2 = baseId + ivec2(1,0);
        vertex3 = baseId + ivec2(0,1);
    }
}

#endif