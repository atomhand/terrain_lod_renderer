#include "shader.h"
#include "asset_helper.h"
#include "stb_include.h"

#include <iostream>

Engine::Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
    data = std::make_shared<ShaderId>();

    // Process the relative paths into absolute paths
    std::string shadersDirectory = AssetHelper::assetPath("shaders/").string();
    std::string fullVertexPath = AssetHelper::assetPath(vertexPath).string();
    std::string fullFragmentPath = AssetHelper::assetPath(fragmentPath).string();

    // STB include 
    char error[256];
    char* vShaderCode = stb_include_file(fullVertexPath.c_str(), "", shadersDirectory.c_str(), error);
    if(!vShaderCode) {
        std::cout << "ERROR::SHADER::VERTEX::LOADING_FAILED\n" << error << std::endl;
    }
    char* fShaderCode = stb_include_file(fullFragmentPath.c_str(), "", shadersDirectory.c_str(), error);
    if(!fShaderCode) {
        std::cout << "ERROR::SHADER::VERTEX::LOADING_FAILED\n" << error << std::endl;
    }
    
    // Attempt to compile the shaders
    GLuint vertex, fragment;
    int success;
    char infoLog[512];
    
    // vertex Shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    // print compile errors if any
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << vertexPath << "\n" << infoLog << std::endl;
    };
    free(vShaderCode);

     // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    // print compile errors if any
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::SHADER::COMPILATION_FAILED\n" << fragmentPath << "\n" << infoLog << std::endl;
    };
    free(fShaderCode);

    GLuint id = programId();

    // shader Program
    glAttachShader(id, vertex);
    glAttachShader(id, fragment);
    glLinkProgram(id);
    // print linking errors if any
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(programId(), 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << vertexPath << "\n" << fragmentPath << "\n" << infoLog << std::endl;
    }
    
    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Engine::Shader::use()
{
    glUseProgram(programId());
}

void Engine::Shader::setBool(const std::string &name, bool value) const
{
    glUniform1i(glGetUniformLocation(programId(), name.c_str()), (int)value); 
}

void Engine::Shader::setInt(const std::string &name, int value) const
{
    glUniform1i(glGetUniformLocation(programId(), name.c_str()), value); 
}

void Engine::Shader::setFloat(const std::string &name, float value) const
{    
    glUniform1f(glGetUniformLocation(programId(), name.c_str()), value); 
}