#pragma once
#include <vector>
#include <glm/glm.hpp>

#include <glad/glad.h>
#include "FastNoiseLite.h"
#include "mesh.h"
#include "world.h"
#include "render_item.h"
#include "texture.h"
#include "culling.h"

class Terrain : public Engine::SceneNode {
    class Chunk : public Engine::SceneNode {
        float scale;
        RenderItem* water;        
        std::vector<float> heightMap;
        bool generated;
        int offsetX,offsetY;
        GLuint arrayIndex;

        float SampleHeightmap(int x, int z) {
            int idx = (x+1) + (z+1) * (CELL_RESOLUTION+2);
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

        void FillTerrainTexture(Engine::Texture2DArray& tex) {
            //tex.bind();
            
            std::vector<glm::vec4> pixels;
            //glm::vec4 pixels[(CHUNK_WIDTH+1)*(CHUNK_WIDTH+1)];
            for(int y=0; y<CELL_RESOLUTION; y++) {            
                for(int x=0; x<CELL_RESOLUTION; x++) {
                    //pixels[x + y * (CHUNK_WIDTH+1)] = glm::vec4(Normal(x,y),GetPos(x,y).y);
                    pixels.push_back(glm::vec4(Normal(x,y),SampleHeightmap(x,y)));
                }
            }

            glTextureSubImage3D(tex.textureObject(), 0, 0, 0, arrayIndex, CELL_RESOLUTION,CELL_RESOLUTION, 1, GL_RGBA, GL_FLOAT, pixels.data());
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
            heightMap.assign((CELL_RESOLUTION+2)*(CELL_RESOLUTION+2),0.);
            float fXOffset = offsetX * scale;
            float fZOffset = offsetY * scale;

            terrain.chunkOffsets[arrayIndex] = glm::vec2(fXOffset,fZOffset);

            glm::vec3 min = glm::vec3(fXOffset,4096.,fZOffset);
            glm::vec3 max = glm::vec3(fXOffset+scale*CHUNK_WIDTH, -4096.,fZOffset+scale*CHUNK_WIDTH);

            water->localTransform = glm::translate(glm::mat4(1.0),glm::vec3(fXOffset,0.0,fZOffset));
            //land->localTransform = glm::translate(glm::mat4(1.0),glm::vec3(fXOffset,0.0,fZOffset));

            bool anyLand = false;
            bool anyWater = false;

            float texScale = scale * float(CHUNK_WIDTH) / float(CELL_RESOLUTION);
            for(int z=0; z<CELL_RESOLUTION+2; z++)
                for(int x=0; x<CELL_RESOLUTION+2; x++) {
                    // extra offset of scale (1 cell) to account for the margin on the heightmap
                    float X = (x-1)*texScale + fXOffset;
                    float Z = (z-1)*texScale + fZOffset;
                    float height = terrain.Height(X,Z);
                    min.y = std::min(height,min.y);
                    max.y = std::max(height,max.y);
                    heightMap[x+z*(CELL_RESOLUTION+2)] = height;
                    if(height > 0.f) {
                        anyLand = true;
                    } else {
                        anyWater = true;
                    }
                }

            terrain.aabbs[arrayIndex] = Engine::AABB(min,max);
            
            water->enabled = anyWater;

            //if(anyLand)
            {
                FillTerrainTexture(terrain.terrainData);
            }

            generated = true;
            return true;
        }

        std::shared_ptr<Engine::PbrMaterial> waterMaterial;
        Engine::Mesh waterMesh;

        Chunk(GLuint textureOffset, int baseOffsetX,int baseOffsetY,std::shared_ptr<Engine::PbrMaterial> waterMaterial, Engine::Mesh waterMesh)
            : arrayIndex(textureOffset), waterMaterial(waterMaterial), waterMesh(waterMesh), offsetX(baseOffsetX), offsetY(baseOffsetY) {
            generated = false;
        }

        void OnEnter(Engine::SceneGraph& sceneGraph) override {            
            water = new RenderItem();

            water->material = waterMaterial;
            water->mesh = waterMesh;

            sceneGraph.SetParent(water,this);
        }
    };

    Engine::Texture2DArray terrainData;

    int width;
    float scale;
    static const int CHUNK_WIDTH = 32;
    static const int CELL_RESOLUTION = 127;
    FastNoiseLite noise;
    float heightScale = 1024.0;

