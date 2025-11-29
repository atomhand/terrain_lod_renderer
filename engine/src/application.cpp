#include "application.h"
#include <iostream>
#include <GLFW/glfw3.h>

static double scrollDelta = 0.f;
static glm::vec2 mousePos;

static float yAxisKeyDelta;
static float xAxisKeyDelta;

static float numInput = -1.0;

static float lastFrameTime;

static bool wireframe = false;

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

	world.input.xAxisKeyDelta = xAxisKeyDelta;	
	world.input.yAxisKeyDelta = yAxisKeyDelta;

	xAxisKeyDelta = 0.0;
	yAxisKeyDelta = 0.0;

	scrollDelta = 0.f;

	world.input.wireFrame = wireframe;

	float currentFrame = float(glfwGetTime());
	world.input.deltaTime = currentFrame - lastFrameTime;
	lastFrameTime = currentFrame;

	if(numInput >= 0.0) {
		world.input.animSpeed = numInput;
		numInput = -1.0;
	}
}

void Engine::Application::keyCallback(GLFWwindow* window, int k, int s, int action, int mods)
{
	if (k == GLFW_KEY_1 && action == GLFW_PRESS)
		numInput = 1.0;
	if (k == GLFW_KEY_2 && action == GLFW_PRESS)
		numInput = 2.0;
	if (k == GLFW_KEY_3 && action == GLFW_PRESS)
		numInput = 3.0;
	if (k == GLFW_KEY_4 && action == GLFW_PRESS)
		numInput = 4.0;
	if (k == GLFW_KEY_5 && action == GLFW_PRESS)
		numInput = 5.0;
	if (k == GLFW_KEY_6 && action == GLFW_PRESS)
		numInput = 6.0;
	if (k == GLFW_KEY_7 && action == GLFW_PRESS)
		numInput = 7.0;
	if (k == GLFW_KEY_8 && action == GLFW_PRESS)
		numInput = 8.0;
	if (k == GLFW_KEY_9 && action == GLFW_PRESS)
		numInput = 9.0;
	if (k == GLFW_KEY_0 && action == GLFW_PRESS)
		numInput = 0.0;

	if (k == GLFW_KEY_C && action == GLFW_PRESS)
		wireframe = !wireframe;

	if(k == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS))
		yAxisKeyDelta += -1.0;
	if(k == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS))
		yAxisKeyDelta += 1.0;
	if(k == GLFW_KEY_A && (action == GLFW_REPEAT || action == GLFW_PRESS))
		xAxisKeyDelta += -1.0;
	if(k == GLFW_KEY_D && (action == GLFW_REPEAT || action == GLFW_PRESS))
		xAxisKeyDelta += 1.0;

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
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

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