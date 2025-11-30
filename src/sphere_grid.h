#pragma once

#include "world.h"
#include "render_item.h"
#include "shapes.h"

class PbrSphereGrid : public Engine::SceneNode {
    int rows, columns;
public:
    void OnEnter(Engine::SceneGraph& sceneGraph) override {
        this->localTransform = glm::translate(glm::mat4(1.), glm::vec3(0.,16.,0.));

        Engine::Shader pbr = Engine::Shader("shaders/pbr.vert","shaders/pbr.frag");
        Engine::Mesh mesh = Sphere(32,32);

        float xOffset = float(columns-1)*1.5f;
        float yOffset = float(rows-1)*1.5f;

        for(int x=0; x<columns; x++) {
            for(int y=0; y<rows; y++) {
                auto mat = Engine::PbrMaterial(pbr);
                mat.albedo = glm::vec3(0.5,0.0,0.0);
                mat.metallic = y / 7.f;
                mat.roughness = x / 7.f + 0.03;

                RenderItem* item = new RenderItem();
                item->material = std::make_shared<Engine::PbrMaterial>(mat);
                item->mesh = mesh;
                item->localTransform = glm::translate(glm::mat4(1.),glm::vec3(x*3.f-xOffset,y*3.f-yOffset, 0.f));

                sceneGraph.SetParent(item,this);
            }
        }
    }

    PbrSphereGrid(int rows, int columns) : rows(rows), columns(columns) {}
};