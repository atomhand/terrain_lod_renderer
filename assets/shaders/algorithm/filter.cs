#version 460

#inject

#extension GL_KHR_shader_subgroup_ballot: enable
#extension GL_KHR_shader_subgroup_shuffle: enable
#extension GL_KHR_shader_subgroup_arithmetic: enable

#define WARP_SIZE 32
#define BLOCK_SIZE NUM_WARPS*WARP_SIZE
#define PARTITION_SIZE BLOCK_SIZE*KEYS_PER_THREAD

uniform int totalCount;

layout(local_size_x = BLOCK_SIZE, local_size_y = 1, local_size_z = 1) in;

shared uint sharedBlockId[2];
shared uint countShared[NUM_WARPS];

shared uint sortedKeys[PARTITION_SIZE];

layout(binding = 0, std430) readonly buffer inputSsbo {
    uint inputKeys[];
};
layout(binding = 1, std430) writeonly buffer outputSsbo {
    uint outputKeys[];
};

layout(binding = 2, std430) coherent buffer blockPrefixSsbo {
    // stores a local histogram for each block
    // size numBlocks
    uint blockPrefix[];
};

layout(binding = 3, std430) coherent buffer blockCounterSsbo {
    // stores a local histogram for each block
    // size numBlocks
    uint blockCounter[];
};

uint EncodeBlockHistogramEntry(uint value, uint status) {
    // 3 states
    // 0 - unassigned
    // 1 - sum assigned
    // 2 - prefix assigned

    return value | (status << 30);
}

void DecodeBlockHistogramEntry(uint entry, out uint value, out uint status) {
    // mask out 2 most signficant bits
    value = entry & 0x3fffffff;
    // shift status mask

    status = (entry >> 30) & 0x3;
}

uint GetPrefixChainedLookback(uint startBlock) {
    uint accumulatedSum = 0;
    for(int index = int(startBlock)-1; index >= 0; index--) {
        uint value, status;
        do {
            DecodeBlockHistogramEntry(blockPrefix[index],
                value, status);
        } while(status == 0);

        if(status == 1) {
            // Entry contains the sum for the previous block
            accumulatedSum += value;
        } else { // status == 2     gop
            // Entry contains the global prefix       
            return value + accumulatedSum;
        }
    }

    return accumulatedSum;
}

bool filterOp(uint key) {
    return key % 2 == 0;
}

void main() {
    // acquire block id from atomic counter
    // (because GPU cannot be trusted to schedule blocks in order)
    if(gl_LocalInvocationIndex == 0) {
        sharedBlockId[0] = atomicAdd(blockCounter[0], 1);
    }
    
    barrier();
    uint blockId = sharedBlockId[0];

    uint numPassed = 0;
    uint keys[KEYS_PER_THREAD];
    bool passed[KEYS_PER_THREAD];

    for(uint i =0; i<KEYS_PER_THREAD; i++) {        
        uint keyId = blockId * PARTITION_SIZE + gl_SubgroupID * 32 * KEYS_PER_THREAD + gl_SubgroupInvocationID + i * 32;

        if(keyId < totalCount) {
            uint key = inputKeys[keyId];
            passed[i] = filterOp(key);
            if(passed[i]) {
                keys[i] = key;
                numPassed++;
            }
        } else {
            passed[i] = false;
        }
    }

    uint warpTotal = subgroupAdd(numPassed);
    if(subgroupElect()) {
        countShared[gl_SubgroupID] = warpTotal;
    }

    barrier();

    // get offset for this warp within the block 
    uint warpOffset = subgroupAdd(gl_SubgroupInvocationID < gl_SubgroupID ? countShared[gl_SubgroupInvocationID] : 0);

    uint blockTotal;
    if(gl_LocalInvocationIndex == BLOCK_SIZE-1) {
        blockTotal = warpOffset + warpTotal;
        blockPrefix[blockId] = EncodeBlockHistogramEntry(blockTotal,1);
    }

    // Scatter keys to sort them within locations within block
    uint withinWarpBaseOffset = 0;
    for(uint i =0; i<KEYS_PER_THREAD; i++) {
        if(passed[i]) {
            uint internalOffset = warpOffset + withinWarpBaseOffset + subgroupExclusiveAdd(1);    
            sortedKeys[internalOffset] = keys[i];
        }
        withinWarpBaseOffset += subgroupAdd(passed[i] ? 1 : 0);
    }

    if(gl_LocalInvocationIndex == BLOCK_SIZE-1) {
        sharedBlockId[0] = GetPrefixChainedLookback(blockId);
        sharedBlockId[1] = blockTotal;
        blockPrefix[blockId] = EncodeBlockHistogramEntry(sharedBlockId[0]+blockTotal,2);
    }

    barrier();
    
    blockTotal = sharedBlockId[1];
    uint blockOffset = sharedBlockId[0];
    for(uint i =0; i<KEYS_PER_THREAD; i++) {
        uint idx = gl_LocalInvocationIndex + i*BLOCK_SIZE;
        uint offset = blockOffset + idx;

        if(idx < blockTotal)
            outputKeys[offset] = sortedKeys[idx];
    }
}