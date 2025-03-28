// ConfigManager.h
#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <string>
#include <vector>
#include <mutex>
#include <functional>
#include "INIReader.h"
#include "nlohmann/json.hpp"

namespace MyRenderer {

/**
 * @brief 窗口配置结构体，表示一个ImGui窗口的布局
 */
struct WindowConfig {
    std::string id;
    bool visible = true;
    int posX = 0, posY = 0;
    int width = 400, height = 400;
    std::string dockId; // 停靠的DockSpace ID
    std::string dockSide; // 停靠方向：left, right, top, bottom, center
    bool floating = false; // 是否浮动
    bool fixed = false; // 是否固定（如MenuBar）
};

/**
 * @brief 布局配置结构体，存储layout_config.ini中的配置
 */
struct LayoutConfig {
    std::string dockSpaceId = "MainDockSpace";
    int dockSpaceWidth = 1920, dockSpaceHeight = 1080;
    std::vector<WindowConfig> windows; // 所有窗口配置
};

/**
 * @brief 快捷键配置结构体，存储keymap_config.json中的映射
 */
struct KeymapConfig {
    std::map<std::string, std::string> shortcuts;
};

/**
 * @brief 配置加载错误回调函数类型
 */
using ConfigErrorCallback = std::function<void(const std::string&)>;

/**
 * @brief 配置管理器类，负责加载和管理配置文件
 */
class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();

    bool LoadConfig();
    bool SaveConfig();

    void SetLayoutConfigValue(const std::string& windowId, const std::string& key, const std::string& value);
    const LayoutConfig& GetLayoutConfig() const;
    const KeymapConfig& GetKeymapConfig() const;

    void SetErrorCallback(ConfigErrorCallback callback);

    // 支持多布局切换
    bool LoadLayout(const std::string& layoutName);
    bool SaveCurrentLayout(const std::string& layoutName);

private:
    bool LoadLayoutConfig(const std::string& path);
    bool SaveLayoutConfig(const std::string& path);
    bool LoadKeymapConfig();
    bool SaveKeymapConfig();

    void ReportError(const std::string& message);

    LayoutConfig layoutConfig_;
    KeymapConfig keymapConfig_;
    std::mutex configMutex_;
    ConfigErrorCallback errorCallback_;

    static const std::string LAYOUT_CONFIG_DIR; // 布局配置目录
    static const std::string KEYMAP_CONFIG_PATH; // 快捷键配置文件路径
    std::string currentLayoutPath_; // 当前加载的布局路径
};

} // namespace MyRenderer

#endif // CONFIGMANAGER_H