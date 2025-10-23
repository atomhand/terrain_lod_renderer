/**
  wrapper_glfw.cpp
  Modified from Iain Martin's wrapper example -- Iain Martin August 2022
*/

#include "wrapper_glfw.h"

#include <iostream>
using namespace std;

/* Constructor for wrapper object */
Engine::GlfwWrapper::GlfwWrapper(int width, int height, const char *title) {

	this->width = width;
	this->height = height;
	this->title = title;
	this->fps = 60;
	this->running = true;
	this->renderer = NULL;

	/* Initialise GLFW and exit if it fails */
	if (!glfwInit()) 
	{
		cout << "Failed to initialize GLFW." << endl;
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_SAMPLES, 8);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef DEBUG
	glfwOpenWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

	window = glfwCreateWindow(width, height, title, 0, 0);
	if (!window){
		cout << "Could not open GLFW window." << endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	/* Obtain an OpenGL context and assign to the just opened GLFW window */
	glfwMakeContextCurrent(window);

	/* Initialise GLLoad library. You must have obtained a current OpenGL */
	 // glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD - exiting" << std::endl;
		glfwTerminate();
		return;
	}

	glfwSetInputMode(window, GLFW_STICKY_KEYS, true);

	glEnable(GL_MULTISAMPLE);
}


/* Terminate GLFW on destruvtion of the wrapepr object */
Engine::GlfwWrapper::~GlfwWrapper() {
	glfwTerminate();
}

/* Returns the GLFW window handle, required to call GLFW functions outside this class */
GLFWwindow* Engine::GlfwWrapper::getWindow()
{
	return window;
}


/*
 * Print OpenGL Version details
 */
void Engine::GlfwWrapper::DisplayVersion()
{
	/* One way to get OpenGL version*/
	int major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MAJOR_VERSION, &minor);
	cout << "OpenGL Version = " << major << "." << minor << endl;
	
	/* A more detailed way to the version strings*/
	cout << "Vender: " << glGetString(GL_VENDOR) << endl;
    cout << "Version:" << glGetString(GL_VERSION) << endl;
	cout << "Renderer:" << glGetString(GL_RENDERER) << endl;
}


/*
GLFW_Main function normally starts the windows system, calls any init routines
and then starts the event loop which runs until the program ends
*/
int Engine::GlfwWrapper::eventLoop()
{
	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		// Call function to draw your graphics
		renderer(this);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	return 0;
}


/* Register an error callback function */
void Engine::GlfwWrapper::setErrorCallback(void(*func)(int error, const char* description))
{
	glfwSetErrorCallback(func);
}

/* Register a display function that renders in the window */
void Engine::GlfwWrapper::setRenderer(void(*func)(GlfwWrapper*)) {
	this->renderer = func;
}

/* Register a callback that runs after the window gets resized */
void Engine::GlfwWrapper::setReshapeCallback(void(*func)(GLFWwindow* window, int w, int h)) {
	glfwSetFramebufferSizeCallback(window, func);
}


/* Register a callback to respond to keyboard events */
void Engine::GlfwWrapper::setKeyCallback(void(*func)(GLFWwindow* window, int key, int scancode, int action, int mods))
{
	glfwSetKeyCallback(window, func);
}

