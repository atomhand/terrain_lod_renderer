#pragma once
#include <glm/glm.hpp>

struct  Input {
public:
    glm::vec2 mousePos;
    float scrollDelta;

    float yAxisKeyDelta;
    float xAxisKeyDelta;

    float animSpeed = 1.0;

    float deltaTime;
};

class World {
public:
    Input input;
};