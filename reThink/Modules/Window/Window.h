#pragma once
#include <GLFW/glfw3.h>

class Window {
    GLFWwindow* window;
    
public:
    bool Init(int width, int height, const char* title);
    void PreRender();
    void PostRender();
    GLFWwindow* GetNativeWindow() const { return window; }
};