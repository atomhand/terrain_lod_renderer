#ifndef COREPASS_SHARED_GLSL
#define COREPASS_SHARED_GLSL

struct DrawElementsIndirectCommand {
    uint  count;
    uint  instanceCount;
    uint  firstIndex;
    uint  baseVertex;
    uint  baseInstance;
};

struct MaterialHeader {
    uint id;
    uint drawBufferOffset;
    uint drawCount;
    uint filteredDrawBufferOffset;

    uint renderPassesMask;
    uint shadowMaterialRedirect;
};

struct MeshHeader {
    vec3 aabbMin;
    uint id;

    vec3 aabbMax;
    uint baseVertex;

    uint count;
    uint firstIndex;
    uint stride;
    uint PACK;

    uint uvOffset;
    uint tangentOffset;
    uvec2 PACK2;
};

uint MaterialIdFromKey(uint key) {
    return (key >> 18) & 0x3fff;
}

uint MeshIdFromKey(uint key) {
    return key & 0x3ffff;
}

void DecodeKey(in uint key, out uint materialId, out uint meshId) {
    materialId = MaterialIdFromKey(key);
    meshId = MeshIdFromKey(key);
};

uint EncodeKey(uint materialId, uint meshId) {
    return (materialId << 18) | meshId;
}

struct RenderItem {
    mat4 model;
    vec4 aabbMin;
    vec4 aabbMax;
};

#endif