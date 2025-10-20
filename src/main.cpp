#include <iostream>
#include "shader_helpers.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "ui.h"

#include "wrapper_glfw.h"
using namespace std;

GLuint positionBufferObject;
GLuint program;
GLuint vao;

Ui ui;

const char* glsl_version = "#version 130";
/*
This function is called before entering the main rendering loop.
Use it for all you initialisation stuff
*/
void init(GlfwWrapper *glfw)
{	
	float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    glfwMakeContextCurrent(glfw->window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
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
    ImGui_ImplGlfw_InitForOpenGL(glfw->window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);


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
		program = ShaderHelpers::LoadShader("shaders/basic.vert", "shaders/basic.frag");
	}
	catch (exception &e)
	{
		cout << "Caught exception: " << e.what() << endl;
		cin.ignore();
		exit(0);
	}

	glfw->DisplayVersion();

}

//Called to update the display.
//You should call glfwSwapBuffers() after all of your rendering to display what you rendered.
void display(GlfwWrapper* glfw)
{// Create window with graphics context
	if (glfwGetWindowAttrib(glfw->window, GLFW_ICONIFIED) != 0)
	{
		ImGui_ImplGlfw_Sleep(10);
		return;
	}

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ui.draw_frame();

	// Rendering
	int display_w, display_h;
	glfwGetFramebufferSize(glfw->window, &display_w, &display_h);
	glViewport(0, 0, display_w, display_h);
	glClearColor(ui.clear_color.x * ui.clear_color.w, ui.clear_color.y * ui.clear_color.w, ui.clear_color.z * ui.clear_color.w, ui.clear_color.w);
	glClear(GL_COLOR_BUFFER_BIT);

	// mesh drawing
	glUseProgram(program);
	glBindBuffer(GL_ARRAY_BUFFER, positionBufferObject);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	glDisableVertexAttribArray(0);

	// imgui render
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


/* Called whenever the window is resized. The new window size is given, in pixels. */
static void reshape(GLFWwindow* window, int w, int h)
{
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
}

/* change view angle, exit upon ESC */
static void keyCallback(GLFWwindow* window, int k, int s, int action, int mods)
{
	if (action != GLFW_PRESS) return;

	cout << "KEY: " << (char)k << endl;

	if (k == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

/* An error callback function to output GLFW errors*/
static void error_callback(int error, const char* description)
{
	fputs(description, stderr);
}

int main()
{
    const char * title = "Crowd Simulation Test";
    GlfwWrapper *glfw = new GlfwWrapper(1024,768,title);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD. Exiting." << endl;
        return -1;
    }

    glfw->setRenderer(display);
    glfw->setKeyCallback(keyCallback);
    glfw->setReshapeCallback(reshape);
    glfw->setErrorCallback(error_callback);

    init(glfw);

    glfw->eventLoop();

    delete(glfw);
    return 0;
}