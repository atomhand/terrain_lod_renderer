#version 460

#inject

#extension GL_KHR_shader_subgroup_ballot: enable
#extension GL_KHR_shader_subgroup_shuffle: enable
#extension GL_KHR_shader_subgroup_arithmetic: enable

#include "algorithm/filter_shared.glsl"

bool filterOp(uint key) {
    return key % 2 == 0;
}

void main() {
    uint blockId = BlockId();

    uint numPassed = 0;
    uint keys[KEYS_PER_THREAD];

    for(uint i =0; i<KEYS_PER_THREAD; i++) {        
        uint keyId = blockId * PARTITION_SIZE + gl_SubgroupID * 32 * KEYS_PER_THREAD + gl_SubgroupInvocationID + i * 32;

        keys[i] = 0xffffffff;
        if(keyId < inputCount[0]) {
            uint key = inputKeys[keyId];
            if(filterOp(key)) {
                keys[i] = key;
                numPassed++;
            }
        }
    }

    ApplyFilter(blockId,numPassed,keys);
}