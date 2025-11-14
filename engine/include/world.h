#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>

struct  Input {
public:
    glm::vec2 mousePos;
    float scrollDelta;
};

class World {
public:
    entt::registry registry;
    Input input;
};