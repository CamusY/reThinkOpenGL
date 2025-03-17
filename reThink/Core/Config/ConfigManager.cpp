// ConfigManager.cpp
#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <set>

using namespace nlohmann;

ConfigManager::ConfigManager(const std::string& layoutPath, const std::string& keymapPath)
    : layoutPath_(layoutPath), keymapPath_(keymapPath) {
    try {
        LoadLayoutConfig();
        LoadKeymapConfig();
    } catch (const std::exception& e) {
        throw std::runtime_error("配置初始化失败: " + std::string(e.what()));
    }
}

bool ConfigManager::Reload() {
    try {
        LoadLayoutConfig();
        LoadKeymapConfig();
        return true;
    } catch (...) {
        return false;
    }
}

void ConfigManager::LoadLayoutConfig() {
    std::ifstream file(layoutPath_);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开布局文件: " + layoutPath_);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    layoutConfig_ = buffer.str();
    
    // 基础格式验证
    if (layoutConfig_.find("[Window]") == std::string::npos) {
        throw std::runtime_error("无效的布局文件格式");
    }
}

void ConfigManager::LoadKeymapConfig() {
    std::ifstream file(keymapPath_);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开快捷键文件: " + keymapPath_);
    }

    json j;
    try {
        file >> j;
        ValidateKeymap(j);
    } catch (const json::exception& e) {
        throw std::runtime_error("JSON解析错误: " + std::string(e.what()));
    }

    keyMap_.clear();
    for (auto& [key, value] : j.items()) {
        keyMap_[key] = value.get<std::string>();
    }
}

void ConfigManager::ValidateKeymap(const json& j) {
    if (!j.is_object()) {
        throw std::runtime_error("快捷键配置必须为JSON对象");
    }
    
    const std::set<std::string> requiredKeys = {"save", "load", "compile"};
    for (const auto& key : requiredKeys) {
        if (!j.contains(key)) {
            throw std::runtime_error("缺少必需快捷键: " + key);
        }
    }
}