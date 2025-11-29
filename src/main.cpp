#include <iostream>
#include <utility>
#include "shader.h"


#include "application.h"
#include "assimp_wrapper.h"
#include "renderpasses.h"

#include "rts_camera.h"
#include "world.h"
#include "demo_world.h"

#include "duck.h"
#include "windmill.h"
#include "crane.h"

#include "material.h"


using namespace std;

float wind_angle = 0.0;
const char * title = "GPU Programming Coursework App";
Engine::Application app = Engine::Application(1024,768,title);
DemoWorld world;


int main()
{
	Duck duck;
	Windmill windmill;
	Crane crane;

	try
	{
		Engine::Shader transparent_shader = Engine::Shader("shaders/normalmap.vert", "shaders/normalmap.frag");
		crane.Setup(world,transparent_shader);

		Engine::Shader basic_shader = Engine::Shader("shaders/basic.vert", "shaders/basic.frag");
		duck.Setup(world,basic_shader);
		windmill.Setup(world,basic_shader);
		
		Engine::Shader pbr_shader = Engine::Shader("shaders/pbr.vert", "shaders/pbr.frag");
		auto pbr_material = Engine::PbrMaterial(pbr_shader);

		RenderItem* sphere = new RenderItem();
		sphere->mesh = Sphere(32,32);
		sphere->material = std::make_shared<Engine::PbrMaterial>(pbr_material);
		sphere->localTransform = glm::translate(glm::mat4(1.0), glm::vec3(4.0,4.0,4.0)) * glm::scale(glm::mat4(1.0), glm::vec3(2.0,2.0,2.0));
		world.scenegraph.SetParent(sphere, world.scenegraph.root);
	}
	catch (exception &e)
	{
		cout << "Caught exception in scene setup: " << e.what() << endl;
		cin.ignore();
		exit(0);
	}

	// event loop
	while(!app.shouldClose()) {
		app.frameStart(world);
		world.camera.update(world);

		wind_angle += world.input.deltaTime * world.input.animSpeed;
		windmill.update(wind_angle,world.input.deltaTime * world.input.animSpeed * 0.5f);
		duck.update(world.input.deltaTime * world.input.animSpeed * 0.5f);

		world.scenegraph.PropagateTransforms();

		RenderPasses::preRender(world, app);
		RenderPasses::opaqueRenderPass(world,app);
		RenderPasses::transparentRenderPass(world,app);

		app.frameEnd(world);
	}
	
    return 0;
}