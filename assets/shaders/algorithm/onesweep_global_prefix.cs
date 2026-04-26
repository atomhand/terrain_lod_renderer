// onesweep algorithm: Andy Adinets and Duane Merrill. Onesweep: A Faster Least Significant Digit Radix Sort for GPUs. 2022. arXiv: 2206.01784 
#version 460
#inject
#extension GL_KHR_shader_subgroup_arithmetic: enable

#include "algorithm/onesweep_shared.glsl"


layout(local_size_x = WORD_SIZE, local_size_y = 1, local_size_z = 1) in;

void main() {
    clearHistogram[gl_GlobalInvocationID.x] = 0;

    uint blockOffset = 256 * gl_WorkGroupID.x;

    uint value = histogram[gl_GlobalInvocationID.x];
    histogram[gl_GlobalInvocationID.x] = subgroupInclusiveAdd(value);

    barrier();

    uint warpOffset = subgroupAdd(gl_SubgroupInvocationID < gl_SubgroupID ? histogram[blockOffset + gl_SubgroupInvocationID * 32 + 31] : 0);

    barrier();

    histogram[gl_GlobalInvocationID.x] += warpOffset - value;
}