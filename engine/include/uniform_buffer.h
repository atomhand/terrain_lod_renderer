#pragma once

#include <memory>
#include <glad/gl.h>

namespace Engine {
    class UniformBuffer {
    private:
        struct UniformBufferData {
            GLuint object;

            UniformBufferData() {
                glCreateBuffers(1,&object);
            }
            ~UniformBufferData() {
                glDeleteBuffers(1,&object);
            }

            void Resize(size_t size) {
                glNamedBufferData(object, size, NULL, GL_STATIC_DRAW);
            }
        };

        std::shared_ptr<UniformBufferData> buffer;
    public:
        template<typename T> void Set(T* data) {
            glNamedBufferData(buffer->object, sizeof(T), data, GL_STATIC_DRAW);
        }

        void BindBase(GLuint bindingpoint) {
            glBindBufferBase(GL_UNIFORM_BUFFER, bindingpoint, buffer->object);
        }

        UniformBuffer() {
            buffer = std::make_shared<UniformBufferData>();
        }
    };
}