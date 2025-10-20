#version 460
#inject

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

#include "algorithm/onesweep_shared.glsl"

void main() {
    if(gl_LocalInvocationIndex == 0) {
        // exclusive prefix sum
        // Calculated over multiple histograms at once
        // dispatch number blocks = number histograms
        uint offset = WORD_SIZE * gl_GlobalInvocationID.x;
        uint total = 0;
        for(uint i=0; i<WORD_SIZE; i++) {
            uint tmp = histogram[offset + i];
            histogram[offset + i] = total;
            total += tmp;
        }
    }
}