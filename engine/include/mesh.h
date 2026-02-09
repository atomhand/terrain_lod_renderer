// Tom Kellett 2025
#pragma once
#include <glad/gl.h>
#include <vector>
#include <memory>
#include <stddef.h>
#include <glm/glm.hpp>
#include "culling.h"
#include <iostream>

namespace Engine {
    struct VertexFormat {
        bool normalsEnabled = false;
        bool uvEnabled = false;
        bool hasTangents = false;

        GLsizei stride() {
            return sizeof(glm::vec3) + (normalsEnabled ? sizeof(glm::vec3) : 0) + (uvEnabled ? sizeof(glm::vec2) : 0) + (hasTangents ? sizeof(glm::vec3) * 2: 0);
        }

        size_t normalsOffset() {
            return sizeof(glm::vec3);
        }
        size_t uvOffset() {
            return normalsOffset()  + (normalsEnabled ? sizeof(glm::vec3) : 0);
        }
        size_t tangentOffset() {
            return uvOffset() + (uvEnabled ? sizeof(glm::vec2) : 0);
        }
        size_t bitangentOffset() {
            return tangentOffset() + sizeof(glm::vec3);
        }

        std::vector<const char*> GetShaderDefs() {
            std::vector<const char*> defs;
            if(normalsEnabled) defs.push_back("#DEFINE VERTEX_NORMAL");
            if(uvEnabled) defs.push_back("#DEFINE VERTEX_UV");
            if(hasTangents) defs.push_back("#DEFINE VERTEX_TANGENT");
            return defs;
        }

        GLuint positionAttributeIndex() { return 0; }
        GLuint normalAttributeIndex() { return 1; }
        GLuint uvAttributeIndex() { return 2; }
        GLuint tangentAttributeIndex() { return 3; }
        GLuint bitangentAttributeIndex() { return 4; }
    };

    class Mesh
    {
    private:
        struct MeshData {    
            GLuint vao;        
            GLuint vertexBuffer;
            GLuint indexBuffer;
            std::vector<GLuint> lodOffsets;
            size_t activeLod;

            MeshData & operator=(const MeshData&) = delete;
            MeshData(const MeshData&) = delete;

            bool generated =false;

            MeshData() {
                glGenBuffers(1, &vertexBuffer);
                glGenBuffers(1, &indexBuffer);
                glGenVertexArrays(1, &vao);
            }

            ~MeshData() {    
                if(vao != 0) {
                    glDeleteVertexArrays(1,&vao);
                    glDeleteBuffers(1,&indexBuffer);
                    glDeleteBuffers(1,&vertexBuffer);
                }            
            }

            // note to self
            // It is valid to call this function on a mesh that has already been generated, so make sure to clean pre-exisitng data properly...
            void Apply(std::vector<glm::vec3>& verts, std::vector<glm::vec3>& normals, std::vector<glm::vec2>& uvs, std::vector<glm::vec3>& tangents, std::vector<glm::vec3>& bitangents, std::vector<std::vector<GLuint>>& lodIndices, VertexFormat& vertexFormat) {
                generated = true;
                //lodIndices = inLodIndices;
                std::vector<float> bufferData;
                for(int i = 0; i<verts.size(); i++) {
                    bufferData.push_back(verts[i].x);
                    bufferData.push_back(verts[i].y);
                    bufferData.push_back(verts[i].z);
                    if(vertexFormat.normalsEnabled) {
                        if(i < normals.size()) {
                            bufferData.push_back(normals[i].x);
                            bufferData.push_back(normals[i].y);
                            bufferData.push_back(normals[i].z);
                        } else {
                            // Default value
                            bufferData.push_back(0.f);
                            bufferData.push_back(0.f);
                            bufferData.push_back(0.f);
                        }
                    }
                    if(vertexFormat.uvEnabled) {
                        if(i < uvs.size()) {
                            bufferData.push_back(uvs[i].x);
                            bufferData.push_back(uvs[i].y);
                        } else {
                            // Default value
                            bufferData.push_back(0.f);
                            bufferData.push_back(0.f);
                        }
                    }
                    if(vertexFormat.hasTangents) {
                        if(i < tangents.size()) {
                            bufferData.push_back(tangents[i].x);
                            bufferData.push_back(tangents[i].y);
                            bufferData.push_back(tangents[i].z);
                            bufferData.push_back(bitangents[i].x);
                            bufferData.push_back(bitangents[i].y);
                            bufferData.push_back(bitangents[i].z);
                        } else {
                            bufferData.push_back(0.f);
                            bufferData.push_back(0.f);
                            bufferData.push_back(0.f);
                            bufferData.push_back(0.f);
                            bufferData.push_back(0.f);
                            bufferData.push_back(0.f);
                        }
                    }
                }

                // vertex data buffer
                glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
                glBufferData(GL_ARRAY_BUFFER, bufferData.size() * sizeof(float), bufferData.data(), GL_STATIC_DRAW);

                // index buffer
                activeLod = 0;
                lodOffsets.clear();
                std::vector<GLuint> indexData;
                for(int i=0; i<lodIndices.size(); i++) {
                    lodOffsets.push_back(indexData.size());
                    indexData.insert(indexData.end(),lodIndices[i].begin(),lodIndices[i].end());
                }
                lodOffsets.push_back(indexData.size()); // marks end for the final LoD
                       
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexData.size() * sizeof(GLuint), indexData.data(), GL_STATIC_DRAW);
                
