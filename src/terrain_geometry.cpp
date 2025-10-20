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

    int updateQuota = std::min(PoolSlotsAvailable(), budget);
    int numGenerated = 0;
    double generationDuration = 0.0;

    auto& terrainCache = world.GetSingle<TerrainMaterial::Cache>();

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
                node.entity = world.registry.create();
                
                auto start = std::chrono::steady_clock::now();
                Heightmap heightMap(terrain, chunk.positionOffset + uvMin*scale, chunk.positionOffset + uvMax*scale, terrainGeometry.CHUNK_SIZE);
                generationDuration +=  std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
                heightMap.FillData(terrainCache, topIdx);

                node.longestEdge = heightMap.longestEdge;

                auto& terrainMat = world.registry.emplace<TerrainMaterial>(node.entity, topIdx);

                auto& transform = world.registry.emplace<Engine::Transform>(node.entity);
                transform.global = glm::translate(glm::mat4(1.), nodePos) * glm::scale(glm::mat4(1.), glm::vec3(extent.x,1.f,extent.z));

                auto& aabb = world.registry.emplace<Engine::AABB>(node.entity, heightMap.aabb);

                if(heightMap.anyWater) {
                    node.water_entity = world.registry.create();
                    auto& waterItem = world.registry.emplace<WaterMaterial>(node.water_entity);
                    auto& transform = world.registry.emplace<Transform>(node.water_entity);
                    transform.global = glm::translate(glm::mat4(1.), nodePos) * glm::scale(glm::mat4(1.), glm::vec3(extent.x,256.f,extent.z));

                    world.registry.emplace<Engine::AABB>(node.water_entity, glm::vec3(-0.2,-0.5,-0.2), glm::vec3(1.2,0.5,1.2));
                }
            }

            float targetDepth = TargetLodDepth(node.depth, nodePos, extent, world.registry.get<Engine::AABB>(node.entity),camera, world.input.lodControlParam, node.longestEdge);
            if(targetDepth > node.depth) {
                // wants to split
                // (or keep children, if they already exist)
                // no children, try split
                if(node.childIdx == 0
                    && numGenerated +4 <= updateQuota) {
                    AllocChildren(node);
                    numGenerated += 4;

                    node = nodePool[topIdx];
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
                }
            }

            bool shouldDraw = node.childIdx == 0;

            auto& mat = world.registry.get<TerrainMaterial>(node.entity);
            mat.enabled = shouldDraw;

            if(shouldDraw) {
                if(!node.hasRenderComponents) {
                    world.registry.emplace<Engine::CullingResult>(node.entity);
                    world.registry.emplace<Engine::ShadowCaster>(node.entity);
                    world.registry.emplace<Engine::OpaqueRenderTag>(node.entity);

                    if(node.water_entity != entt::null) {                            
                        world.registry.emplace<Engine::CullingResult>(node.water_entity);
                    }
                }
                node.hasRenderComponents = true;
            } else {
                if(node.hasRenderComponents) {
                    world.registry.remove<Engine::CullingResult,Engine::ShadowCaster,Engine::OpaqueRenderTag>(node.entity);
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

Engine::Mesh TerrainGeometry::MakeTerrainMesh() {
    Engine::Mesh::MeshBuilder meshBuilder;
    auto& verts = meshBuilder.verts;
    auto& normals = meshBuilder.normals;
    meshBuilder.lodIndices.resize(1);
    auto& indices = meshBuilder.lodIndices[0];

    meshBuilder.vertexFormat.normalsEnabled = true;

    verts.clear();
    indices.clear();

    int cw = CHUNK_SIZE;
    assert(cw >= 2);

    int vw = cw+3;

    int iz;
    for(iz=0; iz<vw; iz++) {
        for(int ix=0; ix<vw; ix++) {
            int x = std::clamp((ix-1),0,cw);
            int z = std::clamp((iz-1),0,cw);

            glm::vec3 pos = glm::vec3(x / float(cw),0.f,z / float(cw));
            if(ix == 0 || iz == 0 || ix == vw-1 || iz == vw-1) {
                pos.y -= 64.f;
            }

            verts.push_back(pos);
        }
    }

    for(iz=0; iz<vw-1; iz++) {            
        for(int ix=0; ix<vw-1; ix++) {
            GLuint i00 = ix + iz*(vw);
            GLuint i10 = (ix+1) + iz*(vw);
            GLuint i01 = ix + (iz+1)*(vw);
            GLuint i11 = (ix+1) + (iz+1)*(vw);

            indices.push_back(i00);
            indices.push_back(i01);
            indices.push_back(i11);

            indices.push_back(i00);
            indices.push_back(i11);
            indices.push_back(i10);
        }
    }

    meshBuilder.aabb = Engine::AABB(glm::vec3(0.,-1.,0.),glm::vec3(1.0,1.0,1.0));
    return meshBuilder.CreateMesh();
}

Engine::Mesh TerrainGeometry::MakeWaterMesh() {
    Engine::Mesh::MeshBuilder meshBuilder;
    auto& verts = meshBuilder.verts;
    auto& normals = meshBuilder.normals;
    meshBuilder.lodIndices.resize(1);
    auto& indices = meshBuilder.lodIndices[0];

    meshBuilder.vertexFormat.normalsEnabled = true;

    verts.clear();
    normals.clear();
    indices.clear();

    int cw = CHUNK_SIZE;
    assert(cw >= 2);

    // no skirts
    int vw = cw+1;
    int iz;
    for(iz=0; iz<vw; iz++) {
        for(int ix=0; ix<vw; ix++) {
            glm::vec3 pos = glm::vec3(ix / float(cw),0.f,iz / float(cw));
            verts.push_back(pos);
            normals.push_back(glm::vec3(0,1,0));
        }
    }

    for(iz=0; iz<vw-1; iz++) {            
        for(int ix=0; ix<vw-1; ix++) {
            GLuint i00 = ix + iz*(vw);
            GLuint i10 = (ix+1) + iz*(vw);
            GLuint i01 = ix + (iz+1)*(vw);
            GLuint i11 = (ix+1) + (iz+1)*(vw);

            indices.push_back(i00);
            indices.push_back(i01);
            indices.push_back(i11);

            indices.push_back(i00);
            indices.push_back(i11);
            indices.push_back(i10);
        }
    }

    meshBuilder.aabb = Engine::AABB(glm::vec3(0.,-16.,0.),glm::vec3(scale*(CHUNK_SIZE+1),16.,scale*(CHUNK_SIZE+1)));
    return meshBuilder.CreateMesh();
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

        ImGui::SeparatorText("Shared Pool");
        ImGui::Text("Pooled entities available %i", terrainGeometry.quadtree.freeList.size());
        ImGui::Text("Total %i / %i slots used",  terrainGeometry.quadtree.poolCapacity - terrainGeometry.quadtree.PoolSlotsAvailable(), terrainGeometry.quadtree.poolCapacity);
        ImGui::Text("Total %i / %i slots allocated",  terrainGeometry.quadtree.nextPoolId, terrainGeometry.quadtree.poolCapacity);

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

    world.registry.emplace<WaterMaterial::Cache>(world.registry.create(), terrain.BASE_POOL_SIZE, terrain.MakeWaterMesh());
    world.registry.emplace<TerrainMaterial::Cache>(world.registry.create(), terrain.BASE_POOL_SIZE, terrain.CHUNK_SIZE, terrain.MakeTerrainMesh());

    for(size_t x=0; x<width; x++)
        for(size_t y=0; y<width; y++) {
            auto chunk_entity = world.registry.create();
            auto& chunk = world.registry.emplace<TerrainChunkHeader>(chunk_entity, terrain.quadtree.MakeChunk(glm::ivec2(x,y)));
        }
    
    return terrain;
}