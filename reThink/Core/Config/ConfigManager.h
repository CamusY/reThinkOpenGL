// ConfigManager.h
#pragma once
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <filesystem>

namespace fs = std::filesystem;

class ConfigManager {
public:
    // 配置数据结构定义
    using LayoutConfig = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;
    using KeymapConfig = nlohmann::json;
    void SetLayoutConfigValue(const std::string& section, const std::string& key, const std::string& value);
    void LoadLayoutConfig(const fs::path& path);

    /**
     * @brief 加载所有配置文件
     * @throws std::runtime_error 配置文件加载或解析失败时抛出
     */
    void LoadConfig();

    /**
     * @brief 获取布局配置数据
     * @return 包含所有布局配置的嵌套字典
     */
    const LayoutConfig& GetLayoutConfig() const noexcept { return layout_config_; }

    /**
     * @brief 获取快捷键配置数据
     * @return 包含快捷键映射的JSON对象
     */
    const KeymapConfig& GetKeymapConfig() const noexcept { return keymap_config_; }

private:
    // 配置文件路径
    static const inline fs::path LAYOUT_PATH = "config/layout_config.ini";
    static const inline fs::path KEYMAP_PATH = "config/keymap_config.json";

    // 配置数据存储
    LayoutConfig layout_config_;
    KeymapConfig keymap_config_;

    // 内部解析方法
    void ParseLayoutConfig(const std::string& content);
    void ParseKeymapConfig(const std::string& content);
    std::string LoadFileContent(const fs::path& path);
};