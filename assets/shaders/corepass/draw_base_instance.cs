#version 460

#inject

#extension GL_KHR_shader_subgroup_ballot: enable
#extension GL_KHR_shader_subgroup_shuffle: enable
#extension GL_KHR_shader_subgroup_arithmetic: enable

#define FILTER_KEY_INPUT_TYPE uvec2

#include "algorithm/filter_shared.glsl"
#include "corepass/corepass_shared.glsl"

layout(binding = 6, std430) buffer materialHeaderSsbo {
    MaterialHeader materialHeaders[];
};

uint KeyId(uint blockId, uint i) {
    return blockId * PARTITION_SIZE + gl_SubgroupID * gl_SubgroupSize * KEYS_PER_THREAD + gl_SubgroupInvocationID + i * gl_SubgroupSize;
}

void main() {
    uint blockId = BlockId();

    uint numPassed = 0;
    uint keyIdx[KEYS_PER_THREAD];
    uint firstMaterialInstanceKey[KEYS_PER_THREAD];

    for(uint i =0; i<KEYS_PER_THREAD; i++) {
        uint keyId = KeyId(blockId,i);
        
        // 0xffffffff is marker value indicating key did not pass the filter
        keyIdx[i] = 0xffffffff;    
        firstMaterialInstanceKey[i] = 0xffffffff;
        if(keyId < inputCount[0]) {
            uint key = inputKeys[keyId].x;
            uint prevKey = keyId > 0 ? inputKeys[keyId-1].x : 0xffffffff;

            // If key is different from prev key (i.e. either material id or draw id is different)
            // it will be appended to the output buffer
            if(key != prevKey) {
                keyIdx[i] = keyId;
                numPassed++;

                uint materialId = MaterialIdFromKey(key);
                uint prevMaterialId = MaterialIdFromKey(prevKey);

                // If we are the first key of our material, later we need to write our
                // offset into the drawBaseInstance buffer to the material header
                firstMaterialInstanceKey[i] = materialId != prevMaterialId ? key : 0xffffffff;
            }
        }
    }

    // overwrites each key with the offset it was written to
    ApplyFilter(blockId,numPassed,keyIdx);
    
    // For any key that is the first instance of a material, write the instance offset
    // (into the newly filtered buffer) to the material header
    uint blockOffset = sharedBlockOffset[0];
    for(uint i=0; i<KEYS_PER_THREAD; i++) {
        if(firstMaterialInstanceKey[i] != 0xffffffff) {
            uint offset = blockOffset + keyIdx[i];
            uint materialId = MaterialIdFromKey(firstMaterialInstanceKey[i]);
            materialHeaders[materialId].filteredDrawBufferOffset = offset;
        }
    }
}