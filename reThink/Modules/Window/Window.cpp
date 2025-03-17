#include "Window.h"
#include <./imgui-docking/imgui.h>
#include <./imgui-docking/imgui_impl_glfw.h>
#include <./imgui-docking/imgui_impl_opengl3.h>

bool Window::Init(int width, int height, const char* title) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, title, NULL, NULL);
    glfwMakeContextCurrent(window);
    
    // ImGui初始化
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    return true;
}

void Window::PreRender() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // 创建DockSpace
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::DockSpaceOverViewport(viewport);
}

void Window::PostRender() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}