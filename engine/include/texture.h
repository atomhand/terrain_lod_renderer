// Tom Kellett 2025
#pragma once
#include <iostream>
#include <memory>
#include <glad/gl.h>

namespace Engine {
    class Texture {
    private:
        struct TextureData {
            TextureData & operator=(const TextureData&) = delete;
            TextureData(const TextureData&) = delete;

            GLuint textureObject;

            int width;
            int height;
            GLenum internalformat = GL_RGB;
            GLenum filterMode;
            GLenum clampMode;
            bool hasMips;

            TextureData() {
                glGenTextures(1, &textureObject);
            }
            ~TextureData() {
                glDeleteTextures(1, &textureObject);
            }
        };

        std::shared_ptr<TextureData> data;
    public:
        GLuint textureObject() { return data->textureObject; }
        void Import(const char* path);

        GLenum internalFormat = GL_RGB;
        GLenum filterMode = GL_LINEAR;
        GLenum clampMode = GL_REPEAT;
        int width;
        int height;

        // Apply changes to texture config
        // If size or format have changed, this will clear the texture
        // return true if texture is modified
        bool Apply(GLenum inFormat = GL_RGB, const void* inData = NULL, bool generateMips = false);

        explicit Texture(const char* path) : data(std::make_shared<TextureData>()) {
            Import(path);
        }

        Texture() : data(std::make_shared<TextureData>()) {
        }

        void bind() {
            glBindTexture(GL_TEXTURE_2D,data->textureObject);
        }
    };

    class Texture2DArray {
    private:
        struct TextureData{
            TextureData & operator=(const TextureData&) = delete;
            TextureData(const TextureData&) = delete;

            GLsizei width = 2;
            GLsizei height = 2;
            GLsizei layerCount = 2;
            GLsizei mipLevelCount = 1;
            GLuint textureObject;
            GLenum internalFormat;
            GLenum wrapMode = GL_CLAMP_TO_EDGE;

            GLenum filterMode = GL_LINEAR;

            void Apply() {
                glBindTexture(GL_TEXTURE_2D_ARRAY,textureObject);
                glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevelCount, internalFormat, width, height, layerCount);
                //glTexImage3D(GL_TEXTURE_2D_ARRAY,0,GL_RGBA16, width, height, layerCount, 0, GL_RGBA,GL_FLOAT, nullptr);

                glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,filterMode);
                glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,filterMode);
                glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,wrapMode);
                glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,wrapMode);
                glBindTexture(GL_TEXTURE_2D_ARRAY,0);
            }
            
            TextureData() {
                glGenTextures(1,&textureObject);
            }
            ~TextureData() {
                glDeleteTextures(1,&textureObject);
            }
        };

        std::shared_ptr<TextureData> data;

        bool Serialize(const char* path);
        bool TryDeserialize(const char* path);
    public:
        GLuint textureObject() {return data-> textureObject; }

        Texture2DArray() {
            data = std::make_shared<TextureData>();
        }

        static Texture2DArray Import(std::vector<const char*> paths, const char* compressedPath = nullptr);

        // Sets the filter mode of the texture
        // Should be called before calling Configure
        void SetFilterMode(GLenum filterMode) {
            data->filterMode = filterMode;
        }

        void Configure(GLsizei mipLevelCount, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei layerCount, GLenum filterMode = GL_LINEAR, GLenum wrapMode = GL_CLAMP_TO_EDGE) {
            data->width = width;
            data->height = height;
            data->layerCount = layerCount;
            data->mipLevelCount = mipLevelCount;
            data->internalFormat = internalFormat;
            data->filterMode = filterMode;
            data->wrapMode = wrapMode;
            data->Apply();
        }

        void bind() {
            glBindTexture(GL_TEXTURE_2D_ARRAY,data->textureObject);
        }
    };
}