#ifndef HASH_SHARED_GLSL
#define HASH_SHARED_GLSL

// From https://github.com/Angelo1211/2020-Weekly-Shader-Challenge/blob/master/hashes.glsl
uint murmurHash14(uvec4 src) {
    const uint M = 0x5bd1e995u;
    uint h = 1190494759u;
    src *= M; src ^= src>>24u; src *= M;
    h *= M; h ^= src.x; h *= M; h ^= src.y; h *= M; h ^= src.z; h *= M; h ^= src.w;
    h ^= h>>13u; h *= M; h ^= h>>15u;
    return h;
}

// From https://github.com/Angelo1211/2020-Weekly-Shader-Challenge/blob/master/hashes.glsl
// 1 output, 4 inputs
float hash14(vec4 src) {
    uint h = murmurHash14(floatBitsToUint(src));
    return uintBitsToFloat(h & 0x007fffffu | 0x3f800000u) - 1.0;
}

vec2 hash22(vec2 src) {
    return fract(sin((src) * mat2(127.1, 311.7, 269.5, 183.3)) * 43758.5453);
}

#endif