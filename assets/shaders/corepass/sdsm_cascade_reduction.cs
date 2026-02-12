#version 460

#extension GL_KHR_shader_subgroup_arithmetic: enable

#include "shared/coordinate_shared.glsl"
#include "shared/uniforms_shared.glsl"

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0, std430) buffer ssbo1 {
    int depthOutput[];
};

layout(binding = 1, std430) readonly buffer ssbo2 {
    mat4 s_lightSpaceMatrices[];
};

layout(binding = 2, std430) readonly buffer ssbo3 {
    float s_cascadePlaneDistances[];
};

layout(binding=0) uniform sampler2D depthBuffer;

// layout
// min X
// max X
// min Y
// max Y
// min Z
// max Z
shared int sharedDepth[16*6];

int GetCascadeLayer(vec3 pos) {
    vec4 fragPosViewSpace = cascadeDeterminationView * vec4(pos,1.0);
    float depthValue = -fragPosViewSpace.z;

    int layer = cascadeCount-1;
    for(int i =0; i<cascadeCount; i++) {
        if(depthValue < s_cascadePlaneDistances[i].x) {
            layer = i;
            break;
        }
    }
    return layer;
}

void main()
{
    if(gl_LocalInvocationIndex < 16*6) {        
        sharedDepth[gl_LocalInvocationIndex] = depthOutput[gl_LocalInvocationIndex];
    }

    barrier();

    uvec2 dim = textureSize(depthBuffer, 0).xy;

    if(gl_GlobalInvocationID.x < dim.x && gl_GlobalInvocationID.y < dim.y) {
        //float depth = imageLoad(depthBuffer, ivec2(gl_GlobalInvocationID.xy)).x;
        vec2 uv = vec2(gl_GlobalInvocationID.xy) / vec2(dim);
        float depthSample = texture2D(depthBuffer, uv).x;

       
        if(depthSample > 1e-6f) {
             // project sample to world space
            vec3 worldPos = WorldPosFromDepth(uv,depthSample);

            int layer = GetCascadeLayer(worldPos);

            mat4 lightView = s_lightSpaceMatrices[layer];

            vec4 lightSpacePos = lightView * vec4(worldPos,1.0);
            
            atomicMin(sharedDepth[layer*6 + 0],int(lightSpacePos.x * 1000.0));
            atomicMax(sharedDepth[layer*6 + 1],int(lightSpacePos.x * 1000.0));
            atomicMin(sharedDepth[layer*6 + 2],int(lightSpacePos.y * 1000.0));
            atomicMax(sharedDepth[layer*6 + 3],int(lightSpacePos.y * 1000.0));
            atomicMin(sharedDepth[layer*6 + 4],int(-lightSpacePos.z * 1000.0));
            atomicMax(sharedDepth[layer*6 + 5],int(-lightSpacePos.z * 1000.0));
        }
    }

    barrier();

    if(gl_LocalInvocationIndex < 3*16) {
        atomicMin(depthOutput[gl_LocalInvocationIndex*2+0],sharedDepth[gl_LocalInvocationIndex*2+0]);
        atomicMax(depthOutput[gl_LocalInvocationIndex*2+1],sharedDepth[gl_LocalInvocationIndex*2+1]);
    }
}