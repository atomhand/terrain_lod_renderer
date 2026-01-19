#pragma once

#include <cassert>
#include <memory>
#include <glad/gl.h>

namespace Engine {
    class StorageBuffer {
    private:
        struct StorageBufferData {
            GLuint object;
            size_t m_size = 0;
            GLuint m_flags = 0;

            StorageBufferData() {
                glCreateBuffers(1,&object);
            }
            ~StorageBufferData() {
                glDeleteBuffers(1,&object);
            }

            void SetSize(size_t size, GLuint flags) {
                //assert(m_size == 0); // cannot resize buffer once the size is set
                if(m_size == size && m_flags == flags) return;
                m_flags = flags;
                if(size == 0) return;
                m_size = size;
                glNamedBufferStorage(object,size,nullptr,flags);
            }
            
            void Resize(size_t size) {
                if(m_size >= size) return;
                m_size = size;
                
                glDeleteBuffers(1,&object);
                glCreateBuffers(1,&object);

                glNamedBufferStorage(object,size,nullptr,m_flags);
            }
        };

        std::shared_ptr<StorageBufferData> buffer;
    public:
        GLuint object() {
            return buffer->object;
        }

        void Set(void* data, size_t size, size_t offset = 0) {
            assert(offset+size <= buffer->m_size);
            glNamedBufferSubData(buffer->object,offset,size,data);
        }

        void ReadbackBytes(void* data, size_t size, size_t offset = 0) {
            assert(offset+size <= buffer->m_size);
            glGetNamedBufferSubData(buffer->object,offset,size,data);
        }

        template <typename T>
        void Readback(T* data, size_t count, size_t offset) {
            ReadbackBytes((void*)data, count * sizeof(T), offset * sizeof(T));
        }

        void BindBase(GLuint bindingpoint) {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingpoint, buffer->object);
        }

        template <typename T>
        void Resize(size_t size) {
            buffer->Resize(size * sizeof(T));
        }

        void ResizeBytes(size_t size) {
            buffer->Resize(size);
        }

        StorageBuffer(size_t size, GLbitfield flags = GL_DYNAMIC_STORAGE_BIT) {
            buffer = std::make_shared<StorageBufferData>();
            buffer->SetSize(size, flags);
        }
    };
}