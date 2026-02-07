#version 460
#inject

#include "corepass/corepass_shared.glsl"

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

/*
layout(binding = 0, std430) readonly buffer renderItemKeysSsbo {
    uvec2 renderItemKeys[];
}
*/

layout(binding = 0, std430) readonly buffer renderItemKeysSsbo {
    uvec2 renderItemKeys[];
};

layout(binding = 1, std430) readonly buffer keyCountSsbo {
    uint keyCount[];
};

layout(binding = 2, std430) readonly buffer materialHeaderSsbo {
    MaterialHeader materialHeaders[];
};

layout(binding = 3, std430) readonly buffer drawBaseInstanceSsbo {
    uint drawBaseInstance[];
};

layout(binding = 4, std430) writeonly buffer drawCommandsSsbo {
    DrawElementsIndirectCommand drawCommands[];
};

layout(binding = 5, std430) readonly buffer meshHeaderSsbo {
    MeshHeader meshHeaders[];
};

uniform int numDraws;

void main() {
    if(gl_GlobalInvocationID.x >= numDraws) return;

    uint baseInstance = drawBaseInstance[gl_GlobalInvocationID.x];
    uint nextBaseInstance;

    if(gl_GlobalInvocationID.x == numDraws-1) {
        nextBaseInstance = keyCount[0];
    } else {
        nextBaseInstance = drawBaseInstance[gl_GlobalInvocationID.x + 1];
    }

    DrawElementsIndirectCommand drawCmd;
    drawCmd.instanceCount = nextBaseInstance - baseInstance;
    drawCmd.count = 0;
    drawCmd.firstIndex = 0;
    drawCmd.baseVertex = 0;
    drawCmd.baseInstance = 0; // unused

    if(drawCmd.instanceCount > 0) {        
        uint key = renderItemKeys[baseInstance].x;
        uint materialId, meshId;
        DecodeKey(key,materialId,meshId);
        MeshHeader meshHeader = meshHeaders[meshId];
        MaterialHeader materialHeader = materialHeaders[materialId];

        drawCmd.count = meshHeader.count;
    }

    drawCommands[gl_GlobalInvocationID.x] = drawCmd;
}