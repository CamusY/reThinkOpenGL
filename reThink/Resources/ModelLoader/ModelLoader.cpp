// ModelLoader.cpp
#include "ModelLoader.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <assimp/postprocess.h>
#include <glad/glad.h>

namespace fs = std::filesystem;
void ModelLoader::InitializeGLResources() {
    // 确保在主线程调用
    if (!gladLoadGL()) {
        throw std::runtime_error("Failed to initialize OpenGL loader");
    }
}

void ModelLoader::CleanupGLResources()
{
    // 遍历所有加载的模型清理资源
    for (auto& model : loadedModels_) {
        for (auto& mesh : model->meshes) {
            if (mesh.buffersInitialized) {
                glDeleteVertexArrays(1, &mesh.vao);
                glDeleteBuffers(1, &mesh.vbo);
                glDeleteBuffers(1, &mesh.ebo);
                mesh.buffersInitialized = false;
            }
        }
    }
}


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

const std::vector<std::shared_ptr<ModelData>>& ModelLoader::GetLoadedModels() const
{
    return loadedModels_;
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
        auto mesh = ProcessMesh(scene->mMeshes[i]);
        SetupMeshBuffers(mesh); // 新增资源初始化
        data->meshes.push_back(std::move(mesh));
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

void ModelLoader::SetupMeshBuffers(ModelData::Mesh& mesh) {
    // 生成缓冲区对象
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);
    // 绑定VAO
    glBindVertexArray(mesh.vao);
    // 顶点数据 (VBO)
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, 
        mesh.vertices.size() * sizeof(glm::vec3),
        mesh.vertices.data(), 
        GL_STATIC_DRAW
    );
    // 索引数据 (EBO)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        mesh.indices.size() * sizeof(uint32_t),
        mesh.indices.data(),
        GL_STATIC_DRAW
    );
    // 属性指针 (位置)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    // 法线数据（如果存在）
    if (!mesh.normals.empty()) {
        GLuint normalVBO;
        glGenBuffers(1, &normalVBO);
        glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
        glBufferData(GL_ARRAY_BUFFER,
            mesh.normals.size() * sizeof(glm::vec3),
            mesh.normals.data(),
            GL_STATIC_DRAW
        );
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    }
    // 解绑
    glBindVertexArray(0);
    mesh.buffersInitialized = true;
    ValidateGLState();
}



// 处理材质
ModelData::Material ModelLoader::ProcessMaterial(aiMaterial* mat, const std::string& basePath) {
    ModelData::Material result;

    // 漫反射颜色
    aiColor3D color;
    if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
        result.diffuseColor = {color.r, color.g, color.b};
    }

    float roughness;
    if (mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS) {
        result.roughness = roughness;
    }
    
    float metallic;
    if (mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS) {
        result.metallic = metallic;
    }

    // 漫反射贴图
    aiString path;
    if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
        result.diffuseTexture = (fs::path(basePath) / path.C_Str()).string();
    }

    return result;
}
void ModelLoader::ValidateGLState() const {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::stringstream ss;
        ss << "OpenGL错误: 0x" << std::hex << err;
        throw std::runtime_error(ss.str());
    }
}