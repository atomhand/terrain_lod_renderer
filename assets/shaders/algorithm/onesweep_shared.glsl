#ifndef ONESWEEP_SHARED_GLSL
#define ONESWEEP_SHARED_GLSL

#define WORD_BITS 8u
#define WORD_SIZE 256u
#define NUM_PASSES  4u

#define PARTITION_SIZE 256*KEYS_PER_THREAD

#define HISTOGRAM_SIZE WORD_SIZE*NUM_PASSES /* 1024 */
#define WORD_MASK 0xFFu // 8 bit word 

layout(binding = 0, std430) buffer inputSsbo {
    uint inputKeys[];
};

layout(binding = 1, std430) buffer outputSsbo {
    uint outputKeys[];
};

layout(binding = 2, std430) buffer histogramSsbo {
    // Multiple histograms are packed into the same buffer 1 after another
    // ordered ascending from Least to Most significant words
    uint histogram[];
};

layout(binding = 4, std430) buffer blockCounterSsbo {
    uint blockCounter[];
};

layout(binding = 5, std430) buffer histogramToClearSsbo {
    // Multiple histograms are packed into the same buffer 1 after another
    // ordered ascending from Least to Most significant words
    uint clearHistogram[];
};

// status
// 2 bits
// values
// - 0: Unset
// - 1: Contains block sum
// - 2: Contains global prefix


uniform int totalCount;


#endif