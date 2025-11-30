#pragma once
#include "world.h"
#include "rts_camera.h"
#include "shader.h"

class DemoWorld : public Engine::World {
public:
    Engine::Shader shadowShader;

    DemoWorld() : shadowShader(Engine::Shader("shaders/shadow.vert","shaders/shadow.frag")) {
    }
};