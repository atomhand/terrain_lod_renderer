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

layout(binding = 2, std430) buffer ssbo3 {
    vec4 s_cascadePlaneDistances[];
};

float GetCascadeDepth(float near, float far, float iCascade) {
    float m = float(cascadeCount);
    return mix(near * pow(far/near, iCascade / m), near + far * iCascade/m, pssmFactor);
}

void main()
{
    if(gl_LocalInvocationIndex >= cascadeCount) {        
        return;
    }

    float near = max(nearPlane,depthMinMax[0]/1000.0f);
    float far = min(farPlane,depthMinMax[1]/1000.0f);

    float iCascade = float(gl_LocalInvocationIndex);

    float cascadeNear = GetCascadeDepth(near,far,iCascade);
    float cascadeFar = GetCascadeDepth(near,far,iCascade+1.f);

    s_cascadePlaneDistances[gl_LocalInvocationIndex] = vec4(cascadeFar, 0.f,0.f,0.f);

    vec3 frustumCorners[8] = {
        vec3(-1.0,    1.0,   1.0),
        vec3(1.0,     1.0,   1.0),
        vec3(-1.0,    -1.0,  1.0),
        vec3(1.0,     -1.0,  1.0),
        
        vec3(-1.0,    1.0,   0.0),
        vec3(1.0,     1.0,   0.0),
        vec3(-1.0,    -1.0,  0.0),
        vec3(1.0,     -1.0,  0.0),
    };

    // get view frustum corners
    for(int i=0; i<8; i++) {
        vec4 h = boundedInverseViewProjection * vec4(frustumCorners[i],1.0);
        frustumCorners[i] = h.xyz / h.w;
    }

    vec4 farP = boundedInverseViewProjection * vec4(0.f,0.f,0.f,1.0);
    farP /= farP.w;
    vec4 nearP  = boundedInverseViewProjection * vec4(0.f,0.f,1.f,1.0);
    nearP /= nearP.w;

    vec3 ro = nearP.xyz;
    vec3 rd = normalize(farP.xyz-nearP.xyz);
    vec3 center = ro + rd * (0.5 * cascadeNear + 0.5 * cascadeFar);

    /*
    // get cascade frustum
    for(int i =0; i<4; i++) {
        vec3 ro = frustumCorners[i];
        vec3 rd = normalize(frustumCorners[i+4]-frustumCorners[i]);

        frustumCorners[i] = ro + cascadeNear * rd;
        frustumCorners[i+4] = ro + cascadeFar * rd;
    }

    vec3 center = vec3(0.0f);
    for(int i=0; i<8; i++) {
        center += frustumCorners[i];
    }
    center /= 8.0f;
    */
    
    vec3 worldUp = vec3(0,1,0);
    vec3 forward = normalize(-lightDirection.xyz);
    vec3 right = normalize(cross(worldUp,forward));
    vec3 up = cross(forward,right);
    mat4 lightMatrix;
    lightMatrix[0] = vec4(right[0],right[1],right[2],0.0);
    lightMatrix[1] = vec4(up[0],up[1],up[2],0.0);
    lightMatrix[2] = vec4(forward[0],forward[1],forward[2],0.0);
    lightMatrix[3] = vec4(center,1.0);

    /*
    mat4 translation;
    translation[0] = vec4(1.f,0.f,0.f,0.f);
    translation[1] = vec4(0.f,1.f,0.f,0.f);
    translation[2] = vec4(0.f,0.f,1.f,0.f);
    translation[3][0] = -center.x;
    translation[3][1] = -center.y;
    translation[3][2] = -center.z;
    translation[3][3] = 1.0;
    */

    s_lightSpaceMatrices[gl_LocalInvocationIndex] = inverse(lightMatrix);
}