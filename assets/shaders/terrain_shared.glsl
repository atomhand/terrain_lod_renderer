#ifndef TERRAIN_SHARED_GLSL
#define TERRAIN_SHARED_GLSL

#include "shared/texture_shared.glsl"

layout(binding=1) uniform sampler2DArray diffuseTex;
layout(binding=2) uniform sampler2DArray normalMap;
layout(binding=3) uniform sampler2DArray armMap; // ao, roughness, metalness
layout(binding=4) uniform sampler2DArray dispMap; 

struct TriplanarSample {
#ifndef TERRAIN_VERTEX
    vec3 albedo;
    vec3 normal;
    vec3 arm;
#endif
    float h; // displacement
};

TriplanarSample TriBlend(vec3 splat, TriplanarSample a, TriplanarSample b, TriplanarSample c) {
    TriplanarSample result;
#ifndef TERRAIN_VERTEX
    result.normal = splat.x * a.normal + splat.y * b.normal + splat.z * c.normal;
    result.albedo = splat.x * a.albedo + splat.y * b.albedo + splat.z * c.albedo;
    result.arm = splat.x * a.arm + splat.y * b.arm + splat.z * c.arm;
#endif
    result.h = splat.x * a.h + splat.y * b.h + splat.z * c.h;
    return result;
}

// Reoriented Normal Mapping
// http://discourse.selfshadow.com/t/blending-in-detail/21/18
// via https://bgolus.medium.com/normal-mapping-for-a-triplanar-shader-10bf39dca05a
vec3 rnmBlendUnpacked(vec3 n1, vec3 n2)
{
    n1 += vec3( 0,  0, 1);
    n2 *= vec3(-1, -1, 1);
    return n1*dot(n1, n2)/n1.z - n2;
}

void TriplanarUvs(vec3 worldPos, out vec2 uvX, out vec2 uvY, out vec2 uvZ) {
    float triplanarScale = 64.f;
    uvX = worldPos.zy / triplanarScale;
    uvY = worldPos.xz / triplanarScale;
    uvZ = worldPos.xy / triplanarScale;
    uvX.y += 0.5;
    uvZ.x += 0.5;
}

vec3 GetSplat(vec3 geometryNormal, vec3 worldPos, float hY1, float hY2, float hY3, vec2 erosionFactor) {
    float erosion =max(0.f, (0.1+erosionFactor.x)*smoothstep(-0.25,1.0, erosionFactor.y));
    
    float sandThreshold = 32.0 + erosion * 256.0;
    float snowThreshold = 256.0 ;//+  * 2048.0;

    vec3 splat;
    float slope = clamp(dot(geometryNormal,vec3(0.,1.,0.)),0.,1.);
    splat.y = max(0.f,1.0 - worldPos.y/sandThreshold) + erosion * 15.0; // Sand - low lying and flat areas

    float snowErosionFactor = 2.0 * smoothstep(1.0,-1.0,erosionFactor.y) - 1.0;

    splat.z = clamp((worldPos.y-snowThreshold)/2048.*snowErosionFactor,0.0,1.0) * (1.0 - splat.y); // In high altitudes grass is replaced with snow
    splat.x = 1.0 - splat.y - splat.z;

    splat = HeightBlend(splat, hY1, hY2, hY3, 0.4);

    return splat;
}

