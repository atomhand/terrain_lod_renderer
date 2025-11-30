#pragma once
#include "world.h"
#include "rts_camera.h"
#include "shader.h"
#include "environment_map.h"

class DemoWorld : public Engine::World {
public:
    Engine::Shader shadowShader;
    Engine::EnvironmentMap skybox = Engine::EnvironmentMap("skybox/citrus_orchard_road_puresky_4k.hdr");

    DemoWorld() : shadowShader(Engine::Shader("shaders/shadow.vert","shaders/shadow.frag")) {
    }
};