#include <iostream>
#include "shader.h"

#include "demo_world.h"

#include "application.h"
#include "mesh.h"
#include "rts_camera.h"
#include "world.h"

#include "renderpasses.h"

using namespace std;

DemoWorld world;

float wind_angle = 0.0;

int main()
{
    const char * title = "GPU Programming Coursework App";
	Engine::Application app = Engine::Application(1024,768,title);

	try
	{
		Engine::Shader shader = Engine::Shader("shaders/basic.vert", "shaders/basic.frag");
		Mesh cubemesh = Mesh::Cube();
		Mesh spheremesh = Mesh::Sphere(8,8);

		MaterialRenderGroup windmill = MaterialRenderGroup(shader,cubemesh);
		MaterialRenderGroup duck = MaterialRenderGroup(shader,spheremesh);
		duck.shininess = 5.0;

		world.material_render_groups.push_back(windmill);
		world.material_render_groups.push_back(duck);
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

		world.material_render_groups[0].transforms = world.windmill.getTransforms();
		world.material_render_groups[0].colours = world.windmill.getColours();

		world.material_render_groups[1].transforms = world.duck.getTransforms();
		world.material_render_groups[1].colours = world.duck.getColours();

		RenderPasses::preRender(world, app);
		RenderPasses::materialRenderPass(world,app);

		app.frameEnd(world);
	}
	
    return 0;
}