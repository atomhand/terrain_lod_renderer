#version 460
#inject

// NOTE - local_size_x must be >= WORD_SIZE * NUM_PASSES
layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

#include "algorithm/onesweep_shared.glsl"

shared uint histogramShared[HISTOGRAM_SIZE];

void main() {
    for(int i =0; i<NUM_PASSES; i++) {
        histogramShared[gl_LocalInvocationIndex+i*256] = 0;
    }

    barrier();

    for(int w=0; w<KEYS_PER_THREAD; w++) {
        uint keyId = gl_WorkGroupID.x*PARTITION_SIZE + gl_LocalInvocationIndex + w*256;
        if(keyId < totalCount) {
            uint key = inputKeys[keyId];
            for(uint i =0; i<NUM_PASSES; i++) {
                uint wordOffset = i * WORD_BITS;
                uint word = (key >> wordOffset) & WORD_MASK;
                atomicAdd(histogramShared[word + i * WORD_SIZE], 1);
            }
        }
    }

    barrier();

    for(int i =0; i<NUM_PASSES; i++) {
        atomicAdd(histogram[gl_LocalInvocationIndex+i*256], histogramShared[gl_LocalInvocationIndex+i*256]);
    }

    if(gl_GlobalInvocationID.x == 0) {
        blockCounter[0] = 0;
    }
}