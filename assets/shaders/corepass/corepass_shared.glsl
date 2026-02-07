#ifndef COREPASS_SHARED_GLSL
#define COREPASS_SHARED_GLSL

struct DrawElementsIndirectCommand {
    uint  count;
    uint  instanceCount;
    uint  firstIndex;
    uint  baseVertex;
    uint  baseInstance;
};

struct  MaterialHeader {
    uint id;
    uint drawBufferOffset;
    uint drawCount;
    uint drawKeyOffset;
};

struct MeshHeader {
    uint id;
    uint count;
};

void DecodeKey(in uint key, out uint materialId, out uint meshId) {
    materialId = (key >> 18) & 0x3fff;
    meshId = key & 0x3ffff;
};

struct RenderItem {
    mat4 model;
    vec3 aabbMin;
    vec3 aabbMax;
};

#endif