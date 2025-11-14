#pragma once

#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <vector>
#include "world.h"

namespace Engine {
    class Application
    {
    private:
        int width;
        int height;
        const char *title;
        
        GLFWwindow* window;
        
        static void reshapeCallback(GLFWwindow* window, int w, int h);
        static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void errorCallback(int erorr, const  char* description);
        static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
        static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);

        void passInputs(World& world);
    public:
        Application(int width, int height, const char *title);
        ~Application();

        int eventLoop(World& world);

        void getFramebufferSize(int& w, int& h) {
	        glfwGetFramebufferSize(window, &w, &h);
        };


        // TODO - Remove Glfw from the public API
        GLFWwindow* getWindow() {
            return window;
        }

        bool isIconified() {
            return (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0);
        }

        void frameStart(World& world) {
            glfwPollEvents();
            passInputs(world);
        }

        void frameEnd(World& world) {
            // Swap buffers
            glfwSwapBuffers(window);
        }

        bool shouldClose() {
            return glfwWindowShouldClose(window);
        }
    };
}