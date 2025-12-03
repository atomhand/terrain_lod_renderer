#include <iostream>
#include <utility>
#include "shader.h"


#include "application.h"
#include "assimp_wrapper.h"
#include "renderpasses.h"

#include "camera_controller.h"
#include "fly_camera.h"
#include "world.h"
#include "demo_world.h"

#include "duck.h"
#include "windmill.h"
#include "crane.h"
#include "terrain.h"

#include "material.h"

#include "sphere_grid.h"

using namespace std;

const char * title = "GPU Programming Coursework App";
Engine::Application app = Engine::Application(1024,768,title);
DemoWorld world;

int main()
{
	RenderPasses renderPasses;
	RenderItem* sphere;
	try
	{
		// Main camera controller
		world.scenegraph.SetParent(new CameraController(), world.scenegraph.root);
		world.scenegraph.SetParent(new FlyCamera(), world.scenegraph.root);

		// Sun
		Engine::DirectionalLight* sun = new Engine::DirectionalLight();
		sun->direction= glm::normalize(glm::vec3(-2.0,-2.,4.0));
		sun->color = glm::vec3(25.0);
		world.scenegraph.SetParent(sun,world.scenegraph.root);

		// Scene objects
		Engine::Shader pbr_shader = Engine::Shader("shaders/pbr.vert", "shaders/pbr.frag");
		world.scenegraph.SetParent(new Duck(pbr_shader), world.scenegraph.root);
		world.scenegraph.SetParent(new Windmill(pbr_shader), world.scenegraph.root);
		world.scenegraph.SetParent(new Crane(), world.scenegraph.root);
		world.scenegraph.SetParent(new Terrain(1024,0.5), world.scenegraph.root);
		world.scenegraph.SetParent(new PbrSphereGrid(8,8), world.scenegraph.root);

		sphere = new RenderItem();
		sphere->mesh = Sphere(32,32);
		auto spheremat = Engine::PbrMaterial(pbr_shader);
		spheremat.roughness = 0.03;
		sphere->material = std::make_shared<Engine::PbrMaterial>(spheremat);
		sphere->localTransform = glm::translate(glm::mat4(1.0), glm::vec3(4.0,4.0,4.0)) * glm::scale(glm::mat4(1.0), glm::vec3(2.0,2.0,2.0));
		world.scenegraph.SetParent(sphere, world.scenegraph.root);

		Engine::PointLight* light = new Engine::PointLight();
        light->color = glm::vec3(16.0,4.0,4.0);
        world.scenegraph.SetParent(light,sphere);
	}
	catch (exception &e)
	{
		cout << "Caught exception in scene setup: " << e.what() << endl;
		cin.ignore();
		exit(0);
	}

	renderPasses.Init(world);
	// event loop
	while(!app.shouldClose()) {
		app.frameStart(world);

		/*
		glm::vec3 dummy[8];
		auto c = world.cameraMain().FrustumCorners(dummy);
		sphere->localTransform = glm::translate(glm::mat4(1.),c);
		*/

		world.scenegraph.PropagateTransforms();
		world.scenegraph.NodeTickUpdate(world);

		renderPasses.RunAll(world,app);

		world.time = fmod(world.time + world.input.deltaTime * world.input.animSpeed, 1000.f);

		app.frameEnd(world);
	}
	
    return 0;
}