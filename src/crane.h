#pragma once
#include <memory>
#include "world.h"
#include "assimp_wrapper.h"
#include "material.h"

class Crane : public Engine::SceneNode {
public:
	void OnEnter(Engine::SceneGraph& sceneGraph) override {
		Engine::Shader shader = Engine::Shader("shaders/textured_pbr.vert", "shaders/textured_pbr.frag");
		auto crane_meshes = Engine::AssimpWrapper::ImportMesh("models/Anim_RedCrownedCraneFlap1Forward.FBX");

		// Choose the transform scale to match a realistic crane wingspan (assuming units are metres)
		float width = crane_meshes[0].aabb.max.x - crane_meshes[0].aabb.min.x;
		float scale = 2.4 / width;

		SceneNode* container = new SceneNode();
		container->localTransform = glm::rotate(glm::mat4(1.0f), glm::radians(180.f), glm::vec3(0.f,1.f,0.f)) * glm::rotate(glm::mat4(1.0f), glm::radians(-90.f), glm::vec3(1.f,0.f,0.f)) *  glm::scale(glm::mat4(1.0f), glm::vec3(scale));
		sceneGraph.SetParent(container,this);

		Engine::PbrMaterial craneMaterial(shader);
		craneMaterial.textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneBodyB.TGA"));
		craneMaterial.textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneBodyM.TGA"));
		craneMaterial.textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneBodyN.TGA"));

		RenderItem* body = new RenderItem();
		body->material = std::make_shared<Engine::PbrMaterial>(craneMaterial);
		body->mesh = crane_meshes[0];
		sceneGraph.SetParent(body, container);
		//crane.transparent = true;

		Engine::PbrMaterial feathersMaterial(shader);
		feathersMaterial.textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneFeatherB.TGA"));
		feathersMaterial.textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneFeatherM.TGA"));
		feathersMaterial.textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneFeatherN.TGA"));

		RenderItem* feathers = new RenderItem();
		feathers->material = std::make_shared<Engine::PbrMaterial>(feathersMaterial);
		feathers->mesh = crane_meshes[1];
		feathers->renderPass = RenderPass::TRANSPARENT;
		sceneGraph.SetParent(feathers, container);
	}
};