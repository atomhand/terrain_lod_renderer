#version 460

#inject

#extension GL_KHR_shader_subgroup_ballot: enable
#extension GL_KHR_shader_subgroup_shuffle: enable
#extension GL_KHR_shader_subgroup_arithmetic: enable

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

#define NUM_WARPS 8
#define DIGITS_PER_WARP_THREAD 8 /* = WORD_SIZE / warp size */

#include "algorithm/onesweep_shared.glsl"

uniform int blocksPerPass;

shared uint sharedBlockId[1];
shared uint prefixShared[WORD_SIZE];
shared uint histogramShared[WORD_SIZE * NUM_WARPS];
shared uint internalBinOffset[WORD_SIZE];

shared uint sortedKeys[PARTITION_SIZE];

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

uint Warp32Multisplit(uint word) {
    uint mask = 0xffffffff;
    for(uint k=0; k<WORD_BITS; k++) {
        const bool t = bool((word >> k) & 0x01u);
        mask &= (t ? 0 : 0xffffffff) ^ subgroupBallot(t).x;
    }
    return mask;
}

// Fills total warp-level bin counts
// and each thread acquires within-warp bin offsets for its keys
void WarpLevelPrefix(uint keys[KEYS_PER_THREAD], out uint warpLocalOffsets[KEYS_PER_THREAD], uint wordOffset) {
    for(int i=0; i<KEYS_PER_THREAD; i++) {
        uint word = (keys[i] >> wordOffset) & WORD_MASK;
        uint mask = Warp32Multisplit(word);

        // - count of threads in warp which share the same digit 
        uint totalBits = bitCount(mask);
        // - count of threads in warp which share the same digit and have a lower thread id
        uint peerBits = bitCount(mask & gl_SubgroupLtMask.x);

        // lowest rank thread in warp with the same word
        uint lowestRankPeer = findLSB(mask);

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
    uint warpOffset = subgroupAdd(gl_SubgroupInvocationID < gl_SubgroupID ? internalBinOffset[gl_SubgroupInvocationID * 32 + 31] : 0);

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
    // Clear shared offsets
    for(int i =0; i<NUM_WARPS; i++)
        histogramShared[gl_LocalInvocationIndex+i*256] = 0;
    
    barrier();
    uint blockId = sharedBlockId[0]  % blocksPerPass;    
    uint currentPass = sharedBlockId[0] / blocksPerPass;
    uint wordOffset = WORD_BITS * currentPass;

    uint baseKeyOffset = blockId * PARTITION_SIZE + gl_SubgroupID * 32 * KEYS_PER_THREAD + gl_SubgroupInvocationID;

    uint keys[KEYS_PER_THREAD];
    for(uint i =0; i<KEYS_PER_THREAD; i++) {        
        uint keyId = baseKeyOffset + i * 32;
        keys[i] = keyId < totalCount ? inputKeys[keyId] : 0xffffffff;
    }

    uint warpLocalOffsets[KEYS_PER_THREAD];
    WarpLevelPrefix(keys, warpLocalOffsets, wordOffset);

    barrier();

    // histogram shared layout
    // num rows = num warps
    // row size = word num of unique values (256)
    // each position contains the count for that value for that warp

    // Prefix sum over warp histograms
    // Each thread performs the prefix sum for the word value corresponding
    // to gl_LocalInvocationIndex
    uint binTotal = 0;
    for(int subgroup=0; subgroup<NUM_WARPS; subgroup++) {
        uint tmp = histogramShared[subgroup * WORD_SIZE + gl_LocalInvocationIndex];
        histogramShared[subgroup * WORD_SIZE + gl_LocalInvocationIndex] = binTotal;
        binTotal += tmp;
    }
    internalBinOffset[gl_LocalInvocationIndex] = binTotal;
    // Write total count for this word value to the global buffer
    blockLocalHistogram[gl_LocalInvocationIndex + WORD_SIZE * blockId] = EncodeBlockHistogramEntry(binTotal,1,currentPass);
    
    BlockPrefixScan(binTotal);

    barrier();

    for(uint i =0; i<KEYS_PER_THREAD; i++) {      
        uint word = (keys[i] >> wordOffset) & WORD_MASK;
        // offset within the bin for this tile
        uint localBinOffset = histogramShared[gl_SubgroupID * WORD_SIZE + word] + warpLocalOffsets[i];
        sortedKeys[internalBinOffset[word] + localBinOffset] = keys[i];
    }

    // Get the prefix (chained lookback)
    uint prefix = GetPrefixChainedLookback(blockId, gl_LocalInvocationIndex, currentPass);
    blockLocalHistogram[gl_LocalInvocationIndex + WORD_SIZE * blockId] = EncodeBlockHistogramEntry(prefix + binTotal,2,currentPass);

    prefixShared[gl_LocalInvocationIndex] = prefix +
        histogram[gl_LocalInvocationIndex + currentPass * WORD_SIZE] - internalBinOffset[gl_LocalInvocationIndex];

    barrier();

    for(uint i =0; i<KEYS_PER_THREAD; i++) {
        uint idx = gl_LocalInvocationIndex + i*256;
        uint word = (sortedKeys[idx] >> wordOffset) & WORD_MASK;

        uint offset = prefixShared[word] + idx;
        if(offset < totalCount)
            outputKeys[offset] = sortedKeys[idx];
    }
}