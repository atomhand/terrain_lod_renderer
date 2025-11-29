#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>
#include "glm/gtc/random.hpp"

#include <glad/glad.h>
#include "mesh.h"
#include "demo_world.h"
#include "render_item.h"


class Terrain {
    int width;
    float scale;
    Engine::Mesh mesh;

    float heightScale = 32.0;

    float Height(float x, float z) {
        float fwidth = width*scale;

        float freq = 2.0 / fwidth;
        float amp = 1.0;

        float result = 0.f;
        float normalise_sum = 0.f;

        int octaves = 8;
        for(int i =0; i<octaves; i++) {
            result += glm::perlin(glm::vec2(x,z) * freq) * amp;

            normalise_sum += amp;
            freq *= 2.f;
            amp *= 0.5f;
        }

        return heightScale * result / normalise_sum;
    }

    Engine::Mesh terrainMesh() {
        std::vector<glm::vec3> verts;
        std::vector<glm::vec3> normals;
        std::vector<GLuint> indices;
        indices.reserve(6 * (width-1)*(width-1));
        verts.reserve(width*width);
        normals.reserve(width*width);

        int y;
        for(y=0; y<width; y++) {            
            for(int x=0; x<width; x++) {
                float X = (float(x)-width/2.f) * scale;
                float Z = (float(y)-width/2.f) * scale;
                verts.push_back(glm::vec3(X,Height(X,Z),Z));
            }
        }

        for(y=0; y<width-1; y++) {            
            for(int x=0; x<width-1; x++) {
                GLuint i00 = x + y*width;
                GLuint i10 = (x+1) + y*width;
                GLuint i01 = x + (y+1)*width;
                GLuint i11 = (x+1) + (y+1)*width;

                // Choose the diagonal to split the quad along

                // We choose the split that minimises the length
                // of the diagonal edge
                // (Hopefully this slightly improves the appearance)
                float e0 = std::abs(verts[i11].y - verts[i00].y);
                float e1 = std::abs(verts[i10].y - verts[i01].y);

                if(e0 < e1) {
                    indices.push_back(i00);
                    indices.push_back(i01);
                    indices.push_back(i11);

                    indices.push_back(i00);
                    indices.push_back(i11);
                    indices.push_back(i10);
                } else {
                    indices.push_back(i01);
                    indices.push_back(i11);
                    indices.push_back(i10);

                    indices.push_back(i01);
                    indices.push_back(i10);
                    indices.push_back(i00);
                }
            }
        }

        Engine::Mesh mesh;
        mesh.SetVerts(verts);
        mesh.SetIndices(indices);
        mesh.CalculateNormals();
        mesh.Apply();
        return mesh;
    }

    Engine::Mesh waterMesh() {
        std::vector<glm::vec3> verts;
        std::vector<glm::vec3> normals;
        std::vector<GLuint> indices;
        indices.reserve(6 * (width-1)*(width-1));
        verts.reserve(width*width);
        normals.reserve(width*width);

        int y;
        for(y=0; y<width; y++) {            
            for(int x=0; x<width; x++) {
                float X = (float(x)-width/2.f) * scale;
                float Z = (float(y)-width/2.f) * scale;
                verts.push_back(glm::vec3(X,0.f,Z));
            }
        }

        for(y=0; y<width-1; y++) {            
            for(int x=0; x<width-1; x++) {
                GLuint i00 = x + y*width;
                GLuint i10 = (x+1) + y*width;
                GLuint i01 = x + (y+1)*width;
                GLuint i11 = (x+1) + (y+1)*width;

                indices.push_back(i00);
                indices.push_back(i01);
                indices.push_back(i11);

                indices.push_back(i00);
                indices.push_back(i11);
                indices.push_back(i10);
            }
        }

        Engine::Mesh mesh;
        mesh.SetVerts(verts);
        mesh.SetIndices(indices);
        mesh.CalculateNormals();
        mesh.Apply();
        return mesh;
    }
public:

    void Setup(DemoWorld& world) {
        Engine::Shader pbr_shader = Engine::Shader("shaders/pbr.vert", "shaders/pbr.frag");

		auto terrain_material = Engine::PbrMaterial(pbr_shader);
        terrain_material.roughness = 1.0;
        terrain_material.albedo = glm::vec3(0.2,0.5,0.2);
        RenderItem* terrainItem = new RenderItem();
		terrainItem->mesh = terrainMesh();
		terrainItem->material = std::make_shared<Engine::PbrMaterial>(terrain_material);
		terrainItem->localTransform = glm::mat4(1.0);
		world.scenegraph.SetParent(terrainItem, world.scenegraph.root);

        auto water_material = Engine::PbrMaterial(pbr_shader);
        water_material.roughness = 0.03;
        RenderItem* waterItem = new RenderItem();
		waterItem->mesh = waterMesh();
		waterItem->material = std::make_shared<Engine::PbrMaterial>(water_material);
		waterItem->localTransform = glm::mat4(1.0);
		world.scenegraph.SetParent(waterItem, world.scenegraph.root);
    }

    // width specified in number of verts per side
    Terrain(int width, float scale) : width(width), scale(scale) {};
};