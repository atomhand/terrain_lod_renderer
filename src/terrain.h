#pragma once
#include <vector>
#include <glm/glm.hpp>

#include <glad/glad.h>
#include "FastNoiseLite.h"
#include "mesh.h"
#include "world.h"
#include "render_item.h"


class Terrain : public Engine::SceneNode {
    class Chunk : public Engine::SceneNode {
        float scale;        
        RenderItem* land;
        RenderItem* water;        
        std::vector<float> heightMap;
        bool generated;
        int offsetX,offsetY;

        float SampleHeightmap(int x, int z) {
            int idx = (x+1) + (z+1) * (CHUNK_WIDTH+3);
            return heightMap[idx];
        }

        glm::vec3 GetPos(int x, int z) {
            return glm::vec3(x*scale,SampleHeightmap(x,z),z*scale);
        }

        // Calculate vertex normals directly from heightmap
        // This avoids the issue of seams along the chunks that would arise from calculating face normals
        // formula from https://www.reddit.com/r/opengl/comments/8myqys/normals_of_a_heightmap_terrain/dzrpya2/
        glm::vec3 Normal(int x, int y) {
            glm::vec3 L = GetPos(x-1,y);
            glm::vec3 R = GetPos(x+1,y);
            glm::vec3 U = GetPos(x,y-1);
            glm::vec3 D = GetPos(x,y+1);
            return glm::normalize(glm::cross(R-L,U-D));
        }

