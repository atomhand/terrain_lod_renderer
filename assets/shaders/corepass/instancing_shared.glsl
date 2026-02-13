#ifndef COREPASS_INSTANCING_SHARED_GLSL
#define COREPASS_INSTANCING_SHARED_GLSL

#include "corepass/corepass_shared.glsl"

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

layout(binding = 4, std430) readonly buffer attributesSsbo {
    float attributes[];
};

layout(binding = 5, std430) readonly buffer meshHeaderSsbo {
    MeshHeader meshHeaders[];
};

struct Vertex {
    vec3 position;
#ifdef VERTEX_NORMAL
    vec3 normal;
#endif
#ifdef VERTEX_UV
    vec2 uv;
#endif
#ifdef VERTEX_TANGENT
    vec3 tangent;
    vec3 bitangent; // lol
#endif
};

Vertex FetchVertex(uint index, MeshHeader mesh) {
    Vertex vertex;
    vertex.position = vec3(attributes[index],attributes[index+1],attributes[index+2]);

#ifdef VERTEX_NORMAL
    vertex.normal = vec3(attributes[index+3],attributes[index+4],attributes[index+5]);
#endif

#ifdef VERTEX_UV
    vertex.uv = vec2(attributes[index+mesh.uvOffset],attributes[index+mesh.uvOffset+1]);
#endif

#ifdef VERTEX_TANGENT    
    vertex.tangent = vec3(attributes[index+mesh.tangentOffset+0],attributes[index+mesh.tangentOffset+1],attributes[index+mesh.tangentOffset+2]);
    vertex.bitangent = vec3(attributes[index+mesh.tangentOffset+3],attributes[index+mesh.tangentOffset+4],attributes[index+mesh.tangentOffset+5]);
#endif
    return vertex;
}

uint GetModelVertex(out mat4 model, out Vertex vertexAttributes) {    
    MaterialHeader header = materialHeaders[materialId];
    uint baseInstance = drawBaseInstance[gl_DrawID+header.filteredDrawBufferOffset];
    uvec2 key = keys[baseInstance+gl_InstanceID];

    MeshHeader mesh = meshHeaders[MeshIdFromKey(key.x)];
    vertexAttributes = FetchVertex(mesh.baseVertex + gl_VertexID*mesh.stride,mesh);

    model = renderItems[key.y].model;

    return renderItems[key.y].materialInstanceId;
}

#endif