// Tom Kellett 2025

// references (not copied) https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
// and https://learnopengl.com/Guest-Articles/2021/CSM
//
// Compared to the learnopengl Cascades implementation, mine is quite different (possibly worse)
// because I came up with my own algorithm to calculate the frustum/projection for each cascade

#include "light.h"
#include "render_item.h"
#include "imgui.h"

void Engine::DirectionalLight::MakeLightSpaceMatrices(World& world, Camera& camera, Texture& depthBuffer, UniformBuffer& lightUniforms) {

    ImGui::Begin("Cascades Debug window");

    int depthOutput[2] = {0x7fffffff,-0x7fffffff};
    depthAnalysisOutput.Set<int>(&depthOutput[0], 2, 0);

    std::vector<int> cascadeOutput;
    for(int i =0; i<NumCascades*3; i++) {
        cascadeOutput.push_back(0x7fffffff);
        cascadeOutput.push_back(-0x7fffffff);
    }
    cascadeAnalysisOutput.Set<int>(cascadeOutput.data(), cascadeOutput.size(), 0);

    depthAnalysisKernel.use();    
    glActiveTexture(GL_TEXTURE0);
    depthBuffer.bind();
    depthAnalysisOutput.BindBase(0);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
    depthAnalysisKernel.Dispatch((depthBuffer.width+15)/16,(depthBuffer.height+15)/16,1);

    //depthAnalysisOutput.Readback<int>(&depthOutput[0], 2, 0);

    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, lightUniforms.object());
    lightSpaceMatricesBuffer.BindBase(1);
    cascadePlaneDistancesBuffer.BindBase(2);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    chooseMatricesKernel.Dispatch(1,1,1);

    cascadeAnalysisKernel.use();    
    glActiveTexture(GL_TEXTURE0);
    depthBuffer.bind();
    cascadeAnalysisOutput.BindBase(0);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    cascadeAnalysisKernel.Dispatch((depthBuffer.width+15)/16,(depthBuffer.height+15)/16,1);

    
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
    cascadeAnalysisOutput.Readback<int>(cascadeOutput.data(), cascadeOutput.size(), 0);
    
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    finishMatricesKernel.Dispatch(1,1,1);

    //float farPlane = std::min(camera.far,camera.furthestItem);
    //float nearPlane = std::max(camera.near,camera.nearestItem);
    //float farPlane = std::min(camera.far,float(depthOutput[1])/1000.f);
    //float nearPlane = std::max(camera.near,float(depthOutput[0])/1000.f);

    //float zScale = farPlane - nearPlane;
    /*
    for(unsigned int i =0; i<NumCascades; i++) {
        float p = float(i+1) / float(NumCascades);
        float logSplit = nearPlane * std::pow(farPlane/nearPlane, p);
        //float uniformSplit = nearPlane + (farPlane-nearPlane)*p;
        cascadeLevels.push_back(logSplit);
    }*/

    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

    lightSpaceMatrices.resize(NumCascades);
    lightSpaceMatricesBuffer.Readback<glm::mat4>(lightSpaceMatrices.data(), NumCascades,0);

    cascadeLevels.resize(NumCascades);
    cascadePlaneDistancesBuffer.Readback<float>(cascadeLevels.data(), NumCascades,0);

    for(int i =0; i<NumCascades; i++) {
        glm::mat4 proj = glm::ortho(-500.f,500.f,-500.f,500.f,-500.f,500.f);
        //lightSpaceMatrices[i] = proj * lightSpaceMatrices[i];

        ImGui::Text("cascade %i ", i);

        ImGui::Text("plane distance %f", cascadeLevels[i]);

        
        ImGui::Text("ortho ranges (%f, %f) (%f, %f) (%f, %f)",
            cascadeOutput[i*6+0]/1000.f,
            cascadeOutput[i*6+1]/1000.f,
            cascadeOutput[i*6+2]/1000.f,
            cascadeOutput[i*6+3]/1000.f,
            cascadeOutput[i*6+4]/1000.f,
            cascadeOutput[i*6+5]/1000.f);


        for(int j=0; j<4; j++)
            ImGui::Text("[ %f, %f, %f, %f]", lightSpaceMatrices[i][0][j],lightSpaceMatrices[i][1][j],lightSpaceMatrices[i][2][j],lightSpaceMatrices[i][3][j]);

        ImGui::Separator();
    }

    ImGui::End();
}