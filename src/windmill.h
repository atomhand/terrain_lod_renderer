#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

class Windmill {
private:
    const float HEIGHT = 4.5;
    const float BASE_HEIGHT = 0.5;
    const float BASE_WIDTH = 2.5;
    const float WIDTH = 2.0;

    
    const float BLADES_LENGTH = 4.0;
    const float BLADES_WIDTH = 0.5;
    const float BLADES_THICKNESS = 0.125f;

    float current_angle = 0.7f;
    float blades_angle = 1.5;

    glm::mat4 root_transform;

    glm::mat4 mainRotation() {
        return glm::rotate(glm::mat4(1.0), current_angle, glm::vec3(0.0,1.0,0.0));
    }

    glm::mat4 bladesRotation() {
        return glm::rotate(glm::mat4(1.0), blades_angle, glm::vec3(1.0,0.0,0.0));
    }
public:

    void update(float wind_angle, float deltaTime) {
        if(wind_angle > current_angle) {
            current_angle -= deltaTime;
        } else {
            current_angle += deltaTime;
        }

        blades_angle += deltaTime;
    }

    Windmill() {
        root_transform = glm::mat4(1.0);
    }

    std::vector<glm::mat4> getTransforms() {
        std::vector<glm::mat4> transforms;

        // base
        transforms.push_back(
            glm::scale(glm::mat4(1.0), glm::vec3(BASE_WIDTH, BASE_HEIGHT, BASE_HEIGHT))
        );
        transforms.push_back(
            glm::scale(glm::mat4(1.0), glm::vec3(BASE_HEIGHT, BASE_HEIGHT, BASE_WIDTH))
        );

        glm::mat4 body_transform = glm::translate(glm::mat4(1.0), glm::vec3(0.0,BASE_HEIGHT/2.0+HEIGHT/2.0,0.0)) * mainRotation();

        // body
        transforms.push_back(            
            body_transform * glm::scale(glm::mat4(1.0), glm::vec3(WIDTH, HEIGHT, WIDTH))
        );
        
        glm::vec3 blade_offset = glm::vec3(WIDTH/2.0+BLADES_THICKNESS/2.0,HEIGHT/2.0,0.0);

        transforms.push_back(
            body_transform * glm::translate(glm::mat4(1.0),blade_offset) *  glm::rotate(glm::mat4(1.0), blades_angle, glm::vec3(1.0,0.0,0.0)) * glm::scale(glm::mat4(1.0), glm::vec3(BLADES_THICKNESS, BLADES_WIDTH, BLADES_LENGTH))
        );
         transforms.push_back(
            body_transform * glm::translate(glm::mat4(1.0),blade_offset) * glm::rotate(glm::mat4(1.0), blades_angle, glm::vec3(1.0,0.0,0.0)) * glm::scale(glm::mat4(1.0), glm::vec3(BLADES_THICKNESS, BLADES_LENGTH, BLADES_WIDTH))
        );

        return transforms;
    }

    std::vector<glm::vec4> getColours() {
        std::vector<glm::vec4> colours;
        // base
        colours.push_back(glm::vec4(0.1,0.1,0.1,1.0));
        colours.push_back(glm::vec4(0.1,0.1,0.1,1.0));

        // body
        colours.push_back(glm::vec4(0.7,0.2,0.0,1.0));

        // blades
        colours.push_back(glm::vec4(0.7,0.7,0.7,1.0));
        colours.push_back(glm::vec4(0.7,0.7,0.7,1.0));

        return colours;
    }
};