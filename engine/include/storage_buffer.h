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

        uint64_t capacityBytes() {
            return buffer->m_size;
        }

        template <typename T>
        uint64_t capacity() {
            return buffer->m_size / sizeof(T);
        }

        void ResizeBytes(size_t size) {
            buffer->Resize(size);
        }

        template <typename T>
        void Resize(size_t size) {
            buffer->Resize(size * sizeof(T));
        }

        void SmartResizeBytes(size_t target) {
            if(target > buffer->m_size) {
                ResizeBytes(std::max(buffer->m_size*2, target));
            }
        }

        void SetBytes(void* data, size_t size, size_t offset = 0, bool allowResize = false) {
            if(!allowResize)
                assert(offset+size <= buffer->m_size);
            else if(offset+size > buffer->m_size) {
                assert(offset == 0); // dont have provision to copy over old data when resizing
                SmartResizeBytes(offset+size);
            }
            glNamedBufferSubData(buffer->object,offset,size,data);
        }

        template <typename T>
        void Set(T* data, size_t size, size_t offset = 0, bool allowResize = false) {
            SetBytes((void*)data, size*sizeof(T), offset*sizeof(T), allowResize);
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

        StorageBuffer(size_t size, GLbitfield flags = GL_DYNAMIC_STORAGE_BIT) {
            buffer = std::make_shared<StorageBufferData>();
            buffer->SetSize(size, flags);
        }
    };
}