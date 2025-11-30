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
RenderItem* frustSpheres[8];
Engine::SceneNode* testCube;

int main()
{
	Engine::DirectionalLight* sun = new Engine::DirectionalLight();
	sun->direction= glm::normalize(glm::vec3(4.0,-2.0,4.0));
	sun->color = glm::vec3(10.0);
	world.scenegraph.SetParent(sun,world.scenegraph.root);

	auto quad_mat = Engine::Material(Engine::Shader("shaders/fullscreen_quad.vert","shaders/fullscreen_quad.frag"));
	quad_mat.textures.push_back(sun->depthMap());
	testQuad = new RenderItem();
	testQuad->mesh = BasicQuad();
	testQuad->shadowEnabled = false;
	testQuad->enabled = false;
	testQuad->material = std::make_shared<Engine::Material>(quad_mat);
	world.scenegraph.SetParent(testQuad,world.scenegraph.root);

	Duck duck;
	Windmill windmill;
	Crane crane;
	Terrain terrain(256,0.5f);

	try
	{
		crane.Setup(world);
		//terrain.Setup(world);

		Engine::Shader pbr_shader = Engine::Shader("shaders/pbr.vert", "shaders/pbr.frag");
		auto pbr_material = Engine::PbrMaterial(pbr_shader);
		windmill.Setup(world,pbr_shader);
		duck.Setup(world,pbr_shader);

		auto spheremesh = Sphere(32,32);
		auto pbr_mat = std::make_shared<Engine::PbrMaterial>(pbr_material);

		sphere = new RenderItem();
		sphere->mesh = spheremesh;
		sphere->material = pbr_mat;
		sphere->localTransform = glm::translate(glm::mat4(1.0), glm::vec3(4.0,4.0,4.0)) * glm::scale(glm::mat4(1.0), glm::vec3(2.0,2.0,2.0));
		world.scenegraph.SetParent(sphere, world.scenegraph.root);

		testCube = new Engine::SceneNode();
		world.scenegraph.SetParent(testCube, world.scenegraph.root);

		auto childCube = new RenderItem();
		childCube->mesh = Cube();
		childCube->material = pbr_mat;
		//childCube->localTransform = glm::scale(glm::mat4(1.0), glm::vec3(0.5));
		world.scenegraph.SetParent(childCube,testCube);

		for(int i =0; i<8; i++) {
			frustSpheres[i] = new RenderItem();
			frustSpheres[i]->mesh = spheremesh;
			frustSpheres[i]->material = pbr_mat;
			world.scenegraph.SetParent(frustSpheres[i], world.scenegraph.root);
		}

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

		world.camera.update(world);

		wind_angle += world.input.deltaTime * world.input.animSpeed;
		windmill.update(wind_angle,world.input.deltaTime * world.input.animSpeed * 0.2f);
		duck.update(world.input.deltaTime * world.input.animSpeed * 0.2f);

		glm::vec3 frustum[8];
		glm::vec3 center = world.camera.FrustumCorners(frustum);
		sphere->localTransform = glm::translate(glm::mat4(1.0), center);
		for(int i =0; i<8; i++)
		{
			frustSpheres[i]->localTransform = glm::translate(glm::mat4(1.0), frustum[8]);
		}

		testCube->localTransform = glm::inverse(world.camera.get_proj() * world.camera.get_view());

		world.scenegraph.PropagateTransforms();

		RenderPasses::DrawShadowMaps(world,app);

		RenderPasses::PrepareMain(world, app);
		RenderPasses::DrawOpaque(world,app);
		RenderPasses::DrawTransparent(world,app);

		world.time = fmod(world.time + world.input.deltaTime * world.input.animSpeed, 1000.f);

		app.frameEnd(world);
	}
	
    return 0;
}