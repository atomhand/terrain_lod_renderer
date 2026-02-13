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

layout(binding = 6, std430) readonly buffer totalDrawCountSsbo {
    uint totalDrawCount[];
};

layout(binding = 7, std430) writeonly buffer materialDrawCountSsbo {
    uint materialDrawCount[];
};

uniform int numDraws;

void main() {
    if(gl_GlobalInvocationID.x >= totalDrawCount[0]) return;

    uint baseInstance = drawBaseInstance[gl_GlobalInvocationID.x];
    uint nextBaseInstance;
        
    uint key = renderItemKeys[baseInstance].x;
    uint materialId, meshId;
    DecodeKey(key,materialId,meshId);

    MaterialHeader materialHeader = materialHeaders[materialId];
    MeshHeader meshHeader = meshHeaders[meshId];
    
    if(gl_GlobalInvocationID.x == totalDrawCount[0]-1) {
        // if we are the last draw, "base instance" of the next draw is just the total num of draws
        nextBaseInstance = keyCount[0];
        
        // Last overall draw is also the last draw of its material, so write the count into the indirect draw parameter buffer
        materialDrawCount[materialId] = gl_GlobalInvocationID.x + 1 - materialHeader.filteredDrawBufferOffset;
    } else {
        nextBaseInstance = drawBaseInstance[gl_GlobalInvocationID.x + 1];

        // if we are the last draw of our material, we write the draw count to an indirect draw parameter buffer
        uint nextMaterialId, nextMeshId;
        DecodeKey(renderItemKeys[nextBaseInstance].x,nextMaterialId,nextMeshId);
        if(nextMaterialId != materialId)
            materialDrawCount[materialId] = gl_GlobalInvocationID.x + 1 - materialHeader.filteredDrawBufferOffset;
    }

    DrawElementsIndirectCommand drawCmd;
    drawCmd.instanceCount = nextBaseInstance - baseInstance;
    drawCmd.count = meshHeader.count;
    drawCmd.firstIndex = meshHeader.firstIndex;
    drawCmd.baseVertex = 0;
    drawCmd.baseInstance = 0; // unused
    drawCommands[gl_GlobalInvocationID.x - materialHeader.filteredDrawBufferOffset + materialHeader.drawBufferOffset] = drawCmd;
}