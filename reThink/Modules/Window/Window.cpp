// Window.cpp
#include "Window.h"
#include <sstream>
#include <array>
#include <imgui-docking/imgui_impl_glfw.h>
#include <imgui-docking/imgui_impl_opengl3.h>
#include "imgui-docking/imgui_internal.h"

static void error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// GLFW窗口删除器实现
void Window::GLFWwindowDeleter::operator()(GLFWwindow* window) const {
    if (window) {
        glfwDestroyWindow(window);
    }
}

Window::Window(std::shared_ptr<ConfigManager> configManager) 
    : configManager_(configManager) {
    
    // 防御性检查：必须注入有效配置管理器
    if (!configManager_) {
        throw std::invalid_argument("配置管理器指针不能为空");
    }
    // 初始化GLFW错误回调
    glfwSetErrorCallback(error_callback);
}

void Window::Initialize() {
    // 初始化GLFW
    if (!glfwInit()) {
        throw std::runtime_error("GLFW初始化失败");
    }

    // 创建主窗口
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Main Window", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("窗口创建失败");
    }
    glfwWindow_.reset(window);

    // 初始化ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(glfwWindow_.get(), true);
    ImGui_ImplOpenGL3_Init("#version 460");
}

void Window::ApplyLayout() {
    const auto& layoutConfig = configManager_->GetLayoutConfig().at("DockSpace");
    
    try {
        // 获取节点数量
        int nodeCount = std::stoi(layoutConfig.at("NodeCount"));
        
        // 创建主DockSpace
        if (!layoutApplied_) {
            dockspaceID_ = ImGui::GetID("MyDockSpace"); // 为 DockSpace 生成一个 ID
        }
        dockspaceID_ = ImGui::DockSpaceOverViewport(
            dockspaceID_, // 传入 dockspaceID_
            ImGui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode,
            nullptr // 可选的 window_class，这里可以传入 nullptr
        );
        // 清除现有布局
        ImGui::DockBuilderRemoveNode(dockspaceID_);
        ImGui::DockBuilderAddNode(dockspaceID_, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceID_, ImGui::GetMainViewport()->Size);

        // 解析每个节点配置
        std::array<ImGuiID, 3> nodeIDs{};
        for (int i = 0; i < nodeCount; ++i) {
            const std::string nodeKey = "Node" + std::to_string(i);
            auto [parentID, splitDir, sizeRatio] = 
                ParseNodeConfig(layoutConfig.at(nodeKey));

            // 分割节点
            ImGuiID newID = ImGui::DockBuilderSplitNode(
                parentID, splitDir, sizeRatio, nullptr, &parentID
            );
            nodeIDs[i] = newID;
        }

        // 提交布局
        ImGui::DockBuilderFinish(dockspaceID_);
        layoutApplied_ = true;
    } catch (const std::exception& e) {
        // 配置无效时回退默认布局
        CreateFallbackLayout();
        throw std::runtime_error(std::string("布局配置错误: ") + e.what());
    }
}

std::tuple<ImGuiID, ImGuiDir, float> Window::ParseNodeConfig(const std::string& nodeStr) {
    std::istringstream iss(nodeStr);
    std::string part;
    
    // 解析父节点ID
    std::getline(iss, part, '|');
    ImGuiID parentID = static_cast<ImGuiID>(std::stoul(part));

    // 解析方向参数
    std::getline(iss, part, ',');
    ImGuiDir direction = static_cast<ImGuiDir>(std::stoi(part));

    // 解析尺寸比例
    float ratio;
    iss >> ratio;

    return {parentID, direction, ratio};
}

void Window::CreateFallbackLayout() {
    // 清除现有布局
    ImGui::DockBuilderRemoveNode(dockspaceID_);
    ImGui::DockBuilderAddNode(dockspaceID_, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceID_, ImGui::GetMainViewport()->Size);

    // 创建默认三栏布局
    ImGuiID mainID = dockspaceID_;
    ImGuiID rightID = ImGui::DockBuilderSplitNode(mainID, ImGuiDir_Right, 0.2f, nullptr, &mainID);
    ImGuiID bottomID = ImGui::DockBuilderSplitNode(mainID, ImGuiDir_Down, 0.3f, nullptr, &mainID);
    
    // 提交布局
    ImGui::DockBuilderFinish(dockspaceID_);
    layoutApplied_ = true;
}

void Window::Render() {
    while (!glfwWindowShouldClose(glfwWindow_.get())) {
        glfwPollEvents();
        
        // 开始ImGui帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 应用布局（首次运行）
        if (!layoutApplied_) {
            try {
                ApplyLayout();
            } catch (const std::exception& e) {
                // 记录错误并继续使用默认布局
                fprintf(stderr, "%s\n", e.what());
            }
        }

        // 主DockSpace渲染
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        
        ImGui::Begin("MainDockSpace", nullptr, 
            ImGuiWindowFlags_NoTitleBar | 
            ImGuiWindowFlags_NoCollapse | 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoBringToFrontOnFocus
        );
        ImGui::End();

        // 渲染
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(glfwWindow_.get(), &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(glfwWindow_.get());
    }
}

void Window::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
}