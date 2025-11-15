#pragma once

#include <string>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>
#include "world.h"

struct GLFWwindow;

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
    public:
        void updateWorld(World& world);

        Application(int width, int height, const char *title);
        ~Application();

        void getFramebufferSize(int& w, int& h) const;

        bool isIconified() const;

        void frameStart(World& world);

        void frameEnd(World& world);

        bool shouldClose() const;

        float getUiContentScale() const;
    };
}