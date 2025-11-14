#include <iostream>
#include "shader.h"

#include "demo_world.h"

#include "application.h"
#include "terrain_mesh.h"
#include "rts_camera.h"
#include "world.h"

#include "renderpasses.h"

using namespace std;

GLuint positionBufferObject;
GLuint vao;

DemoWorld world;
TerrainMesh* terrain_mesh;

/*
This function is called before entering the main rendering loop.
Use it for all you initialisation stuff
*/
void init(Engine::Application &app)
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	float vertexPositions[] = {
		0.75f, 0.75f, 0.0f, 1.0f,
		0.75f, -0.75f, 0.0f, 1.0f,
		-0.75f, -0.75f, 0.0f, 1.0f,
	};

	glGenBuffers(1, &positionBufferObject);
	glBindBuffer(GL_ARRAY_BUFFER, positionBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	try
	{
		Engine::Shader shader = Engine::Shader("shaders/basic.vert", "shaders/basic.frag");
		terrain_mesh = new TerrainMesh(TerrainConfig(32),shader);
	}
	catch (exception &e)
	{
		cout << "Caught exception: " << e.what() << endl;
		cin.ignore();
		exit(0);
	}
}

//Called to update the display.
//You should call glfwSwapBuffers() after all of your rendering to display what you rendered.
void display(Engine::Application &app)
{

	terrain_mesh->shader.setCamera(world.camera);
	// mesh drawing
	terrain_mesh->draw();
}

int main()
{
    const char * title = "GPU Programming Coursework App";
	Engine::Application app = Engine::Application(1024,768,title);

    init(app);

	try
	{
		Engine::Shader shader = Engine::Shader("shaders/basic.vert", "shaders/basic.frag");
		
		InstancedRenderBatch batch(shader);
		batch.vao = terrain_mesh->vao;
		batch.positionBufferObject = terrain_mesh->positionBufferObject;
		batch.transforms.push_back(glm::mat4x4(1.0));
		batch.num_indices = 6;

		world.instanced_render_batches.push_back(batch);
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

		for(auto &transform : world.instanced_render_batches[0].transforms) {
			transform = glm::translate(transform, glm::vec3(0.01,0.0f,0.0f));
		}

		RenderPasses::preRender(world, app);
		RenderPasses::instancedRenderPasses(world,app);

		app.frameEnd(world);
	}

	delete(terrain_mesh);
    return 0;
}