// Procedural Stochastic Textures by Tiling and Blending
TriplanarSample ProceduralTilingAndBlending(vec2 uv, float layer) {
    TriplanarSample result;
    // Get triangle info
    float w1, w2, w3;

    // Assing random offset to each triangle vertex
    vec2 uv1, uv2, uv3;
    if(enableStochasticBlending != 0.f) {
        ivec2 vertex1 , vertex2, vertex3;
        TriangleGrid(uv, w1, w2, w3, vertex1, vertex2, vertex3);

        uv1 = uv + hash22(vertex1);
        uv2 = uv + hash22(vertex2);
        uv3 = uv + hash22(vertex3);
    } else {
        uv1 = uv;
        w1 = 1.0;
        w2 = 0.0;
        w3 = 0.0;
    }

#ifndef TERRAIN_VERTEX
    // Precompute uv derivatives
    vec2 duvdx = dFdx(uv);
    vec2 duvdy = dFdy(uv);

    float h1 = textureGrad(dispMap, vec3(uv1,layer), duvdx, duvdy).r;
    float h2 = textureGrad(dispMap, vec3(uv2,layer), duvdx, duvdy).r;
    float h3 = textureGrad(dispMap, vec3(uv3,layer), duvdx, duvdy).r;
#else
    float h1 = textureLod(dispMap, vec3(uv1,layer), 0).r;
    float h2 = textureLod(dispMap, vec3(uv2,layer), 0).r;
    float h3 = textureLod(dispMap, vec3(uv3,layer), 0).r;
#endif

    vec3 weights = HeightBlend(vec3(w1,w2,w3), h1, h2, h3, 0.05);
    //weights = pow(weights,vec3(2,2,2));
    //weights /= dot(weights, vec3(1,1,1));

#ifndef TERRAIN_VERTEX
    // Fetch input
    vec3 albedo1 = weights.x > 0.01 ? textureGrad(diffuseTex, vec3(uv1,layer), duvdx, duvdy).rgb : vec3(0);
    vec3 albedo2 = weights.y > 0.01 ? textureGrad(diffuseTex, vec3(uv2,layer), duvdx, duvdy).rgb : vec3(0);
    vec3 albedo3 = weights.z > 0.01 ? textureGrad(diffuseTex, vec3(uv3,layer), duvdx, duvdy).rgb : vec3(0);
    
    vec3 arm1 = weights.x > 0.01 ? textureGrad(armMap, vec3(uv1,layer), duvdx, duvdy).rgb : vec3(0);
    vec3 arm2 = weights.y > 0.01 ? textureGrad(armMap, vec3(uv2,layer), duvdx, duvdy).rgb : vec3(0);
    vec3 arm3 = weights.z > 0.01 ? textureGrad(armMap, vec3(uv3,layer), duvdx, duvdy).rgb : vec3(0);

    vec3 n1 = weights.x > 0.01 ? textureGrad(normalMap, vec3(uv1,layer), duvdx, duvdy).rgb * 2.0 - 1.0 : vec3(0) * 2.0 - 1.0;
    vec3 n2 = weights.y > 0.01 ? textureGrad(normalMap, vec3(uv2,layer), duvdx, duvdy).rgb * 2.0 - 1.0 : vec3(0) * 2.0 - 1.0;
    vec3 n3 = weights.z > 0.01 ? textureGrad(normalMap, vec3(uv3,layer), duvdx, duvdy).rgb * 2.0 - 1.0 : vec3(0) * 2.0 - 1.0;

    // Blending
    result.albedo = weights.x*albedo1 + weights.y*albedo2 + weights.z*albedo3;
    result.normal = normalize(weights.x*n1 + weights.y*n2 + weights.z*n3);
    result.arm = weights.x*arm1 + weights.y*arm2 + weights.z*arm3;

    // Use the erosion displacement to calculate a cheap and dumb AO term
    // 
    float hackAO= clamp(smoothstep(0.75,-0.5, erosionFactor.y),0.,1.);
    hackAO *= hackAO;

    result.arm.x *= hackAO;
#endif
    result.h = weights.x*h1 + weights.y*h2 + weights.z*h3;
    return result;
}

void GetTriplanarSamples(vec3 worldPos, vec3 normal, out TriplanarSample X, out TriplanarSample Y, out TriplanarSample Z, vec2 erosionFactor) {
    vec2 uvX, uvY, uvZ;
    TriplanarUvs(worldPos, uvX, uvY, uvZ);

    TriplanarSample Y1 = ProceduralTilingAndBlending(uvY, 0);
    TriplanarSample Y2 = ProceduralTilingAndBlending(uvY, 2);
    TriplanarSample Y3 = ProceduralTilingAndBlending(uvY, 3);

    //Y3.h = smoothstep(-0.15,1.0,Y3.h);// + 0.15f;

    // splat blending for Y-facing plane
    vec3 splat = GetSplat(normal,worldPos, Y1.h, Y2.h, Y3.h, erosionFactor);
    Y = TriBlend(splat, Y1,Y2,Y3);

    // Procedural tiling for X and Z facing planes
    X = ProceduralTilingAndBlending(uvX, 1);
    Z = ProceduralTilingAndBlending(uvZ, 1);
}

vec3 TriplanarWeights(vec3 geometryNormal, float hx, float hy, float hz, vec2 erosionFactor) {
    float e = 0.f;//smoothstep(-1.0,1.0,erosionFactor.y);
    vec3 weights = abs(geometryNormal);
    weights = HeightBlend(weights, hx+e, hy, hz+e, 0.1f);
    weights /= dot(weights, vec3(1,1,1));
    return weights;
}

#endif