#version 460
#inject

#include "shared/uniforms_shared.glsl"

#define ITER (PAGE_SIZE+15)/16

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(rgba32f, binding=0) uniform image2DArray terrainData;

#define TERRAIN_VERTEX
#include "terrain_shared.glsl"

#include "shared/fastnoise_gpu.glsl"

struct NodeHeader {
    vec2 worldOffset;
    vec2 worldExtent;
    int page;
    int padding;
};

layout(binding = 0, std430) readonly buffer nodeHeaderSsbo {
    NodeHeader headers[];
};


#ifdef COMPUTE_TERRAIN

float fbm(vec2 p, int octaves, float freq, int seedOffset) {
    int seed = noiseSeed + seedOffset;
    float amp = 1.f;
    float lacunarity = 2.f;
    float gain = 0.5f;

    //const float weightedStrength = 0.f;

    float noise = GenPerlin2D(seed, p.x, p.y, freq);
    float sum = noise * amp;

    for(int i =1; i<octaves; i++) {
        seed += 1;
        //amp *= mix(1.f, (noise+1.f) * 0.5f, weightedStrength);
        amp *= gain;

        p *= lacunarity;
        noise = GenPerlin2D(seed, p.x, p.y, freq);
        sum += noise * amp;
    }

    return sum;
}

// output (h, dfdx, dfdy)
float TerrainHeightFunction(vec2 p) {
    float foothillsFbm = fbm(p  , noiseFoothillOctaves, noiseFoothillsFreq/ noisePeriod, 32112) ;

    float mountainsModifier = (foothillsFbm +2.f)/3.f;
    float foothills = foothillsFbm * noiseFoothillsScale * 0.5f;

    float mountains = fbm(p , noiseMountainOctaves, noiseMountainFreq/ noisePeriod, 4432) * mountainsModifier * 0.5f;
    mountains = mountains * pow(abs(mountains), noiseMountainExponent-1) * noiseMountainScale;

    return (foothills + mountains) * noiseScale + 4.f;
}

vec3 GetLocalPos(int x, int z, NodeHeader header) {    
    vec2 uv = vec2(x,z) / vec2(PAGE_SIZE-1);
    vec3 p = vec3(header.worldOffset.x + uv.x*header.worldExtent.x, 0.f, header.worldOffset.y + uv.y*header.worldExtent.y);
    p.y = TerrainHeightFunction(p.xz);
    return p;
}

vec3 GetNormal(int x, int z, NodeHeader header) {
    vec3 L = GetLocalPos(x-1,z, header);
    vec3 R = GetLocalPos(x+1,z, header);
    vec3 U = GetLocalPos(x,z-1, header);
    vec3 D = GetLocalPos(x,z+1, header);
    return normalize(cross(R-L,U-D));
}

#endif

void main() {
    NodeHeader header = headers[gl_WorkGroupID.x];

    for(int x=0; x<ITER; x++) {
        for(int y=0; y<ITER; y++) {
            ivec3 coord = ivec3(gl_LocalInvocationID.x+x*16,gl_LocalInvocationID.y+y*16, header.page);
            if(coord.x >= PAGE_SIZE || coord.y >= PAGE_SIZE) continue;

            vec2 uv = vec2(coord.xy) / vec2(PAGE_SIZE-1);

            vec3 Normal;
            vec3 worldPos = vec3(header.worldOffset.x + uv.x*header.worldExtent.x, 0.f, header.worldOffset.y + uv.y*header.worldExtent.y);

#ifdef COMPUTE_TERRAIN
            worldPos.y = TerrainHeightFunction(worldPos.xz);
            Normal = GetNormal(coord.x,coord.y,header);
#else            
            vec4 texel = imageLoad(terrainData, coord);
            Normal = vec3(texel.x,sqrt(1.0-texel.x*texel.x-texel.z*texel.z),texel.z);
            worldPos.y = texel.w;
#endif
            // Triplanar displacement
            TriplanarSample X, Y, Z;
            GetTriplanarSamples(worldPos, Normal, X,Y,Z, vec2(0.f));
            vec3 triplanarWeights = TriplanarWeights(Normal, X.h, Y.h, Z.h, vec2(0.f));

            // Triplanar blend weights    
            float displacement = X.h * triplanarWeights.x + Y.h * triplanarWeights.y + Z.h * triplanarWeights.z - 0.5;

            vec4 finalTexel = vec4(Normal.x,Normal.z,displacement,worldPos.y);
            imageStore(terrainData, coord, finalTexel);
        }
    }
}