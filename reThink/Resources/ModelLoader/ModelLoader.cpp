#include "ModelLoader.h"
#include <stdexcept>
#include <sstream>
#include <random>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include "MaterialManager/MaterialManager.h"

ModelLoader::ModelLoader(std::shared_ptr<EventBus> eventBus, std::shared_ptr<ThreadPool> threadPool, std::shared_ptr<MaterialManager> materialManager)
    : eventBus_(std::move(eventBus)), threadPool_(std::move(threadPool)), materialManager_(std::move(materialManager)) {
    if (!eventBus_) throw std::invalid_argument("ModelLoader: EventBus 不能为空");
    if (!threadPool_) throw std::invalid_argument("ModelLoader: ThreadPool 不能为空");
    if (!materialManager_) throw std::invalid_argument("ModelLoader: MaterialManager 不能为空");
    threadPool_->SetErrorCallback([this](const std::string& errorMsg) {
        std::cerr << errorMsg << std::endl;
    });
    // 订阅层级更新事件
    eventBus_->Subscribe<MyRenderer::Events::HierarchyUpdateEvent>(
        [this](const MyRenderer::Events::HierarchyUpdateEvent& event) {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto& [uuid, model] : loadedModels_) {
                if (model.parentUUID == event.parentUUID) {
                    model.transform = event.transform * model.transform; // 应用父变换
                    eventBus_->Publish(MyRenderer::Events::ModelTransformedEvent{uuid, model.transform});
                }
            }
        });
}

ModelLoader::~ModelLoader() {
    std::lock_guard<std::mutex> lock(mutex_);
    loadedModels_.clear();
}

std::future<ModelData> ModelLoader::LoadModelAsync(const std::string& filepath, int priority) {
    if (filepath.empty()) throw std::invalid_argument("ModelLoader: 文件路径不能为空");
    return threadPool_->EnqueueTask([this, filepath]() -> ModelData {
        const aiScene* scene = importer_.ReadFile(filepath,
            aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            throw std::runtime_error("ModelLoader: 无法加载模型 '" + filepath + "': " + importer_.GetErrorString());
        }
        ModelData modelData = ProcessScene(filepath, scene);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            loadedModels_[modelData.uuid] = modelData;
        }
        eventBus_->Publish(MyRenderer::Events::ModelLoadedEvent{modelData});
        return modelData;
    }, priority);
}

void ModelLoader::DeleteModel(const std::string& modelUUID) {
    if (modelUUID.empty()) throw std::invalid_argument("ModelLoader: 模型 UUID 不能为空");

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = loadedModels_.find(modelUUID);
    if (it != loadedModels_.end()) {
        eventBus_->Publish(MyRenderer::Events::ModelDeletedEvent{modelUUID});
        loadedModels_.erase(it);
    }
}

ModelData ModelLoader::GetModelData(const std::string& modelUUID) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = loadedModels_.find(modelUUID);
    return (it != loadedModels_.end()) ? it->second : ModelData{};
}

ModelData ModelLoader::ProcessScene(const std::string& filepath, const aiScene* scene) {
    ModelData modelData;
    modelData.uuid = GenerateUUID();
    modelData.filepath = filepath;
    modelData.transform = glm::mat4(1.0f);
    modelData.vertexShaderPath = "";
    modelData.fragmentShaderPath = "";
    modelData.parentUUID = "";

    ProcessNode(scene->mRootNode, scene, modelData, "");
    return modelData;
}

void ModelLoader::ProcessNode(const aiNode* node, const aiScene* scene, ModelData& modelData, const std::string& parentUUID) {
    aiMatrix4x4 aiTransform = node->mTransformation;
    glm::mat4 transform(
        aiTransform.a1, aiTransform.b1, aiTransform.c1, aiTransform.d1,
        aiTransform.a2, aiTransform.b2, aiTransform.c2, aiTransform.d2,
        aiTransform.a3, aiTransform.b3, aiTransform.c3, aiTransform.d3,
        aiTransform.a4, aiTransform.b4, aiTransform.c4, aiTransform.d4
    );
    modelData.transform = transform;
    modelData.parentUUID = parentUUID;

    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        ProcessMesh(scene->mMeshes[node->mMeshes[i]], modelData, scene);
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        ModelData childModelData;
        childModelData.uuid = GenerateUUID();
        childModelData.filepath = modelData.filepath;
        childModelData.vertexShaderPath = "";
        childModelData.fragmentShaderPath = "";
        ProcessNode(node->mChildren[i], scene, childModelData, modelData.uuid);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            loadedModels_[childModelData.uuid] = childModelData;
        }
        eventBus_->Publish(MyRenderer::Events::ModelLoadedEvent{childModelData});
    }
}

void ModelLoader::ProcessMesh(const aiMesh* mesh, ModelData& modelData, const aiScene* scene) {
    // 加载顶点和法线数据
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        glm::vec3 vertex(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        modelData.vertices.push_back(vertex);
        if (mesh->HasNormals()) {
            glm::vec3 normal(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            modelData.normals.push_back(normal);
        } else {
            // 如果没有法线，可以选择生成默认值或抛出警告
            modelData.normals.push_back(glm::vec3(0.0f));
        }
    }

    // 加载索引数据
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            modelData.indices.push_back(face.mIndices[j]);
        }
    }

    // 处理材质
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* aiMat = scene->mMaterials[mesh->mMaterialIndex];
        aiColor3D diffuse;
        aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
        aiColor3D specular;
        aiMat->Get(AI_MATKEY_COLOR_SPECULAR, specular);
        float shininess;
        aiMat->Get(AI_MATKEY_SHININESS, shininess);

        aiString texturePath;
        std::string filepath = (aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) ? texturePath.C_Str() : "";

        // 委托给 MaterialManager 创建材质
        std::string materialUUID = materialManager_->LoadMaterial(
            glm::vec3(diffuse.r, diffuse.g, diffuse.b),
            glm::vec3(specular.r, specular.g, specular.b),
            shininess,
            filepath
        );
        modelData.materialUUIDs.push_back(materialUUID);
    }
}

std::string ModelLoader::GenerateUUID() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);

    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::stringstream ss;
    ss << std::hex << now << dis(gen) << dis2(gen);
    return ss.str();
}