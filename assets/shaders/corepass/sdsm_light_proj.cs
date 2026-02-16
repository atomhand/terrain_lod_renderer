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


layout(binding = 3, std430) buffer ssbo3 {
    mat4 s_lightViewMatrices[];
};

layout(binding = 4, std430) buffer ssbo4 {
    float s_frustumPlanes[];
};

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

    // reverse Z orthographic projection
    mat4 ortho = mat4(1.f );
    ortho[0] = vec4(2.f / (r-l), 0.f,0.f,0.f);
    ortho[1] = vec4(0.f,2.f/(t-b), 0.f,0.f);
    ortho[2] = vec4(0.f,0.f, 1.f/(far-near),0.f);
    ortho[3] =  vec4(-(r+l)/(r-l),-(t+b)/(t-b),(far)/(far-near),1.0f);
    //ortho = normalize_range * ortho;

    //mat4 translation = mat4(1.f);
    //translation[3] = vec4(-(r+l)/2.f,-(t+b)/2.f,(far)/2.f,1.0f);

    s_frustumPlanes[gl_LocalInvocationIndex*6+0] = l;
    s_frustumPlanes[gl_LocalInvocationIndex*6+1] = r;
    s_frustumPlanes[gl_LocalInvocationIndex*6+2] = b;
    s_frustumPlanes[gl_LocalInvocationIndex*6+3] = t;
    s_frustumPlanes[gl_LocalInvocationIndex*6+4] = near;
    s_frustumPlanes[gl_LocalInvocationIndex*6+5] = far;

    // view matrix
    s_lightViewMatrices[gl_LocalInvocationIndex] = s_lightSpaceMatrices[gl_LocalInvocationIndex];

    // combined projection+view
    s_lightSpaceMatrices[gl_LocalInvocationIndex] = ortho * s_lightSpaceMatrices[gl_LocalInvocationIndex];
}