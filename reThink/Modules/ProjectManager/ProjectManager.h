// ProjectManager.h
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <filesystem>
#include <mutex>
#include "../Core/EventBus/EventBus.h"
#include "./Utils/JSONSerializer.h"
#include "../Core/EventBus/EventTypes.h"
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace MyRenderer {

/**
 * @brief 项目数据结构体，包含需要持久化的项目信息
 * @note 项目文件为 JSON 格式，使用 JSONSerializer 进行序列化和反序列化
 */
struct ProjectData {
    std::string projectName;                    // 项目名称
    fs::path projectDir;                        // 项目目录
    std::string layoutName;                     // 当前窗口布局配置名称
    std::vector<ModelData> models;              // 模型列表
    std::vector<MaterialData> materials;        // 材质列表
    std::vector<TextureData> textures;          // 纹理列表
    fs::path animationDataPath;                 // 动画数据文件路径
    fs::path projectPath;                       // 项目文件路径
};


    
/**
 * @brief 项目管理器类，负责项目文件的生命周期管理
 * @note 通过依赖注入 JSONSerializer 和 EventBus 实现序列化与事件通信
 */
class ProjectManager {
public:
    ProjectManager(std::shared_ptr<JSONSerializer> jsonSerializer, std::shared_ptr<EventBus> eventBus);
    bool CreateProject(const std::string& projectName, const fs::path& projectDir);
    bool OpenProject(const fs::path& filePath, std::string& errorMsg);
    bool SaveProject(std::string& errorMsg);
    void SaveProjectAs(const fs::path& filePath);
    void DeleteModel(const std::string& modelUUID);
    const ProjectData& GetCurrentProjectData() const;
    bool IsProjectOpen() const noexcept;
    void LoadDefaultProject();

private:
    std::shared_ptr<JSONSerializer> jsonSerializer_;
    std::shared_ptr<EventBus> eventBus_;
    mutable std::mutex projectMutex_;
    std::shared_ptr<ProjectData> currentProject_;

    void ValidateProjectPath(const fs::path& path, bool requireExists = true) const;
    std::shared_ptr<ProjectData> InitializeNewProject(const std::string& projectName, const fs::path& projectDir) const;
    void SubscribeToEvents();
    ModelData CreateDefaultCubeModel(const fs::path& projectDir);
    std::string ConvertUTF8ToSystemEncoding(const std::string& utf8Str) const;
};

} // namespace MyRenderer

// JSON序列化特化定义（全局可见）
namespace nlohmann {
    
    template<> struct adl_serializer<MyRenderer::ProjectData> {
        static void to_json(json& j, const MyRenderer::ProjectData& project);
        static void from_json(const json& j, MyRenderer::ProjectData& project);
    };

    template<> struct adl_serializer<ModelData> {
        static void to_json(json& j, const ModelData& model) {
            j["uuid"] = model.uuid;
            j["filepath"] = model.filepath;
            j["transform"] = model.transform;
            j["materialUUIDs"] = model.materialUUIDs;
            j["vertexShaderPath"] = model.vertexShaderPath;
            j["fragmentShaderPath"] = model.fragmentShaderPath;
            j["vertices"] = model.vertices;
            j["indices"] = model.indices;
            j["parentUUID"] = model.parentUUID;
        }
        static void from_json(const json& j, ModelData& model) {
            j.at("uuid").get_to(model.uuid);
            j.at("filepath").get_to(model.filepath);
            j.at("transform").get_to(model.transform);
            j.at("materialUUIDs").get_to(model.materialUUIDs);
            j.at("vertexShaderPath").get_to(model.vertexShaderPath);
            j.at("fragmentShaderPath").get_to(model.fragmentShaderPath);
            j.at("vertices").get_to(model.vertices);
            j.at("indices").get_to(model.indices);
            j.at("parentUUID").get_to(model.parentUUID);
        }
    };

    template<> struct adl_serializer<MaterialData> {
        static void to_json(json& j, const MaterialData& material) {
            j["uuid"] = material.uuid;
            j["diffuseColor"] = material.diffuseColor;
            j["specularColor"] = material.specularColor;
            j["shininess"] = material.shininess;
            j["textureUUID"] = material.textureUUID;
            j["vertexShaderPath"] = material.vertexShaderPath;
            j["fragmentShaderPath"] = material.fragmentShaderPath;
        }
        static void from_json(const json& j, MaterialData& material) {
            j.at("uuid").get_to(material.uuid);
            j.at("diffuseColor").get_to(material.diffuseColor);
            j.at("specularColor").get_to(material.specularColor);
            j.at("shininess").get_to(material.shininess);
            j.at("textureUUID").get_to(material.textureUUID);
            j.at("vertexShaderPath").get_to(material.vertexShaderPath);
            j.at("fragmentShaderPath").get_to(material.fragmentShaderPath);
        }
    };

    template<> struct adl_serializer<TextureData> {
        static void to_json(json& j, const TextureData& texture) {
            j["uuid"] = texture.uuid;
            j["filepath"] = texture.filepath;
            j["width"] = texture.width;
            j["height"] = texture.height;
            j["channels"] = texture.channels;
            j["textureID"] = texture.textureID;
        }
        static void from_json(const json& j, TextureData& texture) {
            j.at("uuid").get_to(texture.uuid);
            j.at("filepath").get_to(texture.filepath);
            j.at("width").get_to(texture.width);
            j.at("height").get_to(texture.height);
            j.at("channels").get_to(texture.channels);
            j.at("textureID").get_to(texture.textureID);
        }
    };
}