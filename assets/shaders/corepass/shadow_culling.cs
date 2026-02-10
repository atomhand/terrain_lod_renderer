#version 460

#inject

#extension GL_KHR_shader_subgroup_ballot: enable
#extension GL_KHR_shader_subgroup_shuffle: enable
#extension GL_KHR_shader_subgroup_arithmetic: enable

#define FILTER_KEY_INPUT_TYPE uvec2
#define FILTER_KEY_OUTPUT_TYPE uvec2

#include "shared/uniforms_shared.glsl"
#include "algorithm/filter_shared.glsl"
#include "corepass/corepass_shared.glsl"

layout(binding = 6, std430) readonly buffer materialHeaderSsbo {
    MaterialHeader materialHeaders[];
};

layout(binding = 7, std430) readonly buffer renderItemSsbo {
    RenderItem renderItemData[];
};

uint KeyId(uint blockId, uint i) {
    return blockId * PARTITION_SIZE + gl_SubgroupID * 32 * KEYS_PER_THREAD + gl_SubgroupInvocationID + i * 32;
}

bool FrustumAABBTest(mat4 model, vec3 aabbMin, vec3 aabbMax) {
    vec4 corners[8] = {
        vec4(aabbMin.x, aabbMin.y, aabbMin.z, 1.0),
        vec4(aabbMax.x, aabbMin.y, aabbMin.z, 1.0),
        vec4(aabbMin.x, aabbMax.y, aabbMin.z, 1.0),
        vec4(aabbMax.x, aabbMax.y, aabbMin.z, 1.0),

        vec4(aabbMin.x, aabbMin.y, aabbMax.z, 1.0),
        vec4(aabbMax.x, aabbMin.y, aabbMax.z, 1.0),
        vec4(aabbMin.x, aabbMax.y, aabbMax.z, 1.0),
        vec4(aabbMax.x, aabbMax.y, aabbMax.z, 1.0)
    };

    mat4 mvp = cullingVP * model;// model;

    for (uint corner_idx = 0; corner_idx < 8; corner_idx++) {
        vec4 corner = mvp * corners[corner_idx];

        // shadows don't cull in light-space Z
        if(-corner.w < corner.x && corner.x < corner.w &&
            -corner.w < corner.y && corner.y < corner.w) { return true; }
    }

    return false;
}

bool CullingTest(uvec2 key) {
    uint materialId, meshId;
    DecodeKey(key.x,materialId,meshId);

    MaterialHeader header = materialHeaders[materialId];

    if(((header.renderPassesMask >> renderPassId)&1) !=1) {
        return false;
    }

    RenderItem item = renderItemData[key.y];

    return FrustumAABBTest(item.model, item.aabbMin.xyz, item.aabbMax.xyz);
}

void main() {
    uint blockId = BlockId();

    uint numPassed = 0;
    uvec2 keys[KEYS_PER_THREAD];
    uint firstMaterialInstanceKey[KEYS_PER_THREAD];

    for(uint i =0; i<KEYS_PER_THREAD; i++) {
        uint keyId = KeyId(blockId,i);        
        // 0xffffffff is marker value indicating key did not pass the filter
        keys[i] = uvec2(0xffffffff);
        if(keyId < inputCount[0]) {
            uvec2 key = inputKeys[keyId];
            if(CullingTest(key)) {
                keys[i] = key;
                numPassed++;
            }
        }
    }

    ApplyFilter(blockId,numPassed,keys);
}