#include "Window.h"
#include <iostream>
#include "EventBus/EventTypes.h"
#include "imgui_internal.h"

namespace MyRenderer {

Window::Window(std::shared_ptr<EventBus> eventBus)
    : eventBus_(eventBus), window_(nullptr), dockSpaceId_(0), firstRun_(true) {
    std::cout << "[初始化] 窗口构造函数调用" << std::endl;
    if (!eventBus_) {
        std::cerr << "[错误] EventBus为空指针！" << std::endl;
        throw std::runtime_error("EventBus不可为空");
    }
}

Window::~Window() {
    Shutdown();
}

void Window::Initialize() {
    std::cout << "[初始化] 开始窗口初始化..." << std::endl;

    // 初始化GLFW
    std::cout << "[初始化] 正在初始化GLFW..." << std::endl;
    if (!glfwInit()) {
        std::cerr << "[错误] GLFW初始化失败" << std::endl;
        throw std::runtime_error("GLFW初始化失败");
    }
    std::cout << "[初始化] GLFW初始化成功" << std::endl;

    // 配置OpenGL上下文
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 创建窗口
    std::cout << "[初始化] 正在创建GLFW窗口..." << std::endl;
    window_ = glfwCreateWindow(1920, 1080, "MyRenderer", nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        std::cerr << "[错误] 无法创建GLFW窗口" << std::endl;
        throw std::runtime_error("窗口创建失败");
    }
    std::cout << "[初始化] GLFW窗口创建成功" << std::endl;

    // 设置当前上下文
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // 启用垂直同步
    std::cout << "[初始化] OpenGL上下文设置成功" << std::endl;

    // 初始化GLAD
    std::cout << "[初始化] 正在初始化GLAD..." << std::endl;
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwDestroyWindow(window_);
        glfwTerminate();
        std::cerr << "[错误] GLAD初始化失败" << std::endl;
        throw std::runtime_error("GLAD初始化失败");
    }
    std::cout << "[初始化] GLAD初始化成功" << std::endl;

    // 打印OpenGL信息
    std::cout << "[信息] OpenGL版本: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "[信息] GLSL版本: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    // 初始化ImGui
    std::cout << "[初始化] 正在设置ImGui..." << std::endl;
    SetupImGui();
    std::cout << "[初始化] ImGui设置成功" << std::endl;

    // 加载配置并提供默认值保护
    std::cout << "[初始化] 正在加载配置文件..." << std::endl;
    if (!configManager_.LoadConfig()) {
        std::cerr << "[警告] 配置文件加载失败，使用默认配置" << std::endl;
        layoutConfig_.dockSpaceId = "MainDockSpace";
        layoutConfig_.dockSpaceWidth = 1920;
        layoutConfig_.dockSpaceHeight = 1080;
        WindowConfig defaultWindow;
        defaultWindow.id = "DefaultWindow";
        defaultWindow.visible = true;
        defaultWindow.posX = 0;
        defaultWindow.posY = 0;
        defaultWindow.width = 400;
        defaultWindow.height = 400;
        defaultWindow.dockId = "MainDockSpace";
        defaultWindow.dockSide = "center";
        layoutConfig_.windows.push_back(defaultWindow);
    } else {
        std::cout << "[初始化] 配置文件加载成功" << std::endl;
    }
    layoutConfig_ = configManager_.GetLayoutConfig();
    std::cout << "[初始化] 获取布局配置成功，DockSpace ID: " << layoutConfig_.dockSpaceId << std::endl;

    if (layoutConfig_.dockSpaceId.empty()) {
        std::cerr << "[错误] DockSpace ID 为空，使用默认值" << std::endl;
        layoutConfig_.dockSpaceId = "MainDockSpace";
    }

    // 初始化模块
    std::cout << "[初始化] 正在初始化模块..." << std::endl;
    InitializeModules();
    std::cout << "[初始化] 模块初始化完成" << std::endl;

    // 订阅事件
    std::cout << "[初始化] 正在订阅事件..." << std::endl;
    SubscribeToEvents();
    std::cout << "[初始化] 事件订阅完成" << std::endl;

    std::cout << "[初始化] 窗口初始化全部完成" << std::endl;
}

