// Reference https://learnopengl.com/Getting-started/Textures
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <cassert>

#include "stb_image.h"

#include "asset_helper.h"
#include "texture.h"

bool Engine::Texture::Apply(GLenum inFormat, const void* inData, bool generateMips) {
    bool bound = false;
    if( width != data->width ||
        height != data->height ||
        internalFormat != data->internalformat
    ) {
        glBindTexture(GL_TEXTURE_2D,textureObject());
        bound = true;
        if(internalFormat == GL_DEPTH_COMPONENT || internalFormat == GL_DEPTH_COMPONENT16 || internalFormat == GL_DEPTH_COMPONENT24 || internalFormat == GL_DEPTH_COMPONENT32F) {
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_DEPTH_COMPONENT,  GL_UNSIGNED_BYTE, nullptr);

        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, inFormat, GL_UNSIGNED_BYTE, inData);
        }

        if(generateMips) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        data->hasMips = generateMips;

        data->width = width;
        data->height = height;
        data->internalformat = internalFormat;
    }

    if( bound || // always config filter/wrap if texture was modified
        filterMode != data->filterMode ||
        clampMode != data->clampMode
    ) {                
        if(!bound) {
            glBindTexture(GL_TEXTURE_2D,textureObject());
            bound = true;
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (data->hasMips && filterMode == GL_LINEAR) ? GL_LINEAR_MIPMAP_LINEAR : filterMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clampMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clampMode);

        data->filterMode = filterMode;
        data->clampMode = clampMode;
    }  
    if(bound) glBindTexture(GL_TEXTURE_2D,0);

    return bound;
}

// Written with close reference to https://learnopengl.com/Getting-started/Textures
void Engine::Texture::Import(const char* filePath) {
    std::filesystem::path path = AssetHelper::assetPath(filePath);

    // load and generate the texture
    int inWidth, inHeight, nrChannels;
    unsigned char *inData = stbi_load(path.string().c_str(), &inWidth, &inHeight, &nrChannels, 0);

    if(inData) {
        GLenum inFormat;
        switch (nrChannels)
        {
        case 1:
            inFormat = GL_RED;
            break;
        case 2:
            inFormat = GL_RG;
            break;
        case 3:
            inFormat = GL_RGB;
            break;
        case 4:
            inFormat = GL_RGBA;
            break;
        default:
            std::cout << "Warning : loading texture with supported numbers of channels " << nrChannels << ") " << filePath << std::endl;
            break;
        }

        internalFormat = inFormat;
        width = inWidth;
        height = inHeight;
        
        Apply(inFormat, inData, true);
    } else {
        std::cout << "Failed to load texture at path " << path.string() << std::endl;
    }
    stbi_image_free(inData);
}

struct CompressedTextureHeader {
    GLint internalFormat;

    GLint mipLevels;
    GLint layerCount;
    GLint width[16], height[16], compressedSize[16];
};

bool Engine::Texture2DArray::Serialize(const char* path) {
    auto start = std::chrono::steady_clock::now();
    bind();

    // Get compressed texture data

    GLint compressed;
    glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_COMPRESSED, &compressed);
    assert(compressed == GL_TRUE);

    GLint internalFormat;
    glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);

    std::ofstream fout;
    fout.open(path, std::ios::binary | std::ios::out);
    if(!fout.is_open())
        return false;

    CompressedTextureHeader header;
    header.layerCount = data->layerCount;
    header.internalFormat = internalFormat;

    header.mipLevels = 0;

    for(int i =0; i<16; i++) {                
        glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, i,  GL_TEXTURE_WIDTH, &header.width[i]);
        glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, i,  GL_TEXTURE_HEIGHT, &header.height[i]);

        if(header.width[i] > 0 && header.height[i] > 0) {
            header.mipLevels = i+1;
            glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, i, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &header.compressedSize[i]);

            int formatTest;
            glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, i, GL_TEXTURE_INTERNAL_FORMAT, &formatTest);
            assert(formatTest == internalFormat);
        }
    }

    fout.write((char*)&header, sizeof(header));

    for(int i=0; i<header.mipLevels; i++) {        
        unsigned char* pixels = new unsigned char[header.compressedSize[i] * sizeof(unsigned char)];
        glGetCompressedTexImage(GL_TEXTURE_2D_ARRAY, i, pixels);
        fout.write((char*)pixels, header.compressedSize[i]);
        delete pixels;
    }
    fout.close();

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double,std::milli> elapsed = (end-start);
    std::cout << "Compressed texture serialisation took " << elapsed << " | ";

    return true;
}

