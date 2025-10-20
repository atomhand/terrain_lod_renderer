#pragma once

#include <span>
#include <string>
#include <iostream>
#include "asset_helper.h"
#include "stb_include.h"

#include <unordered_map>
#include "glad/gl.h"

struct ProgramId{
    const GLuint vertex;
    const GLuint fragment;
    const GLuint geometry;

     bool operator==(const ProgramId &other) const {
        return (vertex == other.vertex
                && fragment == other.fragment
                && geometry == other.geometry);
    }
};

template<>
struct std::hash<ProgramId> {
    std::size_t operator()(const ProgramId& k) const {            
        using std::size_t;
        using std::hash;
        using std::string;

        // Compute individual hash values for first,
        // second and third and combine them using XOR
        // and bit shifting:

        return ((hash<unsigned int>()(k.vertex)
                ^ (hash<unsigned int>()(k.fragment) << 1)) >> 1)
                ^ (hash<unsigned int>()(k.geometry) << 1);
    }
};

class ShaderCache {
    static inline std::unordered_map<std::string,GLuint> shaders;
    static inline std::unordered_map<ProgramId,GLuint> programs;
    static inline std::unordered_map<GLuint,GLuint> computePrograms;

    static const char* GetTypeString(GLenum type) {        
        switch(type) {
            case GL_VERTEX_SHADER:
                return "VERTEX";
                break;
            case GL_FRAGMENT_SHADER:
                return"FRAGMENT";
                break;
            case GL_GEOMETRY_SHADER:
                return "GEOMETRY";
                break;
            case GL_COMPUTE_SHADER:
                return "COMPUTE";
                break;
        }
        return "ERROR_UNRECOGNISED_SHADER_TYPE";
    }

    static GLuint LoadShader(const char* path, const char* inject, GLenum type) {
        std::string shadersDirectory = Engine::AssetHelper::assetPath("shaders/").string();
        std::string fullPath = Engine::AssetHelper::assetPath(path).string();

        // STB include 
        char error[256];
        char* shaderCode = stb_include_file(fullPath.c_str(), inject, shadersDirectory.c_str(), error);
        if(!shaderCode) {
            std::cout << "ERROR::SHADER::" << GetTypeString(type) << "::LOADING_FAILED\n" << error << std::endl;
        }    

        int success;
        char infoLog[512];
        
        // vertex Shader
        GLuint object = glCreateShader(type);
        glShaderSource(object, 1, &shaderCode, NULL);
        glCompileShader(object);
        // print compile errors if any
        glGetShaderiv(object, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(object, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::" << GetTypeString(type) << "::COMPILATION_FAILED\n" << path << "\n" << infoLog << std::endl;
        };
        free(shaderCode);
        return object;
    }

    static GLuint GetShader(const char* path, std::span<const char*> pushDefines, GLenum type) {
        auto id = std::string(path) + "\n";
        std::string inject = "";
        for(auto push : pushDefines) {
            inject += push;
            inject += "\n";
        }
        id += inject;

        if(shaders.contains(id)) {
            return shaders[id];
        } else {
            GLuint shader = LoadShader(path, inject.c_str(), type);
            shaders[id] = shader;
            return shader;
        }
    }

    static GLuint MakeComputeProgram(GLuint compute, const char* computePath) {
        int success;
        char infoLog[512];

        GLuint program = glCreateProgram();        
        glAttachShader(program, compute);

        glLinkProgram(program);
        // print linking errors if any
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if(!success)
        {
            glGetProgramInfoLog(program, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" <<
                "Compute: " << computePath << std::endl <<
                infoLog << std::endl;
        }

        return program;
    }

    static GLuint MakeProgram(GLuint vertex, GLuint fragment, GLuint geometry, const char* vertexPath, const char* fragmentPath, const char* geometryPath) {
        int success;
        char infoLog[512];

        GLuint program = glCreateProgram();        
        glAttachShader(program, vertex);
        glAttachShader(program, fragment);
        if(geometry != 0) {
            glAttachShader(program, geometry);
        }

        glLinkProgram(program);
        // print linking errors if any
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if(!success)
        {
            glGetProgramInfoLog(program, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" <<
                "Vertex: " << vertexPath << "\n" <<
                "Fragment: " << fragmentPath << "\n";
            if(geometryPath != nullptr)
                std::cout << "Geometry: " << geometryPath << "\n";
            std::cout << infoLog << std::endl;
        }

        return program;
    }
public:
    static GLuint GetProgram(const char* vertexPath, const char* fragmentPath, const char* geometryPath, std::span<const char*> pushDefines = std::span<const char*>()) {
        ProgramId programId = ProgramId{
            GetShader(vertexPath, pushDefines, GL_VERTEX_SHADER),
            GetShader(fragmentPath, pushDefines, GL_FRAGMENT_SHADER),
            geometryPath != nullptr ? GetShader(geometryPath, pushDefines, GL_GEOMETRY_SHADER) : 0,
        };

        if(!programs.contains(programId)) {
            programs[programId] = MakeProgram(programId.vertex, programId.fragment, programId.geometry, vertexPath, fragmentPath, geometryPath);
        }
        return programs[programId];
    }

    static GLuint GetProgram(const char* vertexPath, const char* fragmentPath, std::span<const char*> pushDefines = std::span<const char*>()) {
        return GetProgram(vertexPath, fragmentPath, nullptr, pushDefines);
    }

    static GLuint GetComputeProgram(const char* computePath, std::span<const char*> pushDefines = std::span<const char*>()) {
        GLuint shader = GetShader(computePath, pushDefines, GL_COMPUTE_SHADER);

        if(!computePrograms.contains(shader)) {
            computePrograms[shader] = MakeComputeProgram(shader,computePath);
        }
        return computePrograms[shader];
    };
};