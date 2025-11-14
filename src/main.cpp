#include <iostream>
#include "shader.h"

#include "world.h"

#include "ui.h"
#include "application.h"
#include "terrain_mesh.h"
#include "rts_camera.h"
#include "world.h"

using namespace std;

GLuint positionBufferObject;
GLuint vao;

World world;
TerrainMesh* terrain_mesh;
Ui ui;

RtsCamera camera;

/*
This function is called before entering the main rendering loop.
Use it for all you initialisation stuff
*/
void init(Engine::Application &window)
{
    ui.init(window);

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
void display(Engine::Application& window)
{
	camera.update_zoom(world.input.scrollDelta);
	camera.update_transform();

	ui.frame_update(window);

	// Rendering
	int display_w, display_h;
	window.getFramebufferSize(display_w,display_h);
	glViewport(0, 0, display_w, display_h);
	glClearColor(ui.clear_color.x * ui.clear_color.w, ui.clear_color.y * ui.clear_color.w, ui.clear_color.z * ui.clear_color.w, ui.clear_color.w);
	glClear(GL_COLOR_BUFFER_BIT);

	terrain_mesh->shader.setCamera(camera);
	// mesh drawing
	terrain_mesh->draw();

	// render ui
	ui.render();
}

int main()
{
    const char * title = "Crowd Simulation Test";
	Engine::Application app = Engine::Application(1024,768,title);

    init(app);

	// event loop
	while(!app.shouldClose()) {
		app.frameStart(world);

		display(app);

		app.frameEnd(world);
	}

	delete(terrain_mesh);
    return 0;
}