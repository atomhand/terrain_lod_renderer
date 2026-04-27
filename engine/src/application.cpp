
#include <iostream>
#include <sstream>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "application.h"
#include "world.h"

#include "stb_image_write.h"
#include "asset_helper.h"

static Engine::Input* input;

static double lastFrameTime;

// for imgui
const char* glsl_version = "#version 130";

void APIENTRY glDebugOutput(GLenum source, 
                            GLenum type, 
                            unsigned int id, 
                            GLenum severity, 
                            GLsizei length, 
                            const char *message, 
                            const void *userParam)
{
    // ignore non-significant error/warning codes
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return; 

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " <<  message << std::endl;

    switch (source)
    {
        case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    } std::cout << std::endl;

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break; 
        case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    } std::cout << std::endl;
    
    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    } std::cout << std::endl;
    std::cout << std::endl;
}

void Engine::Application::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
	input->mousePosDelta = glm::vec2((float)xpos,(float)ypos) - input->mousePos;
	input->mousePos = glm::vec2((float)xpos,(float)ypos);
};
		
/* Called whenever the window is resized. The new window size is given, in pixels. */
void Engine::Application::reshapeCallback(GLFWwindow* window, int w, int h)
{
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
}

void Engine::Application::passInputs(World& world) {
	static double secsSinceAvg = 0.0;
	static int framesSinceAvg = 0;
	framesSinceAvg++;

	double currentFrame = glfwGetTime();
	world.input.deltaTime = currentFrame - lastFrameTime;

	secsSinceAvg += (currentFrame-lastFrameTime);
	lastFrameTime = currentFrame;
	if(secsSinceAvg > 1.0) {
		double fps = double(framesSinceAvg) / secsSinceAvg;
		double frametime = secsSinceAvg * 1000. / double(framesSinceAvg);
		std::stringstream strTitle;
		strTitle << title << " " << " [" << fps << " FPS] [" << frametime << " ms]";
		if(vsync)
			strTitle << " (VSYNC ON)";
		else
			strTitle << " (VSYNC  OFF)";
		glfwSetWindowTitle(window, strTitle.str().c_str());

		secsSinceAvg = 0.0;
		framesSinceAvg = 0;
	}
}

void Engine::Application::keyCallback(GLFWwindow* window, int k, int s, int action, int mods)
{
	if (k == GLFW_KEY_F && action == GLFW_PRESS)
		input->flyCamera = !input->flyCamera;

	if (k == GLFW_KEY_Q && action == GLFW_PRESS)
		input->debugMetaCam = !input->debugMetaCam;
	
	if (k == GLFW_KEY_M && action == GLFW_PRESS)
		input->shadowTestingMode = (input->shadowTestingMode+1)%3;

	if (k == GLFW_KEY_G && action == GLFW_PRESS)
		input->drawFog = !input->drawFog;
	if (k == GLFW_KEY_T && action == GLFW_PRESS)
		input->noClip = !input->noClip;
	if (k == GLFW_KEY_C && action == GLFW_PRESS)
			input->wireFrame = !input->wireFrame;
	if (k == GLFW_KEY_B && action == GLFW_PRESS) 
		input->stochasticBlending = !input->stochasticBlending;
	if (k == GLFW_KEY_U && action == GLFW_PRESS)
		input->previewCascades = !input->previewCascades;

	if (k == GLFW_KEY_N && action == GLFW_PRESS)
		input->previewNormalsMode = !input->previewNormalsMode;
	if (k == GLFW_KEY_P && action == GLFW_PRESS)
	{		
		input->profilerWindow = !input->profilerWindow;
	}

	if(k == GLFW_KEY_H && action == GLFW_PRESS) {
		input->testFlythrough = !input->testFlythrough;
	}

	if (k == GLFW_KEY_V && action == GLFW_PRESS)
		input->vsync = !input->vsync;

	if (k == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		if(input->testFlythrough) {
			input->testFlythrough = false;
		} else {			
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}
}

void Engine::Application::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	input->scrollDelta += yoffset;
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

	//glfwWindowHint(GLFW_SAMPLES, 8);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif
	float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
	window = glfwCreateWindow(width*main_scale, height*main_scale, title, 0, 0);
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
	int version = gladLoadGL(glfwGetProcAddress);
	printf("GL %d.%d\n", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

	// GL debug output
	int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glDebugOutput, NULL);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
	}

	glEnable(GL_MULTISAMPLE);

    // set callbacks

    glfwSetKeyCallback(window,keyCallback);
    glfwSetErrorCallback(errorCallback);

    glfwSetFramebufferSizeCallback(window,reshapeCallback);
	glfwSetScrollCallback(window,scrollCallback);
	glfwSetCursorPosCallback(window,cursorPosCallback);

	glfwSetWindowTitle(window, title);

	// Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    imGuiCtx = ImGui::CreateContext();
	ImGui::SetCurrentContext(imGuiCtx);
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

