#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "demo_world.h"
#include "mesh.h"
#include "material_render_group.h"
#include "shapes.h"

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

    Engine::SceneNode* duck;
public:
    glm::mat4 main_transform() {
        return duck->globalTransform;
    }

    void update(float deltaTime) {
        current_angle += deltaTime;
        duck->localTransform = glm::translate(glm::mat4(1.0), glm::vec3(CIRCLE_OFFSET,CIRCLE_ELEVATION,0.0)) * mainRotation() * glm::translate(glm::mat4(1.0), glm::vec3(0.0,0.0,CIRCLE_RADIUS));
    }

    void Setup(DemoWorld& world, Engine::Shader shader) {
        // root
        duck = new Engine::SceneNode();
        world.scenegraph.SetParent(duck, world.scenegraph.root);

        Engine::Mesh mesh = Sphere(8,8);

        // body
        RenderItem* body = new RenderItem();
        body->mesh = mesh;
        body->shininess = 5.0;
        body->shader = shader;
        body->localTransform = glm::scale(glm::mat4(1.0), glm::vec3(LENGTH, HEIGHT, WIDTH));
        body->colour = glm::vec4(0.4,0.2,0.0,1.0);
        world.scenegraph.SetParent(body, duck);

        // head
        RenderItem* head = new RenderItem();
        head->mesh = mesh;
        body->shininess = 5.0;
        head->shader = shader;
        head->localTransform = glm::translate(glm::mat4(1.0), glm::vec3(LENGTH/2.0,HEIGHT/2.0+HEAD_SIZE,0.0)) * glm::scale(glm::mat4(1.0), glm::vec3(HEAD_SIZE));
        head->colour = glm::vec4(0.2,0.4,0.2,1.0);
        world.scenegraph.SetParent(head, duck);
    }
};