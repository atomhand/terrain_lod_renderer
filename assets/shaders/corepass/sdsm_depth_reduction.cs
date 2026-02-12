#version 460

#extension GL_KHR_shader_subgroup_arithmetic: enable

#include "shared/coordinate_shared.glsl"

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0, std430) buffer ssbo1 {
    int depthOutput[];
};

layout(binding=0) uniform sampler2D depthBuffer;

shared int sharedDepth[2];

void main()
{
    if(gl_LocalInvocationIndex.x < 2) {        
        sharedDepth[gl_LocalInvocationIndex] = depthOutput[gl_LocalInvocationIndex];
    }

    barrier();

    uvec2 dim = textureSize(depthBuffer, 0).xy;

    if(gl_GlobalInvocationID.x < dim.x && gl_GlobalInvocationID.y < dim.y) {
        //float depth = imageLoad(depthBuffer, ivec2(gl_GlobalInvocationID.xy)).x;
        vec2 uv = vec2(gl_GlobalInvocationID.xy) / vec2(dim);
        float depthSample = texture2D(depthBuffer, uv).x;

        if(depthSample > 1e-6f) {
            int depth = int(LinearizeDepth(depthSample) * 1000.f);
            
            int minDepth = subgroupMin(depth);
            int maxDepth = subgroupMax(depth);

            if(subgroupElect()) {
                atomicMin(sharedDepth[0],minDepth);
                atomicMax(sharedDepth[1],maxDepth);
            }
        }
    }

    barrier();

    if(gl_LocalInvocationIndex.x == 0) {
        atomicMin(depthOutput[0],sharedDepth[0]);
        atomicMax(depthOutput[1],sharedDepth[1]);
    }
}