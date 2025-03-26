// InputHandler.cpp
#include "InputHandler.h"
#include <algorithm>
#include <sstream>
#include "imgui.h"

using namespace std;

InputHandler::InputHandler(shared_ptr<EventBus> eventBus,
                           shared_ptr<ConfigManager> configManager)
    : eventBus_(move(eventBus)), configManager_(move(configManager)) {
    
    // 防御性检查
    if (!eventBus_ || !configManager_) {
        throw invalid_argument("EventBus和ConfigManager依赖不能为空");
    }

    // 初始化工具操作映射
    toolOperations_ = {
        {"G", EventTypes::TransformToolEvent::Operation::TRANSLATE},
        {"R", EventTypes::TransformToolEvent::Operation::ROTATE},
        {"S", EventTypes::TransformToolEvent::Operation::SCALE}
    };

    try {
        LoadKeymapConfig();
    } catch (const exception& e) {
        throw runtime_error("快捷键配置加载失败: " + string(e.what()));
    }
}

void InputHandler::Update(GLFWwindow* window) {
    // 防御性检查
    if (!window) return;

    UpdateModifierKeys(window);
    
    // 轮询所有按键状态
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
        if (glfwGetKey(window, key) == GLFW_PRESS) {
            ProcessKeyEvent(key, GLFW_PRESS);
        }
    }

    // 处理视口焦点事件
    static bool lastFocusState = false;
    if (isViewportFocused_ != lastFocusState) {
        EventTypes::ViewportFocusEvent event;
        event.viewportID = focusedViewportID_;
        event.isFocused = isViewportFocused_;
        eventBus_->Publish(event);
        lastFocusState = isViewportFocused_;
    }
}

void InputHandler::SetViewportFocus(const string& viewportID, bool focused) {
    focusedViewportID_ = viewportID;
    isViewportFocused_ = focused;
}

void InputHandler::LoadKeymapConfig() {
    // 从ConfigManager获取配置
    const auto& config = configManager_->GetKeymapConfig();
    
    try {
        // 加载核心快捷键
        const auto& core = config.at("Core");
        keymapConfig_["SaveProject"] = core.at("SaveProject");
        keymapConfig_["OpenProject"] = core.at("OpenProject");
        keymapConfig_["NewProject"] = core.at("NewProject");

        // 加载编辑器快捷键
        const auto& editor = config.at("Editor");
        keymapConfig_["Translate"] = editor.at("Translate");
        keymapConfig_["Rotate"] = editor.at("Rotate");
        keymapConfig_["Scale"] = editor.at("Scale");
        keymapConfig_["ToggleEditMode"] = editor.at("ToggleEditMode");

        // 加载视口快捷键
        const auto& viewport = config.at("Viewport");
        keymapConfig_["FocusViewport"] = viewport.at("FocusViewport");
    } catch (const exception& e) {
        throw runtime_error("快捷键配置解析错误: " + string(e.what()));
    }
}

void InputHandler::ProcessKeyEvent(int key, int action) {
    if (action != GLFW_PRESS) return;

    // 转换键值到字符串表示
    string keyStr = KeyToString(key);
    if (keyStr.empty()) return;

    // 检查工具操作快捷键
    if (toolOperations_.count(keyStr)) {
        PublishToolEvent(toolOperations_.at(keyStr));
        return;
    }

    // 检查其他快捷键
    CheckShortcuts(key);
}

void InputHandler::CheckShortcuts(int key) {
    auto currentKey = KeyToString(key);
    if (currentKey.empty()) return;

    // 构建当前按键组合字符串
    stringstream shortcut;
    if (currentModifiers_.ctrl) shortcut << "Ctrl+";
    if (currentModifiers_.shift) shortcut << "Shift+";
    if (currentModifiers_.alt) shortcut << "Alt+";
    shortcut << currentKey;

    // 匹配配置文件
    for (const auto& [action, configShortcut] : keymapConfig_) {
        auto parsed = ParseShortcut(configShortcut);
        bool ctrlMatch = parsed.ctrl == currentModifiers_.ctrl;
        bool shiftMatch = parsed.shift == currentModifiers_.shift;
        bool altMatch = parsed.alt == currentModifiers_.alt;
        bool keyMatch = parsed.mainKey == key;

        if (ctrlMatch && shiftMatch && altMatch && keyMatch) {
            if (action == "ToggleEditMode") {
                EventTypes::EditModeToggleEvent event;
                eventBus_->Publish(event);
            }
            // 其他事件处理...
        }
    }
}

string InputHandler::KeyToString(int key) {
    // 处理字母键
    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
        return string(1, 'A' + (key - GLFW_KEY_A));
    }

    // 处理数字键
    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
        return to_string(key - GLFW_KEY_0);
    }

    // 特殊键处理
    switch (key) {
        case GLFW_KEY_TAB: return "Tab";
        case GLFW_KEY_F11: return "F11";
        case GLFW_KEY_G: return "G";
        case GLFW_KEY_R: return "R";
        case GLFW_KEY_S: return "S";
        // 添加其他需要的键...
        default: return "";
    }
}

InputHandler::ParsedShortcut InputHandler::ParseShortcut(const string& shortcut) const {
    ParsedShortcut result;
    istringstream iss(shortcut);
    string part;

    while (getline(iss, part, '+')) {
        transform(part.begin(), part.end(), part.begin(), ::toupper);
        
        if (part == "CTRL") {
            result.ctrl = true;
        } else if (part == "SHIFT") {
            result.shift = true;
        } else if (part == "ALT") {
            result.alt = true;
        } else {
            // 转换主键
            if (part == "TAB") result.mainKey = GLFW_KEY_TAB;
            else if (part == "G") result.mainKey = GLFW_KEY_G;
            else if (part == "R") result.mainKey = GLFW_KEY_R;
            else if (part == "S") result.mainKey = GLFW_KEY_S;
            // 添加其他键转换...
        }
    }
    return result;
}

void InputHandler::PublishToolEvent(EventTypes::TransformToolEvent::Operation operation) {
    EventTypes::TransformToolEvent event;
    event.operation = operation;
    eventBus_->Publish(event);
}

void InputHandler::UpdateModifierKeys(GLFWwindow* window) {
    currentModifiers_.ctrl = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                            glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
    currentModifiers_.shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                             glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    currentModifiers_.alt = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
                           glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
}