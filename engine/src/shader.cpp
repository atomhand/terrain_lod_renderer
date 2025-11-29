#include "shader.h"
#include "asset_helper.h"

#include <iostream>

Engine::Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
    data = std::make_shared<ShaderId>();

    std::string vertexCode = AssetHelper::readFile(vertexPath);
    std::string fragmentCode = AssetHelper::readFile(fragmentPath);
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    
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
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    };

     // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    // print compile errors if any
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    };

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
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
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