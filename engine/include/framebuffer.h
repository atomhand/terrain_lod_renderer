#pragma once
#include <memory>
#include <cassert>
#include "texture.h"

namespace Engine {
    struct FrameBuffer {
    private:
        struct FrameBufferData {
            GLuint fbo;

            FrameBufferData() {                
                glGenFramebuffers(1, &fbo);
            }
            ~FrameBufferData() {
                glDeleteFramebuffers(1, &fbo);
            }
        };
        std::shared_ptr<FrameBufferData> data;

    public:
        std::vector<Texture> colorAttachments;
        Texture depthAttachment;

        int width;
        int height;

        void Apply(bool forceUpdate = false) {
            bool updated = forceUpdate;
            for(auto& attachment : colorAttachments) {                
                attachment.width = width;
                attachment.height = height;
                updated |= attachment.Apply();
            }

            depthAttachment.width = width;
            depthAttachment.height = height;
            updated |= depthAttachment.Apply();

            if(updated) {                
                glBindFramebuffer(GL_FRAMEBUFFER, data->fbo);

                const GLuint ca[10] = {GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2,GL_COLOR_ATTACHMENT3,GL_COLOR_ATTACHMENT4,
                                GL_COLOR_ATTACHMENT5,GL_COLOR_ATTACHMENT6,GL_COLOR_ATTACHMENT7,GL_COLOR_ATTACHMENT8,GL_COLOR_ATTACHMENT9};
                assert(colorAttachments.size() < 10);

                for(int i =0; i<colorAttachments.size(); i++) {                    
                    glFramebufferTexture2D(GL_FRAMEBUFFER, ca[i], GL_TEXTURE_2D, colorAttachments[i].textureObject(), 0);
                }
                glDrawBuffers(colorAttachments.size(), ca);

                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthAttachment.textureObject(), 0);

                assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
        }

        FrameBuffer(size_t numAttachments) {
            data = std::make_shared<FrameBufferData>();
            assert(numAttachments < 10);
            colorAttachments.resize(numAttachments);

            depthAttachment.internalFormat = GL_DEPTH_COMPONENT;
            depthAttachment.filterMode = GL_NEAREST;
        }

        void Bind() {
            Apply();
            glBindFramebuffer(GL_FRAMEBUFFER, data->fbo);
        }
    };
}