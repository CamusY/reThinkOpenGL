// ProjectManager.cpp
#include "ProjectManager.h"
#include <fstream>
#include <sstream>
#include <iostream> // 添加日志输出

namespace MyRenderer {

using namespace nlohmann;

ProjectManager::ProjectManager(std::shared_ptr<JSONSerializer> jsonSerializer, std::shared_ptr<EventBus> eventBus)
    : jsonSerializer_(jsonSerializer), eventBus_(eventBus) {
    if (!jsonSerializer_) {
        throw std::invalid_argument("JSONSerializer dependency cannot be null");
    }
    if (!eventBus_) {
        throw std::invalid_argument("EventBus dependency cannot be null");
    }
    SubscribeToEvents();
    LoadDefaultProject(); // 在构造时加载默认项目
}

bool ProjectManager::CreateProject(const std::string& projectName, const fs::path& projectDir) {
    try {
        currentProject_ = InitializeNewProject(projectName, projectDir);
        fs::create_directories(projectDir / "Shaders");
        fs::path projPath = projectDir / (projectName + ".proj");
        fs::path animPath = projectDir / (projectName + ".json");
        currentProject_->projectPath = projPath;
        currentProject_->animationDataPath = animPath;

        jsonSerializer_->SerializeToFile(*currentProject_, projPath);

        std::ofstream vsFile(projectDir / "Shaders" / "default.vs");
        std::ofstream fsFile(projectDir / "Shaders" / "default.fs");
        vsFile << "#version 430 core\n"
                  "layout(location = 0) in vec3 aPos;\n"
                  "uniform mat4 model;\n"
                  "uniform mat4 view;\n"
                  "uniform mat4 projection;\n"
                  "void main() {\n"
                  "    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
                  "}\n";
        fsFile << "#version 430 core\n"
                  "out vec4 FragColor;\n"
                  "void main() {\n"
                  "    FragColor = vec4(1.0, 0.5, 0.2, 1.0);\n" // 默认橙色
                  "}\n";
        vsFile.close();
        fsFile.close();

        eventBus_->Publish(Events::ProjectOpenedEvent{projPath.string()});
        return true;
    } catch (const std::exception& e) {
        currentProject_.reset();
        eventBus_->Publish(Events::ProjectLoadFailedEvent{std::string("Failed to create project: ") + e.what()});
        return false;
    }
}

bool ProjectManager::OpenProject(const fs::path& filePath, std::string& errorMsg) {
    std::lock_guard<std::mutex> lock(projectMutex_);
    try {
        ValidateProjectPath(filePath, true);
    } catch (const std::invalid_argument& e) {
        errorMsg = e.what();
        eventBus_->Publish(Events::ProjectLoadFailedEvent{errorMsg});
        return false;
    }

    try {
        auto data = jsonSerializer_->DeserializeFromFile<ProjectData>(filePath);
        currentProject_ = std::make_shared<ProjectData>(std::move(data));
        currentProject_->projectPath = filePath;

        if (!currentProject_->animationDataPath.empty() && fs::exists(currentProject_->animationDataPath)) {
            eventBus_->Publish(Events::RequestAnimationDataLoadEvent{currentProject_->animationDataPath.string()});
        }

        eventBus_->Publish(Events::ProjectOpenedEvent{filePath.string()});
        return true;
    } catch (const JsonSerializationException& e) {
        errorMsg = std::string("Failed to parse project file: ") + e.what();
        std::cerr << "[ProjectManager] " << errorMsg << std::endl; // 添加日志输出
        eventBus_->Publish(Events::ProjectLoadFailedEvent{errorMsg});
        return false;
    } catch (const std::exception& e) {
        errorMsg = std::string("Unexpected error while opening project: ") + e.what();
        std::cerr << "[ProjectManager] " << errorMsg << std::endl; // 添加日志输出
        eventBus_->Publish(Events::ProjectLoadFailedEvent{errorMsg});
        return false;
    }
}

bool ProjectManager::SaveProject(std::string& errorMsg) {
    std::lock_guard<std::mutex> lock(projectMutex_);
    if (!currentProject_) {
        errorMsg = "No valid project to save";
        eventBus_->Publish(Events::ProjectLoadFailedEvent{errorMsg});
        return false;
    }

    if (currentProject_->projectPath.empty()) {
        errorMsg = "Project path not specified, please use Save As";
        eventBus_->Publish(Events::ProjectLoadFailedEvent{errorMsg});
        return false;
    }

    try {
        jsonSerializer_->SerializeToFile(*currentProject_, currentProject_->projectPath);
        if (!currentProject_->animationDataPath.empty()) {
            eventBus_->Publish(Events::RequestAnimationDataSaveEvent{currentProject_->animationDataPath.string()});
        }
        eventBus_->Publish(Events::ProjectSavedEvent{currentProject_->projectPath.string()});
        return true;
    } catch (const JsonSerializationException& e) {
        errorMsg = std::string("Failed to save project: ") + e.what();
        eventBus_->Publish(Events::ProjectLoadFailedEvent{errorMsg});
        return false;
    }
}

void ProjectManager::SaveProjectAs(const fs::path& filePath) {
    std::lock_guard<std::mutex> lock(projectMutex_);
    ValidateProjectPath(filePath, false);

    try {
        jsonSerializer_->SerializeToFile(*currentProject_, filePath);
        currentProject_->projectPath = filePath;
        if (!currentProject_->animationDataPath.empty()) {
            eventBus_->Publish(Events::RequestAnimationDataSaveEvent{currentProject_->animationDataPath.string()});
        }
        eventBus_->Publish(Events::ProjectSavedEvent{filePath.string()});
    } catch (const JsonSerializationException& e) {
        throw std::runtime_error(std::string("Failed to save project: ") + e.what());
    }
}

void ProjectManager::DeleteModel(const std::string& modelUUID) {
    std::lock_guard<std::mutex> lock(projectMutex_);
    if (!currentProject_) return;

    auto it = std::find_if(currentProject_->models.begin(), currentProject_->models.end(),
        [&modelUUID](const ModelData& m) { return m.uuid == modelUUID; });
    if (it != currentProject_->models.end()) {
        ModelData deletedModel = *it;

        Operation op;
        op.execute = [this, modelUUID]() {
            eventBus_->Publish(Events::ModelDeletedEvent{modelUUID});
        };
        op.undo = [this, deletedModel]() {
            currentProject_->models.push_back(deletedModel);
            eventBus_->Publish(Events::ModelLoadedEvent{deletedModel});
        };

        eventBus_->Publish(Events::PushUndoOperationEvent{op});
        currentProject_->models.erase(it);
        eventBus_->Publish(Events::ModelDeletedEvent{modelUUID});
    }
}

bool ProjectManager::IsProjectOpen() const noexcept {
    std::lock_guard<std::mutex> lock(projectMutex_);
    return currentProject_ != nullptr;
}

const ProjectData& ProjectManager::GetCurrentProjectData() const {
    std::lock_guard<std::mutex> lock(projectMutex_);
    if (!currentProject_) {
        throw std::runtime_error("No valid project data");
    }
    return *currentProject_;
}

void ProjectManager::ValidateProjectPath(const fs::path& path, bool requireExists) const {
    if (path.empty()) {
        throw std::invalid_argument("Project path cannot be empty");
    }
    if (path.extension() != ".proj") {
        throw std::invalid_argument("Project file must use .proj extension");
    }
    if (requireExists && !fs::exists(path)) {
        throw std::invalid_argument("Project file does not exist: " + path.string());
    }
}

std::shared_ptr<ProjectData> ProjectManager::InitializeNewProject(const std::string& projectName, const fs::path& projectDir) const {
    auto project = std::make_shared<ProjectData>();
    project->projectName = projectName;
    project->projectDir = projectDir;
    project->layoutName = "DefaultLayout";
    project->projectPath.clear();
    project->animationDataPath.clear();
    return project;
}

void ProjectManager::SubscribeToEvents() {
    eventBus_->Subscribe<Events::RequestNewProjectEvent>([this](const Events::RequestNewProjectEvent& e) {
        CreateProject(e.projectName, e.projectDir);
    });

    eventBus_->Subscribe<Events::RequestOpenProjectEvent>([this](const Events::RequestOpenProjectEvent& e) {
        std::string errorMsg;
        OpenProject(e.projectPath, errorMsg);
    });

    eventBus_->Subscribe<Events::RequestSaveProjectEvent>([this](const Events::RequestSaveProjectEvent& e) {
        std::string errorMsg;
        SaveProject(errorMsg);
    });

    eventBus_->Subscribe<Events::MaterialCreatedEvent>([this](const Events::MaterialCreatedEvent& e) {
        std::lock_guard<std::mutex> lock(projectMutex_);
        if (currentProject_) {
            currentProject_->materials.push_back({e.materialUUID, {}, {}, 32.0f, "", "", ""});
        }
    });

    eventBus_->Subscribe<Events::TextureLoadedEvent>([this](const Events::TextureLoadedEvent& e) {
        std::lock_guard<std::mutex> lock(projectMutex_);
        if (currentProject_ && e.success) {
            currentProject_->textures.push_back({e.uuid, e.filepath, 0, 0, 0, 0});
        }
    });
}

void ProjectManager::LoadDefaultProject() {
    fs::path defaultProjDir = "Core/defaultProj/defaultProject";
    fs::path defaultProjPath = defaultProjDir / "defaultProject.proj";

    if (!fs::exists(defaultProjDir)) {
        // 创建默认项目目录和文件
        CreateProject("defaultProject", defaultProjDir);

        // 添加默认立方体模型
        ModelData cubeModel = CreateDefaultCubeModel(defaultProjDir);
        currentProject_->models.push_back(cubeModel);
        jsonSerializer_->SerializeToFile(*currentProject_, defaultProjPath);

        // 编译默认着色器
/*        eventBus_->Publish(Events::ShaderCompileRequestEvent{
            (defaultProjDir / "Shaders" / "default.vs").string(),
            (defaultProjDir / "Shaders" / "default.fs").string()
        });
*/
        // 发布模型加载事件
        eventBus_->Publish(Events::ModelLoadedEvent{cubeModel});
    } else {
        // 如果默认项目已存在，直接打开
        std::string errorMsg;
        if (!OpenProject(defaultProjPath, errorMsg)) {
            std::cerr << "[ProjectManager] Failed to load default project: " << errorMsg << std::endl;
        }
    }
}

ModelData ProjectManager::CreateDefaultCubeModel(const fs::path& projectDir) {
    ModelData cube;
    cube.uuid = "default-cube-uuid";
    cube.filepath = "default-cube";
    cube.transform = glm::mat4(1.0f);
    cube.vertexShaderPath = (projectDir / "Shaders" / "default.vs").string();
    cube.fragmentShaderPath = (projectDir / "Shaders" / "default.fs").string();
    cube.parentUUID = "";

    // 立方体顶点数据（8个顶点）
    cube.vertices = {
        {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f},
        {-0.5f, -0.5f, 0.5f},  {0.5f, -0.5f, 0.5f},  {0.5f, 0.5f, 0.5f},  {-0.5f, 0.5f, 0.5f}
    };

    // 立方体索引数据（12个三角形，36个索引）
    cube.indices = {
        0, 1, 2, 2, 3, 0, // 前面
        1, 5, 6, 6, 2, 1, // 右面
        5, 4, 7, 7, 6, 5, // 后面
        4, 0, 3, 3, 7, 4, // 左面
        3, 2, 6, 6, 7, 3, // 上面
        4, 5, 1, 1, 0, 4  // 下面
    };

    return cube;
}

} // namespace MyRenderer

