#version 460

#extension GL_KHR_shader_subgroup_ballot: enable

#inject

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

#define NUM_WARPS 8
#define DIGITS_PER_WARP_THREAD 8 /* = WORD_SIZE / warp size */

#include "algorithm/onesweep_shared.glsl"

shared uint sharedBlockId[1];
shared uint prefixShared[WORD_SIZE];
shared uint histogramShared[WORD_SIZE * NUM_WARPS];
shared uint internalBinOffset[WORD_SIZE];
uniform int wordOffset;
uniform int currentPass;

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
            // atomicAdd(x, 0) is necessary to force a fresh atomic read
            // (Vulkan offers atomicLoad as an alternative)
            DecodeBlockHistogramEntry(atomicAdd(blockLocalHistogram[index], 0),
                value, status);
        } while(status == 0);

        if(status == 1) {
            // Entry contains the sum for the previous block
            accumulatedSum += value;
        } else { // status == 2     
            // Entry contains the global prefix       
            return value + accumulatedSum;
        }
    }

    return accumulatedSum;
}

// GPU Multisplit
// returns output offset for word
void Warp32Multisplit(uint keys[KEYS_PER_THREAD], out uint warpLocalOffsets[KEYS_PER_THREAD]) {
    /*
    Each thread is responsible for some keys (num KEYS_PER_THREAD)
    and some digits (num DIGITS_PER_WARP_THREAD)

    The threads broadcast 
    - 
    */

    uint baseDigit = gl_SubgroupInvocationID * DIGITS_PER_WARP_THREAD;
    uint[DIGITS_PER_WARP_THREAD] digitMasks;
    uint[DIGITS_PER_WARP_THREAD] digitCounts;

    uint[KEYS_PER_THREAD] wordMasks;

    for(int i=0;i<DIGITS_PER_WARP_THREAD;i++)
        digitCounts[i] = 0;
    for(int i=0;i<KEYS_PER_THREAD;i++)
        warpLocalOffsets[i] = 0;
    
    // Each thread broadcasts has a set of keys
    // Process
    // Outer loop
    // - Each thread broadcasts each of its keys
    // Inner loops
    // - Each thread observes for each of its keys
    // - as well as each of its digits
    for(int k=0; k<KEYS_PER_THREAD; k++) {
        for(int i=0;i<8;i++)
            digitMasks[i] = 0xffffffff;
        for(int i=0;i<KEYS_PER_THREAD;i++)
            wordMasks[i] = 0xffffffff;

        uint keyBucketId = (keys[k] >> wordOffset) & WORD_MASK;

        for(uint b =0; b<WORD_BITS; b++) {
            uint tmpmask = subgroupBallot(bool(keyBucketId & 0x01u)).x;

            // Warp size is only 32, but key has 256 unique values
            // so handle 8 values per warp thread
            for(int iDigit =0; iDigit<DIGITS_PER_WARP_THREAD; iDigit++) {
                uint currentDigitValue = baseDigit + iDigit;
                if(bool((currentDigitValue>>b) & 0x01u))
                    digitMasks[iDigit] &= tmpmask;
                else            
                    digitMasks[iDigit] &= tmpmask ^ 0xffffffff;
            }

            for(int w=0; w<KEYS_PER_THREAD; w++) {
                uint word = (keys[w] >> wordOffset) & WORD_MASK;
                if(bool((word>>b) & 0x01u))
                    wordMasks[w] &= tmpmask;
                else            
                    wordMasks[w] &= tmpmask ^ 0xffffffff;
            }
            
            keyBucketId >>= 1;
        }

        for(int w=0; w<KEYS_PER_THREAD; w++) {
            // 
            uint mask = k < w ? gl_SubgroupLeMask.x : gl_SubgroupLtMask.x;
            warpLocalOffsets[w] += bitCount(wordMasks[w] & mask);
        }

        for(int batch =0; batch<DIGITS_PER_WARP_THREAD; batch++) {
            digitCounts[batch] += bitCount(digitMasks[batch]);
        }
    }

    for(int batch =0; batch<DIGITS_PER_WARP_THREAD; batch++) {
        histogramShared[gl_SubgroupID * WORD_SIZE + baseDigit + batch] = digitCounts[batch];
    }
}

void main() {
    if(gl_LocalInvocationIndex == 0) {
        sharedBlockId[0] = atomicAdd(blockCounter[0], 1);
    }
    barrier();
    uint blockId = sharedBlockId[0];
    uint partitionKeyOffset = blockId * PARTITION_SIZE + gl_LocalInvocationIndex * KEYS_PER_THREAD;
    uint blockHistogramOffset = blockId * WORD_SIZE;

    uint keys[KEYS_PER_THREAD];
    for(uint i =0; i<KEYS_PER_THREAD; i++) {        
        uint keyId = partitionKeyOffset + i;
        if(keyId < totalCount) {
            keys[i] = inputKeys[keyId];
        } else {
            keys[i] = 0xffffffff;
        }
    }

    uint warpLocalOffsets[KEYS_PER_THREAD];
    Warp32Multisplit(keys, warpLocalOffsets);

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

    // Write prefix to shared buffer
    prefixShared[gl_LocalInvocationIndex] = prefix;

    barrier();

    // prefix sum over internal bin offsets
    // todo - less terrible impl
    total = 0;
    if(gl_LocalInvocationIndex == 0) {
        for(int i =0; i<WORD_SIZE; i++) {            
            uint tmp = internalBinOffset[i];
            internalBinOffset[i] = total;
            //prefixShared[i] -= internalBinOffset[i];
            total += tmp;
        }
    }

    barrier();

    prefixShared[gl_LocalInvocationIndex] = prefixShared[gl_LocalInvocationIndex] +
        histogram[gl_LocalInvocationIndex + currentPass * WORD_SIZE] -
        internalBinOffset[gl_LocalInvocationIndex];

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
        uint key = sortedKeys[idx];
        uint word = (key >> wordOffset) & WORD_MASK;
        outputKeys[prefixShared[word] + idx] = sortedKeys[idx];
    }
}