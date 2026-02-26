#pragma once

#include "world.h"
#include "material.h"
#include "framebuffer.h"
#include "light.h"
#include "profiler.h"

using Engine::FrameBuffer;

class Deferred {
private:
    Engine::DirectDrawMaterial deferredMaterial = Engine::DirectDrawMaterial(Engine::Shader("shaders/primitive/fullscreen_quad.vert","shaders/deferred.frag"));
public:
    FrameBuffer gBuffer;

    Deferred() : gBuffer(3) {
        // Normal buffer
        gBuffer.colorAttachments[0].internalFormat = GL_RGBA16F;
        gBuffer.colorAttachments[0].filterMode = GL_NEAREST;

        // Colour buffer
        gBuffer.colorAttachments[1].internalFormat = GL_RGBA; // unsigned byte
        gBuffer.colorAttachments[1].filterMode = GL_NEAREST;

        // Arm buffer
        gBuffer.colorAttachments[2].internalFormat = GL_RGBA; // unsigned byte
        gBuffer.colorAttachments[2].filterMode = GL_NEAREST;
    }

    void BindGBuffer(int display_w, int display_h, bool clear = true) {
        gBuffer.width = display_w;
        gBuffer.height = display_h;
        gBuffer.Apply();

        gBuffer.Bind();

        if(clear)
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    // Assumes hdr buffer is already bound
    void LightingPass(Engine::World& world) {
        auto profile = Engine::Profiler::StartCpu("Deferred::LightingPass");
        auto gpuProfileHandle = Engine::Profiler::StartGpu("Deferred::LightingPass");

        auto lightsView = world.registry.view<Engine::DirectionalLight>();
        auto& sun = lightsView.get<Engine::DirectionalLight>(lightsView.front());

        glDisable(GL_DEPTH_TEST);
        deferredMaterial.use();

        // gbuffer binds

        glActiveTexture(GL_TEXTURE0);
        gBuffer.colorAttachments[0].bind();

        glActiveTexture(GL_TEXTURE1);
        gBuffer.colorAttachments[1].bind();

        glActiveTexture(GL_TEXTURE2);
        gBuffer.colorAttachments[2].bind();

        glActiveTexture(GL_TEXTURE3);
        gBuffer.depthAttachment.bind();

        // lighting binds

        glActiveTexture(GL_TEXTURE0 + 5);
        sun.shadowMap.BindDepthMap();

        world.skybox.Bind(6);

        Engine::DrawUtil::DrawQuad();

        glBindTexture(GL_TEXTURE_2D, 0);
    }
};