                // set up VAO
                glBindVertexArray(vao);

                // positions
                glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
                glEnableVertexAttribArray(vertexFormat.positionAttributeIndex()); 
                glVertexAttribPointer(vertexFormat.positionAttributeIndex(), 3, GL_FLOAT, GL_FALSE, vertexFormat.stride(), 0);
                
                // normals
                //glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
                if(vertexFormat.normalsEnabled) {
                    glEnableVertexAttribArray(vertexFormat.normalAttributeIndex());
                    glVertexAttribPointer(vertexFormat.normalAttributeIndex(), 3, GL_FLOAT, GL_FALSE, vertexFormat.stride(), (const void*)vertexFormat.normalsOffset());
                }

                if(vertexFormat.uvEnabled) {
                    glEnableVertexAttribArray(vertexFormat.uvAttributeIndex());
                    glVertexAttribPointer(vertexFormat.uvAttributeIndex(), 2, GL_FLOAT, GL_FALSE, vertexFormat.stride(), (const void*)vertexFormat.uvOffset());
                }

                if(vertexFormat.hasTangents) {                    
                    glEnableVertexAttribArray(vertexFormat.tangentAttributeIndex());
                    glVertexAttribPointer(vertexFormat.tangentAttributeIndex(), 3, GL_FLOAT, GL_FALSE, vertexFormat.stride(), (const void*)vertexFormat.tangentOffset());
                    
                    glEnableVertexAttribArray(vertexFormat.bitangentAttributeIndex());
                    glVertexAttribPointer(vertexFormat.bitangentAttributeIndex(), 3, GL_FLOAT, GL_FALSE, vertexFormat.stride(), (const void*)vertexFormat.bitangentOffset());
                }
                
                // indices
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

                // unbind
                glBindVertexArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }

            void Draw() {                         
                glBindVertexArray(vao);
                GLuint start = lodOffsets[activeLod];
                GLuint end = lodOffsets[activeLod+1];
                glDrawElements(GL_TRIANGLES, end-start,GL_UNSIGNED_INT,(void*)(start*sizeof(GLuint)));
                glBindVertexArray(0);
            }
            
            void DrawInstanced(GLuint count) {                         
                glBindVertexArray(vao);
                GLuint start = lodOffsets[activeLod];
                GLuint end = lodOffsets[activeLod+1];
                glDrawElementsInstanced(GL_TRIANGLES, end-start,GL_UNSIGNED_INT,(void*)(start*sizeof(GLuint)),count);
                glBindVertexArray(0);
            }
        };
        std::shared_ptr<MeshData> data;
    public:    
        class MeshBuilder {      
        public:  
            std::vector<glm::vec3> verts;
            std::vector<glm::vec2> uvs;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec3> tangents;
            std::vector<glm::vec3> bitangents;
            std::vector<std::vector<GLuint>> lodIndices;
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
            void SetLodIndices(std::vector<std::vector<GLuint>>& indices) {
                this->lodIndices = indices;
            }
            void SetIndices(std::vector<GLuint>& indices) {
                this->lodIndices = { indices };
            }
            void SetTangentsAndBitangents(std::vector<glm::vec3>& tangents, std::vector<glm::vec3>& bitangents) {
                this->tangents = tangents;
                this->bitangents = bitangents;
                vertexFormat.hasTangents = true;
            }

            void CalculateNormals() {           
                normals.assign(verts.size(), glm::vec3(0.f));

                for(auto& indices : lodIndices) {
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
                }
                for(int i=0; i<normals.size(); i++) {
                    normals[i] = glm::normalize(normals[i]);
                }

                vertexFormat.normalsEnabled = true;
            }

            void ApplyToMesh(Mesh& mesh) {
                assert(verts.size() > 0);
                assert(lodIndices.size() > 0);
                assert(aabb.IsValid());
                mesh.data->Apply(verts,normals,uvs,tangents,bitangents,lodIndices,vertexFormat);
                if(lodIndices[0].size() == 0)
                    mesh.data->generated = false;

                mesh.aabb = aabb;
            }

            Mesh CreateMesh() {
                Mesh mesh;
                ApplyToMesh(mesh);
                return mesh;
            }
        };

        bool IsValid() {
            return data->generated != 0;
        }

        int GetIndexCount(int lodLevel) {
            assert(lodLevel < data->lodOffsets.size()-1);
            int start = data->lodOffsets[lodLevel];
            int end = data->lodOffsets[lodLevel+1];
            return end-start;
        }

        AABB aabb;

        GLuint vao() {
            assert(data->vao != 0);
            return data->vao;
        }

        void SetLod(size_t level) {
            assert(level < data->lodOffsets.size()-1);
            data->activeLod = level;
        }

        size_t NumLods() {
            return data->lodOffsets.size()-1;
        }

        void Draw() {
            data->Draw();
        }
        void DrawInstanced(GLuint count) {
            data->DrawInstanced(count);
        }

        Mesh() {
            data = std::make_shared<MeshData>();
        };

        ~Mesh() {
        }
    };
}