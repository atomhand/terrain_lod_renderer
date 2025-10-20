#version 460

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

// Uniforms:
uniform vec4 u_params;
#define minLogLum u_params.x
#define logLumRange u_params.y
#define timeCoeff u_params.z
#define numPixels u_params.w

uniform vec2 t_params;
#define deltaTime t_params.x // delta time in seconds
#define tau t_params.y // exposure adaptation period (high is slower)

layout(r16f, binding=0) uniform image2D target;

layout(binding = 0, std430) buffer ssbo1 {
    uint histogram[];
};

shared uint histogramShared[256];

void main() {
    // Transfer histogram count to the shared histogram
    // + weight the value according to log luminance
    uint count = histogram[gl_LocalInvocationIndex];
    histogramShared[gl_LocalInvocationIndex] = count * gl_LocalInvocationIndex;

    barrier();

    // reset count for next frame
    histogram[gl_LocalInvocationIndex] = 0;

    for(uint cutoff = 256 >> 1; cutoff > 0; cutoff >>= 1) {
        if(uint(gl_LocalInvocationIndex) < cutoff) {
            histogramShared[gl_LocalInvocationIndex] += histogramShared[gl_LocalInvocationIndex + cutoff];
        }

        barrier();
    }

    if(gl_LocalInvocationIndex == 0) {
        // Take weighted sum and divide it by the number of pixels that had luminance greater than zero
        float weightedLogAverage = histogramShared[0] / max(numPixels - float(count), 1.0) - 1.0;

        // Map from log luminance to actual luminance
        float weightedAvgLum = exp2(((weightedLogAverage / 254.0) * logLumRange) + minLogLum);

        // Interpolate with last frame's value to prevent sudden shift in exposure
        float lumLastFrame = imageLoad(target, ivec2(0,0)).x;
        float blendedLum = lumLastFrame + (weightedAvgLum - lumLastFrame) * (1.0 - exp(-deltaTime*tau));
        imageStore(target, ivec2(0,0), vec4(blendedLum, 0.0, 0.0, 0.0));
    }
}