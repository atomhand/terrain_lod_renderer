#include "application.h"
#include <iostream>
#include <GLFW/glfw3.h>

static double scrollDelta = 0.f;
static glm::vec2 mousePos;

void Engine::Application::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
	mousePos = glm::vec2((float)xpos,(float)ypos);
};
		
/* Called whenever the window is resized. The new window size is given, in pixels. */
void Engine::Application::reshapeCallback(GLFWwindow* window, int w, int h)
{
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
}

void Engine::Application::passInputs(World& world) {
	world.input.scrollDelta = (float)scrollDelta;
	world.input.mousePos = mousePos;

	scrollDelta = 0.f;
}

void Engine::Application::keyCallback(GLFWwindow* window, int k, int s, int action, int mods)
{
	if (action != GLFW_PRESS) return;

	std::cout << "KEY: " << (char)k << std::endl;

	if (k == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

void Engine::Application::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	scrollDelta += yoffset;
}

/* An error callback function to output GLFW errors*/
void Engine::Application::errorCallback(int error, const char* description)
{
	fputs(description, stderr);
}

Engine::Application::Application(int width, int height, const char *title) {
    this->width = width;
    this->height = height;
    this->title = title;

    /* Initialise GLFW and exit if it fails */
	if (!glfwInit()) 
	{
		std::cout << "Failed to initialize GLFW." << std::endl;
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
		std::cout << "Could not open GLFW window." << std::endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	/* Obtain an OpenGL context and assign to the just opened GLFW window */
	glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

	/* Initialise GLLoad library. You must have obtained a current OpenGL */
	 // glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD - exiting" << std::endl;
		glfwTerminate();
		return;
	}

	glEnable(GL_MULTISAMPLE);

    // set callbacks

    glfwSetKeyCallback(window,keyCallback);
    glfwSetErrorCallback(errorCallback);

    glfwSetFramebufferSizeCallback(window,reshapeCallback);
	glfwSetScrollCallback(window,scrollCallback);
	glfwSetCursorPosCallback(window,cursorPosCallback);

	glfwSetWindowTitle(window, title);
}

Engine::Application::~Application() {    
	glfwTerminate();
}

void Engine::Application::getFramebufferSize(int& w, int& h) {
	glfwGetFramebufferSize(window, &w, &h);
};

bool Engine::Application::isIconified() {
	return (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0);
}

void Engine::Application::frameStart(World& world) {
	glfwPollEvents();
	passInputs(world);
}

void Engine::Application::frameEnd(World& world) {
	// Swap buffers
	glfwSwapBuffers(window);
}

bool Engine::Application::shouldClose() {
	return glfwWindowShouldClose(window);
}