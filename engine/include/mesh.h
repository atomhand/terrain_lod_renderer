#pragma once
#include <glad/glad.h>

namespace Engine {
    class Mesh
    {
    private:
        GLuint buffers[3];

        std::vector<glm::vec3> verts;
        std::vector<glm::vec3> normals;
        std::vector<GLuint> indices;
    public:
        GLuint positionBufferObject() { return buffers[0]; }
        GLuint normalsBufferObject() { return buffers[1]; }
        GLuint indexBufferObject() { return buffers[2]; }

        GLsizei count() { return int(indices.size()); }
        GLuint vao;

        Mesh(std::vector<glm::vec3> _vertexPositions, 
            std::vector<glm::vec3> _normals,    
            std::vector<GLuint> _indices) : verts(_vertexPositions) , normals(_normals), indices(_indices)
        {
            glGenBuffers(3, buffers);
            
            // fill buffer data
            glBindBuffer(GL_ARRAY_BUFFER, positionBufferObject());
            glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(glm::vec3), verts.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER, normalsBufferObject());
            glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferObject());
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
            
            // set up VAO
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            // positions
            glBindBuffer(GL_ARRAY_BUFFER, positionBufferObject());
            glEnableVertexAttribArray(0); 
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
            
            // normals
            glBindBuffer(GL_ARRAY_BUFFER, normalsBufferObject());
            glEnableVertexAttribArray(1); 
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
            
            // indices
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferObject());

            // unbind
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        ~Mesh() {
        }
    };
}