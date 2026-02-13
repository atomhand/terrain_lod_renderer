#pragma once
#include <chrono>
#include "terrain_geometry.h"
#include "water_material.h"
#include "terrain_material.h"
#include "culling.h"
#include "profiler.h"

struct TraversalItem {
    unsigned short id;
    unsigned short chunkId;
    float score;

    bool operator< (const TraversalItem& other) const {
        return score < other.score;
    };
};

void TerrainQuadtree::TraverseUpdate(Engine::World& world, Terrain& terrain, TerrainGeometry& terrainGeometry, std::vector<TerrainChunkHeader*> chunks, Engine::Camera& camera) {
    auto profile = Engine::Profiler::StartCpu("TerrainQuadtree::TraverseUpdate");
    auto gpuProfileHandle = Engine::Profiler::StartGpu("TerrainQuadtree::TraverseUpdate");

    const int budget = std::max(4.0, 5.0 / terrainGeometry.timePerGeneratedChunk);

    int updateQuota = budget;
    int numGenerated = 0;
    double generationDuration = 0.0;

    auto& terrainCache = world.GetSingle<TerrainMaterial::Cache>();
    auto& waterCache = world.GetSingle<WaterMaterial::Cache>();

    std::priority_queue<TraversalItem> traversalQueue;
    for(int i =0; i<chunks.size(); i++) {        
        traversalQueue.push(TraversalItem{unsigned short(chunks[i]->rootId),unsigned short(i), 99999.f});
        chunks[i]->nodeCount = 0;
    }

    std::stack<unsigned short> mergeQueue;

    while(!traversalQueue.empty() || !mergeQueue.empty()) {
        if(!mergeQueue.empty()) {
            unsigned short topIdx = mergeQueue.top();
            InternalNode& node = nodePool[topIdx];
            mergeQueue.pop();

            if(node.childIdx != 0) {
                freeList.push_back(node.childIdx);
                for(unsigned short i =0; i<4; i++) {
                    mergeQueue.push(node.childIdx + i);
                }
                node.childIdx = 0;
            }

            if(node.textureId > 0) {
                FreeTextureId(node.textureId);
                node.textureId = -1;
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
        } else {
            TraversalItem item = traversalQueue.top();
            int topIdx = item.id;
            TerrainChunkHeader& chunk = *chunks[item.chunkId];
            chunk.nodeCount += 1;
            InternalNode& node = nodePool[topIdx];
            glm::vec2 uvMin, uvMax;
            NodeUvs(node.x, node.z, node.depth, uvMin, uvMax);
            traversalQueue.pop();

            glm::vec3 nodePos = glm::vec3(chunk.positionOffset.x,0.f,chunk.positionOffset.y) + glm::vec3(uvMin.x,0.f,uvMin.y) * scale;
            glm::vec3 extent = (glm::vec3(uvMax.x,0.f,uvMax.y) - glm::vec3(uvMin.x,0.f,uvMin.y))*scale;

            if(node.entity == entt::null) {
                if(node.parentIdx == node.idx) {
                    GetTextureId(node.textureId);
                    // special handling for root nodes
                    auto start = std::chrono::steady_clock::now();
                    Heightmap heightMap(terrain, chunk.positionOffset + uvMin*scale, chunk.positionOffset + uvMax*scale, terrainGeometry.CHUNK_SIZE*2+1);
                    generationDuration +=  std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
                    heightMap.FillData(terrainCache, node.textureId);

                    //node.longestEdge = heightMap.longestEdge * 2.f;
                    node.aabb = heightMap.aabb;
                    node.entity = terrainCache.CreateTerrainItem(world, nodePos, extent, node.aabb, node.textureId*4 + node.localIdx);
                } else {
                    InternalNode& parent = nodePool[node.parentIdx];
                    //node.longestEdge = parent.longestEdge / 2.f;

                    node.entity = terrainCache.CreateTerrainItem(world, nodePos, extent, parent.aabb, parent.textureId*4 + node.localIdx);
                }

                node.longestEdge = extent.x / float(terrainGeometry.CHUNK_SIZE) * 1.73;
            }

            if(node.water_entity == entt::null) {
                node.water_entity = waterCache.CreateWaterItem(world, nodePos, extent);
            }

            float targetDepth = TargetLodDepth(node.depth, nodePos, extent, world.registry.get<Engine::AABB>(node.entity),camera, world.input.lodControlParam, node.longestEdge);
            if(targetDepth > node.depth) {
                // wants to split
                // (or keep children, if they already exist)
                // no children, try split
                if(node.childIdx == 0
                    && numGenerated +4 <= updateQuota) {
                    bool heightmapGenerated = node.textureId >= 0;

                    if(!heightmapGenerated) {
                        bool idAvailable = GetTextureId(node.textureId);

                        if(idAvailable) {
                            // non-root nodes need to generate map at the point of allocating their cihldren
                            auto start = std::chrono::steady_clock::now();
                            Heightmap heightMap(terrain, chunk.positionOffset + uvMin*scale, chunk.positionOffset + uvMax*scale, terrainGeometry.CHUNK_SIZE*2+1);
                            generationDuration +=  std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
                            heightMap.FillData(terrainCache, node.textureId);

                            //node.longestEdge = heightMap.longestEdge * 2.f;
                            node.aabb = heightMap.aabb;

                            heightmapGenerated = true;
                        }
                    }

                    if(heightmapGenerated) {
                        AllocChildren(node);
                        numGenerated += 4;

                        node = nodePool[topIdx];
                    }                    
                }
                
                if(node.childIdx != 0) {
                    float priority = targetDepth - node.depth;
                    
                    // Reduce split priority significantly for cells that failed culling
                    Engine::CullingResult* cullingResult = world.registry.try_get<Engine::CullingResult>(node.entity);
                    if(cullingResult == nullptr || cullingResult->viewResult == false) {
                        priority -= maxDepth;
                    }

                    for(unsigned short i =0; i<4; i++) {
                        traversalQueue.push(TraversalItem{unsigned short(node.childIdx + i),item.chunkId, priority});
                    }
                }                      
            } else {
                // Wants to merge
                if(node.childIdx != 0) {
                    freeList.push_back(node.childIdx);
                    for(int i =0; i<4; i++) {
                        mergeQueue.push(node.childIdx + i);
                    }
                    node.childIdx = 0;
                    if(node.textureId >= 0) {
                        FreeTextureId(node.textureId);
                        node.textureId = -1;
                    }
                }
            }

            bool shouldDraw = node.childIdx == 0;

            auto& mat = world.registry.get<TerrainMaterial>(node.entity);
            mat.enabled = shouldDraw;

            if(shouldDraw) {
                if(!node.hasRenderComponents) {
                    world.registry.emplace<Engine::CullingResult>(node.entity);
                    world.registry.emplace<Engine::ShadowCaster>(node.entity);

                    if(node.water_entity != entt::null) {                            
                        world.registry.emplace<Engine::CullingResult>(node.water_entity);
                    }
                }
                node.hasRenderComponents = true;
            } else {
                if(node.hasRenderComponents) {
                    world.registry.remove<Engine::CullingResult,Engine::ShadowCaster>(node.entity);
                    if(node.water_entity != entt::null) {
                        world.registry.remove<Engine::CullingResult>(node.water_entity);
                    }
                }
                node.hasRenderComponents = false;
            }
        }       
    }    

    if(numGenerated > 0) {
        terrainGeometry.timePerGeneratedChunk =  std::lerp(terrainGeometry.timePerGeneratedChunk,generationDuration / numGenerated,0.1);
    }
}

void DebugUi(Engine::World& world) {
    if(world.input.terrainGeometryDebug) {
        ImGui::Begin("Terrain Geometry Debug", &world.input.terrainGeometryDebug);

        auto camView = world.registry.view<Engine::Camera,Engine::Transform>();
        auto camTransform = camView.get<Engine::Transform>(camView.front());
        glm::vec3 camPos = camTransform.position();

        auto terrain_view = world.registry.view<TerrainGeometry>();
        auto [terrainGeometry] = terrain_view.get(terrain_view.front());        

        int cX = std::floor(camPos.x / terrainGeometry.scale);
        int cZ = std::floor(camPos.z / terrainGeometry.scale);
        int hW = terrainGeometry.width/2;

        ImGui::Text("Camera posk: (%f, %f, %f)",  camPos.x/ terrainGeometry.scale,camPos.y/ terrainGeometry.scale,camPos.z/ terrainGeometry.scale);
        ImGui::Text("Centre chunk: (%i, %i)",  cX, cZ);
        ImGui::Text("Start chunk: (%i, %i)",  cX-hW, cZ-hW);

        ImGui::Text("Time per generated chunk: %fms",  terrainGeometry.timePerGeneratedChunk);

        ImGui::Text("Active nodes %i", terrainGeometry.quadtree.nextPoolId - terrainGeometry.quadtree.freeList.size());

        ImGui::SeparatorText("Shared Pool");
        ImGui::Text("Pooled texture slots available %i", terrainGeometry.quadtree.textureIdFreelist.size());
        ImGui::Text("Total %i / %i slots used",  terrainGeometry.quadtree.nextTextureId - terrainGeometry.quadtree.textureIdFreelist.size(), terrainGeometry.quadtree.poolCapacity);
        ImGui::Text("Total %i / %i slots allocated",  terrainGeometry.quadtree.nextTextureId, terrainGeometry.quadtree.poolCapacity);

        auto view = world.registry.view<TerrainChunkHeader>();
        for(auto entity : view) {
            auto& chunk = view.get<TerrainChunkHeader>(entity);        
            ImGui::SeparatorText("Quad");
            ImGui::Text("Offset: (%i, %i)",  chunk.chunkOffset.x,  chunk.chunkOffset.y);
            ImGui::Text("Position: (%f, %f)",  chunk.positionOffset.x,   chunk.positionOffset.y);
            ImGui::Text("Active nodes: %i",  chunk.nodeCount);
        }

        ImGui::End();
    }
}

// Update - Chunks get a chance to regenerate if their current position is invalid (too far from the camera)
// The number of chunks that can generate per frame is rate-limited, hopefully preventing any significant loading stutter.
// The limit is waived on startup
void TerrainGeometry::Update(Engine::World& world) {
    auto camView = world.registry.view<Engine::Camera,Engine::Transform>();
    auto [camera,camTransform] = camView.get(camView.front());
    glm::vec3 camPos = camTransform.position();

    auto terrain_view = world.registry.view<Terrain,TerrainGeometry>();
    auto [terrain,terrainGeometry] = terrain_view.get(terrain_view.front());


    bool terrainConfigChanged = terrain.CalibrationUi(world);

    int cX = std::floor(camPos.x / terrainGeometry.scale);
    int cZ = std::floor(camPos.z / terrainGeometry.scale);
    int hW = terrainGeometry.width/2;

    std::vector<TerrainChunkHeader*> chunks;

    auto view = world.registry.view<TerrainChunkHeader>();
    for(auto entity : view) {
        auto& chunk = view.get<TerrainChunkHeader>(entity);
        chunks.push_back(&chunk);

        bool offsetsChanged = chunk.ApplyNewOffsets(world, cX-hW, cZ-hW, terrainGeometry.width);

        if(terrainConfigChanged || offsetsChanged)
            terrainGeometry.quadtree.Reset(world,chunk);
    }

    terrainGeometry.quadtree.TraverseUpdate(world, terrain, terrainGeometry, chunks, camera);

    DebugUi(world);
}

// Suggest an appropriate far plane for the main camera, given the size of the terrain
float TerrainGeometry::SuggestFarPlane() const {
    return std::floor(width * 0.5f) * scale;
}

// Setup on enter scene
// Create material data and initialise the chunks
TerrainGeometry& TerrainGeometry::Insert(Engine::World& world, entt::entity terrain_entity, unsigned int width, float scale) {
    auto& terrain = world.registry.emplace<TerrainGeometry>(terrain_entity, width, scale);

    WaterMaterial::Setup(world, terrain.BASE_POOL_SIZE, terrain.scale, terrain.CHUNK_SIZE);
    TerrainMaterial::Setup(world, terrain.BASE_POOL_SIZE, terrain.scale, terrain.CHUNK_SIZE);

    for(size_t x=0; x<width; x++)
        for(size_t y=0; y<width; y++) {
            auto chunk_entity = world.registry.create();
            auto& chunk = world.registry.emplace<TerrainChunkHeader>(chunk_entity, terrain.quadtree.MakeChunk(glm::ivec2(x,y)));
        }
    
    return terrain;
}