#include <iostream>
#include "shader.h"

#include "world.h"

#include "ui.h"
#include "application.h"
#include "terrain_mesh.h"
#include "rts_camera.h"
#include "world.h"
#include "render_passes.h"

using namespace std;

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

	terrain_mesh->shader.setCamera(camera);
	// mesh drawing
	terrain_mesh->draw();
}

int main()
{
    const char * title = "Crowd Simulation Test";
	Engine::Application app = Engine::Application(1024,768,title);

    init(app);

	// event loop
	while(!app.shouldClose()) {
		app.frameStart(world);

		RenderPasses::clear(app, ui);

		display(app);

		app.frameEnd(world);
	}

	delete(terrain_mesh);
    return 0;
}