// InputHandler.h
#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include <functional>
#include <GLFW/glfw3.h>
#include "EventBus/EventBus.h"
#include "EventBus/EventTypes.h"
#include "Config/ConfigManager.h"

/**
 * @brief 输入处理模块，负责管理键盘鼠标输入和快捷键处理
 * @dependency 强制依赖 EventBus 和 ConfigManager
 */
class InputHandler {
public:
    /**
     * @brief 构造函数注入必要依赖
     * @param eventBus 事件总线实例（必须非空）
     * @param configManager 配置管理器实例（必须非空）
     * @throws std::invalid_argument 参数为空时抛出
     */
    InputHandler(std::shared_ptr<EventBus> eventBus,
                 std::shared_ptr<ConfigManager> configManager);

    /**
     * @brief 每帧更新输入状态（应在主循环中调用）
     * @param window GLFW窗口指针（用于获取输入状态）
     */
    void Update(GLFWwindow* window);

    /**
     * @brief 设置视口焦点状态
     * @param viewportID 视口标识符
     * @param focused 是否获得焦点
     */
    void SetViewportFocus(const std::string& viewportID, bool focused);

private:
    // 依赖组件
    std::shared_ptr<EventBus> eventBus_;
    std::shared_ptr<ConfigManager> configManager_;

    // 输入状态
    struct ModifierKeys {
        bool ctrl = false;
        bool shift = false;
        bool alt = false;
    } currentModifiers_;

    // 快捷键映射配置
    std::unordered_map<std::string, std::string> keymapConfig_;
    std::unordered_map<std::string, EventTypes::TransformToolEvent::Operation> toolOperations_;

    // 当前焦点视口
    std::string focusedViewportID_;
    bool isViewportFocused_ = false;

    /**
     * @brief 加载快捷键配置
     * @throws std::runtime_error 配置加载失败时抛出
     */
    void LoadKeymapConfig();

    /**
     * @brief 处理单个键盘事件
     * @param key GLFW键码
     * @param action 按键动作（按下/释放）
     */
    void ProcessKeyEvent(int key, int action);

    /**
     * @brief 处理快捷键匹配
     * @param key 按下的主键
     */
    void CheckShortcuts(int key);

    /**
     * @brief 将GLFW键码转换为可读字符串
     * @param key GLFW键码
     * @return 标准化键名字符串
     */
    static std::string KeyToString(int key);

    /**
     * @brief 解析快捷键字符串
     * @param shortcut 快捷键字符串（如"Ctrl+S"）
     * @return 包含修饰键和主键的结构体
     */
    struct ParsedShortcut {
        bool ctrl = false;
        bool shift = false;
        bool alt = false;
        int mainKey = 0;
    };
    ParsedShortcut ParseShortcut(const std::string& shortcut) const;

    /**
     * @brief 发布工具切换事件
     * @param operation 工具操作类型
     */
    void PublishToolEvent(EventTypes::TransformToolEvent::Operation operation);

    /**
     * @brief 更新修饰键状态
     * @param window GLFW窗口指针
     */
    void UpdateModifierKeys(GLFWwindow* window);
};