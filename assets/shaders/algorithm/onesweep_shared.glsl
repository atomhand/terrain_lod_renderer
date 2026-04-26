// onesweep algorithm: Andy Adinets and Duane Merrill. Onesweep: A Faster Least Significant Digit Radix Sort for GPUs. 2022. arXiv: 2206.01784 
#ifndef ONESWEEP_SHARED_GLSL
#define ONESWEEP_SHARED_GLSL

#define WORD_BITS 8u
#define WORD_SIZE 256u
#define NUM_PASSES  4u

#define PARTITION_SIZE NUM_WARPS*32*KEYS_PER_THREAD

#define HISTOGRAM_SIZE WORD_SIZE*NUM_PASSES /* 1024 */
#define WORD_MASK 0xFFu // 8 bit word


#ifdef PAIRED
#define KEY_TYPE uvec2
#else
#define KEY_TYPE uint
#endif

layout(binding = 0, std430) buffer inputSsbo {
    KEY_TYPE inputKeys[];
};

layout(binding = 1, std430) buffer outputSsbo {
    KEY_TYPE outputKeys[];
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