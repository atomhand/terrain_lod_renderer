#ifndef ONESWEEP_SHARED_GLSL
#define ONESWEEP_SHARED_GLSL

#define WORD_BITS 8u
#define WORD_SIZE 256u
#define NUM_PASSES  4u
#define HISTOGRAM_SIZE WORD_SIZE*NUM_PASSES /* 1024 */
#define WORD_MASK 0xFFu // 8 bit word 

layout(binding = 0, std430) readonly buffer inputSsbo {
    uint inputKeys[];
};

layout(binding = 1, std430) writeonly buffer outputSsbo {
    uint outputKeys[];
};

layout(binding = 2, std430) buffer ssbo1 {
    // Multiple histograms are packed into the same buffer 1 after another
    // ordered ascending from Least to Most significant words
    uint histogram[];
};

layout(binding = 3, std430) buffer blockLocalHistogramSsbo {
    // stores a local histogram for each block
    // size WORD_SIZE * numBlocks
    uint blockLocalHistogram[];
};

// status
// 2 bits
// values
// - 0: Unset
// - 1: Contains block sum
// - 2: Contains global prefix

uint EncodeBlockHistogramEntry(uint value, uint status) {
    return value | (status << 30);
}

void DecodeBlockHistogramEntry(uint entry, out uint value, out uint status) {
    // mask out 2 most signficant bits
    value = entry & 0x3fffffff;
    // shift status mask
    status = (entry >> 30) & 0x3;
}
uniform int totalCount;

#endif