namespace nlohmann {
    void adl_serializer<MyRenderer::ProjectData>::to_json(json& j, const MyRenderer::ProjectData& project) {
        j["project_name"] = project.projectName;
        j["project_dir"] = project.projectDir.string();
        j["layout_name"] = project.layoutName;

        json modelsArray = json::array();
        for (const auto& model : project.models) {
            json modelJson;
            adl_serializer<ModelData>::to_json(modelJson, model);
            modelsArray.push_back(modelJson);
        }
        j["models"] = modelsArray;

        json materialsArray = json::array();
        for (const auto& material : project.materials) {
            json materialJson;
            adl_serializer<MaterialData>::to_json(materialJson, material);
            materialsArray.push_back(materialJson);
        }
        j["materials"] = materialsArray;

        json texturesArray = json::array();
        for (const auto& texture : project.textures) {
            json textureJson;
            adl_serializer<TextureData>::to_json(textureJson, texture);
            texturesArray.push_back(textureJson);
        }
        j["textures"] = texturesArray;

        j["animation_data_path"] = project.animationDataPath.string();
        j["project_path"] = project.projectPath.string();
    }

    void adl_serializer<MyRenderer::ProjectData>::from_json(const json& j, MyRenderer::ProjectData& project) {
        j.at("project_name").get_to(project.projectName);
        j.at("project_dir").get_to(project.projectDir);
        j.at("layout_name").get_to(project.layoutName);

        project.models.clear();
        if (j.contains("models") && j["models"].is_array()) {
            for (const auto& modelJson : j["models"]) {
                ModelData model;
                adl_serializer<ModelData>::from_json(modelJson, model);
                project.models.push_back(model);
            }
        }

        project.materials.clear();
        if (j.contains("materials") && j["materials"].is_array()) {
            for (const auto& materialJson : j["materials"]) {
                MaterialData material;
                adl_serializer<MaterialData>::from_json(materialJson, material);
                project.materials.push_back(material);
            }
        }

        project.textures.clear();
        if (j.contains("textures") && j["textures"].is_array()) {
            for (const auto& textureJson : j["textures"]) {
                TextureData texture;
                adl_serializer<TextureData>::from_json(textureJson, texture);
                project.textures.push_back(texture);
            }
        }

        j.at("animation_data_path").get_to(project.animationDataPath);
        j.at("project_path").get_to(project.projectPath);
    }
}