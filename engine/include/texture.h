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
        void Import(const char* path);

        Texture() {
            data = std::make_shared<TextureData>();
        }

        void bind() {
            glBindTexture(GL_TEXTURE_2D,data->textureObject);
        }
    };
}