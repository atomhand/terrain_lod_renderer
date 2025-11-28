#include <iostream>
#include <utility>
#include "shader.h"


#include "application.h"
#include "shapes.h"
#include "rts_camera.h"
#include "world.h"
#include "demo_world.h"

#include "renderpasses.h"

#include "assimp_wrapper.h"

using namespace std;

float wind_angle = 0.0;
const char * title = "GPU Programming Coursework App";
Engine::Application app = Engine::Application(1024,768,title);
DemoWorld world;

int main()
{

	try
	{
		Engine::Shader shader = Engine::Shader("shaders/basic.vert", "shaders/basic.frag");
		Engine::Shader transparent_shader = Engine::Shader("shaders/normalmap.vert", "shaders/normalmap.frag");

		world.meshes.push_back(Cube());
		world.meshes.push_back(Sphere(8,8));
		auto crane_meshes = Engine::AssimpWrapper::ImportMesh("models/Anim_RedCrownedCraneFlap1Forward.FBX");
		world.meshes.push_back(crane_meshes[0]);
		world.meshes.push_back(crane_meshes[1]);

		MaterialRenderGroup windmill = MaterialRenderGroup(shader,world.meshes[0]);
		MaterialRenderGroup duck = MaterialRenderGroup(shader,world.meshes[1]);
		duck.shininess = 5.0;


		MaterialRenderGroup crane = MaterialRenderGroup(transparent_shader,world.meshes[2]);
		crane.textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneBodyB.TGA"));
		crane.textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneBodyM.TGA"));
		crane.textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneBodyN.TGA"));
		//crane.transparent = true;

		MaterialRenderGroup crane_feathers = MaterialRenderGroup(transparent_shader,world.meshes[3]);
		crane_feathers.textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneFeatherB.TGA"));
		crane_feathers.textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneFeatherM.TGA"));
		crane_feathers.textures.push_back(Engine::Texture::Import("textures/Tex_BlackCrownedCraneFeatherN.TGA"));
		crane_feathers.transparent = true;

		world.material_render_groups.push_back(windmill);
		world.material_render_groups.push_back(duck);
		world.material_render_groups.push_back(crane);
		world.material_render_groups.push_back(crane_feathers);
	}
	catch (exception &e)
	{
		cout << "Caught exception: " << e.what() << endl;
		cin.ignore();
		exit(0);
	}

	// event loop
	while(!app.shouldClose()) {
		app.frameStart(world);
		world.camera.update(world);

		wind_angle += world.input.deltaTime * world.input.animSpeed;
		world.windmill.update(wind_angle,world.input.deltaTime * world.input.animSpeed * 0.5f);
		world.duck.update(world.input.deltaTime * world.input.animSpeed * 0.5f);

		glm::vec4 duckPos = world.duck.getTransforms()[0] * glm::vec4(0.0,0.0,0.0,1.0);
		world.lightPos = glm::vec4(duckPos.x,duckPos.y,duckPos.z,world.lightPos.w);

		world.material_render_groups[0].transforms = world.windmill.getTransforms();
		world.material_render_groups[0].colours = world.windmill.getColours();

		world.material_render_groups[1].transforms = world.duck.getTransforms();
		world.material_render_groups[1].colours = world.duck.getColours();

		glm::mat4 crane_transform =   glm::translate(glm::mat4(1.0f), glm::vec3(10.0,0.0,0.0)) * glm::rotate(glm::mat4(1.0f), -1.57f, glm::vec3(1.f,0.f,0.f)) *  glm::scale(glm::mat4(1.0f), glm::vec3(0.05,0.05,0.05));

		world.material_render_groups[2].colours = std::vector{glm::vec4(1.0,1.0,1.0,1.0)};
		world.material_render_groups[2].transforms = std::vector{crane_transform};
		
		world.material_render_groups[3].colours = std::vector{glm::vec4(1.0,1.0,1.0,1.0)};
		world.material_render_groups[3].transforms = std::vector{crane_transform};

		RenderPasses::preRender(world, app);
		RenderPasses::materialRenderPass(world,app);

		app.frameEnd(world);
	}
	
    return 0;
}