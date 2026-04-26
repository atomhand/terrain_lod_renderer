#pragma once
#include <vector>
#include <stack>
#include <queue>
#include <cassert>

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <entt/entt.hpp>

#include "world.h"
#include "terrain.h"
#include "culling.h"
#include "material.h"
#include "light.h"

#include "terrain_material.h"
#include "compute_shader.h"
#include "uniform_buffer.h"
#include "profiler.h"

#include "imgui.h"

using Engine::World, Engine::Transform;

struct TerrainGeometry;

struct Heightmap {
private:
    static inline std::vector<float> height;
    
    static inline std::vector<glm::vec4> outData;
    
    const int internalWidth;
    const int width;
    const glm::vec2 posMin;
    const glm::vec2 posMax;
    const glm::vec2 extent;

    glm::vec3 GetLocalPos(int x, int z) {
        return glm::vec3(
            extent.x * x / float(width-1),
            height[(x+1) + (z+1) * internalWidth],
            extent.y * z / float(width-1)
        );
    }

    glm::vec3 Normal(int x, int z) {
        glm::vec3 L = GetLocalPos(x-1,z);
        glm::vec3 R = GetLocalPos(x+1,z);
        glm::vec3 U = GetLocalPos(x,z-1);
        glm::vec3 D = GetLocalPos(x,z+1);
        return glm::normalize(glm::cross(R-L,U-D));
    }

public:
    Engine::AABB aabb;
    bool anyLand;
    bool anyWater;

    float longestEdge = 0.f;

    Heightmap(Terrain& terrain, glm::vec2 posMin, glm::vec2 posMax, int width) : posMin(posMin), posMax(posMax), extent(posMax-posMin), width(width), internalWidth(width+3) {
        height.assign(internalWidth*internalWidth, 0.f);

        aabb.min = glm::vec3(0., 99999999.f, 0.);
        aabb.max = glm::vec3(1.f, -99999999.f, 1.f);

        anyLand = false;
        anyWater = false;

        glm::vec2 step = extent / float(width-1);
        terrain.SampleRegion(posMin - step, step, internalWidth, height);


        for(int x=0; x<internalWidth; x++)
            for(int z=0; z<internalWidth; z++) {
                float h = height[x + z * internalWidth];
                /*
                terrain.Height(
                    posMin.x + extent.x * float(x-1) / float(width-1),
                    posMin.y + extent.y * float(z-1) / float(width-1)
                );
                */

                aabb.min.y = std::min(h, aabb.min.y);
                aabb.max.y = std::max(h, aabb.max.y);
                
                //height[x + z * internalWidth] = h;

                if(h > 0.f) {
                    anyLand = true;
                } else {
                    anyWater = true;
                }
                    anyLand = true;
            }
    }

    void FillData(TerrainMaterial::Cache& cache, int page) {
        // not taking account of height for now
        longestEdge = glm::length(glm::vec3(extent.x, 0.f, extent.y)) / float(width);

        outData.clear();
        for(int iz=0; iz<width; iz++) {
            for(int ix=0; ix<width; ix++) {
                float h = height[(ix+1) + (iz+1) * internalWidth];

                glm::vec3 normal = Normal(ix,iz);

                outData.push_back(glm::vec4(normal,h));
            }
        }

        glTextureSubImage3D(cache.terrainDataTex.textureObject(), 0, 0, 0, page, width, width, 1, GL_RGBA, GL_FLOAT, outData.data());
    }
};

struct TerrainChunkHeader {    
    const uint32_t rootId;
    int nodeCount;

    glm::ivec2 chunkOffset;
    glm::vec2 positionOffset;
    
    const float scale;

    // CPP modulo operator: -5 % 100 == -5
    // This function: mod(-5,100) == 95
    int mod(int k, int n) {
        return ((k %= n) < 0) ? k+n : k;
    }

    bool ApplyNewOffsets(Engine::World& world, int startX, int startZ, int w) {
        int newOffsetX = mod(chunkOffset.x-startX, w) + startX;
        int newOffsetZ = mod(chunkOffset.y-startZ, w) + startZ;

        if(newOffsetX != chunkOffset.x || newOffsetZ != chunkOffset.y) {
            chunkOffset = glm::ivec2(newOffsetX,newOffsetZ);
            positionOffset = glm::vec2(chunkOffset) * scale;
            return true;
        }
        return false;
    }

    TerrainChunkHeader(glm::ivec2 chunkOffset, float scale, uint32_t rootId) : scale(scale), chunkOffset(chunkOffset), rootId(rootId) {
        positionOffset = glm::vec2(chunkOffset) * scale;
    }
};