void Window::Update() {
    if (!window_) {
        std::cerr << "[错误] 窗口指针无效，跳过更新" << std::endl;
        return;
    }

    // 添加 GLFW 事件轮询
    glfwPollEvents();

    // 核心更新逻辑
    inputHandler_->Update();
    textureManager_->ProcessTextureUploadQueue();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

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
    ImGui::DockSpace(dockSpaceId_, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    if (firstRun_) {
        std::cout << "[更新] 应用初始布局..." << std::endl;
        ApplyInitialLayout();
        firstRun_ = false;
    }

    // 仅在需要时更新模块
    menuBar_->Update();
    controlPanel_->Update();
    sceneViewport_->Update();
    // shaderEditor_->Update();
    projectTree_->Update();
    proceduralWindow_->Update();
    animation_->Update();

    ImGui::End();

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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
        std::cout << "[清理] 正在关闭场景视口..." << std::endl;
        sceneViewport_->Shutdown();

        std::cout << "[清理] 正在关闭ImGui..." << std::endl;
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        std::cout << "[清理] 正在销毁GLFW窗口..." << std::endl;
        glfwDestroyWindow(window_);
        glfwTerminate();
        window_ = nullptr;
        std::cout << "[清理] 窗口清理完成" << std::endl;
    }
}

bool Window::ShouldClose() const {
    return window_ && glfwWindowShouldClose(window_);
}

    void Window::SetupImGui() {
    std::cout << "[ImGui] 创建ImGui上下文..." << std::endl;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // 设置字体
    std::cout << "[ImGui] 加载中文字体..." << std::endl;
    io.Fonts->Clear();
    
    // 加载默认字体（英文字体）
    io.Fonts->AddFontDefault();
    
    // 加载中文字体（确保字体文件路径正确）
    const char* fontPath = "SarasaMonoSlabSC-Regular.ttf"; // 字体文件路径
    float fontSize = 18.0f; // 字体大小
    ImFont* chineseFont = io.Fonts->AddFontFromFileTTF(fontPath, fontSize, nullptr, io.Fonts->GetGlyphRangesChineseFull());
    
    if (!chineseFont) {
        std::cerr << "[错误] 中文字体加载失败: " << fontPath << std::endl;
        throw std::runtime_error("中文字体加载失败");
    }
    std::cout << "[ImGui] 中文字体加载成功" << std::endl;

    // 设置样式
    ImGui::StyleColorsDark();

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    std::cout << "[ImGui] 初始化GLFW和OpenGL后端..." << std::endl;
    if (!ImGui_ImplGlfw_InitForOpenGL(window_, true)) {
        std::cerr << "[错误] ImGui GLFW后端初始化失败" << std::endl;
        throw std::runtime_error("ImGui GLFW后端初始化失败");
    }
    if (!ImGui_ImplOpenGL3_Init("#version 430")) {
        std::cerr << "[错误] ImGui OpenGL3后端初始化失败" << std::endl;
        throw std::runtime_error("ImGui OpenGL3后端初始化失败");
    }
}

void Window::ApplyInitialLayout() {
    std::cout << "[布局] 开始应用初始布局..." << std::endl;
    ImGui::DockBuilderRemoveNode(dockSpaceId_);
    ImGui::DockBuilderAddNode(dockSpaceId_, ImGuiDockNodeFlags_DockSpace);
    ImVec2 dockSpaceSize(
        static_cast<float>(layoutConfig_.dockSpaceWidth > 0 ? layoutConfig_.dockSpaceWidth : 1920),
        static_cast<float>(layoutConfig_.dockSpaceHeight > 0 ? layoutConfig_.dockSpaceHeight : 1080)
    );
    ImGui::DockBuilderSetNodeSize(dockSpaceId_, dockSpaceSize);
    std::cout << "[布局] 设置DockSpace尺寸: " << dockSpaceSize.x << "x" << dockSpaceSize.y << std::endl;

    ImGuiID dockMain = dockSpaceId_;
    ImGuiID dockLeft = 0, dockRight = 0, dockTop = 0, dockBottom = 0;

    for (const auto& window : layoutConfig_.windows) {
        std::cout << "[布局] 设置窗口: " << window.id << " 到 " << window.dockSide << std::endl;
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
        } else if (window.dockSide == "center") {
            ImGui::DockBuilderDockWindow(window.id.c_str(), dockMain);
        }
    }

    ImGui::DockBuilderFinish(dockSpaceId_);
    std::cout << "[布局] 初始布局应用完成" << std::endl;
}

