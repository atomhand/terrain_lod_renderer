#pragma once

#include <glm/glm.hpp>
#include "glad/gl.h"
#include "texture.h"
#include "compute_shader.h"
#include "storage_buffer.h"
#include "framebuffer.h"
#include "profiler.h"

class HDR {
    struct Params {
        float minLogLum;
        float logLumRange;
        float timeCoeff;
        float numPixels;
    };

    float minLogLum = -10.0;
    float maxLogLum = 2.0;
    float timeCoeff = 0.1f;

    Engine::ComputeShader luminanceHistogramShader = Engine::ComputeShader("shaders/luminance_histogram.cs");
    Engine::ComputeShader computeExposureShader = Engine::ComputeShader("shaders/luminance_compute_exposure.cs");
    Engine::StorageBuffer luminanceHistogramBuffer;

    Engine::DirectDrawMaterial tonemapMaterial = Engine::DirectDrawMaterial(Engine::Shader("shaders/primitive/fullscreen_quad.vert","shaders/tonemap.frag"));
    Engine::DirectDrawMaterial heatmapMaterial = Engine::DirectDrawMaterial(Engine::Shader("shaders/primitive/fullscreen_quad.vert","shaders/heatmap.frag"));

    Engine::Texture outputTexture;
    void ComputeExposure(Engine::World& world) {
        auto& buffTexture = hdrFramebuffer.colorAttachments[0];
        // bind hdr buffer texture as image        
        glBindImageTexture(0, buffTexture.textureObject(), 0, false, 0, GL_READ_ONLY, GL_RGBA16F);

        // bind storage buffer
        luminanceHistogramBuffer.BindBase(0);

        luminanceHistogramShader.use();
        glm::vec4 params = glm::vec4(
            minLogLum, // minLogLum
            1.0 / (maxLogLum - minLogLum), // inverse logLumRange
            timeCoeff, // timeCoeff
            buffTexture.width * buffTexture.height // numPixels
        );
        luminanceHistogramShader.setVec4("u_params", params);
        glDispatchCompute((unsigned int)std::ceil(buffTexture.width/16.0), (unsigned int)std::ceil(buffTexture.height/16.0), 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // bind target texture
        glBindImageTexture(0, outputTexture.textureObject(), 0, false, 0, GL_READ_WRITE, GL_R16F);

        computeExposureShader.use();
        params.y = maxLogLum - minLogLum;
        computeExposureShader.setVec4("u_params", params);
        computeExposureShader.setVec2("t_params", glm::vec2(world.input.deltaTime,1.1f));
        glDispatchCompute(1, 1, 1);

        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
    }
public:
    Engine::FrameBuffer hdrFramebuffer = Engine::FrameBuffer(1);

    HDR() : luminanceHistogramBuffer(sizeof(float)*256) {
        outputTexture.width = 1;
        outputTexture.height = 1;
        outputTexture.internalFormat = GL_R16F;
        outputTexture.Apply();

        hdrFramebuffer.colorAttachments[0].internalFormat = GL_RGBA16F;
    }

    void BindHdrFramebuffer(int display_w, int display_h) {
        glViewport(0,0,display_w,display_h);
        hdrFramebuffer.width = display_w;
        hdrFramebuffer.height = display_h;
        hdrFramebuffer.Apply();

        hdrFramebuffer.Bind();
    }

    void ApplyTonemapping(Engine::World& world) {
        auto profile = Engine::Profiler::StartCpu("Hdr::ApplyTonemapping");
        auto gpuProfileHandle = Engine::Profiler::StartGpu("Hdr::ApplyTonemapping");
        
        ComputeExposure(world);

        glDisable(GL_DEPTH_TEST);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        tonemapMaterial.use();
        glActiveTexture(GL_TEXTURE0);
        hdrFramebuffer.colorAttachments[0].bind();

        glActiveTexture(GL_TEXTURE1);
        outputTexture.bind();

        Engine::DrawUtil::DrawQuad();

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    void ApplyHeatmapping(Engine::World& world) {
        ComputeExposure(world);

        glDisable(GL_DEPTH_TEST);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        heatmapMaterial.use();
        glActiveTexture(GL_TEXTURE0);
        hdrFramebuffer.colorAttachments[0].bind();

        glActiveTexture(GL_TEXTURE1);
        outputTexture.bind();

        Engine::DrawUtil::DrawQuad();

        glBindTexture(GL_TEXTURE_2D, 0);
    }
};