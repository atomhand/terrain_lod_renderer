#version 460
#inject

#include "corepass/corepass_shared.glsl"

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;


layout(binding = 0, std430) readonly buffer renderItemKeysSsbo {
    uvec2 renderItemKeys[];
};

layout(binding = 1, std430) readonly buffer keyCountSsbo {
    uint keyCount[];
};

layout(binding = 2, std430) readonly buffer materialHeaderSsbo {
    MaterialHeader materialHeaders[];
};

layout(binding = 3, std430) writeonly buffer drawBaseInstanceSsbo {
    uint drawBaseInstance[];
};

void main() {
    if(gl_GlobalInvocationID.x >= keyCount[0]) return;

    uint key = renderItemKeys[gl_GlobalInvocationID.x].x;

    uint materialId, meshId;
    DecodeKey(key.x, materialId, meshId);

    bool isFirstInstance;

    if(gl_GlobalInvocationID.x == 0) {
        isFirstInstance = true;
    } else {        
        uint prevKey = renderItemKeys[gl_GlobalInvocationID.x -1].x;
        uint prevMaterialId, prevMeshId;
        DecodeKey(key.x, materialId, meshId);
        
        //isFirstInstance = meshId != prevMeshId || materialId != prevMaterialId;
        isFirstInstance = key != prevKey;
    }

    /*
    if(isFirstInstance) {
        MaterialHeader material = materialHeaders[materialId];
        drawBaseInstance[material.drawBufferOffset + drawId] = gl_GlobalInvocationID.x;
    }
    */


    // TODO
    // if it's the first instance of a draw -- append index to drawBaseInstance (gl_GlobalInvocationID.x)
    // if it's the first instance of a material, also update the material's draw index offset
    // if it's the last instance of a material, also update the material's count
}