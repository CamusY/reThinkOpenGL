// ModelLoader.cpp
#include "ModelLoader.h"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <assimp/postprocess.h>

namespace fs = std::filesystem;

// 构造函数
ModelLoader::ModelLoader(
    std::shared_ptr<ThreadPool> threadPool,
    std::shared_ptr<EventBus> eventBus
) : threadPool_(threadPool), eventBus_(eventBus) {
    if (!threadPool_ || !eventBus_) {
        throw std::invalid_argument("ModelLoader: 依赖项不能为空");
    }
}

// 异步加载入口
void ModelLoader::LoadAsync(const std::string& filePath, const std::string& modelUUID) {
    // 防御性检查
    if (!fs::exists(filePath)) {
        throw std::invalid_argument("文件不存在: " + filePath);
    }
    if (modelUUID.empty()) {
        throw std::invalid_argument("modelUUID 不能为空");
    }

    // 提交到线程池
    threadPool_->Enqueue([=]() {
        try {
            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(
                filePath,
                aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs
            );

            if (!scene || !scene->mRootNode) {
                throw std::runtime_error("模型加载失败: " + std::string(importer.GetErrorString()));
            }

            auto modelData = ProcessScene(scene, filePath, modelUUID);
            
            // 发布成功事件
            EventTypes::ModelLoadedEvent event;
            event.modelUUID = modelUUID;
            event.filePath = filePath;
            event.data = modelData;
            eventBus_->Publish(event);
        } catch (const std::exception& e) {
            // 发布失败事件
            EventTypes::ModelLoadFailedEvent event;
            event.modelUUID = modelUUID;
            event.filePath = filePath;
            event.errorMessage = e.what();
            eventBus_->Publish(event);
        }
    });
}

// 处理场景数据
std::shared_ptr<ModelData> ModelLoader::ProcessScene(
    const aiScene* scene,
    const std::string& filePath,
    const std::string& modelUUID
) {
    auto data = std::make_shared<ModelData>();
    data->modelUUID = modelUUID;
    data->filePath = filePath;
    const fs::path basePath = fs::path(filePath).parent_path();

    // 处理网格
    for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
        data->meshes.push_back(ProcessMesh(scene->mMeshes[i]));
    }

    // 处理材质
    for (unsigned i = 0; i < scene->mNumMaterials; ++i) {
        std::string matName = "Material_" + std::to_string(i);
        data->materials[matName] = ProcessMaterial(scene->mMaterials[i], basePath.string());
    }

    return data;
}

// 处理单个网格
ModelData::Mesh ModelLoader::ProcessMesh(aiMesh* mesh) {
    ModelData::Mesh result;

    // 顶点数据
    for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
        result.vertices.emplace_back(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        );

        if (mesh->HasNormals()) {
            result.normals.emplace_back(
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            );
        }

        if (mesh->mTextureCoords[0]) {
            result.texCoords.emplace_back(
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            );
        }
    }

    // 索引数据
    for (unsigned i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned j = 0; j < face.mNumIndices; ++j) {
            result.indices.push_back(face.mIndices[j]);
        }
    }

    return result;
}

// 处理材质
ModelData::Material ModelLoader::ProcessMaterial(aiMaterial* mat, const std::string& basePath) {
    ModelData::Material result;

    // 漫反射颜色
    aiColor3D color;
    if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
        result.diffuseColor = {color.r, color.g, color.b};
    }

    // 漫反射贴图
    aiString path;
    if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
        result.diffuseTexture = (fs::path(basePath) / path.C_Str()).string();
    }

    return result;
}