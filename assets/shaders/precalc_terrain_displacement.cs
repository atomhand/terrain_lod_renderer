#version 460
#inject

#include "shared/uniforms_shared.glsl"

#define ITER (PAGE_SIZE+15)/16

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(rgba32f, binding=0) uniform image2DArray terrainData;

#define TERRAIN_VERTEX
#include "terrain_shared.glsl"

struct NodeHeader {
    vec2 worldOffset;
    vec2 worldExtent;
    int page;
    int padding;
};

layout(binding = 0, std430) readonly buffer nodeHeaderSsbo {
    NodeHeader headers[];
};

void main() {
    NodeHeader header = headers[gl_WorkGroupID.x];

    for(int x=0; x<ITER; x++) {
        for(int y=0; y<ITER; y++) {
            ivec3 coord = ivec3(gl_LocalInvocationID.x+x*16,gl_LocalInvocationID.y+y*16, header.page);
            if(coord.x >= PAGE_SIZE || coord.y >= PAGE_SIZE) continue;
            
            vec4 texel = imageLoad(terrainData, coord);

            vec2 uv = vec2(coord.xy) / vec2(PAGE_SIZE-1);

            vec3 worldPos = vec3(header.worldOffset.x + uv.x*header.worldExtent.x, texel.w, header.worldOffset.y + uv.y*header.worldExtent.y);
            vec3 Normal = vec3(texel.x,sqrt(1.0-texel.x*texel.x-texel.z*texel.z),texel.z);

            float displacement = 0.f;
            //if(displacementScale > 0.f) {
                // Triplanar displacement
                TriplanarSample X, Y, Z;
                GetTriplanarSamples(worldPos, Normal, X,Y,Z, vec2(0.f));
                vec3 triplanarWeights = TriplanarWeights(Normal, X.h, Y.h, Z.h, vec2(0.f));

                // Triplanar blend weights    
                float h = X.h * triplanarWeights.x + Y.h * triplanarWeights.y + Z.h * triplanarWeights.z - 0.5;
                displacement = h;// * displacementScale;
            //}

            texel = vec4(Normal.x,Normal.z,displacement,texel.w);
            imageStore(terrainData, coord, texel);
        }
    }
}