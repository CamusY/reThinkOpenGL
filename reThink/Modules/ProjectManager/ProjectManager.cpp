// ProjectManager.cpp
#include "ProjectManager.h"
#include <fstream>
#include <sstream>

using namespace nlohmann;

ProjectManager::ProjectManager(std::shared_ptr<JSONSerializer> jsonSerializer) 
    : jsonSerializer_(jsonSerializer) {
    if (!jsonSerializer_) {
        throw std::invalid_argument("JSONSerializer依赖不能为空");
    }
}

bool ProjectManager::CreateProject(const std::string& layoutName) {
    try {
        currentProject_ = InitializeNewProject(layoutName);
        return true;
    } catch (const std::exception&) {
        currentProject_.reset();
        return false;
    }
}

void ProjectManager::OpenProject(const fs::path& filePath) {
    std::lock_guard<std::mutex> lock(projectMutex_); // 自动加锁
    // 防御性检查
    ValidateProjectPath(filePath, true);

    try {
        // 反序列化项目数据
        auto data = jsonSerializer_->DeserializeFromFile<ProjectData>(filePath);
        currentProject_ = std::make_shared<ProjectData>(std::move(data));
        currentProject_->projectPath = filePath;
    } catch (const JsonSerializationException& e) {
        throw std::runtime_error(std::string("项目文件解析失败: ") + e.what());
    }
}

void ProjectManager::SaveProject() {
    if (!currentProject_) {
        throw std::runtime_error("无有效项目可保存");
    }

    if (currentProject_->projectPath.empty()) {
        throw std::runtime_error("项目路径未指定，请使用另存为");
    }

    SaveProjectAs(currentProject_->projectPath);
}

void ProjectManager::SaveProjectAs(const fs::path& filePath) {
    ValidateProjectPath(filePath, false);

    try {
        jsonSerializer_->SerializeToFile(*currentProject_, filePath);
        currentProject_->projectPath = filePath;
    } catch (const JsonSerializationException& e) {
        throw std::runtime_error(std::string("项目保存失败: ") + e.what());
    }
}

bool ProjectManager::IsProjectOpen() const {
    std::lock_guard<std::mutex> lock(projectMutex_);
    return currentProject_ != nullptr;
}

const ProjectData& ProjectManager::GetCurrentProjectData() const {
    std::lock_guard<std::mutex> lock(projectMutex_);
    if (!currentProject_) {
        throw std::runtime_error("无有效项目数据");
    }
    return *currentProject_;
}

void ProjectManager::ValidateProjectPath(const fs::path& path, bool requireExists) const {
    if (path.empty()) {
        throw std::invalid_argument("项目路径不能为空");
    }

    if (path.extension() != ".proj") {
        throw std::invalid_argument("项目文件必须使用.proj扩展名");
    }

    if (requireExists && !fs::exists(path)) {
        throw std::invalid_argument("项目文件不存在: " + path.string());
    }
}

std::shared_ptr<ProjectData> ProjectManager::InitializeNewProject(
    const std::string& layoutName) const 
{
    auto project = std::make_shared<ProjectData>();
    project->layoutName = layoutName;
    project->projectPath.clear();
    return project;
}