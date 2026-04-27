// onesweep algorithm: Andy Adinets and Duane Merrill. Onesweep: A Faster Least Significant Digit Radix Sort for GPUs. 2022. arXiv: 2206.01784 
// This implementation Tom Kellett 2026
#version 460

#inject

#extension GL_KHR_shader_subgroup_ballot: enable
#extension GL_KHR_shader_subgroup_shuffle: enable
#extension GL_KHR_shader_subgroup_arithmetic: enable

#define BLOCK_SIZE NUM_WARPS*32
// in theory this implementation should work with AMD's 64-wide warps
// But not <32 warps (sorry Intel)
#define TRUE_NUM_WARPS (BLOCK_SIZE/gl_SubgroupSize)

layout(local_size_x = BLOCK_SIZE, local_size_y = 1, local_size_z = 1) in;


#include "algorithm/onesweep_shared.glsl"

uniform int blocksPerPass;

shared uint sharedBlockId[1];
shared uint prefixShared[WORD_SIZE];
shared uint histogramShared[WORD_SIZE * NUM_WARPS];
shared uint internalBinOffset[WORD_SIZE];

shared KEY_TYPE sortedKeys[PARTITION_SIZE];

layout(binding = 3, std430) coherent buffer blockLocalHistogramSsbo {
    // stores a local histogram for each block
    // size WORD_SIZE * numBlocks
    uint blockLocalHistogram[];
};

uint EncodeBlockHistogramEntry(uint value, uint status, uint currentPass) {
    // 4 states
    // 0 - unassigned
    // 1 - sum assigned
    // 2 - prefix assigned (even pass)
    // 3 - prefix assigned (odd pass)
    uint tick = currentPass % 2;
    if(status == 2)
        status += tick;

    return value | (status << 30);
}

void DecodeBlockHistogramEntry(uint entry, out uint value, out uint status, uint currentPass) {
    uint tick = currentPass % 2;
    // mask out 2 most signficant bits
    value = entry & 0x3fffffff;
    // shift status mask

    status = (entry >> 30) & 0x3;
    if(status >= 2)
        status = status == 2+tick ? 2 : 0;
}

