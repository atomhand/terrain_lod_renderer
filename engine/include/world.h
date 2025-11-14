#pragma once
#include <glm/glm.hpp>

struct  Input {
public:
    glm::vec2 mousePos;
    float scrollDelta;
};

class World {
public:
    Input input;
};