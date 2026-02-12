#version 460

#extension GL_KHR_shader_subgroup_arithmetic: enable

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

#include "shared/uniforms_shared.glsl"

layout(binding = 0, std430) buffer ssbo1 {
    int depthMinMax[];
};

layout(binding = 1, std430) buffer ssbo2 {
    mat4 s_lightSpaceMatrices[];
};

const mat4 normalize_range = mat4(1.0f, 0.0f, 0.0f, 0.0f,
                                0.0f, 1.0f, 0.0f, 0.0f,
                                0.0f, 0.0f, 0.5f, 0.0f,
                                0.0f, 0.0f, 0.5f, 1.0f);
const mat4 reverse_z = mat4(1.0f, 0.0f,  0.0f, 0.0f,
                        0.0f, 1.0f,  0.0f, 0.0f,
                        0.0f, 0.0f, -1.0f, 0.0f,
                        0.0f, 0.0f,  1.0f, 1.0f);

void main()
{
    if(gl_LocalInvocationIndex >= cascadeCount) { 
        return;
    }

    float l = float(depthMinMax[gl_LocalInvocationIndex*6 + 0])/1000.f;
    float r = float(depthMinMax[gl_LocalInvocationIndex*6 + 1])/1000.f;
    float b = float(depthMinMax[gl_LocalInvocationIndex*6 + 2])/1000.f;
    float t = float(depthMinMax[gl_LocalInvocationIndex*6 + 3])/1000.f;
    float near = float(depthMinMax[gl_LocalInvocationIndex*6 + 4])/1000.f;
    float far = float(depthMinMax[gl_LocalInvocationIndex*6 + 5])/1000.f;

    mat4 ortho = mat4(1.f );
    ortho[0] = vec4(2.f / (r-l), 0.f,0.f,0.f);
    ortho[1] = vec4(0.f,2.f/(t-b), 0.f,0.f);
    ortho[2] = vec4(0.f,0.f, -2.f/(far-near),0.f);
    ortho[3] = vec4(-(r+l)/(r-l),-(t+b)/(t-b),-(far+near)/(far-near),1.0f);

    s_lightSpaceMatrices[gl_LocalInvocationIndex] = reverse_z * normalize_range * ortho * s_lightSpaceMatrices[gl_LocalInvocationIndex];
}