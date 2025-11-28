#pragma once
#include <glad/glad.h>
#include <vector>
#include <glm/glm.hpp>

namespace Engine {
    struct VertexFormat {
        bool normalsEnabled = false;
        bool uvEnabled = false;

        GLsizei stride() {
            return sizeof(glm::vec3) + (normalsEnabled ? sizeof(glm::vec3) : 0) + (uvEnabled ? sizeof(glm::vec2) : 0);
        }

        GLsizei normalsOffset() {
            return sizeof(glm::vec3);
        }
        GLsizei uvOffset() {
            return sizeof(glm::vec3) + (normalsEnabled ? sizeof(glm::vec3) : 0);
        }

        GLuint positionAttributeIndex() { return 0; }
        GLuint normalAttributeIndex() { return 1; }
        GLuint uvAttributeIndex() { return 2; }
    };


    class Mesh
    {
    private:
        GLuint vertexBuffer;
        GLuint indexBuffer;

        std::vector<glm::vec3> verts;
        std::vector<glm::vec2> uvs;
        std::vector<glm::vec3> normals;
        std::vector<GLuint> indices;
        VertexFormat vertexFormat;

        bool generated =false;

        Mesh & operator=(const Mesh&) = delete;
        Mesh(const Mesh&) = delete;
    public:

        GLsizei count() { return int(indices.size()); }
        GLuint vao;

        void SetVerts(std::vector<glm::vec3> verts) {
            this->verts = verts;
        }
        void SetNormals(std::vector<glm::vec3> normals) {
            this->normals = normals;
            vertexFormat.normalsEnabled = true;
        }
        void SetUvs(std::vector<glm::vec2> uvs) {
            this->uvs = uvs;
            vertexFormat.uvEnabled = true;
        }
        void SetIndices(std::vector<GLuint> indices) {
            this->indices = indices;
        }

        void Apply() {
            if(!generated) {
                glGenBuffers(1, &vertexBuffer);
                glGenBuffers(1, &indexBuffer);
                glGenVertexArrays(1, &vao);
                generated = true;
            }

            std::vector<float> data;
            for(int i = 0; i<verts.size(); i++) {
                data.push_back(verts[i].x);
                data.push_back(verts[i].y);
                data.push_back(verts[i].z);
                if(vertexFormat.normalsEnabled) {
                    if(i < normals.size()) {
                        data.push_back(normals[i].x);
                        data.push_back(normals[i].y);
                        data.push_back(normals[i].z);
                    } else {
                        // Default value
                        data.push_back(0.f);
                        data.push_back(0.f);
                        data.push_back(0.f);
                    }
                }
                if(vertexFormat.uvEnabled) {
                    if(i < uvs.size()) {
                        data.push_back(uvs[i].x);
                        data.push_back(1.0 - uvs[i].y);
                    } else {
                        // Default value
                        data.push_back(0.f);
                        data.push_back(0.f);
                    }
                }
            }

            // vertex data buffer
            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
            glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);

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

        Mesh() {};

        Mesh(std::vector<glm::vec3> _vertexPositions, 
            std::vector<glm::vec3> _normals,    
            std::vector<GLuint> _indices) : verts(_vertexPositions) , normals(_normals), indices(_indices)
        {
            vertexFormat.normalsEnabled = true;
            Apply();
        }
        ~Mesh() {
            glDeleteVertexArrays(1,&vao);
            glDeleteBuffers(1,&indexBuffer);
            glDeleteBuffers(1,&vertexBuffer);
        }
    };
}