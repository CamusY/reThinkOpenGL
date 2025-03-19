// Window.h
#pragma once
#include <memory>
#include <GLFW/glfw3.h>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include "./Config/ConfigManager.h"
#include "./imgui-docking/imgui.h"
#include "./imgui-docking/imgui_internal.h" // 用于DockBuilder API

/**
 * @brief 主窗口管理类，负责窗口创建和布局管理
 * @note 通过依赖注入获取配置管理能力，严格遵循依赖注入原则
 */
class Window {
public:
    /**
     * @brief 构造函数注入配置管理器
     * @param configManager 配置管理器智能指针（必须非空）
     * @throws std::invalid_argument 当configManager为空时抛出
     */
    explicit Window(std::shared_ptr<ConfigManager> configManager);
    

    /**
     * @brief 初始化GLFW窗口和ImGui上下文
     * @throws std::runtime_error 初始化失败时抛出
     */
    void Initialize();

    /**
     * @brief 应用从配置文件加载的布局
     * @throws std::runtime_error 布局配置无效时抛出
     */
    void ApplyLayout();

    /**
     * @brief 主渲染循环
     */
    void Render();

    /**
     * @brief 清理资源
     */
    void Shutdown();

private:
    // 依赖组件
    std::shared_ptr<ConfigManager> configManager_;
    
    // 窗口资源
    struct GLFWwindowDeleter {
        void operator()(GLFWwindow* window) const;
    };
    std::unique_ptr<GLFWwindow, GLFWwindowDeleter> glfwWindow_;

    // 布局状态
    ImGuiID dockspaceID_;
    bool layoutApplied_ = false;

    /**
     * @brief 解析DockSpace节点配置
     * @param nodeStr 节点配置字符串，格式："父节点ID|分割方向,尺寸比例..."
     * @return 包含父节点、分割方向和尺寸比例的元组
     * @throws std::invalid_argument 解析失败时抛出
     */
    std::tuple<ImGuiID, ImGuiDir, float> ParseNodeConfig(const std::string& nodeStr);

    /**
     * @brief 创建默认安全布局（当配置加载失败时使用）
     */
    void CreateFallbackLayout();
};