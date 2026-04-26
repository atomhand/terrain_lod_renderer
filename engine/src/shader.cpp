#include "shader.h"

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