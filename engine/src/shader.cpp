#include "shader.h"
#include "asset_helper.h"
#include "stb_include.h"

#include <iostream>

void CreateShader(GLenum type, const char* path, GLuint& object) {
    std::string shadersDirectory = Engine::AssetHelper::assetPath("shaders/").string();
    std::string fullPath = Engine::AssetHelper::assetPath(path).string();

    // STB include 
    char error[256];
    char* shaderCode = stb_include_file(fullPath.c_str(), "", shadersDirectory.c_str(), error);
    if(!shaderCode) {
        std::cout << "ERROR::SHADER::?::LOADING_FAILED\n" << error << std::endl;
    }    

    int success;
    char infoLog[512];
    
    // vertex Shader
    object = glCreateShader(type);
    glShaderSource(object, 1, &shaderCode, NULL);
    glCompileShader(object);
    // print compile errors if any
    glGetShaderiv(object, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(object, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << path << "\n" << infoLog << std::endl;
    };
    free(shaderCode);
}

Engine::Shader::Shader(const char* vertexPath, const char* fragmentPath, const char* tessControlPath, const char* tessEvalPath)
{
    data = std::make_shared<ShaderId>();

    GLuint vertex, fragment;
    CreateShader(GL_VERTEX_SHADER, vertexPath, vertex);
    CreateShader(GL_FRAGMENT_SHADER, fragmentPath, fragment);

    GLuint tessControl, tessEval;
    if(tessControlPath != nullptr) {
        assert(tessEvalPath != nullptr);
        CreateShader(GL_TESS_CONTROL_SHADER, tessControlPath,tessControl);
        CreateShader(GL_TESS_EVALUATION_SHADER, tessEvalPath,tessEval);
    }

    GLuint id = programId();

    int success;
    char infoLog[512];

    // shader Program
    glAttachShader(id, vertex);
    glAttachShader(id, fragment);
    if(tessControlPath != nullptr) {
        glAttachShader(id, tessControl);
        glAttachShader(id, tessEval);
    }


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