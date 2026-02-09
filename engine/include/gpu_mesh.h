// Tom Kellett 2025
#pragma once
#include <glad/gl.h>
#include <vector>
#include <memory>
#include <stddef.h>
#include <glm/glm.hpp>
#include "culling.h"
#include <iostream>
#include "storage_buffer.h"
#include "mesh.h"

namespace Engine {
    class GpuMeshBuilder {     
    public:  
        std::vector<glm::vec3> verts;
        std::vector<glm::vec2> uvs;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec3> tangents;
        std::vector<glm::vec3> bitangents;
        std::vector<GLuint> indices;
        VertexFormat vertexFormat;
        AABB aabb;

        void SetVerts(std::vector<glm::vec3>& verts) {
            aabb = AABB(verts);
            this->verts = verts;
        }
        void SetNormals(std::vector<glm::vec3>& normals) {
            this->normals = normals;
            vertexFormat.normalsEnabled = true;
        }
        void SetUvs(std::vector<glm::vec2>& uvs) {
            this->uvs = uvs;
            vertexFormat.uvEnabled = true;
        }
        void SetIndices(std::vector<GLuint>& indices) {
            this->indices = indices;
        }
        void SetTangentsAndBitangents(std::vector<glm::vec3>& tangents, std::vector<glm::vec3>& bitangents) {
            this->tangents = tangents;
            this->bitangents = bitangents;
            vertexFormat.hasTangents = true;
        }

        void CalculateNormals() {           
            normals.assign(verts.size(), glm::vec3(0.f));

            if(indices.size() % 3 != 0) {
                std::cout << "Calculating normals error: Bad number of indices " << indices.size() << " is not divisible by 3" << std::endl;
                vertexFormat.normalsEnabled = false;
                normals.clear();
                return;
            }
            for(int t=0; t<indices.size(); t+=3) {
                GLuint i0 = indices[t], i1 = indices[t+1], i2 = indices[t+2];

                glm::vec3 a = verts[i0];
                glm::vec3 b = verts[i1];
                glm::vec3 c = verts[i2];

                glm::vec3 face_normal = glm::cross(b-a,c-a);

                normals[i0] += face_normal;
                normals[i1] += face_normal;
                normals[i2] += face_normal;
            }

            for(int i=0; i<normals.size(); i++) {
                normals[i] = glm::normalize(normals[i]);
            }

            vertexFormat.normalsEnabled = true;
        }
    };
    
    struct MeshHeader {
        glm::vec3 aabbMin;
        uint32_t id;

        glm::vec3 aabbMax;
        uint32_t baseVertex;

        uint32_t count;
        uint32_t firstIndex;
        uint32_t stride;
        uint32_t PACK;

        AABB aabb() { return AABB(aabbMin, aabbMax ); }
    };

    struct MeshCache {
    private:
        std::vector<GLuint> indexStagingBuffer;
        std::vector<float> attributesStagingBuffer;



        const size_t indexCapacity = 1024*1024; // 4 MB
        const size_t attributesCapacity = 16*1024*1024; // 64 MB

    public:
        GLuint vao;
        size_t attributesHead = 0;
        size_t indexHead = 0;

        StorageBuffer indexBuffer;
        StorageBuffer attributesBuffer;
        std::vector<MeshHeader> meshHeaders;

        size_t numMeshes;

        MeshCache() : attributesBuffer(attributesCapacity*4), indexBuffer(indexCapacity*4) {
            glGenVertexArrays(1, &vao);

            glBindVertexArray(vao);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,indexBuffer.object());
            glBindVertexArray(0);
        }

        void FlushStagingBuffer() {
            assert(attributesHead+attributesStagingBuffer.size() < attributesCapacity);
            if(attributesStagingBuffer.size() > 0)
                attributesBuffer.Set<float>(attributesStagingBuffer.data(), attributesStagingBuffer.size(), attributesHead, false);
            attributesHead = attributesHead+attributesStagingBuffer.size();
            attributesStagingBuffer.clear();

            assert(indexHead+indexStagingBuffer.size() < indexCapacity);
            if(indexStagingBuffer.size() > 0)
                indexBuffer.Set<uint32_t>(indexStagingBuffer.data(), indexStagingBuffer.size(), indexHead, false);
            indexHead = indexHead+indexStagingBuffer.size();
            indexStagingBuffer.clear();
        }

        // note to self
        // It is valid to call this function on a mesh that has already been generated, so make sure to clean pre-exisitng data properly...
        uint32_t RegisterMesh(GpuMeshBuilder& meshBuilder) {
            MeshHeader header;
            header.id = numMeshes++;
            header.aabbMin = meshBuilder.aabb.min;
            header.aabbMax = meshBuilder.aabb.max;
            header.count = meshBuilder.indices.size();
            header.firstIndex = indexHead + indexStagingBuffer.size();
            header.baseVertex = attributesHead + attributesStagingBuffer.size();
            header.stride = meshBuilder.vertexFormat.stride() / 4; // stride is in 32 bit units, for now
            meshHeaders.push_back(header);

            std::vector<glm::vec3>&  verts = meshBuilder.verts;
            std::vector<glm::vec3>&  normals = meshBuilder.normals;
            std::vector<glm::vec3>&  tangents = meshBuilder.tangents;
            std::vector<glm::vec3>&  bitangents = meshBuilder.bitangents;
            std::vector<glm::vec2>&  uvs = meshBuilder.uvs;

            for(int i = 0; i<verts.size(); i++) {
                attributesStagingBuffer.push_back(verts[i].x);
                attributesStagingBuffer.push_back(verts[i].y);
                attributesStagingBuffer.push_back(verts[i].z);
                if(meshBuilder.vertexFormat.normalsEnabled) {
                    if(i < normals.size()) {
                        attributesStagingBuffer.push_back(normals[i].x);
                        attributesStagingBuffer.push_back(normals[i].y);
                        attributesStagingBuffer.push_back(normals[i].z);
                    } else {
                        // Default value
                        attributesStagingBuffer.push_back(0.f);
                        attributesStagingBuffer.push_back(0.f);
                        attributesStagingBuffer.push_back(0.f);
                    }
                }
                if(meshBuilder.vertexFormat.uvEnabled) {
                    if(i < uvs.size()) {
                        attributesStagingBuffer.push_back(uvs[i].x);
                        attributesStagingBuffer.push_back(uvs[i].y);
                    } else {
                        // Default value
                        attributesStagingBuffer.push_back(0.f);
                        attributesStagingBuffer.push_back(0.f);
                    }
                }
                if(meshBuilder.vertexFormat.hasTangents) {
                    if(i < tangents.size()) {
                        attributesStagingBuffer.push_back(tangents[i].x);
                        attributesStagingBuffer.push_back(tangents[i].y);
                        attributesStagingBuffer.push_back(tangents[i].z);
                        attributesStagingBuffer.push_back(bitangents[i].x);
                        attributesStagingBuffer.push_back(bitangents[i].y);
                        attributesStagingBuffer.push_back(bitangents[i].z);
                    } else {
                        attributesStagingBuffer.push_back(0.f);
                        attributesStagingBuffer.push_back(0.f);
                        attributesStagingBuffer.push_back(0.f);
                        attributesStagingBuffer.push_back(0.f);
                        attributesStagingBuffer.push_back(0.f);
                        attributesStagingBuffer.push_back(0.f);
                    }
                }
            }

            indexStagingBuffer.insert(indexStagingBuffer.end(),meshBuilder.indices.begin(),meshBuilder.indices.end());
            return meshHeaders.size()-1;
        }
    };
}