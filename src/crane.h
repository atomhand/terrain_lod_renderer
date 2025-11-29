#pragma once
#include "demo_world.h"
#include "assimp_wrapper.h"

class Crane {
public:
    void Setup(DemoWorld& world, Engine::Shader shader) {
		auto crane_meshes = Engine::AssimpWrapper::ImportMesh("models/Anim_RedCrownedCraneFlap1Forward.FBX");

		RenderItem* crane = new RenderItem();
		crane->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(10.0,0.0,0.0)) * glm::rotate(glm::mat4(1.0f), -1.57f, glm::vec3(1.f,0.f,0.f)) *  glm::scale(glm::mat4(1.0f), glm::vec3(0.05,0.05,0.05));
		crane->shader = shader;
		crane->mesh = crane_meshes[0];
		crane->textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneBodyB.TGA"));
		crane->textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneBodyM.TGA"));
		crane->textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneBodyN.TGA"));
		world.scenegraph.SetParent(crane, world.scenegraph.root);
		//crane.transparent = true;

		RenderItem* crane_feathers = new RenderItem();
		crane_feathers->shader = shader;
		crane_feathers->mesh = crane_meshes[1];
		crane_feathers->textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneFeatherB.TGA"));
		crane_feathers->textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneFeatherM.TGA"));
		crane_feathers->textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneFeatherN.TGA"));
		crane_feathers->transparent = true;
		world.scenegraph.SetParent(crane_feathers, world.scenegraph.root);
    }
};