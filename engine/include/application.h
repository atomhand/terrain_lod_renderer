#pragma once

#include <string>

#include <glm/glm.hpp>

#include <vector>
#include "world.h"

// forward declaration
struct GLFWwindow;
struct ImGuiContext;

namespace Engine {
    class Application
    {
    private:
        int width;
        int height;
        const char *title;

        bool cursorLocked;
        bool vsync;
        
        GLFWwindow* window;
        
        static void reshapeCallback(GLFWwindow* window, int w, int h);
        static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void errorCallback(int erorr, const  char* description);
        static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
        static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);

        void passInputs(World& world);
    public:
        ImGuiContext* imGuiCtx;
        
        Application(int width, int height, const char *title);
        ~Application();

        int eventLoop(World& world);

        void getFramebufferSize(int& w, int& h);

        bool isIconified();

        void frameStart(World& world);

        void frameEnd(World& world);

        bool shouldClose();

        void SaveScreenshot(const char* filename);
    };
}