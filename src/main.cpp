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
#include "terrain.h"

#include "material.h"


using namespace std;

float wind_angle = 0.0;
const char * title = "GPU Programming Coursework App";
Engine::Application app = Engine::Application(1024,768,title);
DemoWorld world;

RenderItem* testQuad;
RenderItem* sphere;

int main()
{

	try
	{
		// Main camera controller
		world.scenegraph.SetParent(new RtsCameraController(), world.scenegraph.root);

		// Sun
		Engine::DirectionalLight* sun = new Engine::DirectionalLight();
		sun->direction= glm::normalize(glm::vec3(0.0,-1.,-8.0));
		sun->color = glm::vec3(10.0);
		world.scenegraph.SetParent(sun,world.scenegraph.root);

		// Fullscreen quad used for debug visualisation
		auto quad_mat = Engine::Material(Engine::Shader("shaders/fullscreen_quad.vert","shaders/fullscreen_quad.frag"));
		quad_mat.textures.push_back(sun->depthMap());
		testQuad = new RenderItem();
		testQuad->mesh = BasicQuad();
		testQuad->shadowEnabled = false;
		testQuad->enabled = false;
		testQuad->material = std::make_shared<Engine::Material>(quad_mat);
		world.scenegraph.SetParent(testQuad,world.scenegraph.root);

		// Scene objects
		Engine::Shader pbr_shader = Engine::Shader("shaders/pbr.vert", "shaders/pbr.frag");
		world.scenegraph.SetParent(new Duck(pbr_shader), world.scenegraph.root);
		world.scenegraph.SetParent(new Windmill(pbr_shader), world.scenegraph.root);
		world.scenegraph.SetParent(new Crane(), world.scenegraph.root);
		world.scenegraph.SetParent(new Terrain(256,0.5f), world.scenegraph.root);

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

	// event loop
	while(!app.shouldClose()) {
		app.frameStart(world);
		testQuad->enabled = world.input.testQuad;

		/*
		glm::vec3 dummy[8];
		auto c = world.cameraMain().FrustumCorners(dummy);
		sphere->localTransform = glm::translate(glm::mat4(1.),c);
		*/

		world.scenegraph.PropagateTransforms();
		world.scenegraph.NodeTickUpdate(world);

		RenderPasses::DrawShadowMaps(world,app);

		RenderPasses::PrepareMain(world, app);
		RenderPasses::DrawOpaque(world,app);
		RenderPasses::DrawTransparent(world,app);

		world.time = fmod(world.time + world.input.deltaTime * world.input.animSpeed, 1000.f);

		app.frameEnd(world);
	}
	
    return 0;
}