struct TerrainQuadtree {
    struct InternalNode {
        std::vector<uint32_t> childIds;

        uint32_t idx;
        uint32_t parentIdx;
        bool isFree = true;
        uint32_t localIdx;
        int textureId = -1;

        Engine::AABB aabb;

        bool hasRenderComponents = false;

        unsigned char depth;
        unsigned short x;
        unsigned short z;

        float longestEdge;

        entt::entity entity = entt::null;
        entt::entity water_entity = entt::null;

        InternalNode(uint32_t idx) : idx(idx) {};

        bool HasChildren() const {
            return !childIds.empty();
        }

        void Set(uint32_t parentIdx, uint32_t localIdx, unsigned short x, unsigned short z, unsigned char depth) {
            this->parentIdx = parentIdx;
            this->localIdx = localIdx;
            this->x = x;
            this->z = z;
            this->depth = depth;
            isFree = false;

            hasRenderComponents = false;
        }
    };

    struct LayerProperties {
        int sideNodes;
    };

    void AllocChildren(InternalNode& node) {
        assert(!node.HasChildren());
        int depth = node.depth;
        int baseX = node.x * 2;
        int baseZ = node.z * 2;

        for(int x=0, i=0; x<2; x++)
            for(int z=0; z<2; z++, i++) {
                uint32_t childIdx = NextPoolId();
                nodePool[childIdx].Set(node.idx, i, baseX+x, baseZ+z, depth+1);                

                node.childIds.push_back(childIdx);
            }
    }

    const int maxDepth;
    const float scale;
    const int chunkSize;

    std::stack<uint32_t> mergeQueue;
    std::vector<LayerProperties> layerProperties;

    const int poolCapacity;

    std::vector<uint32_t> freeList;
    std::vector<InternalNode> nodePool;

    std::vector<int> textureIdFreelist;
    int nextTextureId = 0;

    void DestroyChunk(Engine::World& world, InternalNode& node);

    bool GetTextureId(InternalNode& node) {
        if(node.textureId >= 0) {
            assert(node.textureId < poolCapacity);
            return true;
        }

        if(textureIdFreelist.size() > 0) {
            node.textureId = textureIdFreelist.back();
            textureIdFreelist.pop_back();
            return true;
        } else if(nextTextureId < poolCapacity) {
            node.textureId = nextTextureId++;
            return true;
        }
        node.textureId = -1;
        return false;
    }
    void FreeTextureId(int& id) {
        assert(id>=0);
        assert(id<poolCapacity);
        textureIdFreelist.push_back(id);
        id = -1;
    }

    uint32_t NextPoolId() {
        if(freeList.size() > 0) {
            uint32_t id = freeList.back();
            freeList.pop_back();
            return id;
        } else {
            uint32_t newId = nodePool.size();
            nodePool.emplace_back(newId);
            return newId;
        }
    }

    void NodeUvs(int x, int z, int depth, glm::vec2& uvMin, glm::vec2& uvMax) {
        LayerProperties& layer = layerProperties[depth];

        uvMin = glm::vec2(x,z) / float(layer.sideNodes);
        uvMax = glm::vec2(x+1,z+1) / float(layer.sideNodes);
    }

    void Reset(Engine::World& world, TerrainChunkHeader& chunk) {
        mergeQueue.push(chunk.rootId);
    }

    // Ref: "Rendering Massive Terrains using Chunked Level of Detail Control"
    float TargetLodDepth(int inputLod, glm::vec3 offset, glm::vec3 extent, Engine::AABB& aabb, Engine::Camera& camera, int targetTriangleSize, float worldSpaceTriangleSize) {
        extent.y = 1.0f;

        // calculate the nearest position to the camera within the chunk's conservative bounds
        glm::vec3 nearestM = glm::clamp(camera.position,aabb.min*extent+offset,aabb.max*extent+offset);
        // calculate the distance to the camera
        float distance = glm::distance(camera.position,nearestM);

        // Estimate the screen space size of a triangle, in pixels, at the current LOD level
        float screenSpaceTriangleSizeEstimate = (worldSpaceTriangleSize / distance) * camera.lodFovFactor;

        // Calculate the change in LOD levels required to achieve the target triangle size
        float lodDiff = std::log2(screenSpaceTriangleSizeEstimate / targetTriangleSize);
        float targetLod = inputLod + lodDiff;
        
        return std::clamp(targetLod, 0.f, float(maxDepth));
    }

