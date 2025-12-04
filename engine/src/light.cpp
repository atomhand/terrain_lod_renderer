#include "light.h"
#include "render_item.h"

std::vector<size_t> Engine::DirectionalLight::MakeLightSpaceMatrix(Camera& camera,
    std::vector<Engine::RenderItem*> &shadowReceivers,
    std::vector<Engine::RenderItem*> &shadowCasters,
    std::vector<AABB> &terrainAABBs,
    std::vector<bool> terrainCullingResults) {
    glm::vec3 frustumCorners[8];
    glm::vec3 frustumCenter = camera.FrustumCorners(frustumCorners);

    // Light is looking towards the center of the frustum
    lightView = glm::lookAt(frustumCenter,
                            frustumCenter+direction,
                            glm::vec3(0.f,1.f,0.f));

    // Light projection is chosen to tightly fit around the bounds of the shadow-receiving meshes
    // The near plane also needs to be adjusted to make sure all shadow-casters within the projection

    float xMin, xMax, yMin, yMax, zMin, zMax;
    xMin = yMin = zMin = std::numeric_limits<float>::max();
    xMax = yMax = zMax = std::numeric_limits<float>::min();

    glm::vec4 corners[8];
    for(int iReceiver =0; iReceiver<shadowReceivers.size(); iReceiver++) {
        shadowReceivers[iReceiver]->aabb().Corners(corners);
        glm::mat4 MV = lightView * shadowReceivers[iReceiver]->globalTransform;
        for(int i =0; i<8; i++) {
            // Transform AABB corners into the light's coordinate system                    
            glm::vec3 p = MV * corners[i];
            xMin = std::min(xMin,p.x);
            yMin = std::min(yMin,p.y);
            zMin = std::min(zMin,-p.z);
            
            xMax = std::max(xMax,p.x);
            yMax = std::max(yMax,p.y);
            zMax = std::max(zMax,-p.z);
        }
    }
    // Terrain AABBs get special handling for now
    for(int iTerrain = 0; iTerrain<terrainAABBs.size(); iTerrain++) {
        bool passedCulling = terrainCullingResults[iTerrain];        
        if(passedCulling) {
            terrainAABBs[iTerrain].Corners(corners);
            for(int i =0; i<8; i++) {
                // Transform AABB corners into the light's coordinate system                    
                glm::vec3 p = lightView * corners[i];

                zMax = std::max(zMax,-p.z);
                zMin = std::min(zMin,-p.z);
                xMin = std::min(xMin,p.x);
                yMin = std::min(yMin,p.y);            
                xMax = std::max(xMax,p.x);
                yMax = std::max(yMax,p.y);
            }
        }        
    }


    lightProjection = glm::ortho(xMin,xMax,yMin,yMax,zMin,zMax);
    
    // Move the near plane so that all shadow casters (that are within the x,y bounds)
    // are in front of the near plane
    std::vector<size_t> outShadowCasters;
    for(size_t iCaster =0; iCaster<shadowCasters.size(); iCaster++) {
        glm::mat4 MV = lightView * shadowCasters[iCaster]->globalTransform;
        glm::mat4 MVP = lightProjection * MV;
        // We can skip shadowcasters that are outside the x,y bounds of the projection
        if(FrustumAABBTestIgnoreZ(MVP, shadowCasters[iCaster]->aabb())) {
            shadowCasters[iCaster]->aabb().Corners(corners);
            outShadowCasters.push_back(iCaster);
            for(int i =0; i<8; i++) {
                glm::vec3 p = MV * corners[i];
                zMin = std::min(zMin,-p.z);
            }
        }                
    }    
    lightSpaceMatrix = lightProjection * lightView;
    for(int iTerrain = 0; iTerrain<terrainAABBs.size(); iTerrain++) {
        bool passedCulling = terrainCullingResults[iTerrain];        
        if(!passedCulling) {
            auto& aabb = terrainAABBs[iTerrain];
            if(FrustumAABBTestIgnoreZ(lightSpaceMatrix, aabb)) {
                aabb.Corners(corners);
                for(int i =0; i<8; i++) {
                    glm::vec3 p = lightView * corners[i];
                    zMin = std::min(zMin,-p.z);
                }
            }   
        }        
    }

    lightProjection = glm::ortho(xMin,xMax,yMin,yMax,zMin,zMax);
    lightSpaceMatrix = lightProjection * lightView;
    return outShadowCasters;
}