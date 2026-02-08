#ifndef COREPASS_INSTANCING_SHARED_GLSL
#define COREPASS_INSTANCING_SHARED_GLSL

#include "corepass/corepass_shared.glsl"

uniform int materialId;

layout(binding = 0, std430) readonly buffer keysSsbo {
    uvec2 keys[];
};

layout(binding = 1, std430) readonly buffer materialHeaderSsbo {
    MaterialHeader materialHeaders[];
};

layout(binding = 2, std430) readonly buffer baseInstanceSsbo {
    uint drawBaseInstance[];
};

layout(binding = 3, std430) readonly buffer renderItemSsbo {
    RenderItem renderItems[];
};

mat4 GetModel() {    
    MaterialHeader header = materialHeaders[materialId];
    uint baseInstance = drawBaseInstance[gl_DrawID+header.drawBufferOffset];
    uvec2 key = keys[baseInstance+gl_InstanceID];

    return renderItems[key.y].model;
}

#endif