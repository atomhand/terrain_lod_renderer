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
};

struct MeshHeader {
    uint id;
    uint count;
};

uint MaterialIdFromKey(uint key) {
    return (key >> 18) & 0x3fff;
}

void DecodeKey(in uint key, out uint materialId, out uint meshId) {
    materialId = MaterialIdFromKey(key);
    meshId = key & 0x3ffff;
};

struct RenderItem {
    mat4 model;
    vec4 aabbMin;
    vec4 aabbMax;
};

#endif