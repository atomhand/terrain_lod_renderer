#include <iostream>
#include <filesystem>

#include "stb_image.h"

#include "asset_helper.h"
#include "texture.h"

// Reference https://learnopengl.com/Getting-started/Textures
Engine::Texture Engine::Texture::Import(const char* filePath) {   
    Engine::Texture tex; 
    std::filesystem::path path = AssetHelper::assetPath(filePath);


    glBindTexture(GL_TEXTURE_2D, tex.data->textureObject);

    // set filter/wrap options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // load and generate the texture
    int width, height, nrChannels;
    unsigned char *data = stbi_load(path.string().c_str(), &width, &height, &nrChannels, 0);

    if(data) {
        GLenum format;
        switch (nrChannels)
        {
        case 1:
            format = GL_RED;
            break;
        case 2:
            format = GL_RG;
            break;
        case 3:
            format = GL_RGB;
            break;
        case 4:
            format = GL_RGBA;
            break;
        default:
            std::cout << "Warning : loading texture with supported numbers of channels " << nrChannels << ") " << filePath << std::endl;
            break;
        }


        // NOTE - currently, regardless of the input format, I'm just storing it as a 3 channel texture
        // This is clearly something that could be improved on in the future
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture at path " << path.string() << std::endl;
    }

    glBindTexture(GL_TEXTURE_2D,0);
    stbi_image_free(data);

    return tex;
}