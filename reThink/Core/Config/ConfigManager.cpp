// ConfigManager.cpp
#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace MyRenderer {

const std::string ConfigManager::LAYOUT_CONFIG_DIR = "./Core/Config/";
const std::string ConfigManager::KEYMAP_CONFIG_PATH = "./Core/Config/keymap_config.json";

ConfigManager::ConfigManager() : errorCallback_(nullptr), currentLayoutPath_(LAYOUT_CONFIG_DIR + "layout_config.ini") {}

ConfigManager::~ConfigManager() {}

bool ConfigManager::LoadConfig() {
    bool success = true;
    success &= LoadLayoutConfig(currentLayoutPath_);
    success &= LoadKeymapConfig();
    return success;
}

bool ConfigManager::SaveConfig() {
    bool success = true;
    success &= SaveLayoutConfig(currentLayoutPath_);
    success &= SaveKeymapConfig();
    return success;
}

void ConfigManager::SetLayoutConfigValue(const std::string& windowId, const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(configMutex_);
    if (windowId == "DockSpace") {
        if (key == "id") layoutConfig_.dockSpaceId = value;
        else if (key == "size") {
            sscanf_s(value.c_str(), "%d,%d", &layoutConfig_.dockSpaceWidth, &layoutConfig_.dockSpaceHeight);
        }
    } else {
        auto it = std::find_if(layoutConfig_.windows.begin(), layoutConfig_.windows.end(),
            [&](const WindowConfig& w) { return w.id == windowId; });
        if (it != layoutConfig_.windows.end()) {
            if (key == "visible") it->visible = (value == "true");
            else if (key == "pos") sscanf_s(value.c_str(), "%d,%d", &it->posX, &it->posY);
            else if (key == "size") sscanf_s(value.c_str(), "%d,%d", &it->width, &it->height);
            else if (key == "dock") it->dockId = value;
            else if (key == "dock_side") it->dockSide = value;
            else if (key == "floating") it->floating = (value == "true");
            else if (key == "fixed") it->fixed = (value == "true");
        } else {
            ReportError("Window ID '" + windowId + "' not found for setting '" + key + "'");
            return;
        }
    }
    SaveLayoutConfig(currentLayoutPath_);
}

const LayoutConfig& ConfigManager::GetLayoutConfig() const {
    return layoutConfig_;
}

const KeymapConfig& ConfigManager::GetKeymapConfig() const {
    return keymapConfig_;
}

void ConfigManager::SetErrorCallback(ConfigErrorCallback callback) {
    std::lock_guard<std::mutex> lock(configMutex_);
    errorCallback_ = std::move(callback);
}

bool ConfigManager::LoadLayout(const std::string& layoutName) {
    std::string path = LAYOUT_CONFIG_DIR + layoutName + ".ini";
    if (LoadLayoutConfig(path)) {
        currentLayoutPath_ = path;
        return true;
    }
    return false;
}

bool ConfigManager::SaveCurrentLayout(const std::string& layoutName) {
    std::string path = LAYOUT_CONFIG_DIR + layoutName + ".ini";
    if (SaveLayoutConfig(path)) {
        currentLayoutPath_ = path;
        return true;
    }
    return false;
}

bool ConfigManager::LoadLayoutConfig(const std::string& path) {
    std::lock_guard<std::mutex> lock(configMutex_);
    INIReader reader(path);
    if (reader.ParseError() != 0) {
        ReportError("Error loading layout config from " + path + 
                    ": Parse error code " + std::to_string(reader.ParseError()));
        return false;
    }

    layoutConfig_.windows.clear();
    layoutConfig_.dockSpaceId = reader.Get("DockSpace", "id", "MainDockSpace");
    std::string sizeStr = reader.Get("DockSpace", "size", "1920,1080");
    sscanf_s(sizeStr.c_str(), "%d,%d", &layoutConfig_.dockSpaceWidth, &layoutConfig_.dockSpaceHeight);

    std::vector<std::string> windowSections = {
        "Window_ControlPanel", "Window_SceneViewport",
        //"Window_ShaderEditor",
        "Window_ProjectTree", "Window_MenuBar", "Window_Animation", "Window_ProceduralWindow"
    };

    for (const auto& section : windowSections) {
        if (reader.HasSection(section)) {
            WindowConfig window;
            window.id = reader.Get(section, "id", section.substr(7));
            window.visible = reader.GetBoolean(section, "visible", true);
            std::string posStr = reader.Get(section, "pos", "0,0");
            sscanf_s(posStr.c_str(), "%d,%d", &window.posX, &window.posY);
            std::string sizeStr = reader.Get(section, "size", "400,400");
            sscanf_s(sizeStr.c_str(), "%d,%d", &window.width, &window.height);
            window.dockId = reader.Get(section, "dock", "MainDockSpace");
            window.dockSide = reader.Get(section, "dock_side", "center");
            window.floating = reader.GetBoolean(section, "floating", false);
            window.fixed = reader.GetBoolean(section, "fixed", false);
            layoutConfig_.windows.push_back(window);
        }
    }

    return true;
}

