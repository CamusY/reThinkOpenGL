#include "InputHandler.h"
#include <algorithm>
#include <sstream>

namespace MyRenderer {
using namespace Events;
InputHandler::InputHandler(GLFWwindow* window, ConfigManager& configManager, EventBus& eventBus)
    : window_(window), configManager_(configManager), eventBus_(eventBus), 
      currentMode_(OperationModeChangedEvent::Mode::Object) {
    // 初始化按键状态映射
    keyState_["Ctrl"] = GLFW_KEY_LEFT_CONTROL;
    keyState_["Shift"] = GLFW_KEY_LEFT_SHIFT;
    keyState_["Alt"] = GLFW_KEY_LEFT_ALT;
    keyState_["Tab"] = GLFW_KEY_TAB;
    keyState_["G"] = GLFW_KEY_G;
    keyState_["R"] = GLFW_KEY_R;
    keyState_["S"] = GLFW_KEY_S;
    keyState_["Z"] = GLFW_KEY_Z;
    keyState_["Y"] = GLFW_KEY_Y;
    keyState_["F"] = GLFW_KEY_F;
}

void InputHandler::Update() {
    if (!window_ || focusedWindow_.empty()) return;

    const auto& keymap = configManager_.GetKeymapConfig().shortcuts;

    // 检查编辑模式切换 (Tab)
    if (keymap.count("toggle_edit_mode") && IsShortcutPressed(keymap.at("toggle_edit_mode"))) {
        switch (currentMode_) {
            case OperationModeChangedEvent::Mode::Object:
                currentMode_ = OperationModeChangedEvent::Mode::Vertex;
                break;
            case OperationModeChangedEvent::Mode::Vertex:
                currentMode_ = OperationModeChangedEvent::Mode::Edge;
                break;
            case OperationModeChangedEvent::Mode::Edge:
                currentMode_ = OperationModeChangedEvent::Mode::Face;
                break;
            case OperationModeChangedEvent::Mode::Face:
                currentMode_ = OperationModeChangedEvent::Mode::Object;
                break;
        }
        eventBus_.Publish(OperationModeChangedEvent{currentMode_});
    }

    // 检查变换工具快捷键
    if (keymap.count("translate_tool") && IsShortcutPressed(keymap.at("translate_tool"))) {
        eventBus_.Publish(TransformToolEvent{TransformToolEvent::Tool::Translate});
    }
    if (keymap.count("rotate_tool") && IsShortcutPressed(keymap.at("rotate_tool"))) {
        eventBus_.Publish(TransformToolEvent{TransformToolEvent::Tool::Rotate});
    }
    if (keymap.count("scale_tool") && IsShortcutPressed(keymap.at("scale_tool"))) {
        eventBus_.Publish(TransformToolEvent{TransformToolEvent::Tool::Scale});
    }

    // 检查撤销/重做
    if (keymap.count("undo") && IsShortcutPressed(keymap.at("undo"))) {
        eventBus_.Publish(UndoRedoEvent{UndoRedoEvent::Action::Undo});
    }
    if (keymap.count("redo") && IsShortcutPressed("redo")) {
        eventBus_.Publish(UndoRedoEvent{UndoRedoEvent::Action::Redo});
    }

    // 检查聚焦对象
    if (keymap.count("focus_object") && IsShortcutPressed(keymap.at("focus_object"))) {
        eventBus_.Publish(ViewportFocusEvent{true});
    }
}

void InputHandler::SetFocusedWindow(const std::string& windowName) {
    if (focusedWindow_ != windowName) {
        focusedWindow_ = windowName;
        eventBus_.Publish(ViewportFocusEvent{!focusedWindow_.empty()});
    }
}

std::string InputHandler::GetFocusedWindow() const {
    return focusedWindow_;
}

bool InputHandler::IsShortcutPressed(const std::string& shortcut) const {
    std::stringstream ss(shortcut);
    std::string token;
    bool ctrl = false, shift = false, alt = false;
    std::string key;

    // 解析快捷键字符串，例如 "Ctrl+Z"
    while (std::getline(ss, token, '+')) {
        if (token == "Ctrl") ctrl = true;
        else if (token == "Shift") shift = true;
        else if (token == "Alt") alt = true;
        else key = token;
    }

    // 检查修饰键状态
    if (ctrl && glfwGetKey(window_, GLFW_KEY_LEFT_CONTROL) != GLFW_PRESS) return false;
    if (shift && glfwGetKey(window_, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS) return false;
    if (alt && glfwGetKey(window_, GLFW_KEY_LEFT_ALT) != GLFW_PRESS) return false;

    // 检查主按键
    auto it = keyState_.find(key);
    if (it != keyState_.end()) {
        return glfwGetKey(window_, it->second) == GLFW_PRESS;
    }

    return false;
}

} // namespace MyRenderer