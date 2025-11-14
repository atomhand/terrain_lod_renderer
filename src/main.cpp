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

int main()
{
    const char * title = "GPU Programming Coursework App";
	Engine::Application app = Engine::Application(1024,768,title);

	try
	{
		Engine::Shader shader = Engine::Shader("shaders/basic.vert", "shaders/basic.frag");
		Mesh spheremesh = Mesh::Sphere(16,16);
		
		MaterialRenderGroup batch(shader, spheremesh);
		batch.transforms.push_back(glm::mat4x4(1.0));

		world.material_render_groups.push_back(batch);
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

		for(auto &transform : world.material_render_groups[0].transforms) {
			transform = glm::translate(transform, glm::vec3(0.01,0.0f,0.0f));
		}

		RenderPasses::preRender(world, app);
		RenderPasses::materialRenderPass(world,app);

		app.frameEnd(world);
	}
	
    return 0;
}