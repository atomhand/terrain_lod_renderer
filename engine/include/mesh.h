#pragma once
#include <glad/glad.h>
#include <vector>
#include <memory>
#include <stddef.h>
#include <glm/glm.hpp>

namespace Engine {
    struct VertexFormat {
        bool normalsEnabled = false;
        bool uvEnabled = false;

        GLsizei stride() {
            return sizeof(glm::vec3) + (normalsEnabled ? sizeof(glm::vec3) : 0) + (uvEnabled ? sizeof(glm::vec2) : 0);
        }

        size_t normalsOffset() {
            return sizeof(glm::vec3);
        }
        size_t uvOffset() {
            return sizeof(glm::vec3) + (normalsEnabled ? sizeof(glm::vec3) : 0);
        }

        GLuint positionAttributeIndex() { return 0; }
        GLuint normalAttributeIndex() { return 1; }
        GLuint uvAttributeIndex() { return 2; }
    };


    class Mesh
    {
    private:
        struct MeshData {    
            GLuint vao;        
            GLuint vertexBuffer;
            GLuint indexBuffer;

            std::vector<glm::vec3> verts;
            std::vector<glm::vec2> uvs;
            std::vector<glm::vec3> normals;
            std::vector<GLuint> indices;
            VertexFormat vertexFormat;

            MeshData & operator=(const MeshData&) = delete;
            MeshData(const MeshData&) = delete;

            MeshData() {                
                glGenBuffers(1, &vertexBuffer);
                glGenBuffers(1, &indexBuffer);
                glGenVertexArrays(1, &vao);
            }
            ~MeshData() {                
                glDeleteVertexArrays(1,&vao);
                glDeleteBuffers(1,&indexBuffer);
                glDeleteBuffers(1,&vertexBuffer);
            }

            void Apply() {
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
                            bufferData.push_back(1.0 - uvs[i].y);
                        } else {
                            // Default value
                            bufferData.push_back(0.f);
                            bufferData.push_back(0.f);
                        }
                    }
                }

                // vertex data buffer
                glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
                glBufferData(GL_ARRAY_BUFFER, bufferData.size() * sizeof(float), bufferData.data(), GL_STATIC_DRAW);

                // index buffer
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
                
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
                
                // indices
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

                // unbind
                glBindVertexArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
        };
        std::shared_ptr<MeshData> data;

        bool generated =false;
    public:

        GLsizei count() { return int(data->indices.size()); }

        GLuint vao() {
            return data->vao;
        }

        void SetVerts(std::vector<glm::vec3> verts) {
            data->verts = verts;
        }
        void SetNormals(std::vector<glm::vec3> normals) {
            data->normals = normals;
            data->vertexFormat.normalsEnabled = true;
        }
        void SetUvs(std::vector<glm::vec2> uvs) {
            data->uvs = uvs;
            data->vertexFormat.uvEnabled = true;
        }
        void SetIndices(std::vector<GLuint> indices) {
            data->indices = indices;
        }

        void Apply() {
            data->Apply();
        }

        Mesh() {
            data = std::make_shared<MeshData>();
        };

        ~Mesh() {
        }
    };
}