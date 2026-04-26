
// references (not copied) https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
// and https://learnopengl.com/Guest-Articles/2021/CSM
//
// Compared to the learnopengl Cascades implementation, mine is quite different (possibly worse)
// because I came up with my own algorithm to calculate the frustum/projection for each cascade

#include "light.h"

void Engine::DirectionalLight::MakeLightSpaceMatrices(World& world, Camera& camera, Texture& depthBuffer, UniformBuffer& lightUniforms) {
    NumCascades = world.input.numCascades;
    assert(NumCascades > 0);
    assert(NumCascades <= 16);
    if(NumCascades != shadowMap.NumCascades()) {
        shadowMap = DirectionalShadowCascadeMap(2048,NumCascades);
    }


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
    
    lightViewMatricesBuffer.BindBase(3);
    lightFrustumPlanesBuffer.BindBase(4);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    finishMatricesKernel.Dispatch(1,1,1);
}