bool ConfigManager::SaveLayoutConfig(const std::string& path) {
    std::lock_guard<std::mutex> lock(configMutex_);
    std::ofstream file(path);
    if (!file.is_open()) {
        ReportError("Failed to open layout config file for writing: " + path);
        return false;
    }

    file << "[DockSpace]\n";
    file << "id=" << layoutConfig_.dockSpaceId << "\n";
    file << "size=" << layoutConfig_.dockSpaceWidth << "," << layoutConfig_.dockSpaceHeight << "\n\n";

    for (const auto& window : layoutConfig_.windows) {
        std::string section = "Window_" + window.id;
        file << "[" << section << "]\n";
        file << "id=" << window.id << "\n";
        file << "visible=" << (window.visible ? "true" : "false") << "\n";
        file << "pos=" << window.posX << "," << window.posY << "\n";
        file << "size=" << window.width << "," << window.height << "\n";
        file << "dock=" << window.dockId << "\n";
        file << "dock_side=" << window.dockSide << "\n";
        file << "floating=" << (window.floating ? "true" : "false") << "\n";
        file << "fixed=" << (window.fixed ? "true" : "false") << "\n\n";
    }

    file.close();
    if (file.fail()) {
        ReportError("Error writing to layout config file: " + path);
        return false;
    }
    return true;
}

bool ConfigManager::LoadKeymapConfig() {
    std::lock_guard<std::mutex> lock(configMutex_);
    std::ifstream file(KEYMAP_CONFIG_PATH);
    if (!file.is_open()) {
        ReportError("Error opening keymap config file: " + KEYMAP_CONFIG_PATH);
        return false;
    }

    nlohmann::json json;
    try {
        file >> json;
    } catch (const nlohmann::json::exception& e) {
        ReportError("Error parsing keymap config from " + KEYMAP_CONFIG_PATH + ": " + e.what());
        file.close();
        return false;
    }
    file.close();

    keymapConfig_.shortcuts.clear();
    if (json.contains("shortcuts") && json["shortcuts"].is_object()) {
        for (auto& [key, value] : json["shortcuts"].items()) {
            if (value.is_string()) {
                keymapConfig_.shortcuts[key] = value.get<std::string>();
            } else {
                ReportError("Invalid value for shortcut '" + key + "' in " + KEYMAP_CONFIG_PATH + ": expected string");
            }
        }
    } else {
        ReportError("Missing or invalid 'shortcuts' object in " + KEYMAP_CONFIG_PATH);
        return false;
    }
    return true;
}

bool ConfigManager::SaveKeymapConfig() {
    std::lock_guard<std::mutex> lock(configMutex_);
    std::ofstream file(KEYMAP_CONFIG_PATH);
    if (!file.is_open()) {
        ReportError("Failed to open keymap config file for writing: " + KEYMAP_CONFIG_PATH);
        return false;
    }

    nlohmann::json json;
    json["shortcuts"] = nlohmann::json::object();
    for (const auto& [name, key] : keymapConfig_.shortcuts) {
        json["shortcuts"][name] = key;
    }

    try {
        file << json.dump(4);
    } catch (const nlohmann::json::exception& e) {
        ReportError("Error serializing keymap config to " + KEYMAP_CONFIG_PATH + ": " + e.what());
        file.close();
        return false;
    }

    file.close();
    if (file.fail()) {
        ReportError("Error writing to keymap config file: " + KEYMAP_CONFIG_PATH);
        return false;
    }
    return true;
}

void ConfigManager::ReportError(const std::string& message) {
    std::cerr << "[ConfigManager Error] " << message << std::endl;
    if (errorCallback_) {
        errorCallback_(message);
    }
}

} // namespace MyRenderer