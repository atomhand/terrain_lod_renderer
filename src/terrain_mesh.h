#pragma once

#include <glad/glad.h>
#include "shader.h"

class TerrainMesh
{
private:
    GLuint positionBufferObject;
    GLuint vao;
public:
    Engine::Shader shader;
    void draw() {
        shader.use();
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    TerrainMesh(Engine::Shader& shader) : shader(shader) {
        glGenVertexArrays(1, &vao);

        float vertexPositions[] = {
            0.75f, 0.75f, 0.0f, 1.0f,
            0.75f, -0.75f, 0.0f, 1.0f,
            -0.75f, -0.75f, 0.0f, 1.0f,
            0.75f, 0.75f, 0.0f, 1.0f,
            0.75f, -0.75f, 0.0f, 1.0f,
            -0.75f, -0.75f, 0.0f, 1.0f,
        };

        glBindVertexArray(vao);

        glGenBuffers(1, &positionBufferObject);

        // Create and fill position buffer
        glBindBuffer(GL_ARRAY_BUFFER, positionBufferObject);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);

        // Bind to vertex array
        glEnableVertexAttribArray(0); 
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

        // unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    ~TerrainMesh() {

    }
};