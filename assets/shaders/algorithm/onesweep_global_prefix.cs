#version 460
#inject
#extension GL_KHR_shader_subgroup_arithmetic: enable

#include "algorithm/onesweep_shared.glsl"


layout(local_size_x = WORD_SIZE * NUM_PASSES, local_size_y = 1, local_size_z = 1) in;

#define NUM_WARPS_PER_PASS 8

shared uint[NUM_WARPS_PER_PASS*NUM_PASSES] warpTotals;

void main() {
    clearHistogram[gl_GlobalInvocationID.x] = 0;

    uint val = histogram[gl_GlobalInvocationID.x];

    uint subgroupPrefix = subgroupExclusiveAdd(val);

    if(gl_SubgroupInvocationID == 31) {
        warpTotals[gl_SubgroupID] = subgroupPrefix + val;
    }

    barrier();

    if(gl_SubgroupInvocationID.x < NUM_WARPS_PER_PASS && gl_SubgroupID < NUM_PASSES) {
        warpTotals[gl_SubgroupInvocationID.x + gl_SubgroupID * NUM_WARPS_PER_PASS] = subgroupExclusiveAdd(warpTotals[gl_SubgroupInvocationID.x + gl_SubgroupID * NUM_WARPS_PER_PASS]);
    }

    barrier();

    histogram[gl_GlobalInvocationID.x] = warpTotals[gl_SubgroupID] + subgroupPrefix;
}