#pragma once
#include <memory>
#include <glad/glad.h>

namespace Engine {
    class Texture {
    private:
        struct TextureData {
            GLuint textureObject;

            TextureData() {
                glGenTextures(1, &textureObject);
            }
            ~TextureData() {
                glDeleteTextures(1, &textureObject);
            }

            TextureData & operator=(const TextureData&) = delete;
            TextureData(const TextureData&) = delete;
        };

        std::shared_ptr<TextureData> data;
    public:
        GLuint textureObject() { return data->textureObject; }
        static Texture Import(const char* path);

        Texture() {
            data = std::make_shared<TextureData>();
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

            GLenum filterMode = GL_LINEAR;

            void Apply() {
                glBindTexture(GL_TEXTURE_2D_ARRAY,textureObject);
                glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevelCount, internalFormat, width, height, layerCount);
                //glTexImage3D(GL_TEXTURE_2D_ARRAY,0,GL_RGBA16, width, height, layerCount, 0, GL_RGBA,GL_FLOAT, nullptr);

                glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,filterMode);
                glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,filterMode);
                glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            }
            
            TextureData() {
                glGenTextures(1,&textureObject);
            }
            ~TextureData() {
                glDeleteTextures(1,&textureObject);
            }
        };

        std::shared_ptr<TextureData> data;
    public:
        GLuint textureObject() {return data-> textureObject; }

        Texture2DArray() {
            data = std::make_shared<TextureData>();
        }

        // Sets the filter mode of the texture
        // Should be called before calling Configure
        void SetFilterMode(GLenum filterMode) {
            data->filterMode = filterMode;
        }

        void Configure(GLsizei mipLevelCount, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei layerCount) {
            data->width = width;
            data->height = height;
            data->layerCount = layerCount;
            data->mipLevelCount = mipLevelCount;
            data->internalFormat = internalFormat;
            data->Apply();
        }

        void bind() {
            glBindTexture(GL_TEXTURE_2D_ARRAY,data->textureObject);
        }
    };
}