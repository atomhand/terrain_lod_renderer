#version 460

#extension GL_KHR_shader_subgroup_ballot: enable
#extension GL_KHR_shader_subgroup_shuffle: enable

#inject

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

#define NUM_WARPS 8
#define DIGITS_PER_WARP_THREAD 8 /* = WORD_SIZE / warp size */

#include "algorithm/onesweep_shared.glsl"

uniform int wordOffset;
uniform int currentPass;

shared uint sharedBlockId[1];
shared uint prefixShared[WORD_SIZE];
shared uint histogramShared[WORD_SIZE * NUM_WARPS];
shared uint internalBinOffset[WORD_SIZE];

shared uint sortedKeys[PARTITION_SIZE];

layout(binding = 3, std430) buffer blockLocalHistogramSsbo {
    // stores a local histogram for each block
    // size WORD_SIZE * numBlocks
    uint blockLocalHistogram[];
};

layout(binding = 4, std430) buffer blockCounterSsbo {
    uint blockCounter[];
};

uint GetPrefix(uint startBlock, uint word) {
    uint accumulatedSum = 0;
    for(int block = int(startBlock)-1; block >= 0; block--) {
        uint index = uint(block) * WORD_SIZE + word;

        uint value, status;
        do {
            // atomicAdd(x, 0) is necessary to force a fresh atomic read on each loop iteration
            // otherwise the GPU is "smart" and uses a cached value
            // (Vulkan GLSL has an extension that exposes this as atomicLoad)
            DecodeBlockHistogramEntry(atomicAdd(blockLocalHistogram[index], 0),
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

uint Warp32Multisplit(uint word) {
    uint mask = 0xffffffff;
    for(uint k=0; k<WORD_BITS; k++) {
        const bool t = bool((word >> k) & 0x01u);
        mask &= (t ? 0 : 0xffffffff) ^ subgroupBallot(t).x;
    }
    return mask;
}

// Fills total warp-level bin counts
// and each thread acquires within-warp bin offsets its keys
void WarpLevelPrefix(uint keys[KEYS_PER_THREAD], out uint warpLocalOffsets[KEYS_PER_THREAD]) {    
    // Each thread broadcasts has a set of keys
    // Process
    // Outer loop
    // - Each thread broadcasts each of its keys
    // Inner loops
    // - Each thread observes for each of its keys
    // - as well as each of its digits
    for(int i=0; i<KEYS_PER_THREAD; i++) {
        uint word = (keys[i] >> wordOffset) & WORD_MASK;
        uint mask = Warp32Multisplit(word);

        // count of threads in  the batch
        // - which share the same digit
        // - and have a lower thread id
        uint peerBits = bitCount(mask & gl_SubgroupLtMask.x);
        uint totalBits = bitCount(mask);

        uint lowestRankPeer = findLSB(mask);

        uint exclusiveWarpPrefix;
        // lowest rank thread associated with a given digit is responsible for increment total to shared memory
        if(totalBits > 0 && peerBits == 0) {
            //exclusiveWarpPrefix = histogramShared[gl_SubgroupID * WORD_SIZE + word];
            //histogramShared[gl_SubgroupID * WORD_SIZE + word] += totalBits;
            exclusiveWarpPrefix = atomicAdd(histogramShared[gl_SubgroupID * WORD_SIZE + word], totalBits);
        }

        warpLocalOffsets[i] = subgroupShuffle(exclusiveWarpPrefix, lowestRankPeer) + peerBits;
    }
}

void main() {
    // acquire block id from atomic counter
    // (because GPU cannot be trusted to schedule blocks in order)
    if(gl_LocalInvocationIndex == 0) {
        sharedBlockId[0] = atomicAdd(blockCounter[0], 1);
    }
    // Clear shared offsets
    for(int i =0; i<NUM_WARPS; i++)
        histogramShared[gl_LocalInvocationIndex*NUM_WARPS+i] = 0;
    
    barrier();
    uint blockId = sharedBlockId[0];
    uint baseKeyOffset = blockId * PARTITION_SIZE + gl_SubgroupID * 32 * KEYS_PER_THREAD + gl_SubgroupInvocationID;

    uint keys[KEYS_PER_THREAD];
    for(uint i =0; i<KEYS_PER_THREAD; i++) {        
        uint keyId = baseKeyOffset + i * 32;
        keys[i] = keyId < totalCount ? inputKeys[keyId] : 0xffffffff;
    }

    uint warpLocalOffsets[KEYS_PER_THREAD];
    WarpLevelPrefix(keys, warpLocalOffsets);

    barrier();

    // histogram shared layout
    // num rows = num warps
    // row size = word num of unique values (256)
    // each position contains the count for that value for that warp

    // Prefix sum over warp histograms
    // Each thread performs the prefix sum for the word value corresponding
    // to gl_LocalInvocationIndex
    uint total = 0;
    for(int subgroup=0; subgroup<NUM_WARPS; subgroup++) {
        uint tmp = histogramShared[subgroup * WORD_SIZE + gl_LocalInvocationIndex];
        histogramShared[subgroup * WORD_SIZE + gl_LocalInvocationIndex] = total;
        total += tmp;
    }
    internalBinOffset[gl_LocalInvocationIndex] = total;
    // Write total count for this word value to the global buffer
    blockLocalHistogram[gl_LocalInvocationIndex + WORD_SIZE * blockId] = EncodeBlockHistogramEntry(total,1);
    
    // Get the prefix (chained lookback)
    uint prefix = GetPrefix(blockId, gl_LocalInvocationIndex);
    blockLocalHistogram[gl_LocalInvocationIndex + WORD_SIZE * blockId] = EncodeBlockHistogramEntry(prefix + total,2);

    barrier();

    // prefix sum over internal bin offsets
    // todo - less terrible impl
    total = 0;
    if(gl_LocalInvocationIndex == 0) {
        for(int i =0; i<WORD_SIZE; i++) {            
            uint tmp = internalBinOffset[i];
            internalBinOffset[i] = total;
            total += tmp;
        }
    }

    barrier();

    prefixShared[gl_LocalInvocationIndex] = prefix +
        histogram[gl_LocalInvocationIndex + currentPass * WORD_SIZE] - internalBinOffset[gl_LocalInvocationIndex];

    for(uint i =0; i<KEYS_PER_THREAD; i++) {      
        uint word = (keys[i] >> wordOffset) & WORD_MASK;
        uint warpBaseOffset = histogramShared[gl_SubgroupID * WORD_SIZE + word];

        // offset within the bin for this tile
        uint localBinOffset = warpBaseOffset + warpLocalOffsets[i];
        sortedKeys[internalBinOffset[word] + localBinOffset] = keys[i];
    }

    barrier();

    for(uint i =0; i<KEYS_PER_THREAD; i++) {
        uint idx = gl_LocalInvocationIndex*KEYS_PER_THREAD + i;
        uint word = (sortedKeys[idx] >> wordOffset) & WORD_MASK;

        uint offset = prefixShared[word] + idx;
        if(offset < totalCount)
            outputKeys[offset] = sortedKeys[idx];
    }
}