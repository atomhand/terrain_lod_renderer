// onesweep algorithm: Andy Adinets and Duane Merrill. Onesweep: A Faster Least Significant Digit Radix Sort for GPUs. 2022. arXiv: 2206.01784 
#version 460
#inject
#extension GL_KHR_shader_subgroup_arithmetic: enable

#include "algorithm/onesweep_shared.glsl"


layout(local_size_x = WORD_SIZE, local_size_y = 1, local_size_z = 1) in;

void main() {
    clearHistogram[gl_GlobalInvocationID.x] = 0;

    // Each block calculates the prefix sum for one histogram
    // Each thread is responsible for 1 digit 

    uint blockOffset = WORD_SIZE * gl_WorkGroupID.x;

    // Calculate the prefix sum within the warp
    // Inclusive so that the last thread is holding the total for this warp
    uint value = histogram[gl_GlobalInvocationID.x];
    histogram[gl_GlobalInvocationID.x] = subgroupInclusiveAdd(value);

    barrier();

    // Sum the count for every lower warp within this block (retrieved from the last thread of each warp)
    uint warpOffset = subgroupAdd(gl_SubgroupInvocationID < gl_SubgroupID ? histogram[blockOffset + gl_SubgroupInvocationID * gl_SubgroupSize  + gl_SubgroupSize - 1] : 0);

    barrier();

    // final prefix sum for this thread's digit
    // Value is subtracted at the end because we want an exclusive prefix sum but the
    // initial warp-wide prefix sum was inclusive
    histogram[gl_GlobalInvocationID.x] += warpOffset - value;
}