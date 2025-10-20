// Tom Kellett 2025
#pragma once
#include <vector>
#include <stack>
#include <queue>
#include <cassert>

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <entt/entt.hpp>

#include "mesh.h"
#include "world.h"
#include "terrain.h"
#include "culling.h"
#include "material.h"
#include "render_item.h"
#include "light.h"

#include "terrain_material.h"

#include "imgui.h"

using Engine::World, Engine::Mesh, Engine::Transform;

struct TerrainGeometry;

struct Heightmap {
private:
    static inline std::vector<float> height;
    static inline std::vector<glm::vec2> extraData;
    
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
        extraData.assign(internalWidth*internalWidth, glm::vec2(0.f));

        aabb.min = glm::vec3(0., 99999999.f, 0.);
        aabb.max = glm::vec3(1.f, -99999999.f, 1.f);

        anyLand = false;
        anyWater = false;

        glm::vec2 step = extent / float(width-1);
        terrain.SampleRegion(posMin - step, step, internalWidth, height, extraData);


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
        longestEdge = glm::length(glm::vec3(extent.x, aabb.max.y-aabb.min.y, extent.y)) / float(width);

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
    const int rootId;
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

    TerrainChunkHeader(glm::ivec2 chunkOffset, float scale, int rootId) : scale(scale), chunkOffset(chunkOffset), rootId(rootId) {
        positionOffset = glm::vec2(chunkOffset) * scale;
    }
};

struct TerrainQuadtree {
    struct InternalNode {
        unsigned short childIdx = 0; // 0 used to represent disablerd

        bool hasRenderComponents = false;

        unsigned char depth;
        unsigned short x;
        unsigned short z;

        float longestEdge;

        entt::entity entity = entt::null;
        entt::entity water_entity = entt::null;

        void Set(unsigned short x, unsigned short z, unsigned char depth) {
            this->x = x;
            this->z = z;
            this->depth = depth;

            hasRenderComponents = false;
            childIdx = 0;
        }
    };

    struct LayerProperties {
        int sideNodes;
    };

    void AllocChildren(InternalNode& node) {
        assert(node.childIdx == 0);
        assert(PoolSlotsAvailable() >= 4);

        int idx = node.childIdx = TakePoolIds();
        int depth = node.depth;

        int baseX = node.x * 2;
        int baseZ = node.z * 2;

        nodePool[idx].Set(baseX, baseZ, depth+1);
        nodePool[idx+1].Set(baseX, baseZ+1, depth+1);
        nodePool[idx+2].Set(baseX+1, baseZ, depth+1);
        nodePool[idx+3].Set(baseX+1, baseZ+1, depth+1);
    }

    const int maxDepth;
    const float scale;
    const int chunkSize;

    std::vector<LayerProperties> layerProperties;

    const int poolCapacity;
    int nextPoolId = 0;

    // NOTE
    // The free list is used to free/allocate an entire set of children (4 nodes)
    // at a time. Therefore each id on the free list also "owns" the adjacent 3 ids (i+1,i+2,+3)
    std::vector<int> freeList;
    std::vector<InternalNode> nodePool;

    int PoolSlotsAvailable() {
        return 4*freeList.size() + (poolCapacity - nextPoolId);
    }
    int TakePoolIds() {
        if(freeList.size() > 0) {
            auto ret = freeList.back();
            freeList.pop_back();
            return ret;
        } else {
            assert(nextPoolId+4 <= poolCapacity);
            
            for(int i =0; i<4; i++) {
                nodePool.emplace_back(nextPoolId++);
            }

            return nextPoolId-4;
        }
    }
    int TakeSinglePoolId() {
        assert(nextPoolId < poolCapacity);
        nodePool.emplace_back(nextPoolId++);
        return nextPoolId-1;
    }

    void NodeUvs(int x, int z, int depth, glm::vec2& uvMin, glm::vec2& uvMax) {
        LayerProperties& layer = layerProperties[depth];

        uvMin = glm::vec2(x,z) / float(layer.sideNodes);
        uvMax = glm::vec2(x+1,z+1) / float(layer.sideNodes);
    }

    inline void Destroy(Engine::World& world, InternalNode& node) {
        if(node.childIdx != 0) {
            freeList.push_back(node.childIdx);
            node.childIdx = 0;
        }

        if(node.entity != entt::null) {
            world.registry.destroy(node.entity);
            node.entity = entt::null;
        }

        if(node.water_entity != entt::null) {
            world.registry.destroy(node.water_entity);
            node.water_entity = entt::null;
        }
    }

    void Reset(Engine::World& world, TerrainChunkHeader& chunk) {
        std::stack<int> mergeQueue;
        mergeQueue.push(chunk.rootId);
        chunk.nodeCount = 0;

        while(!mergeQueue.empty()) {
            int topIdx = mergeQueue.top();
            InternalNode& node = nodePool[topIdx];
            mergeQueue.pop();

            if(node.childIdx != 0) {
                freeList.push_back(node.childIdx);
                for(int i =0; i<4; i++) {
                    mergeQueue.push(node.childIdx + i);
                }
                node.childIdx = 0;
            }

            if(node.entity != entt::null) {
                world.registry.destroy(node.entity);
                node.entity = entt::null;
            }

            if(node.water_entity != entt::null) {
                world.registry.destroy(node.water_entity);
                node.water_entity = entt::null;
            }

            node.hasRenderComponents = false;
        }
    }

    float TargetLodDepth(int inputLod, glm::vec3 offset, glm::vec3 extent, Engine::AABB& aabb, Engine::Camera& camera, int lodControlParam, float cellHeight) {
        float wh = cellHeight * std::max(camera.height,camera.width);

        extent.y = 1.0f;

        glm::vec3 nearestM = glm::clamp(camera.position,aabb.min*extent+offset,aabb.max*extent+offset);
        float distance = glm::distance(camera.position,nearestM);

        // lodControlParam ~= pixel length of edges (longest dimension)
        // very rough
        float targetW = distance * float(lodControlParam) * 1.4f;

        float lodDiff = std::log2(wh / targetW);
        float targetLod = inputLod + lodDiff;
        
        return std::clamp(targetLod, 0.f, float(maxDepth));
    }

    TerrainChunkHeader MakeChunk(glm::ivec2 chunkOffset) {
        int rootId = TakeSinglePoolId();
        nodePool[rootId].Set(0,0,0);
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
    Engine::Mesh MakeWaterMesh();
    Engine::Mesh MakeTerrainMesh();

    const int CHUNK_SIZE = 128;
    const int MAX_QUADTREE_DEPTH = 16;
    const int BASE_POOL_SIZE = 1500; //4096 * 4 * 4;

    const unsigned int width;
    const float scale;

    double timePerGeneratedChunk = 0.1;

    TerrainQuadtree quadtree;

    // temp
    std::shared_ptr<Engine::PbrMaterial> terrain_mat_ptr;
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

    // width specified in number of verts per side
    TerrainGeometry(unsigned int width, float scale) : width(width), scale(scale), quadtree(MAX_QUADTREE_DEPTH, scale, CHUNK_SIZE,
        BASE_POOL_SIZE /* Add room for the root nodes, so the remaining pool size is divisible by 4 */) {
        };
};