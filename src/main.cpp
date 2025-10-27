#include <iostream>
#include "shader.h"

#include "ui.h"

#include "game_window.h"

#include "terrain_mesh.h"

#include "rts_camera.h"

using namespace std;

GLuint positionBufferObject;
GLuint vao;

TerrainMesh* terrain_mesh;

Ui ui;

RtsCamera camera;

/*
This function is called before entering the main rendering loop.
Use it for all you initialisation stuff
*/
void init(Engine::GameWindow &window)
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
void display(Engine::GameWindow& window)
{
	camera.update_zoom(window.getScrollDelta());
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
	Engine::GameWindow window = Engine::GameWindow(1024,768,title);

    init(window);

	window.addRenderPass(display);

    window.eventLoop();

	delete(terrain_mesh);
    return 0;
}