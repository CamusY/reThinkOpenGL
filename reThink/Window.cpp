#include <iostream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui-docking/imgui.h>
#include <imgui-docking/imgui_impl_glfw.h>
#include <imgui-docking/imgui_impl_opengl3.h>
#include <imgui-docking/ImGuiFileDialog.h>
#include "assimp/Importer.hpp"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui-docking/TextEditor.h>
#include "Tools/ShaderTools/Shader.h"
#include "./Tools/Model/Model.h"
#include "./Tools/Editor/Editor.h"
#include "ProjectManager/ProjectManager.h"
#include "WindowState/WindowState.h"


// 全局变量
GLFWwindow* window;
Shader* shader = nullptr;
Model model;
Editor editor(&model);
ProjectManager projectManager;
WindowState windowState;
bool showFileDialog = true; // 模型加载对话框是否显示
bool modelLoaded = false;; // 默认模型加载状态
TextEditor vertexEditor, fragmentEditor;
bool showEditor = true;
ImVec4 clearColor = ImVec4(0.2f, 0.3f, 0.3f, 1.0f);
std::string lastShaderError;
glm::mat4 projection;

// 回调函数
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    projection = glm::perspective(glm::radians(45.0f), (float)width/(float)height, 0.1f, 100.0f);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
        editor.toggleEditMode();
    }
}

// 重新编译着色器
void reloadShader() {
    if (shader) delete shader;
    try {
        shader = new Shader("", ""); // 路径为空，通过代码加载
        shader->reload(vertexEditor.GetText().c_str(), fragmentEditor.GetText().c_str());
        lastShaderError.clear();
    } catch (const std::exception& e) {
        lastShaderError = e.what();
    }
}

int main() {
    // 初始化 GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 创建窗口
    window = glfwCreateWindow(1280, 720, "OpenGL + ImGui + Assimp", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 初始化 GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);

    // 初始化投影矩阵
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    projection = glm::perspective(glm::radians(45.0f), (float)width/(float)height, 0.1f, 100.0f);

    // 初始化 ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // 启用停靠
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    // 设置字体大小
    io.Fonts->AddFontFromFileTTF("SarasaMonoSlabSC-Regular.ttf", 20.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
    // 加载模型
    ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".obj", ImVec4(0.0f, 0.8f, 1.0f, 0.9f));

    // 初始化代码编辑器
    vertexEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
    vertexEditor.SetText(R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        void main() {
            gl_Position = vec4(aPos, 1.0);
        }
    )");

    fragmentEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
    fragmentEditor.SetText(R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0, 0.5, 0.2, 1.0);
        }
    )");

    // 主循环
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // ImGui 新帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 顶栏菜单
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu(u8"文件")) {
                if (ImGui::MenuItem(u8"新建项目")) {
                    projectManager.newProject();
                }
                if (ImGui::MenuItem(u8"保存项目")) {
                    projectManager.saveProject("project.txt");
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(u8"窗口")) {
                ImGui::MenuItem(u8"控制面板", NULL, &windowState.showControlPanel);
                ImGui::MenuItem(u8"编辑器", NULL, &windowState.showEditor);
                ImGui::MenuItem(u8"属性面板", NULL, &windowState.showProperties);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
        // 创建 DockSpace
        ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);
        if (showFileDialog) {
            ImGui::OpenPopup("选择模型文件");
            showFileDialog = false;
        }
        if (ImGuiFileDialog::Instance()->Display("选择模型文件", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400))) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
                try {
                    model.load(filePath.c_str());
                    modelLoaded = true;
                    lastShaderError.clear();
                } catch (const std::exception& e) {
                    lastShaderError = "模型加载失败: " + std::string(e.what());
                    modelLoaded = false;
                }
            }
            ImGuiFileDialog::Instance()->Close();
        }
        // 控制面板
        if (windowState.showControlPanel) {
            ImGui::Begin(u8"控制面板", &windowState.showControlPanel);
            ImGui::ColorEdit3(u8"背景颜色", (float*)&clearColor);
            ImGui::Separator();
            
            // 模型变换
            ImGui::Text(u8"模型变换");
            ImGui::DragFloat3(u8"位置", &model.position[0], 0.1f);
            ImGui::DragFloat3(u8"旋转", &model.rotation[0], 1.0f, -180.0f, 180.0f);
            ImGui::DragFloat3(u8"缩放", &model.scale[0], 0.01f, 0.01f, 10.0f);
            
            // 状态显示
            ImGui::Separator();
            ImGui::TextColored(model.isLoaded() ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), 
                model.isLoaded() ? u8"模型已加载" : u8"模型未加载");
            ImGui::End();
        }
        
        // 视口窗口
        ImGui::Begin(u8"3D 视口");
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        glViewport(0, 0, (int)viewportSize.x, (int)viewportSize.y);
        glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!modelLoaded) {
            ImGui::SetCursorPos(ImVec2(
                viewportSize.x/2 - ImGui::CalcTextSize(u8"请先选择模型文件").x/2,
                viewportSize.y/2 - 10
            ));
            ImGui::TextColored(ImVec4(1,1,0,1), u8"请先选择模型文件");
        }

        if (shader && modelLoaded) {
            shader->use();
            model.draw();
        }
        ImGui::End();

        if (windowState.showProperties) {
            ImGui::Begin(u8"属性", &windowState.showProperties);
            editor.drawPropertiesPanel();
            ImGui::End();
        }
        
        // 代码编辑器窗口
        if (showEditor) {
            ImGui::Begin(u8"着色器编辑器", &showEditor, ImGuiWindowFlags_MenuBar);
            if (ImGui::BeginMenuBar()) {
                if (ImGui::Button(u8"编译")) {
                    reloadShader();
                }
                ImGui::EndMenuBar();
            }

            ImGui::BeginTabBar(u8"ShaderTabs");
            if (ImGui::BeginTabItem(u8"顶点着色器")) {
                vertexEditor.Render(u8"VertexShader");
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(u8"片段着色器")) {
                fragmentEditor.Render(u8"FragmentShader");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
            ImGui::End();
        }

        // 渲染 ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // 清理资源
    delete shader;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}