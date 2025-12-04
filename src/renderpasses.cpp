#include "renderpasses.h"
#include "terrain.h"

void RenderPasses::RetrieveData(DemoWorld& world) {
        terrain = world.scenegraph.First<Terrain>();
        assert(terrain != nullptr);

        cameraMain = world.cameraMain();
        glm::vec3 cameraPos = cameraMain->position();
        debugCamera -> localTransform = glm::translate(glm::mat4(1.),cameraPos) * glm::rotate(glm::mat4(1.), glm::radians(-45.f), glm::vec3(0.,1.,0.)) * glm::rotate(glm::mat4(1.), glm::radians(-90.f), glm::vec3(1.,0.,0.)) * glm::translate(glm::mat4(1.), glm::vec3(0.,0.,256.));

        pointLights.clear();
        directionalLights.clear();
        opaqueItems.clear();
        transparentItems.clear();
        shadowCasters.clear();

        auto nodes = world.scenegraph.AllNodes();
        glm::mat4 VP = cameraMain->projection() * cameraMain->view();

        for(auto node : nodes) {
            if(RenderItem* item= dynamic_cast<RenderItem*>(node); item != nullptr) {
                if(item->casts_shadow())
                    shadowCasters.push_back(item);

                if(item->enableCulling) {
                    glm::mat4 MVP = VP * item->globalTransform;

                    glm::vec3 max = item->globalTransform * glm::vec4(item->mesh.aabb.max,1.0);
                    glm::vec3 min = item->globalTransform * glm::vec4(item->mesh.aabb.min,1.0);
                    glm::vec3 center = (max+min)*0.5f;
                    // Objects that are close enough to the camera (relative to the size of their AABB)
                    // automatically pass the frustum test. This a compensation for the propensity
                    // for my frustum test implementation to produce false negatives near the camera.
                    bool frustumTest = glm::distance(cameraPos,center) < glm::distance(min,max) * 2.0 ||Engine::FrustumAABBTest(MVP, item->mesh.aabb);
                    if(!frustumTest)
                        continue;
                }

                switch(item->renderPass) {
                    case RenderPass::OPAQUE:
                        opaqueItems.push_back(item);
                        break;
                    case RenderPass::TRANSPARENT:
                        transparentItems.push_back(item);
                        break;
                }
            }
            else if(Engine::DirectionalLight* t= dynamic_cast<Engine::DirectionalLight*>(node); t != nullptr) {
                directionalLights.push_back(t);
            }  else if(Engine::PointLight* t= dynamic_cast<Engine::PointLight*>(node); t != nullptr) {
                pointLights.push_back(t);
            }
        }
        terrainCullingResults.assign(terrain->aabbs.size(), false);
        for(int i =0; i<terrain->aabbs.size(); i++) {
            auto& aabb = terrain->aabbs[i];
            glm::vec3 center = (aabb.max+aabb.min)*0.5f;
            bool frustumTest = glm::distance(cameraPos,center) < glm::distance(aabb.min,aabb.max) * 2.0 ||Engine::FrustumAABBTest(VP, aabb);
            terrainCullingResults[i] = frustumTest;
        }
    }

