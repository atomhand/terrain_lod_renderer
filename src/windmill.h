#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "demo_world.h"
#include "material.h"
#include "render_item.h"
#include "shapes.h"

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

    glm::mat4 mainRotation() {
        return glm::rotate(glm::mat4(1.0), current_angle, glm::vec3(0.0,1.0,0.0));
    }

    glm::mat4 bladesRotation() {
        return glm::rotate(glm::mat4(1.0), blades_angle, glm::vec3(1.0,0.0,0.0));
    }

    Engine::SceneNode* bodyContainer;
    Engine::SceneNode* bladesContainer;
public:

    void update(float wind_angle, float deltaTime) {
        if(wind_angle > current_angle) {
            current_angle -= deltaTime;
        } else {
            current_angle += deltaTime;
        }

        blades_angle += deltaTime;

        bodyContainer->localTransform = glm::translate(glm::mat4(1.0), glm::vec3(0.0,BASE_HEIGHT/2.0+HEIGHT/2.0,0.0)) * mainRotation();
        
        glm::vec3 blade_offset = glm::vec3(WIDTH/2.0+BLADES_THICKNESS/2.0,HEIGHT/2.0,0.0);
        bladesContainer->localTransform = glm::translate(glm::mat4(1.0),blade_offset) *  glm::rotate(glm::mat4(1.0), blades_angle, glm::vec3(1.0,0.0,0.0));
    }

    void Setup(DemoWorld& world, Engine::Shader shader) {
        Engine::SceneNode* container = new Engine::SceneNode;
        //container->localTransform = glm::translate(glm::mat4(1.0), glm::vec3(0.0,0.0,10.0));
        world.scenegraph.SetParent(container, world.scenegraph.root);

        Engine::Mesh cube = Cube();

        std::shared_ptr<Engine::PhongMaterial> baseMaterial = std::make_shared<Engine::PhongMaterial>(shader);
        baseMaterial->color = glm::vec4(0.1,0.1,0.1,1.0);

        std::shared_ptr<Engine::PhongMaterial> bodyMaterial = std::make_shared<Engine::PhongMaterial>(shader);
        bodyMaterial->color = glm::vec4(0.7,0.2,0.0,1.0);
        
        std::shared_ptr<Engine::PhongMaterial> bladeMaterial = std::make_shared<Engine::PhongMaterial>(shader);
        bladeMaterial->color = glm::vec4(0.7,0.7,0.7,1.0);
        //
        RenderItem* base1 = new RenderItem();
        base1->material = baseMaterial;
        base1->mesh = cube;
        base1->localTransform = glm::scale(glm::mat4(1.0), glm::vec3(BASE_WIDTH, BASE_HEIGHT, BASE_HEIGHT));
        world.scenegraph.SetParent(base1,container);

        RenderItem* base2 = new RenderItem();
        base2->material = baseMaterial;
        base2->mesh = cube;
        base2->localTransform = glm::scale(glm::mat4(1.0), glm::vec3(BASE_HEIGHT, BASE_HEIGHT, BASE_WIDTH));
        world.scenegraph.SetParent(base2,container);

        bodyContainer = new Engine::SceneNode();        
        world.scenegraph.SetParent(bodyContainer,container);

        RenderItem* body = new RenderItem();
        body->material = bodyMaterial;
        body->mesh = cube;
        body->localTransform = glm::scale(glm::mat4(1.0), glm::vec3(WIDTH, HEIGHT, WIDTH));
        world.scenegraph.SetParent(body,bodyContainer);

        bladesContainer = new Engine::SceneNode();             
        world.scenegraph.SetParent(bladesContainer,bodyContainer);

        RenderItem* blade1 = new RenderItem();
        blade1->material = bladeMaterial;
        blade1->mesh = cube;
        blade1->localTransform = glm::scale(glm::mat4(1.0), glm::vec3(BLADES_THICKNESS, BLADES_WIDTH, BLADES_LENGTH));
        world.scenegraph.SetParent(blade1,bladesContainer);

        RenderItem* blade2 = new RenderItem();
        blade2->material = bladeMaterial;
        blade2->mesh = cube;
        blade2->localTransform = glm::scale(glm::mat4(1.0), glm::vec3(BLADES_THICKNESS, BLADES_LENGTH, BLADES_WIDTH));
        world.scenegraph.SetParent(blade2,bladesContainer);
    }
};