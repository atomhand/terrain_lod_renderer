#pragma once

#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <vector>

namespace Engine {
    class GameWindow
    {
    private:
        int width;
        int height;
        const char *title;
        
        GLFWwindow* window;

        void frameStart();
        
        static void reshapeCallback(GLFWwindow* window, int w, int h);
        static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void errorCallback(int erorr, const  char* description);
        static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
        static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);

        std::vector<void(*)(GameWindow& window)> renderPasses;
    public:
        glm::vec2 getMousePos();
        float getScrollDelta();

        GameWindow(int width, int height, const char *title);
        ~GameWindow();

        int eventLoop();

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

        void addRenderPass(void(*f)(GameWindow& window)) {
            renderPasses.push_back(f);
        };
    };
}