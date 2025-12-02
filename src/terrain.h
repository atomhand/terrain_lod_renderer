#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>
#include "glm/gtc/random.hpp"

#include <glad/glad.h>
#include "mesh.h"
#include "world.h"
#include "render_item.h"


class Terrain : public Engine::SceneNode {
    int width;
    float scale;
    Engine::Mesh mesh;

    float heightScale = 32.0;

    float Height(float x, float z) {
        float fwidth = width*scale;

        float freq = 1.0 / 64.0;
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
        result /= normalise_sum;

        float center = fwidth/2.f;
        float sq_d = std::min(1.0f, (x*x + z*z)/(center*center));
        result = (result)*(1.0f-sq_d) + 0.5f * (1.f + result) * sq_d;

        float threshold = 0.75;
        float boost = 2.0f;
        if(result > threshold) {
            result = (result-threshold)*boost + threshold;
        }

        return heightScale * result;
    }

    float SampleHeightmap(int x, int z) {
        int idx = x + z * (width+1);
        return heightMap[idx];
    }

    glm::vec3 GetPos(int x, int z) {
        return glm::vec3(x*scale,SampleHeightmap(x,z),z*scale);
    }

    // Calculate vertex normals directly from heightmap
    // This avoids the issue of seams along the chunks that would arise from calculating face normals
    // formula from https://www.reddit.com/r/opengl/comments/8myqys/normals_of_a_heightmap_terrain/dzrpya2/
    glm::vec3 Normal(int x, int y) {
        glm::vec3 L = x > 0 ? GetPos(x-1,y) : GetPos(x,y);
        glm::vec3 R = x <= chunkWidth ? GetPos(x+1,y) : GetPos(x,y);
        glm::vec3 U = y > 0 ? GetPos(x,y-1) : GetPos(x,y);
        glm::vec3 D = y <= chunkWidth ? GetPos(x,y+1) : GetPos(x,y);
        return glm::normalize(glm::cross(R-L,U-D));
    }

    int chunkWidth = 16;

    Engine::Mesh MakeTerrainMesh(int startX, int startY) {
        std::vector<glm::vec3> verts;
        std::vector<glm::vec3> normals;
        std::vector<GLuint> indices;

        int y;
        for(y=0; y<chunkWidth+1; y++) {            
            for(int x=0; x<chunkWidth+1; x++) {
                int idx = (startX+x) + (startY+y) * (width+1);
                verts.push_back(glm::vec3(x * scale,heightMap[idx],y * scale));
                normals.push_back(Normal(x+startX,y+startY));
            }
        }

        for(y=0; y<chunkWidth; y++) {            
            for(int x=0; x<chunkWidth; x++) {
                GLuint i00 = x + y*(chunkWidth+1);
                GLuint i10 = (x+1) + y*(chunkWidth+1);
                GLuint i01 = x + (y+1)*(chunkWidth+1);
                GLuint i11 = (x+1) + (y+1)*(chunkWidth+1);

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
        mesh.SetNormals(normals);
        mesh.Apply();
        return mesh;
    }

    Engine::Mesh MakeWaterMesh() {
        std::vector<glm::vec3> verts;
        std::vector<glm::vec3> normals;
        std::vector<GLuint> indices;

        int y;
        for(y=0; y<chunkWidth+1; y++) {            
            for(int x=0; x<chunkWidth+1; x++) {
                verts.push_back(glm::vec3(x * scale,0.f,y * scale));
                normals.push_back(glm::vec3(0.,1.,0.));
            }
        }

        for(y=0; y<chunkWidth; y++) {            
            for(int x=0; x<chunkWidth; x++) {
                GLuint i00 = x + y*(chunkWidth+1);
                GLuint i10 = (x+1) + y*(chunkWidth+1);
                GLuint i01 = x + (y+1)*(chunkWidth+1);
                GLuint i11 = (x+1) + (y+1)*(chunkWidth+1);

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
        mesh.SetNormals(normals);
        mesh.Apply();
        return mesh;
    }
public:

    void OnEnter(Engine::SceneGraph& sceneGraph) override {
        
        Engine::Shader water_shader = Engine::Shader("shaders/pbr.vert", "shaders/water_pbr.frag");
        auto water_material = Engine::PbrMaterial(water_shader);
        water_material.roughness = 0.03;
        water_material.textures.push_back(Engine::Texture::Import("textures/waterN1.jpg"));
        water_material.textures.push_back(Engine::Texture::Import("textures/waterN2.jpg"));
        water_material.albedo = glm::vec3(0.465f, 0.797f, 0.991f);

        Engine::Shader pbr_shader = Engine::Shader("shaders/pbr.vert", "shaders/terrain_pbr.frag");

		auto terrain_material = Engine::PbrMaterial(pbr_shader);
        terrain_material.roughness = 1.0;
        terrain_material.albedo = glm::vec3(0.2,0.5,0.2);
        terrain_material.textures.push_back(Engine::Texture::Import("textures/grass2/rocky_terrain_02_diff_2k.jpg"));
        terrain_material.textures.push_back(Engine::Texture::Import("textures/grass2/rocky_terrain_02_nor_gl_2k.png"));
        //terrain_material.textures.push_back(Engine::Texture::Import("textures/sand/coast_sand_01_diff_2k.jpg"));
        //terrain_material.textures.push_back(Engine::Texture::Import("textures/sand/coast_sand_01_nor_gl_2k.png"));

        auto terrain_mat_pointer = std::make_shared<Engine::PbrMaterial>(terrain_material);
        auto water_mat_pointer = std::make_shared<Engine::PbrMaterial>(water_material);

        Engine::Mesh waterMesh = MakeWaterMesh();

        int chunks = width/chunkWidth;
        for(int x=0; x<chunks; x++)
            for(int y=0; y<chunks; y++) {
                float X = (x*chunkWidth-width/2.f) * scale;
                float Z = (y*chunkWidth-width/2.f) * scale;

                RenderItem* terrainItem = new RenderItem();                
                terrainItem->mesh = MakeTerrainMesh(x*chunkWidth,y*chunkWidth);
                terrainItem->material = terrain_mat_pointer;
                terrainItem->localTransform = glm::translate(glm::mat4(1.0),glm::vec3(X,0.0,Z));
                sceneGraph.SetParent(terrainItem, this);
        
                RenderItem* waterItem = new RenderItem();
                waterItem->mesh = waterMesh;
                waterItem->material = water_mat_pointer;
                waterItem->shadowEnabled = false;
                waterItem->localTransform = glm::translate(glm::mat4(1.0),glm::vec3(X,0.0,Z));
		        sceneGraph.SetParent(waterItem, this);
            }
    }

    std::vector<float> heightMap;

    // width specified in number of verts per side
    Terrain(int width, float scale) : width(width), scale(scale) {
        float fWidth = width*scale /2.f;
        for(int y=0; y<width+1; y++) {
            for(int x=0; x<width+1; x++) {
                float X = x * scale - fWidth;
                float Z = y * scale - fWidth;
                heightMap.push_back(Height(X,Z));
            }
        }
    };
};