uint GetPrefixChainedLookback(uint startBlock, uint word, uint currentPass) {
    uint accumulatedSum = 0;
    for(int block = int(startBlock)-1; block >= 0; block--) {
        uint index = uint(block) * WORD_SIZE + word;

        uint value, status;
        do {
            DecodeBlockHistogramEntry(blockLocalHistogram[index],
                value, status, currentPass);
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

/* Multisplit could be more efficient if specialised to specific warp size

uint Warp32Multisplit(uint word) {
    uint mask = 0xffffffff;
    for(uint k=0; k<WORD_BITS; k++) {
        const bool t = bool((word >> k) & 0x01u);
        mask &= (t ? 0 : 0xffffffff) ^ subgroupBallot(t).x;
    }
    return mask;
}
*/

uvec4 SubgroupMultisplit(uint word) {
    uvec4 mask = uvec4(0xffffffff);
    for(uint k=0; k<WORD_BITS; k++) {
        const bool t = bool((word >> k) & 0x01u);
        mask &= (t ? uvec4(0) : uvec4(0xffffffff)) ^ subgroupBallot(t);
    }
    return mask;
}

// Fills total warp-level bin counts
// and each thread acquires within-warp bin offsets for its keys
void WarpLevelPrefix(KEY_TYPE keys[KEYS_PER_THREAD], out uint warpLocalOffsets[KEYS_PER_THREAD], uint wordOffset) {
    for(int i=0; i<KEYS_PER_THREAD; i++) {
        uint word = (keys[i].x >> wordOffset) & WORD_MASK;
        uvec4 mask = SubgroupMultisplit(word);

        // - count of threads in warp which share the same digit 
        uint totalBits = subgroupBallotBitCount(mask);
        // - count of threads in warp which share the same digit and have a lower thread id
        uint peerBits = subgroupBallotBitCount(mask & gl_SubgroupLtMask);

        // lowest rank thread in warp with the same word
        uint lowestRankPeer = subgroupBallotFindLSB(mask);

        uint exclusiveWarpPrefix;
        // lowest rank thread associated with a given digit is responsible for increment total to shared memory
        if(peerBits == 0) {
            exclusiveWarpPrefix = atomicAdd(histogramShared[gl_SubgroupID * WORD_SIZE + word], totalBits);
        }

        warpLocalOffsets[i] = subgroupShuffle(exclusiveWarpPrefix, lowestRankPeer) + peerBits;
    }
}

// Prefix scan using warp intrinsics
// The number of values to be scanned must == block size
// (it would be easy to mask out some lanes if required)
void BlockPrefixScan(uint value) {
    // per warp inclusive sums to buffer
    internalBinOffset[gl_LocalInvocationIndex] =  subgroupInclusiveAdd(value);

    barrier();

    // Compute the offset for the warp by summing the totals (I.e. final lane values) for each lower warp
    // It would be possible to calculate the prefix sum once and pushing it to shared memory,
    // but computing it redundantly for each warp saves a shared mem round trip - seems to win on performance
    uint warpOffset = subgroupAdd(gl_SubgroupInvocationID < gl_SubgroupID ? internalBinOffset[gl_SubgroupInvocationID * gl_SubgroupSize + gl_SubgroupSize - 1] : 0);

    barrier();

    // subtract original value to return to an exclusive sum
    internalBinOffset[gl_LocalInvocationIndex] += warpOffset - value;
}

void main() {
    // acquire block id from atomic counter
    // (because GPU cannot be trusted to schedule blocks in order)
    if(gl_LocalInvocationIndex == 0) {
        sharedBlockId[0] = atomicAdd(blockCounter[0], 1);
    }
    if(gl_LocalInvocationIndex < 256) {        
        // Clear shared offsets
        for(int i =0; i< TRUE_NUM_WARPS; i++)
            histogramShared[gl_LocalInvocationIndex+i*256] = 0;
    }
    
    barrier();
    uint blockId = sharedBlockId[0]  % blocksPerPass;    
    uint currentPass = sharedBlockId[0] / blocksPerPass;
    uint wordOffset = WORD_BITS * currentPass;

    KEY_TYPE keys[KEYS_PER_THREAD];
    for(uint i =0; i<KEYS_PER_THREAD; i++) {        
        uint keyId = blockId * PARTITION_SIZE + gl_SubgroupID * gl_SubgroupSize * KEYS_PER_THREAD + gl_SubgroupInvocationID + i * gl_SubgroupSize;
        keys[i] = keyId < totalCount ? inputKeys[keyId] : KEY_TYPE(0xffffffff);
    }

    uint warpLocalOffsets[KEYS_PER_THREAD];
    WarpLevelPrefix(keys, warpLocalOffsets, wordOffset);

    barrier();
    
    uint binTotal = 0;
    if(gl_LocalInvocationIndex < 256) {
        // histogram shared layout
        // num rows = num warps
        // row size = word num of unique values (256)
        // each position contains the count for that value for that warp

        // Prefix sum over warp histograms
        // Each thread performs the prefix sum for the word value corresponding
        // to gl_LocalInvocationIndex
        for(int subgroup=0; subgroup< TRUE_NUM_WARPS; subgroup++) {
            uint tmp = histogramShared[subgroup * WORD_SIZE + gl_LocalInvocationIndex];
            histogramShared[subgroup * WORD_SIZE + gl_LocalInvocationIndex] = binTotal;
            binTotal += tmp;
        }
        internalBinOffset[gl_LocalInvocationIndex] = binTotal;
        // Write total count for this word value to the global buffer
        blockLocalHistogram[gl_LocalInvocationIndex + WORD_SIZE * blockId] = EncodeBlockHistogramEntry(binTotal,1,currentPass);
    
        BlockPrefixScan(binTotal);
    }

    barrier();

    // Scatter keys to sort them within locations within block
    for(uint i =0; i<KEYS_PER_THREAD; i++) {      
        uint word = (keys[i].x >> wordOffset) & WORD_MASK;
        // offset within the bin for this tile
        uint localBinOffset = histogramShared[gl_SubgroupID * WORD_SIZE + word] + warpLocalOffsets[i];
        sortedKeys[internalBinOffset[word] + localBinOffset] = keys[i];
    }

    if(gl_LocalInvocationIndex < 256) {
        // Get the prefix (chained lookback)
        uint prefix = GetPrefixChainedLookback(blockId, gl_LocalInvocationIndex, currentPass);
        blockLocalHistogram[gl_LocalInvocationIndex + WORD_SIZE * blockId] = EncodeBlockHistogramEntry(prefix + binTotal,2,currentPass);

        prefixShared[gl_LocalInvocationIndex] = prefix +
            histogram[gl_LocalInvocationIndex + currentPass * WORD_SIZE] - internalBinOffset[gl_LocalInvocationIndex];
    }

    barrier();

    for(uint i =0; i<KEYS_PER_THREAD; i++) {
        uint idx = gl_LocalInvocationIndex + i*BLOCK_SIZE;
        uint word = (sortedKeys[idx].x >> wordOffset) & WORD_MASK;

        uint offset = prefixShared[word] + idx;
        if(offset < totalCount)
            outputKeys[offset] = sortedKeys[idx];
    }
}