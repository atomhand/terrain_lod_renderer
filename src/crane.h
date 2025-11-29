#pragma once
#include <memory>
#include "demo_world.h"
#include "assimp_wrapper.h"
#include "material.h"

class Crane {
public:
    void Setup(DemoWorld& world) {
		Engine::Shader shader = Engine::Shader("shaders/textured_pbr.vert", "shaders/textured_pbr.frag");

		auto crane_meshes = Engine::AssimpWrapper::ImportMesh("models/Anim_RedCrownedCraneFlap1Forward.FBX");

		Engine::PbrMaterial craneMaterial(shader);
		craneMaterial.textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneBodyB.TGA"));
		craneMaterial.textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneBodyM.TGA"));
		craneMaterial.textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneBodyN.TGA"));

		RenderItem* crane = new RenderItem();
		crane->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(10.0,0.0,0.0)) * glm::rotate(glm::mat4(1.0f), -1.57f, glm::vec3(1.f,0.f,0.f)) *  glm::scale(glm::mat4(1.0f), glm::vec3(0.05,0.05,0.05));
		crane->material = std::make_shared<Engine::PbrMaterial>(craneMaterial);
		crane->mesh = crane_meshes[0];
		world.scenegraph.SetParent(crane, world.scenegraph.root);
		//crane.transparent = true;

		Engine::PbrMaterial feathersMaterial(shader);
		feathersMaterial.textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneFeatherB.TGA"));
		feathersMaterial.textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneFeatherM.TGA"));
		feathersMaterial.textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneFeatherN.TGA"));

		RenderItem* crane_feathers = new RenderItem();
		crane_feathers->material = std::make_shared<Engine::PbrMaterial>(feathersMaterial);
		crane_feathers->mesh = crane_meshes[1];
		crane_feathers->renderPass = RenderPass::TRANSPARENT;
		world.scenegraph.SetParent(crane_feathers, crane);
    }
};