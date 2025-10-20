#version 460

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// ref - https://bruop.github.io/exposure/

uniform vec4 u_params;
// u_params.x = minimum log_2 luminance
// u_params.y = inverse of the log_2 luminance range

layout(binding = 0, std430) buffer ssbo1 {
    uint histogram[];
};

layout(rgba16f, binding=0) uniform image2D hdrBuffer;

shared uint histogramShared[256];

#define EPSILON 0.005
// Taken from RTR vol 4 pg. 278
#define RGB_TO_LUM vec3(0.2125, 0.7154, 0.0721)

uint colorToBin(vec3 hdrColor, float minLogLum, float inverseLogLumRange) {
    float lum = dot(hdrColor, RGB_TO_LUM);

    if(lum < EPSILON) {
        return 0;
    }

    float logLum = clamp((log2(lum) - minLogLum) * inverseLogLumRange, 0.0, 1.0);

    // map to bin
    return uint(logLum * 254.0 + 1.0);
}

void main()
{
    histogramShared[gl_LocalInvocationIndex] = 0;
    barrier();

    uvec2 dim = imageSize(hdrBuffer).xy;

    if(gl_GlobalInvocationID.x < dim.x && gl_GlobalInvocationID.y < dim.y) {
        vec3 hdrColor = imageLoad(hdrBuffer, ivec2(gl_GlobalInvocationID.xy)).xyz;
        uint binIndex = colorToBin(hdrColor, u_params.x, u_params.y);

        atomicAdd(histogramShared[binIndex], 1);
    }

    barrier();

    atomicAdd(histogram[gl_LocalInvocationIndex], histogramShared[gl_LocalInvocationIndex]);
}