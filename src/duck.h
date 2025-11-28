#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

class Duck {
private:
    const float HEIGHT = 0.3f;
    const float WIDTH = 0.3f;
    const float LENGTH = 0.6f;
    const float HEAD_SIZE = 0.2f;

    const float CIRCLE_RADIUS = 6.0;
    const float CIRCLE_OFFSET = 10.0;
    const float CIRCLE_ELEVATION = 4.0;

    float current_angle = 0.7f;

    glm::mat4 mainRotation() {
        return glm::rotate(glm::mat4(1.0), current_angle, glm::vec3(0.0,1.0,0.0));
    }
public:

    void update(float deltaTime) {
            current_angle += deltaTime;
    }

    Duck() {
    }

    std::vector<glm::mat4> getTransforms() {
        std::vector<glm::mat4> transforms;

        glm::mat4 body_transform = glm::translate(glm::mat4(1.0), glm::vec3(CIRCLE_OFFSET,CIRCLE_ELEVATION,0.0)) * mainRotation() * glm::translate(glm::mat4(1.0), glm::vec3(0.0,0.0,CIRCLE_RADIUS));

        // body
        transforms.push_back(            
            body_transform * glm::scale(glm::mat4(1.0), glm::vec3(LENGTH, HEIGHT, WIDTH))
        );
        // head
        transforms.push_back(            
            body_transform * glm::translate(glm::mat4(1.0), glm::vec3(LENGTH/2.0,HEIGHT/2.0+HEAD_SIZE,0.0)) * glm::scale(glm::mat4(1.0), glm::vec3(HEAD_SIZE))
        );

        return transforms;
    }

    std::vector<glm::vec4> getColours() {
        std::vector<glm::vec4> colours;
        // body
        colours.push_back(glm::vec4(0.4,0.2,0.0,1.0));
        // head
        colours.push_back(glm::vec4(0.2,0.4,0.2,1.0));

        return colours;
    }
};