    TerrainChunkHeader MakeChunk(glm::ivec2 chunkOffset) {
        int rootId = NextPoolId();
        nodePool[rootId].Set(rootId, 0, 0,0,0);
        return TerrainChunkHeader(chunkOffset,scale,rootId);
    }

    TerrainQuadtree(int maxDepth, float scale, int chunkSize, int pool_size) :
        maxDepth(maxDepth), scale(scale), chunkSize(chunkSize), poolCapacity(pool_size)
    {
        int w= 1;
        for(int d =0; d<=maxDepth; d++) {
            layerProperties.push_back(
                LayerProperties { w }
            );
            w*=2;
        }
        nodePool.reserve(poolCapacity);
    }
    
    // returns miliseconds per node generated (if any)
    void TraverseUpdate(Engine::World& world, Terrain& terrain, TerrainGeometry& terrainGeometry, std::vector<TerrainChunkHeader*> chunks, Engine::Camera& camera);
};

// Infinite procedurally generated terrain class
struct TerrainGeometry {
    const int CHUNK_SIZE = 128;
    const int MAX_QUADTREE_DEPTH = 16;
    const int BASE_POOL_SIZE = 1500; //4096 * 4 * 4;

    const unsigned int width;
    const float scale;

    double timePerGeneratedChunk = 0.1;

    TerrainQuadtree quadtree;
    Engine::ComputeShader displacementShader;
    Engine::ComputeShader computeTerrainShader;

    bool wasComputeTerrain;

    struct DisplacementUpdateHeader {
        glm::vec2 offset;
        glm::vec2 extent;
        int page;
        int padding;
    };

    std::vector<DisplacementUpdateHeader> displacementUpdates;

    Engine::StorageBuffer displacementUpdateBuffer = Engine::StorageBuffer(0);
    Engine::UniformBuffer terrainNoiseUniforms;
public:
    // Update - Chunks get a chance to regenerate if their current position is invalid (too far from the camera)
    // The number of chunks that can generate per frame is rate-limited, hopefully preventing any significant loading stutter.
    // The limit is waived on startup
    static void Update(Engine::World& world);

    // Suggest an appropriate far plane for the main camera, given the size of the terrain
    float SuggestFarPlane() const;

    // Setup on enter scene
    // Create material data and initialise the chunks
    static TerrainGeometry& Insert(Engine::World& world, entt::entity terrain_entity, unsigned int width, float scale);

    void QueueDisplacementUpdate(glm::vec2 offset, glm::vec2 extent, int page) {
        displacementUpdates.emplace_back(offset,extent,page);
    }

    void UpdateTerrainDisplacement(World& world) {
        if(displacementUpdates.size() == 0) return;
        auto profileHandle = Engine::Profiler::StartCpu("TerrainGeometry::UpdateTerrainDisplacement");
        auto gpuProfileHandle = Engine::Profiler::StartGpu("TerrainGeometry::UpdateTerrainDisplacement");

        auto& terrain = world.GetSingle<Terrain>();

        auto& terrainCache = world.GetSingle<TerrainMaterial::Cache>();

        displacementUpdateBuffer.Set<DisplacementUpdateHeader>(displacementUpdates.data(), displacementUpdates.size(), 0, true);
        displacementUpdateBuffer.BindBase(0);

        glBindImageTexture(0, terrainCache.terrainDataTex.textureObject(), 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
        terrainCache.BindTextures(false);

        if(world.input.computeTerrain) {
            auto terrainNoiseUniformData = terrain.GetTerrainNoiseUniform();
            terrainNoiseUniforms.Set(&terrainNoiseUniformData);
            terrainNoiseUniforms.BindBase(8);
            computeTerrainShader.use();
            computeTerrainShader.Dispatch(displacementUpdates.size(), 1, 1);
        } else {
            displacementShader.use();
            displacementShader.Dispatch(displacementUpdates.size(), 1, 1);
        }

        displacementUpdates.clear();

        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    // width specified in number of verts per side
    TerrainGeometry(unsigned int width, float scale) : width(width), scale(scale), quadtree(MAX_QUADTREE_DEPTH, scale, CHUNK_SIZE,
        BASE_POOL_SIZE /* Add room for the root nodes, so the remaining pool size is divisible by 4 */) {
            std::string d = std::string("#define PAGE_SIZE ") + std::to_string(CHUNK_SIZE*2+1);
            std::vector<const char*> defs = { d.c_str() };
            displacementShader = Engine::ComputeShader("shaders/precalc_terrain_displacement.cs", defs);
            defs.push_back("#define COMPUTE_TERRAIN");
            computeTerrainShader = Engine::ComputeShader("shaders/precalc_terrain_displacement.cs", defs);
        };
};