bool Engine::Texture2DArray::TryDeserialize(const char* path) {
    auto start = std::chrono::steady_clock::now();
    bind();

    if(!std::filesystem::exists(path))
        return false;

    std::ifstream fin;
    fin.open(path, std::ios::binary | std::ios::in);

    CompressedTextureHeader header;
    fin.read((char*)&header, sizeof(header));

    for(int i=0; i<header.mipLevels; i++) {
        unsigned char* pixels = new unsigned char[header.compressedSize[i] * sizeof(unsigned char)];
        fin.read((char*)pixels, header.compressedSize[i]);
        glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, i, header.internalFormat, header.width[i], header.height[i], header.layerCount, 0, header.compressedSize[i], pixels);
        delete pixels;
    }

    data->width = header.width[0];
    data->height = header.height[0];
    data->internalFormat = header.internalFormat;
    data->layerCount = header.layerCount;
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double,std::milli> elapsed = (end-start);
    std::cout << "Compressed texture deserialisation took " << elapsed << " | ";

    return true;
}

// Parts shared with the texture2d code are attributable to https://learnopengl.com/Getting-started/Textures
// everything specific to texture2darray my own
Engine::Texture2DArray Engine::Texture2DArray::Import(std::vector<const char*> paths, const char* compressedPath) {
    auto tStart = std::chrono::steady_clock::now();
    std::chrono::duration<double,std::milli> elapsed;
    Engine::Texture2DArray tex;

    auto start = std::chrono::steady_clock::now();
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex.data->textureObject);

    // set filter/wrap options
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    bool first = true;
    int oldWidth, oldHeight;

    bool loadedFromCompressed = false;
    if(compressedPath != nullptr) {        
        std::filesystem::path fullCompressedPath = AssetHelper::compressedAssetPath(compressedPath);
        std::cout << "Compressed path: " << fullCompressedPath.string() << std::endl;
        loadedFromCompressed = tex.TryDeserialize(fullCompressedPath.string().c_str());
    }

    if(loadedFromCompressed) {

    } else {
        start = std::chrono::steady_clock::now();
        for(int i=0; i<paths.size(); i++) {
            auto rawPath = paths[i];        
            std::filesystem::path path = AssetHelper::assetPath(rawPath);
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
                    std::cout << "Warning : loading texture with unsupported number of channels " << nrChannels << ") " << rawPath << std::endl;
                    break;
                }

                if(first) {
                    first = false;

                    oldWidth = width;
                    oldHeight = height;
            
                    glTexImage3D(GL_TEXTURE_2D_ARRAY,0,GL_COMPRESSED_RGB, width, height, paths.size(), 0, format,GL_FLOAT, nullptr);

                    tex.data->width = width;
                    tex.data->height = height;
                    tex.data->layerCount = paths.size();
                } else {
                    if(width != oldWidth || height != oldHeight) {
                        std::cout << "Warning : loading texture array with mismatched dimensions" << rawPath << std::endl; 
                    }
                }

                glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0,0,i,width,height,1,format,GL_UNSIGNED_BYTE,data);
                
                // NOTE - currently, regardless of the input format, I'm just storing it as a 3 channel texture
                // This is clearly something that could be improved on in the future
                //glTexImage2D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            } else {
                std::cout << "Failed to load texture at path " << path.string() << std::endl;
                break;
            }
            stbi_image_free(data);
        }

        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

        elapsed = std::chrono::steady_clock::now()-start;
        std::cout << "Loading raw texture took " << elapsed << std::endl;

        if(compressedPath != nullptr) {
            std::filesystem::path fullCompressedPath = AssetHelper::compressedAssetPath(compressedPath);
            bool success = tex.Serialize(fullCompressedPath.string().c_str());
            if(success)                
                std::cout << "Texture serialised to " << fullCompressedPath.string().c_str() << std::endl;
            else            
                std::cout << "ERROR: Texture failed to serialise to " << fullCompressedPath.string().c_str() << std::endl;
        }

    }

    elapsed = std::chrono::steady_clock::now()-tStart;
    std::cout << "Texture load total: " << elapsed << std::endl;

    glBindTexture(GL_TEXTURE_2D_ARRAY,0);
    return tex;
}