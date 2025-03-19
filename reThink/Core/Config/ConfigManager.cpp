// ConfigManager.cpp
#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

using namespace std;

void ConfigManager::SetLayoutConfigValue(const std::string& section, const std::string& key, const std::string& value) {
    layout_config_[section][key] = value;
}

void ConfigManager::LoadLayoutConfig(const fs::path& path) {
    layout_config_.clear();
    string content = LoadFileContent(path);
    ParseLayoutConfig(content);
}

void ConfigManager::LoadConfig() {
    try {
        // 加载布局配置
        string layoutContent = LoadFileContent(LAYOUT_PATH);
        ParseLayoutConfig(layoutContent);

        // 加载快捷键配置
        string keymapContent = LoadFileContent(KEYMAP_PATH);
        ParseKeymapConfig(keymapContent);
    } catch (const exception& e) {
        throw runtime_error("配置加载失败: " + string(e.what()));
    }
}

string ConfigManager::LoadFileContent(const fs::path& path) {
    // 检查文件是否存在
    if (!fs::exists(path)) {
        throw runtime_error("配置文件不存在: " + path.string());
    }

    // 读取文件内容
    ifstream file(path);
    if (!file.is_open()) {
        throw runtime_error("无法打开文件: " + path.string());
    }

    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void ConfigManager::ParseLayoutConfig(const string& content) {
    layout_config_.clear();
    string current_section;
    
    istringstream iss(content);
    string line;
    
    while (getline(iss, line)) {
        // 去除前后空白和注释
        line = line.substr(0, line.find_first_of(";#"));
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.empty()) continue;

        // 处理节头
        if (line.front() == '[' && line.back() == ']') {
            current_section = line.substr(1, line.size() - 2);
            layout_config_[current_section];
            continue;
        }

        // 处理键值对
        size_t delimiter = line.find('=');
        if (delimiter != string::npos && !current_section.empty()) {
            string key = line.substr(0, line.find_last_not_of(" \t", delimiter - 1) + 1);
            string value = line.substr(delimiter + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            layout_config_[current_section][key] = value;
        }
    }
}

void ConfigManager::ParseKeymapConfig(const string& content) {
    try {
        keymap_config_ = nlohmann::json::parse(content);
    } catch (const nlohmann::json::exception& e) {
        throw runtime_error("JSON解析错误: " + string(e.what()));
    }
}