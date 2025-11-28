#pragma once
#include <glad/glad.h>

namespace Engine {
    class Texture {
    private:
        GLuint textureObject;

        //Texture & operator=(const Texture&) = delete;
        //Texture(const Texture&) = delete;
    public:
        void Import(const char* path);

        Texture() {
            glGenTextures(1, &textureObject);
        }
        ~Texture() {
            //glDeleteTextures(1, &textureObject);
        }

        void bind() {
            glBindTexture(GL_TEXTURE_2D,textureObject);
        }
    };
}