    void MakeTerrainMesh(GLuint vertexBuffer, GLuint indexBuffer, size_t &nIndices, size_t &nVerts) {
        std::vector<glm::vec2> verts;
        std::vector<glm::vec2> uvs;
        std::vector<GLuint> indices;

        int y;
        for(y=0; y<CHUNK_WIDTH+1; y++) {            
            for(int x=0; x<CHUNK_WIDTH+1; x++) {
                verts.push_back(glm::vec2(x/ float(CHUNK_WIDTH),y/ float(CHUNK_WIDTH)));
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
                indices.push_back(i10);                
                indices.push_back(i11);
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(glm::vec2), verts.data(), GL_STATIC_DRAW);

        // index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

        nVerts = verts.size();
        nIndices = indices.size();
    }

    static const int WATER_CHUNK_WIDTH = 1;

    Engine::Mesh MakeWaterMesh() {
        std::vector<glm::vec3> verts;
        std::vector<glm::vec3> normals;
        std::vector<GLuint> indices;

        int y;
        float wscale = scale * CHUNK_WIDTH / float(WATER_CHUNK_WIDTH);
        for(y=0; y<WATER_CHUNK_WIDTH+1; y++) {            
            for(int x=0; x<WATER_CHUNK_WIDTH+1; x++) {
                verts.push_back(glm::vec3(x * wscale,0.f,y * wscale));
                normals.push_back(glm::vec3(0.,1.,0.));
            }
        }

        for(y=0; y<WATER_CHUNK_WIDTH; y++) {            
            for(int x=0; x<WATER_CHUNK_WIDTH; x++) {
                GLuint i00 = x + y*(WATER_CHUNK_WIDTH+1);
                GLuint i10 = (x+1) + y*(WATER_CHUNK_WIDTH+1);
                GLuint i01 = x + (y+1)*(WATER_CHUNK_WIDTH+1);
                GLuint i11 = (x+1) + (y+1)*(WATER_CHUNK_WIDTH+1);

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


    std::vector<glm::vec3> translations;
    Engine::Mesh terrainMesh;
public:
    Engine::PbrMaterial terrainMaterial;
    Engine::PbrMaterial terrainShadowMaterial;
    std::vector<Engine::AABB> aabbs;

     void DrawShadows(glm::mat4 &lightSpaceMatrix) 
     {
        terrainShadowMaterial.use();
        terrainShadowMaterial.shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE3);
        terrainShadowMaterial.shader.setFloat("scale", scale*CHUNK_WIDTH);
        glBindTexture(GL_TEXTURE_2D_ARRAY,terrainData.textureObject());
        glDrawElementsInstanced(GL_QUADS, nIndices, GL_UNSIGNED_INT, nullptr, nChunks);
        glBindVertexArray(0);
    }
    void Draw() {     
        glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE3);
        terrainMaterial.shader.setFloat("scale", scale*CHUNK_WIDTH);
        glBindTexture(GL_TEXTURE_2D_ARRAY,terrainData.textureObject());
        glDrawElementsInstanced(GL_PATCHES, nIndices, GL_UNSIGNED_INT, nullptr, nChunks);
        glBindVertexArray(0);
    }

    void Update(Engine::World& world) override {
        glm::vec3 camPos = world.cameraMain()->position();

        int cX = int(camPos.x) / scale;
        int cZ = int(camPos.z) / scale;
        int hW = width/2 + CHUNK_WIDTH;

        int regenCount = 0;
        for(auto child : children) {
            Chunk* chunk = (Chunk*)child;
            regenCount += chunk->Apply(cX-hW,cZ-hW,cX+hW,cZ+hW,width,*this);
        }

        if(regenCount > 0) {
            glBindBuffer(GL_ARRAY_BUFFER,chunkVBO);
            glBufferData(GL_ARRAY_BUFFER, chunkOffsets.size() * sizeof(glm::vec2), chunkOffsets.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER,0);
            
            std::cout << "Regenerated " << regenCount << " chunks | " << cX << ", " << cZ << " (" << hW << ")" << std::endl;
        }
    }

    float Height(float x, float z) {
        float freq = 1.0 / 64.0;
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
        result += 0.005;

        return heightScale * result;
    }

    float SuggestFarPlane() {
        return std::max(1,(wChunks/2-2)) * CHUNK_WIDTH * scale;
    }

    void OnEnter(Engine::SceneGraph& sceneGraph) override {
        
        Engine::Shader water_shader = Engine::Shader("shaders/pbr.vert", "shaders/water_pbr.frag");
        auto water_material = Engine::PbrMaterial(water_shader);
        water_material.roughness = 0.03;
        water_material.textures.push_back(Engine::Texture::Import("textures/waterN1.jpg"));
        water_material.textures.push_back(Engine::Texture::Import("textures/waterN2.jpg"));
        water_material.albedo = glm::vec3(0.465f, 0.797f, 0.991f);


        //terrain_material.textures.push_back(Engine::Texture::Import("textures/sand/coast_sand_01_diff_2k.jpg"));
        //terrain_material.textures.push_back(Engine::Texture::Import("textures/sand/coast_sand_01_nor_gl_2k.png"));
        //terrain_material.textures.push_back(Engine::Texture::Import("textures/sand/coast_sand_01_diff_2k.jpg"));
        //terrain_material.textures.push_back(Engine::Texture::Import("textures/sand/coast_sand_01_nor_gl_2k.png"));

        //auto terrain_mat_pointer = std::make_shared<Engine::PbrMaterial>(terrain_material);
        auto water_mat_pointer = std::make_shared<Engine::PbrMaterial>(water_material);

        Engine::Mesh waterMesh = MakeWaterMesh();

        for(int x=0; x<wChunks; x++)
            for(int y=0; y<wChunks; y++) {
                Chunk* chunk = new Chunk(x+y*wChunks,x*CHUNK_WIDTH,y*CHUNK_WIDTH,water_mat_pointer,waterMesh);
		        sceneGraph.SetParent(chunk, this);
            }
        
        std::cout << "Terrain initialised with " << (wChunks*wChunks) << " chunks" << std::endl;
    }

    GLuint vao;        
    GLuint vertexBuffer, indexBuffer;
    GLuint chunkVBO;
    size_t nChunks;
    size_t nVerts, nIndices;
    int wChunks;

    std::vector<glm::vec2> chunkOffsets;

    // width specified in number of verts per side
    Terrain(int width, float scale) : width(width), scale(scale),
        terrainMaterial(Engine::PbrMaterial(Engine::Shader("shaders/terrain.vert", "shaders/terrain_pbr.frag","shaders/terrain_tcs.glsl","shaders/terrain_tes.glsl"))),
        terrainShadowMaterial(Engine::PbrMaterial(Engine::Shader("shaders/terrain_shadow.vert","shaders/shadow.frag")))    
        {
        terrainMaterial.roughness = 1.0;
        terrainMaterial.textures.push_back(Engine::Texture::Import("textures/grass/rocky_terrain_02_diff_2k.jpg"));
        terrainMaterial.textures.push_back(Engine::Texture::Import("textures/grass/rocky_terrain_02_nor_gl_2k.png"));
        terrainMaterial.textures.push_back(Engine::Texture::Import("textures/grass/rocky_terrain_02_arm_2k.jpg"));

        noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        
        wChunks = width/CHUNK_WIDTH;
        nChunks = wChunks*wChunks;

        this->width = wChunks*CHUNK_WIDTH;
        assert(nChunks <= 2048); // limit on texture array layer capacity
        terrainData.Configure(1, GL_RGBA16F, CELL_RESOLUTION,CELL_RESOLUTION, nChunks);
        chunkOffsets.assign(nChunks, glm::vec2(0.f));
        aabbs.assign(nChunks, Engine::AABB(glm::vec3(0.),glm::vec3(0.)));

        glGenBuffers(1, &vertexBuffer);
        glGenBuffers(1, &indexBuffer);
        MakeTerrainMesh(vertexBuffer,indexBuffer, nIndices, nVerts);

        std::cout << "indices " << nIndices << ", verts " << nVerts << std::endl;

        glGenBuffers(1, &chunkVBO);

        // Set up VAO
        glGenVertexArrays(1,&vao);
        glBindVertexArray(vao);

        glPatchParameteri(GL_PATCH_VERTICES, 4);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER,chunkVBO);
        glBufferData(GL_ARRAY_BUFFER, chunkOffsets.size() * sizeof(glm::vec2), chunkOffsets.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER,0);
        glVertexAttribDivisor(1,1);
        glBindVertexArray(0);

        terrainMaterial.shader.setFloat("scale", scale*CHUNK_WIDTH);
    };
};