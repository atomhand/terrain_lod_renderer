#pragma once

#include <glad/glad.h>
#include "shader.h"
#include "terrain_config.h"

class TerrainMesh
{
private:
    GLuint positionBufferObject;
    GLuint vao;
    
    TerrainConfig terrain_config;
public:
    Engine::Shader shader;
    void draw() {
        shader.use();
        glBindBuffer(GL_ARRAY_BUFFER, positionBufferObject);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glDisableVertexAttribArray(0);
    }



    TerrainMesh(TerrainConfig terrain_config, Engine::Shader& shader) : shader(shader), terrain_config(terrain_config) {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        float vertexPositions[] = {
            0.75f, 0.75f, 0.0f, 1.0f,
            0.75f, -0.75f, 0.0f, 1.0f,
            -0.75f, -0.75f, 0.0f, 1.0f,
            0.75f, 0.75f, 0.0f, 1.0f,
            0.75f, -0.75f, 0.0f, 1.0f,
            -0.75f, -0.75f, 0.0f, 1.0f,
        };

        glGenBuffers(1, &positionBufferObject);
        glBindBuffer(GL_ARRAY_BUFFER, positionBufferObject);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    ~TerrainMesh() {

    }
};