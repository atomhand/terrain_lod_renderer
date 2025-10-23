/**
Modified from Iain Martin's wrapper example -- Iain Martin August 2014
*/
#pragma once

#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Engine
{
    class GlfwWrapper
    {
    private:
        int width;
        int height;
        const char *title;
        double fps;
        void(*renderer)(GlfwWrapper* window);
        bool running;

    public:
        GLFWwindow* window;
        GlfwWrapper(int width, int height, const char *title);
        ~GlfwWrapper();

        void setFPW(double fps) {
            this->fps = fps;
        }

        void DisplayVersion();

        void setRenderer(void(*f)(GlfwWrapper* window));
        void setReshapeCallback(void(*f)(GLFWwindow* window, int w, int h));
        void setKeyCallback(void(*f)(GLFWwindow* window, int key, int scancode, int action, int mods));
        void setErrorCallback(void(*f)(int erorr, const  char* description));

        int eventLoop();
        GLFWwindow* getWindow();
    };
}