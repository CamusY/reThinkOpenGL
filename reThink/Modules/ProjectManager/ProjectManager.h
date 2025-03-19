// ProjectManager.h
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <filesystem>
#include <mutex>

#include "./Utils/JSONSerializer.h"

namespace fs = std::filesystem;

/**
 * @brief 项目数据结构体，包含需要持久化的项目信息
 * @note 项目文件为 JSON 格式，使用 JSONSerializer 进行序列化和反序列化
 * @param layoutName 当前窗口布局配置名称
 * @param models 模型列表
 * @param projectPath 项目文件路径
 */
struct ProjectData {
    /**
     * @brief 模型数据结构体，包含模型文件路径、变换矩阵和关联着色器路径列表
     * @note 模型路径为相对路径，相对于项目文件所在目录
     * @note 着色器路径为相对路径，相对于项目文件所在目录
     * @note 模型变换矩阵为列主序矩阵
     * @param modelPath 模型文件路径
     * @param transformMatrix 模型变换矩阵
     * @param shaders 关联着色器路径列表
     */
    struct ModelInfo {
        std::string modelPath;
        glm::mat4 transformMatrix;
        std::vector<std::string> shaders;
    };

    std::string layoutName;
    std::vector<ModelInfo> models;
    fs::path projectPath;
};

/**
 * @brief 项目管理器类，负责项目文件的生命周期管理
 * @note 通过依赖注入 JSONSerializer 实现序列化功能
 */
class ProjectManager {
public:
    /**
     * @brief 构造函数注入 JSONSerializer 依赖
     * @param jsonSerializer JSON序列化器智能指针（必须非空）
     * @throws std::invalid_argument 当jsonSerializer为空时抛出
     */
    explicit ProjectManager(std::shared_ptr<JSONSerializer> jsonSerializer);

    /**
     * @brief 创建新项目
     * @param layoutName 初始布局配置名称
     * @return 是否创建成功
     */
    bool CreateProject(const std::string& layoutName);

    /**
     * @brief 打开项目文件
     * @param filePath 项目文件路径（.proj扩展名）
     * @throws std::runtime_error 文件操作或反序列化失败时抛出
     */
    void OpenProject(const fs::path& filePath);

    /**
     * @brief 保存当前项目
     * @throws std::runtime_error 未创建/加载项目或保存失败时抛出
     */
    void SaveProject();

    /**
     * @brief 另存项目到指定路径
     * @param filePath 新项目文件路径
     * @throws std::runtime_error 保存失败时抛出
     */
    void SaveProjectAs(const fs::path& filePath);

    /**
     * @brief 获取当前项目数据（只读）
     * @return 当前项目数据的常量引用
     * @throws std::runtime_error 无有效项目时抛出
     */
    const ProjectData& GetCurrentProjectData() const;

    bool IsProjectOpen() const noexcept;


private:
    
    // 依赖组件
    std::shared_ptr<JSONSerializer> jsonSerializer_;

    mutable std::mutex projectMutex_; // 添加互斥量
    // 当前项目数据
    std::shared_ptr<ProjectData> currentProject_;

    /**
     * @brief 验证项目文件路径有效性
     * @param path 文件路径
     * @param requireExists 是否要求文件必须存在
     * @throws std::invalid_argument 路径无效时抛出
     */
    void ValidateProjectPath(const fs::path& path, bool requireExists = true) const;

    /**
     * @brief 初始化新项目数据结构
     * @param layoutName 布局名称
     * @return 初始化后的项目数据指针
     */
    std::shared_ptr<ProjectData> InitializeNewProject(const std::string& layoutName) const;
};

// JSON序列化特化声明（在.cpp中实现）
namespace nlohmann {
    template<> struct adl_serializer<ProjectData::ModelInfo> {
        static void to_json(json& j, const ProjectData::ModelInfo& model) {
            j = json{
                    {"model_path", model.modelPath},
                    {"transform", model.transformMatrix},
                    {"shaders", model.shaders}
            };
        }

        static void from_json(const json& j, ProjectData::ModelInfo& model) {
            j.at("model_path").get_to(model.modelPath);
            j.at("transform").get_to(model.transformMatrix);
            j.at("shaders").get_to(model.shaders);
        }
    };

    template<> struct adl_serializer<ProjectData> {
        static void to_json(json& j, const ProjectData& project) {
            j = json{
                    {"metadata", {
                        {"layout_name", project.layoutName},
                        {"version", "1.0.0"}
                    }},
                    {"models", project.models}
            };
        }

        static void from_json(const json& j, ProjectData& project) {
            const auto& meta = j.at("metadata");
            meta.at("layout_name").get_to(project.layoutName);
            j.at("models").get_to(project.models);
        }
    };
}