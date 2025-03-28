#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <GLFW/glfw3.h>
#include <string>
#include <map>
#include "EventBus/EventBus.h"
#include "Config/ConfigManager.h"
#include "EventBus/EventTypes.h"

namespace MyRenderer {

    /**
     * @brief InputHandler 负责处理用户输入并发布相关事件。
     */
    class InputHandler {
    public:
        /**
         * @brief 构造函数，初始化 GLFW 窗口、配置管理器和事件总线。
         * @param window GLFW 窗口指针。
         * @param configManager 配置管理器实例。
         * @param eventBus 事件总线实例。
         */
        InputHandler(GLFWwindow* window, ConfigManager& configManager, EventBus& eventBus);

        /**
         * @brief 析构函数。
         */
        ~InputHandler() = default;

        /**
         * @brief 更新输入状态，检测按键并发布事件。
         */
        void Update();

        /**
         * @brief 设置当前聚焦的窗口。
         * @param windowName 窗口名称。
         */
        void SetFocusedWindow(const std::string& windowName);

        /**
         * @brief 获取当前聚焦的窗口名称。
         * @return 当前聚焦窗口的名称。
         */
        std::string GetFocusedWindow() const;

    private:
        /**
         * @brief 检查快捷键是否被按下。
         * @param shortcut 快捷键字符串（例如 "Ctrl+Z"）。
         * @return 是否按下。
         */
        bool IsShortcutPressed(const std::string& shortcut) const;

        GLFWwindow* window_;                     // GLFW 窗口指针
        ConfigManager& configManager_;           // 配置管理器引用
        EventBus& eventBus_;                     // 事件总线引用
        std::string focusedWindow_;              // 当前聚焦的窗口名称
        std::map<std::string, int> keyState_;    // 按键状态缓存，避免重复检测
        Events::OperationModeChangedEvent::Mode currentMode_; // 当前操作模式
    };

} // namespace MyRenderer

#endif // INPUT_HANDLER_H