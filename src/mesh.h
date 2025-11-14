#pragma once

#include <glad/glad.h>
#include "shader.h"

class Mesh
{
    GLuint buffers[3];

    std::vector<glm::vec4> verts;
    std::vector<glm::vec4> normals;
    std::vector<GLuint> indices;
public:
    GLuint positionBufferObject() { return buffers[0]; }
    GLuint normalsBufferObject() { return buffers[1]; }
    GLuint indexBufferObject() { return buffers[2]; }
    void* indexData() { return indices.data(); }

    GLsizei count() { return int(indices.size()); }

    GLuint vao;

    static Mesh Sphere(int w, int h) {
        std::vector<glm::vec4> verts;
        std::vector<glm::vec4> normals;
        std::vector<GLuint> indices;

        float pi = glm::pi<float>();

        verts.push_back(glm::vec4(0.0,1.0,0.0, 1.0));

        GLuint v0 = 0;

        for(int i=0; i<h; i++) {
            float phi = pi * float(i+1) / float(w);
            for(int j=0; j<w; j++) {
                float theta = 2.0 * pi * float(j) / float(h);
                glm::vec3 pos = glm::vec3(
                        glm::sin(phi) * glm::cos(theta),
                        glm::cos(phi),
                        glm::sin(phi) * glm::sin(theta)
                    );
                verts.push_back(glm::vec4(pos,1.0f));
            }
        }

        verts.push_back(glm::vec4(0.0,-1.0,0.0, 1.0));
        GLuint v1 = verts.size() - 1;

        // add normals
        for(auto vert : verts) {
            glm::vec3 normal = glm::normalize(glm::vec3(vert));
            normals.push_back(glm::vec4(normal,1.0f));
        }

        // add top/bottom triangles
        for(GLuint i =0; i<w; i++) {
            GLuint i0 = i+1;
            GLuint i1 = (i+1) % w + 1;
            indices.push_back(v0);
            indices.push_back(i1);
            indices.push_back(i0);

            i0 = i + w * (h-2) + 1;
            i1 = (i+1) % w + w * (h-2) + 1;
            
            indices.push_back(v1);
            indices.push_back(i0);
            indices.push_back(i1);
        }

        for(int j=0; j<h-2; j++) {
            auto j0 = j*w + 1;
            auto j1 = (j+1) * w + 1;
            for(int i=0; i<w; i++) {
                auto i0 = j0 + i;
                auto i1 = j0 + (i+1) % w;
                auto i2 = j1 + (i+1) % w;
                auto i3 = j1 + i;
                indices.push_back(i0);
                indices.push_back(i2);
                indices.push_back(i1);
                
                indices.push_back(i0);
                indices.push_back(i3);
                indices.push_back(i2);
            }
        }

        return Mesh(verts,normals,indices);
    }

    static Mesh Plane(int w, int h) {
        std::vector<glm::vec4> verts;
        std::vector<glm::vec4> normals;
        std::vector<GLuint> indices;

        verts.push_back(glm::vec4(0.0,0.0,0.0, 1.0));
        verts.push_back(glm::vec4(float(w),0.0,0.0, 1.0));
        verts.push_back(glm::vec4(0.0,0.0,float(h), 1.0));
        verts.push_back(glm::vec4(float(w),0.0,float(h), 1.0));

        normals.push_back(glm::vec4(0.0,1.0,0.0,1.0));
        normals.push_back(glm::vec4(0.0,1.0,0.0,1.0));
        normals.push_back(glm::vec4(0.0,1.0,0.0,1.0));
        normals.push_back(glm::vec4(0.0,1.0,0.0,1.0));
        
        indices.push_back(0);
        indices.push_back(2);
        indices.push_back(1);
        
        indices.push_back(2);
        indices.push_back(3);
        indices.push_back(1);

        return Mesh(verts,normals,indices);
    }

    

    Mesh(std::vector<glm::vec4> _vertexPositions, 
        std::vector<glm::vec4> _normals,    
        std::vector<GLuint> _indices) : verts(_vertexPositions) , normals(_normals), indices(_indices) {

        glGenBuffers(3, buffers);
        
        // fill buffer data
        glBindBuffer(GL_ARRAY_BUFFER, positionBufferObject());
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(glm::vec4), verts.data(), GL_STATIC_DRAW);            
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferObject());
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferObject());
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
        
        // set up VAO
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // positions
        glBindBuffer(GL_ARRAY_BUFFER, positionBufferObject());
        glEnableVertexAttribArray(0); 
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
        
        // normals
        glBindBuffer(GL_ARRAY_BUFFER, normalsBufferObject());
        glEnableVertexAttribArray(1); 
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
        
        // indices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferObject());

        // unbind
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    ~Mesh() {
    }
};