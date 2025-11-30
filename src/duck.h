#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "demo_world.h"
#include "mesh.h"
#include "render_item.h"
#include "shapes.h"
#include "light.h"

class Duck : public Engine::SceneNode {
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
    Engine::Shader shader;
public:
    Duck(Engine::Shader shader) : shader(shader) {};

    void Update(Engine::World& world) override {
        current_angle += world.animDeltaTime();
        localTransform = glm::translate(glm::mat4(1.0), glm::vec3(CIRCLE_OFFSET,CIRCLE_ELEVATION,0.0)) * mainRotation() * glm::translate(glm::mat4(1.0), glm::vec3(0.0,0.0,CIRCLE_RADIUS));
    }

    void OnEnter(Engine::SceneGraph& sceneGraph) override {
        Engine::Mesh mesh = Sphere(8,8);

        Engine::PbrMaterial bodyMaterial(shader);
        bodyMaterial.albedo = glm::vec3(0.4,0.2,0.0);
        bodyMaterial.roughness = 0.1;

        Engine::PbrMaterial headMaterial(shader);
        headMaterial.albedo = glm::vec3(0.2,0.4,0.2);
        headMaterial.roughness = 0.1;

        // body
        RenderItem* body = new RenderItem();
        body->mesh = mesh;
        body->material = std::make_shared<Engine::PbrMaterial>(bodyMaterial);
        body->localTransform = glm::scale(glm::mat4(1.0), glm::vec3(LENGTH, HEIGHT, WIDTH));
        sceneGraph.SetParent(body, this);

        // head
        RenderItem* head = new RenderItem();
        head->mesh = mesh;
        head->material = std::make_shared<Engine::PbrMaterial>(headMaterial);
        head->localTransform = glm::translate(glm::mat4(1.0), glm::vec3(LENGTH/2.0,HEIGHT/2.0+HEAD_SIZE,0.0)) * glm::scale(glm::mat4(1.0), glm::vec3(HEAD_SIZE));
        sceneGraph.SetParent(head, this);

        Engine::PointLight* light = new Engine::PointLight();
        light->color = glm::vec3(1.0,1.0,1.0);
        light->localTransform = glm::translate(glm::mat4(1.0f),glm::vec3(0.0,2.0,0.0)) * glm::mat4(1.f);
        sceneGraph.SetParent(light,head);
    }
};