void RenderPasses::DrawDebug(DemoWorld& world) {        
    glBindFramebuffer(GL_FRAMEBUFFER, debugCameraFBO);
    glViewport(0, 0, DEBUG_WIDTH, DEBUG_HEIGHT);
    glClearColor(0.1f,0.1f,0.25f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto temp = cameraMain;
    cameraMain =debugCamera;
    cameraMain->setFramebufferSize(DEBUG_WIDTH,DEBUG_HEIGHT);
    DrawOpaque(world);
    DrawTransparent(world);
    cameraMain= temp;

    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
    //glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    debugWireframeShader.use();
    if(true) {
        debugWireframeShader.setMat4("view", debugCamera->view());
        debugWireframeShader.setMat4("projection", debugCamera->projection());
    } else {
        auto sun = directionalLights[0];            
        debugWireframeShader.setMat4("view", sun->lightView);
        debugWireframeShader.setMat4("projection", glm::ortho(-32.,32.,-32.,32.,0.1,256.));
    }

    // draw AABBS for items that survived culling
    for(auto item : opaqueItems) {
        auto aabb = item->mesh.aabb;
        glm::vec3 extent = aabb.max - aabb.min;
        glm::vec3 offset = aabb.min + (extent / 2.f);
        glm::mat4 aabbT = item->globalTransform* glm::translate(glm::mat4(1.0),offset) * glm::scale(glm::mat4(1.0), extent/2.f);
        
        debugWireframeShader.setMat4("model", aabbT);
        Engine::DrawUtil::DrawCube();
    }
    for(auto item : transparentItems) {
        auto aabb = item->mesh.aabb;
        glm::vec3 extent = aabb.max - aabb.min;
        glm::vec3 offset = aabb.min + (extent / 2.f);
        glm::mat4 aabbT = item->globalTransform* glm::translate(glm::mat4(1.0),offset) * glm::scale(glm::mat4(1.0), extent/2.f);
        
        debugWireframeShader.setMat4("model", aabbT);
        Engine::DrawUtil::DrawCube();
    }

    glm::mat4 invCamera = glm::inverse(cameraMain->projection() * cameraMain->view());
    debugWireframeShader.setVec3("color", glm::vec3(1.0,0.0,0.0));
    debugWireframeShader.setMat4("model", invCamera);
    Engine::DrawUtil::DrawCube();
    
    for(auto light : directionalLights) {
        glm::mat4 invCamera = glm::inverse(light->lightSpaceMatrix);
        debugWireframeShader.setVec3("color", glm::vec3(1.0,1.1,0.0));
        debugWireframeShader.setMat4("model", invCamera);
        Engine::DrawUtil::DrawCube();
    }
    
    glEnable(GL_DEPTH_TEST);
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPasses::DrawShadowMaps(DemoWorld& world) {
    // No face culling for shadows right now, because it doesn't work with my 
    // non-manifold terrain mesh
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    shadowReceivers.clear();
    for(auto item : opaqueItems) {
        shadowReceivers.push_back(item);
    }
    for(auto item : transparentItems) {
        shadowReceivers.push_back(item);
    }

    world.shadowShader.use();
    for(auto light : directionalLights) {
        auto culledShadowCasters = light->MakeLightSpaceMatrix(*cameraMain, shadowReceivers, shadowCasters, terrain->aabbs, terrainCullingResults);

        light->shadowMap.PrepareFramebuffer();
        world.shadowShader.setMat4("lightSpaceMatrix", light->lightSpaceMatrix);

        // Draw meshes
        for(size_t id : culledShadowCasters) {
            RenderItem* item = shadowCasters[id];               
            world.shadowShader.setMat4("model", item->globalTransform);

            Engine::Mesh& mesh =item->mesh;            
            glBindVertexArray(mesh.vao());
            glDrawElements(GL_TRIANGLES, mesh.count(), GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        }
        
        terrain->DrawShadows(light->lightSpaceMatrix);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    glUseProgram(0);
}

void RenderPasses::DrawOpaque(DemoWorld& world) {
    if(world.input.wireFrame)
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
    
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);

    glm::mat4 view = cameraMain->view();

    for(RenderItem* item : opaqueItems) {            
        // bind and configure material
        item->material->use();
        item->material->setCamera(*cameraMain);
        item->material->setModel(item->globalTransform);
        item->material->shader.setFloat("time",world.time);
        for(int i =0; i<pointLights.size() && i < 4; i++) {
            item->material->setLight(*pointLights[i], view, i);
        }
        for(int i =0; i<directionalLights.size() && i < 4; i++) {
            item->material->setLight(*directionalLights[i], view, i);
        }

        world.skybox.Bind(6);

        // bind and draw mesh
        Engine::Mesh& mesh =item->mesh;            
        glBindVertexArray(mesh.vao());
        glDrawElements(GL_TRIANGLES, mesh.count(), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);

        // bind
        item->material->unbind();
    }
    // draw terrain
    terrain->terrainMaterial.use();
    terrain->terrainMaterial.setCamera(*cameraMain);
    for(int i =0; i<pointLights.size() && i < 4; i++) {
        terrain->terrainMaterial.setLight(*pointLights[i], view, i);
    }
    for(int i =0; i<directionalLights.size() && i < 4; i++) {
        terrain->terrainMaterial.setLight(*directionalLights[i], view, i);
    }
    world.skybox.Bind(6);
    terrain->Draw();
    terrain->terrainMaterial.unbind();
    
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    world.skybox.DrawSkybox(cameraMain->view(),cameraMain->projection());
    glDepthMask(GL_TRUE);
}

 void RenderPasses::DrawTransparent(DemoWorld& world) {
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);        
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 view = cameraMain->view();

    for(RenderItem* item : transparentItems) {
        // bind and configure material
        item->material->use();
        item->material->setCamera(*cameraMain);
        item->material->setModel(item->globalTransform);
        item->material->shader.setFloat("time",world.time);
        for(int i =0; i<pointLights.size() && i < 4; i++) {
            item->material->setLight(*pointLights[i], view, i);
        }
        for(int i =0; i<directionalLights.size() && i < 4; i++) {
            item->material->setLight(*directionalLights[i], view, i);
        }

        // bind and draw mesh
        Engine::Mesh& mesh =item->mesh;            
        glBindVertexArray(mesh.vao());
        glDrawElements(GL_TRIANGLES, mesh.count(), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);

        // bind
        item->material->unbind();
    }
}