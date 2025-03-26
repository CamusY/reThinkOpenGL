#include "Window.h"
#include "glad/glad.h"
#include <iostream>

#include "EventBus/EventTypes.h"
#include "imgui-docking/imgui_internal.h"

namespace MyRenderer {

Window::Window(std::shared_ptr<EventBus> eventBus)
    : eventBus_(eventBus), window_(nullptr), dockSpaceId_(0), firstRun_(true) {
}

Window::~Window() {
    Shutdown();
}

void Window::Initialize() {
    // 初始化GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        throw std::runtime_error("GLFW initialization failed");
    }

    // 设置GLFW窗口提示
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 创建窗口
    window_ = glfwCreateWindow(1920, 1080, "MyRenderer", nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        std::cerr << "Failed to create GLFW window" << std::endl;
        throw std::runtime_error("Window creation failed");
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // 默认启用V-Sync

    // 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwDestroyWindow(window_);
        glfwTerminate();
        std::cerr << "Failed to initialize GLAD" << std::endl;
        throw std::runtime_error("GLAD initialization failed");
    }

    // 初始化ImGui
    SetupImGui();

    // 加载布局配置
    if (!configManager_.LoadConfig()) {
        std::cerr << "Failed to load config, using default layout" << std::endl;
    }
    layoutConfig_ = configManager_.GetLayoutConfig();
    dockSpaceId_ = ImGui::GetID(layoutConfig_.dockSpaceId.c_str());

    // 订阅事件
    SubscribeToEvents();

    std::cout << "Window initialized successfully" << std::endl;
}

void Window::Update() {
    // 处理GLFW事件
    glfwPollEvents();

    // 开始ImGui帧
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 创建DockSpace
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                   ImGuiWindowFlags_NoNavFocus;
    ImGui::Begin("DockSpaceWindow", nullptr, windowFlags);
    ImGui::PopStyleVar(2);

    dockSpaceId_ = ImGui::GetID(layoutConfig_.dockSpaceId.c_str());
    ImGui::DockSpace(dockSpaceId_, ImVec2(layoutConfig_.dockSpaceWidth, layoutConfig_.dockSpaceHeight));

    // 应用初始布局
    if (firstRun_) {
        ApplyInitialLayout();
        firstRun_ = false;
    }

    ImGui::End();

    // 渲染ImGui
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // 更新和渲染额外的ImGui窗口（如果有）
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    glfwSwapBuffers(window_);
}

void Window::Shutdown() {
    if (window_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window_);
        glfwTerminate();
        window_ = nullptr;
    }
}

bool Window::ShouldClose() const {
    return window_ && glfwWindowShouldClose(window_);
}

void Window::SetupImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 430");
}

void Window::ApplyInitialLayout() {
    ImGui::DockBuilderRemoveNode(dockSpaceId_);
    ImGui::DockBuilderAddNode(dockSpaceId_, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockSpaceId_, ImVec2(layoutConfig_.dockSpaceWidth, layoutConfig_.dockSpaceHeight));

    ImGuiID dockMain = dockSpaceId_;
    ImGuiID dockLeft = 0, dockRight = 0, dockTop = 0, dockBottom = 0;

    for (const auto& window : layoutConfig_.windows) {
        ImGui::DockBuilderDockWindow(window.id.c_str(), dockMain);

        if (window.dockSide == "left" && !dockLeft) {
            dockLeft = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.2f, nullptr, &dockMain);
            ImGui::DockBuilderDockWindow(window.id.c_str(), dockLeft);
        } else if (window.dockSide == "right" && !dockRight) {
            dockRight = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.3f, nullptr, &dockMain);
            ImGui::DockBuilderDockWindow(window.id.c_str(), dockRight);
        } else if (window.dockSide == "top" && !dockTop) {
            dockTop = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Up, 0.05f, nullptr, &dockMain);
            ImGui::DockBuilderDockWindow(window.id.c_str(), dockTop);
        } else if (window.dockSide == "bottom" && !dockBottom) {
            dockBottom = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.2f, nullptr, &dockMain);
            ImGui::DockBuilderDockWindow(window.id.c_str(), dockBottom);
        }
    }

    ImGui::DockBuilderFinish(dockSpaceId_);
}

void Window::SubscribeToEvents() {
    eventBus_->Subscribe<Events::LayoutChangeEvent>([this](const Events::LayoutChangeEvent& event) {
        if (configManager_.LoadLayout(event.layoutName)) {
            layoutConfig_ = configManager_.GetLayoutConfig();
            firstRun_ = true; // 重新应用布局
        } else {
            std::cerr << "Failed to load layout: " << event.layoutName << std::endl;
        }
    });
}

} // namespace MyRenderer