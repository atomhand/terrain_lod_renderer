#include "light.h"
#include "render_item.h"

std::vector<size_t> Engine::DirectionalLight::MakeLightSpaceMatrix(Camera& camera, std::vector<Engine::RenderItem*> &shadowReceivers, std::vector<Engine::RenderItem*> &shadowCasters) {
    glm::vec3 frustumCorners[8];
    glm::vec3 frustumCenter = camera.FrustumCorners(frustumCorners);

    // Light is looking towards the center of the frustum
    lightView = glm::lookAt(frustumCenter,
                            frustumCenter+direction,
                            glm::vec3(0.f,1.f,0.f));

    // Light projection is chosen to tightly fit around the corners
    // of the view frustum

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

    lightProjection = glm::ortho(xMin,xMax,yMin,yMax,zMin,zMax);
    
    std::vector<size_t> outShadowCasters;
    // TODO - shadowcasters should be culled against the projection
    for(size_t iCaster =0; iCaster<shadowCasters.size(); iCaster++) {
        shadowCasters[iCaster]->aabb().Corners(corners);
        glm::mat4 MV = lightView * shadowCasters[iCaster]->globalTransform;
        glm::mat4 MVP = lightProjection * MV;
        if(FrustumAABBTestIgnoreZ(MVP, shadowCasters[iCaster]->aabb())) {
            outShadowCasters.push_back(iCaster);
            for(int i =0; i<8; i++) {
                // Transform AABB corners into the light's coordinate system
                glm::vec3 p = MV * corners[i];

                // Shadow casters are only relevant for setting the near plane
                zMin = std::min(zMin,-p.z);
                //zMax = std::max(zMax,-p.z);
            }
        }                
    }

    lightProjection = glm::ortho(xMin,xMax,yMin,yMax,zMin,zMax);
    lightSpaceMatrix = lightProjection * lightView;
    return outShadowCasters;
}