        Engine::Mesh MakeTerrainMesh() {
            std::vector<glm::vec3> verts;
            std::vector<glm::vec3> normals;
            std::vector<GLuint> indices;

            int y;
            for(y=0; y<CHUNK_WIDTH+1; y++) {            
                for(int x=0; x<CHUNK_WIDTH+1; x++) {
                    verts.push_back(GetPos(x,y));
                    normals.push_back(Normal(x,y));
                }
            }

            for(y=0; y<CHUNK_WIDTH; y++) {            
                for(int x=0; x<CHUNK_WIDTH; x++) {
                    GLuint i00 = x + y*(CHUNK_WIDTH+1);
                    GLuint i10 = (x+1) + y*(CHUNK_WIDTH+1);
                    GLuint i01 = x + (y+1)*(CHUNK_WIDTH+1);
                    GLuint i11 = (x+1) + (y+1)*(CHUNK_WIDTH+1);

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

    public:
        // returns true if the chunk needed to regenerate
        bool Apply(int startX, int startZ, int endX, int endZ, int map_width, Terrain& terrain) {
            int newOffsetX = offsetX;
            int newOffsetY = offsetY;
            while(newOffsetX > endX) {
                newOffsetX -= map_width;
            }
            while(newOffsetX < startX) {
                newOffsetX += map_width;
            }
            while(newOffsetY > endZ) {
                newOffsetY -= map_width;
            }
             while(newOffsetY < startZ) {
                newOffsetY += map_width;
            }
            if(generated && offsetX == newOffsetX && offsetY == newOffsetY) {
                // skip if the chunk is already initialized to this position
                return false;
            }
            offsetX = newOffsetX;
            offsetY = newOffsetY;

            scale = terrain.scale;
            heightMap.assign((CHUNK_WIDTH+3)*(CHUNK_WIDTH+3),0.);
            float fXOffset = offsetX * scale;
            float fZOffset = offsetY * scale;

            water->localTransform = glm::translate(glm::mat4(1.0),glm::vec3(fXOffset,0.0,fZOffset));
            land->localTransform = glm::translate(glm::mat4(1.0),glm::vec3(fXOffset,0.0,fZOffset));

            bool anyLand = false;
            bool anyWater = false;

            for(int z=0; z<CHUNK_WIDTH+3; z++)
                for(int x=0; x<CHUNK_WIDTH+3; x++) {
                    // extra offset of scale (1 cell) to account for the margin on the heightmap
                    float X = x*scale + fXOffset - scale;
                    float Z = z*scale + fZOffset - scale;
                    float height = terrain.Height(X,Z);
                    heightMap[x+z*(CHUNK_WIDTH+3)] = height;
                    if(height > 0.f) {
                        anyLand = true;
                    } else {
                        anyWater = true;
                    }
                }

            land->enabled = anyLand;
            water->enabled = anyWater;

            if(anyLand) {
                land->mesh = MakeTerrainMesh();
            }

            generated = true;
            return true;
        }

        std::shared_ptr<Engine::PbrMaterial> terrainMaterial;
        std::shared_ptr<Engine::PbrMaterial> waterMaterial;
        Engine::Mesh waterMesh;

        Chunk(int baseOffsetX,int baseOffsetY,std::shared_ptr<Engine::PbrMaterial> terrainMaterial, std::shared_ptr<Engine::PbrMaterial> waterMaterial, Engine::Mesh waterMesh)
            : terrainMaterial(terrainMaterial), waterMaterial(waterMaterial), waterMesh(waterMesh), offsetX(baseOffsetX), offsetY(baseOffsetY) {
            generated = false;
        }

        void OnEnter(Engine::SceneGraph& sceneGraph) override {            
            water = new RenderItem();
            land = new RenderItem();

            water->material = waterMaterial;
            water->mesh = waterMesh;
            land->material = terrainMaterial;

            sceneGraph.SetParent(water,this);
            sceneGraph.SetParent(land,this);
        }
    };

    int width;
    float scale;
    static const int CHUNK_WIDTH = 32;
    FastNoiseLite noise;
    float heightScale = 256.0;

    float Height(float x, float z) {
        float freq = 1.0 / 16.0;
        float amp = 1.0;

        float result = 0.f;
        float normalise_sum = 0.f;

        int octaves = 8;
        for(int i =0; i<octaves; i++) {
            result += noise.GetNoise(x * freq, z * freq) * amp;

            normalise_sum += amp;
            freq *= 2.f;
            amp *= 0.5f;
        }
        result /= normalise_sum;
        float fac = 3.0;
        if(result > 0.f) result = pow(result,fac);
        // Offset helps to reduce Z-fighting between terrain and water
        result += 0.001;

        return heightScale * result;
    }

    Engine::Mesh MakeWaterMesh() {
        std::vector<glm::vec3> verts;
        std::vector<glm::vec3> normals;
        std::vector<GLuint> indices;

        int y;
        for(y=0; y<CHUNK_WIDTH+1; y++) {            
            for(int x=0; x<CHUNK_WIDTH+1; x++) {
                verts.push_back(glm::vec3(x * scale,0.f,y * scale));
                normals.push_back(glm::vec3(0.,1.,0.));
            }
        }

        for(y=0; y<CHUNK_WIDTH; y++) {            
            for(int x=0; x<CHUNK_WIDTH; x++) {
                GLuint i00 = x + y*(CHUNK_WIDTH+1);
                GLuint i10 = (x+1) + y*(CHUNK_WIDTH+1);
                GLuint i01 = x + (y+1)*(CHUNK_WIDTH+1);
                GLuint i11 = (x+1) + (y+1)*(CHUNK_WIDTH+1);

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
    void Update(Engine::World& world) override {
        glm::vec3 camPos = world.cameraMain()->position();

        int cX = int(camPos.x) / scale;
        int cZ = int(camPos.z) / scale;
        int hW = width/2;

        int regenCount = 0;
        for(auto child : children) {
            Chunk* chunk = (Chunk*)child;
            regenCount += chunk->Apply(cX-hW,cZ-hW,cX+hW,cZ+hW,width,*this);
        }

        if(regenCount > 0) {
            std::cout << "Regenerated " << regenCount << " chunks | " << cX << ", " << cZ << " (" << hW << ")" << std::endl;
        }
    }

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
        terrain_material.textures.push_back(Engine::Texture::Import("textures/grass/rocky_terrain_02_diff_2k.jpg"));
        terrain_material.textures.push_back(Engine::Texture::Import("textures/grass/rocky_terrain_02_nor_gl_2k.png"));
        terrain_material.textures.push_back(Engine::Texture::Import("textures/grass/rocky_terrain_02_arm_2k.jpg"));
        //terrain_material.textures.push_back(Engine::Texture::Import("textures/sand/coast_sand_01_diff_2k.jpg"));
        //terrain_material.textures.push_back(Engine::Texture::Import("textures/sand/coast_sand_01_nor_gl_2k.png"));
        //terrain_material.textures.push_back(Engine::Texture::Import("textures/sand/coast_sand_01_diff_2k.jpg"));
        //terrain_material.textures.push_back(Engine::Texture::Import("textures/sand/coast_sand_01_nor_gl_2k.png"));

        auto terrain_mat_pointer = std::make_shared<Engine::PbrMaterial>(terrain_material);
        auto water_mat_pointer = std::make_shared<Engine::PbrMaterial>(water_material);

        Engine::Mesh waterMesh = MakeWaterMesh();

        int chunks = width/CHUNK_WIDTH;
        for(int x=0; x<chunks; x++)
            for(int y=0; y<chunks; y++) {
                Chunk* chunk = new Chunk(x*CHUNK_WIDTH,y*CHUNK_WIDTH,terrain_mat_pointer,water_mat_pointer,waterMesh);
		        sceneGraph.SetParent(chunk, this);
            }
    }

    // width specified in number of verts per side
    Terrain(int width, float scale) : width(width), scale(scale) {
        noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    };
};