void Window::SubscribeToEvents() {
    if (!eventBus_) {
        std::cerr << "[错误] 事件总线无效，无法订阅事件" << std::endl;
        return;
    }

    eventBus_->Subscribe<Events::LayoutChangeEvent>([this](const Events::LayoutChangeEvent& event) {
        std::cout << "[事件] 收到布局更改事件: " << event.layoutName << std::endl;
        if (configManager_.LoadLayout(event.layoutName)) {
            layoutConfig_ = configManager_.GetLayoutConfig();
            firstRun_ = true;
            std::cout << "[事件] 布局 " << event.layoutName << " 加载成功" << std::endl;
        } else {
            std::cerr << "[错误] 无法加载布局: " << event.layoutName << std::endl;
        }
    });
}

void Window::InitializeModules() {
    try {
        std::cout << "[模块] 初始化线程池..." << std::endl;
        threadPool_ = std::make_shared<ThreadPool>(4);
        if (!threadPool_) throw std::runtime_error("线程池创建失败");
    } catch (const std::exception& e) {
        std::cerr << "[错误] 线程池初始化失败: " << e.what() << std::endl;
        throw;
    }

    try {
        std::cout << "[模块] 初始化JSON序列化器..." << std::endl;
        jsonSerializer_ = std::make_shared<JSONSerializer>();
        if (!jsonSerializer_) throw std::runtime_error("JSON序列化器创建失败");
    } catch (const std::exception& e) {
        std::cerr << "[错误] JSON序列化器初始化失败: " << e.what() << std::endl;
        throw;
    }

    try {
        std::cout << "[模块] 初始化着色器管理器..." << std::endl;
        shaderManager_ = std::make_shared<ShaderManager>(threadPool_);
        if (!shaderManager_) throw std::runtime_error("着色器管理器创建失败");
        // 等待默认着色器加载
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "[模块] 着色器管理器初始化完成" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[错误] 着色器管理器初始化失败: " << e.what() << std::endl;
        throw;
    }

    try {
        std::cout << "[模块] 初始化纹理管理器..." << std::endl;
        textureManager_ = std::make_shared<TextureManager>(eventBus_, threadPool_);
        if (!textureManager_) throw std::runtime_error("纹理管理器创建失败");
    } catch (const std::exception& e) {
        std::cerr << "[错误] 纹理管理器初始化失败: " << e.what() << std::endl;
        throw;
    }

    try {
        std::cout << "[模块] 初始化材质管理器..." << std::endl;
        materialManager_ = std::make_shared<MaterialManager>(eventBus_, textureManager_);
        if (!materialManager_) throw std::runtime_error("材质管理器创建失败");
    } catch (const std::exception& e) {
        std::cerr << "[错误] 材质管理器初始化失败: " << e.what() << std::endl;
        throw;
    }

    try {
        std::cout << "[模块] 初始化模型加载器..." << std::endl;
        modelLoader_ = std::make_shared<ModelLoader>(eventBus_, threadPool_, materialManager_);
        if (!modelLoader_) throw std::runtime_error("模型加载器创建失败");
    } catch (const std::exception& e) {
        std::cerr << "[错误] 模型加载器初始化失败: " << e.what() << std::endl;
        throw;
    }

    try {
        std::cout << "[模块] 初始化菜单栏..." << std::endl;
        menuBar_ = std::make_shared<MenuBar>(eventBus_, configManager_);
        if (!menuBar_) throw std::runtime_error("菜单栏创建失败");
    } catch (const std::exception& e) {
        std::cerr << "[错误] 菜单栏初始化失败: " << e.what() << std::endl;
        throw;
    }

    try {
        std::cout << "[模块] 初始化控制面板..." << std::endl;
        controlPanel_ = std::make_shared<ControlPanel>(eventBus_, materialManager_, textureManager_);
        if (!controlPanel_) throw std::runtime_error("控制面板创建失败");
    } catch (const std::exception& e) {
        std::cerr << "[错误] 控制面板初始化失败: " << e.what() << std::endl;
        throw;
    }

    try {
        std::cout << "[模块] 初始化场景视口..." << std::endl;
        sceneViewport_ = std::make_shared<SceneViewport>(eventBus_, shaderManager_, modelLoader_, materialManager_, textureManager_, window_);
        if (!sceneViewport_) throw std::runtime_error("场景视口创建失败");
        sceneViewport_->Initialize();
        std::cout << "[模块] 场景视口初始化成功" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[错误] 场景视口初始化失败: " << e.what() << std::endl;
        throw;
    }
/*
    try {
        std::cout << "[模块] 初始化着色器编辑器..." << std::endl;
        shaderEditor_ = std::make_shared<ShaderEditor>(eventBus_);
        if (!shaderEditor_) throw std::runtime_error("着色器编辑器创建失败");
    } catch (const std::exception& e) {
        std::cerr << "[错误] 着色器编辑器初始化失败: " << e.what() << std::endl;
        throw;
    }
*/
    try {
        std::cout << "[模块] 初始化项目树..." << std::endl;
        projectTree_ = std::make_shared<ProjectTree>(eventBus_);
        if (!projectTree_) throw std::runtime_error("项目树创建失败");
    } catch (const std::exception& e) {
        std::cerr << "[错误] 项目树初始化失败: " << e.what() << std::endl;
        throw;
    }

    try {
        std::cout << "[模块] 初始化输入处理器..." << std::endl;
        inputHandler_ = std::make_shared<InputHandler>(window_, configManager_, *eventBus_);
        if (!inputHandler_) throw std::runtime_error("输入处理器创建失败");
    } catch (const std::exception& e) {
        std::cerr << "[错误] 输入处理器初始化失败: " << e.what() << std::endl;
        throw;
    }

    try {
        std::cout << "[模块] 初始化程序化窗口..." << std::endl;
        proceduralWindow_ = std::make_shared<ProceduralWindow>(eventBus_, modelLoader_);
        if (!proceduralWindow_) throw std::runtime_error("程序化窗口创建失败");
    } catch (const std::exception& e) {
        std::cerr << "[错误] 程序化窗口初始化失败: " << e.what() << std::endl;
        throw;
    }

    try {
        std::cout << "[模块] 初始化动画模块..." << std::endl;
        animation_ = std::make_shared<Animation>(eventBus_);
        if (!animation_) throw std::runtime_error("动画模块创建失败");
    } catch (const std::exception& e) {
        std::cerr << "[错误] 动画模块初始化失败: " << e.what() << std::endl;
        throw;
    }

    try {
        std::cout << "[模块] 初始化项目管理器..." << std::endl;
        projectManager_ = std::make_shared<ProjectManager>(jsonSerializer_, eventBus_);
        if (!projectManager_) throw std::runtime_error("项目管理器创建失败");
    } catch (const std::exception& e) {
        std::cerr << "[错误] 项目管理器初始化失败: " << e.what() << std::endl;
        throw;
    }

    try {
        std::cout << "[模块] 初始化撤销重做管理器..." << std::endl;
        undoRedoManager_ = std::make_shared<UndoRedoManager>(eventBus_);
        if (!undoRedoManager_) throw std::runtime_error("撤销重做管理器创建失败");
    } catch (const std::exception& e) {
        std::cerr << "[错误] 撤销重做管理器初始化失败: " << e.what() << std::endl;
        throw;
    }

    try {
        std::cout << "[模块] 初始化动画管理器..." << std::endl;
        animationManager_ = std::make_shared<AnimationManager>(jsonSerializer_, eventBus_);
        if (!animationManager_) throw std::runtime_error("动画管理器创建失败");
    } catch (const std::exception& e) {
        std::cerr << "[错误] 动画管理器初始化失败: " << e.what() << std::endl;
        throw;
    }

    std::cout << "[模块] 所有模块初始化完成" << std::endl;
}

} // namespace MyRenderer