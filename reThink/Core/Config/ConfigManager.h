// ConfigManager.h
#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "./imgui-docking/imgui.h"

/**
 * @brief 配置文件管理模块
 * @note 使用依赖注入设计，不采用单例模式
 */
class ConfigManager {
public:
    /**
     * @brief 构造函数加载配置文件
     * @param layoutPath 布局配置文件路径
     * @param keymapPath 快捷键配置文件路径
     * @throws std::runtime_error 文件加载失败或格式错误
     */
    ConfigManager(const std::string& layoutPath, const std::string& keymapPath);
    
    /**
     * @brief 重新加载所有配置文件
     * @return 是否成功重新加载
     */
    bool Reload();

    // 布局配置访问接口
    const std::string& GetLayoutConfig() const { return layoutConfig_; }
    
    // 快捷键配置访问接口
    const std::unordered_map<std::string, std::string>& GetKeyMap() const { return keyMap_; }

private:
    std::string layoutPath_;
    std::string keymapPath_;
    std::string layoutConfig_;  // 原始INI配置内容
    std::unordered_map<std::string, std::string> keyMap_;  // 快捷键映射

    void LoadLayoutConfig();
    void LoadKeymapConfig();
    void ValidateKeymap(const nlohmann::json& j);
};