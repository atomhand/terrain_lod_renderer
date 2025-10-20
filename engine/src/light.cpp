// Tom Kellett 2025

// references (not copied) https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
// and https://learnopengl.com/Guest-Articles/2021/CSM
//
// Compared to the learnopengl Cascades implementation, mine is quite different (possibly worse)
// because I came up with my own algorithm to calculate the frustum/projection for each cascade

#include "light.h"
#include "render_item.h"

void Engine::DirectionalLight::MakeLightSpaceMatrices(World& world, Camera& camera) {
    float farPlane = std::min(camera.far,camera.furthestItem);
    float nearPlane = std::max(camera.near,camera.nearestItem);

    float zScale = farPlane - nearPlane;
    cascadeLevels = { nearPlane + zScale / 25.0f, nearPlane + zScale / 10.0f, nearPlane + zScale / 5.0f, nearPlane + zScale / 2.0f, farPlane };

    lightSpaceMatrices.clear();

    world.registry.clear<SurvivedLightCullingTag>();

    // Set up 
    std::vector<Cascade> cascades;
    cascades.reserve(cascadeLevels.size());
    cascades.push_back(Cascade(nearPlane,cascadeLevels[0],camera,direction));
    for(size_t i =1; i<cascadeLevels.size(); i++) {
        cascades.push_back(Cascade(cascadeLevels[i-1],cascadeLevels[i],camera,direction));
    }

    // Expand cascades to fit visible (non-culled) AABBs
    auto visibleItemsView = world.registry.view<Transform,AABB,CullingResult>();
    glm::vec4 corners[8];
    for(auto entity : visibleItemsView) {
        auto [transform,aabb,cullingResult] = visibleItemsView.get(entity);
        if(!cullingResult.viewResult)
            continue;

        aabb.Corners(corners);
        for(auto& cascade : cascades) {
            if(cullingResult.viewDepthMin > cascade.far || cullingResult.viewDepthMax < cascade.near) {
                continue;
            }
            glm::mat4 MV = cascade.view * transform.global;
            for(int i =0; i<8; i++) {
                // Transform AABB corners into the light's coordinate system
                glm::vec4 p = MV * corners[i];
                p.z = -p.z;
                cascade.min = glm::min(cascade.min,glm::vec3(p));
                cascade.max = glm::max(cascade.max,glm::vec3(p));
            }
        }
    }

    for(auto& cascade : cascades) {
        cascade.updateProjection();
    }
    
    // Move the near plane so that all shadow casters (that are within the x,y bounds)
    // are in front of the near plane
    auto shadowCastersView = world.registry.view<Transform,AABB,ShadowCaster>();
    for(auto entity : shadowCastersView) {
        auto [transform,aabb] = shadowCastersView.get(entity);
        aabb.Corners(corners);

        bool passedCulling = false;
        
        for(auto& cascade : cascades) {
            glm::mat4 MV = cascade.view * transform.global;
            glm::mat4 MVP = cascade.projection * MV;
            // We can skip shadowcasters that are outside the x,y bounds of the projection

            // Could combine the frustum test/min-z determination to save a little work
            if(FrustumAABBTestIgnoreZ(MVP, aabb)) {
                passedCulling = true;
                for(int i =0; i<8; i++) {
                    glm::vec4 p = MV * corners[i];
                    cascade.min.z = std::min(cascade.min.z,-p.z);
                }
            } 
        }

        if(passedCulling)
            world.registry.emplace<SurvivedLightCullingTag>(entity);
    }

    for(auto& cascade : cascades) {
        cascade.updateProjection();
        lightSpaceMatrices.push_back(cascade.projection * cascade.view);
    }
}