Engine::Application::~Application() {
	// Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
	glfwTerminate();
}

void Engine::Application::getFramebufferSize(int& w, int& h) {
	glfwGetFramebufferSize(window, &w, &h);
};

bool Engine::Application::isIconified() {
	return (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0);
}

void Engine::Application::frameStart(World& world) {
	world.input.scrollDelta = 0.0;
	world.input.mousePosDelta = glm::vec2(0.,0.); 
	input = &world.input;

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	
	glfwPollEvents();

	if(vsync != world.input.vsync) {
		vsync = world.input.vsync;
		glfwSwapInterval(vsync ? 1 : 0);
	}

	if(cursorLocked != input->flyCamera) {
		cursorLocked = input->flyCamera;
		if(input->flyCamera) {
			// cursor is hidden and locked while fly camera is active
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			input->mousePosDelta = glm::vec2(0.,0.);
		}
		else
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	glm::vec2 keyAxisDelta = glm::vec2(0.0);
	
	if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		keyAxisDelta.y += 1.0;
	if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		keyAxisDelta.y += -1.0;	
	if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		keyAxisDelta.x += -1.0;
	if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		keyAxisDelta.x += 1.0;


	if(glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
		SaveScreenshot(nullptr);
	
	input->keyAxisDelta = keyAxisDelta.x + keyAxisDelta.y > 0.f ? glm::normalize(keyAxisDelta) : keyAxisDelta;

	passInputs(world);
}

void Engine::Application::frameEnd(World& world) {
    ImGui::SetCurrentContext(imGuiCtx);
	// imgui
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	// Swap buffers
	glfwSwapBuffers(window);
}

bool Engine::Application::shouldClose() {
	return glfwWindowShouldClose(window);
}

void Engine::Application::SaveScreenshot(const char* filename) {
	int w,h;
	getFramebufferSize(w,h);

	std::vector<uint8_t> pixels(3*w*h);

	glReadPixels(0,0,w,h, GL_RGB,GL_UNSIGNED_BYTE, pixels.data());

	// flip (opengl framebuffer is upside down relative to image data format)
	for(int line = 0; line != h/2; ++line) {
    std::swap_ranges(
            pixels.begin() + 3 * w * line,
            pixels.begin() + 3 * w * (line+1),
            pixels.begin() + 3 * w * (h-line-1));
	}

	std::filesystem::path path;

	if(filename == nullptr) {
		std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::string pathStr = std::ctime(&time);
		std::replace(pathStr.begin(), pathStr.end(), ' ', '_');
		std::replace(pathStr.begin(), pathStr.end(), ':', '-');
		pathStr.erase(std::remove(pathStr.begin(), pathStr.end(), '\n'), pathStr.cend());
		pathStr += ".png";
		path = AssetHelper::screenshotPath(pathStr.c_str());
	} else {
		path = AssetHelper::screenshotPath(filename);
	}

	std::cout << "Saving screenshot to: " << path.string() << std::endl;
	stbi_write_png(path.string().c_str(), w, h, 3, pixels.data(), 3 * w);
}