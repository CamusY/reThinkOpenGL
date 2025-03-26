// ModelLoader.h
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include "ThreadPool/ThreadPool.h"
#include "EventBus/EventBus.h"
#include "EventBus/EventTypes.h"

// 模型数据结构定义
struct ModelData {
    struct Mesh {
        // OpenGL资源标识符
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ebo = 0;
        
        // 原始数据
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texCoords;
        std::vector<uint32_t> indices;
        std::string materialName;

        // 资源状态标记
        bool buffersInitialized = false;
    };

    struct Material {
        glm::vec3 diffuseColor;
        std::string diffuseTexture;
        float roughness = 0.5f;
        float metallic = 0.0f;
    };

    std::vector<Mesh> meshes;
    std::unordered_map<std::string, Material> materials;
    std::string modelUUID;
    std::string filePath;
};


/**
 * @brief 模型加载器，封装 Assimp 实现多线程异步加载
 * @dependency 强制依赖 ThreadPool 和 EventBus
 */
class ModelLoader {
public:
    
    void InitializeGLResources();
    void CleanupGLResources();
    /**
     * @brief 构造函数注入依赖
     * @param threadPool 线程池实例（必须非空）
     * @param eventBus 事件总线实例（必须非空）
     * @throws std::invalid_argument 参数为空时抛出
     */
    ModelLoader(
        std::shared_ptr<ThreadPool> threadPool,
        std::shared_ptr<EventBus> eventBus
    );

    /**
     * @brief 异步加载模型文件
     * @param filePath 模型文件路径（需有效）
     * @param modelUUID 模型唯一标识符
     * @throws std::invalid_argument 路径无效时抛出
     */
    void LoadAsync(const std::string& filePath, const std::string& modelUUID);

    const std::vector<std::shared_ptr<ModelData>>& GetLoadedModels() const;

    std::shared_ptr<ModelData> ProcessScene(
       const aiScene* scene,
       const std::string& filePath,
       const std::string& modelUUID
   );


private:
    std::shared_ptr<ThreadPool> threadPool_;
    std::shared_ptr<EventBus> eventBus_;
    std::vector<std::shared_ptr<ModelData>> loadedModels_;

    void SetupMeshBuffers(ModelData::Mesh& mesh);
    void ValidateGLState() const;
    ModelData::Mesh ProcessMesh(aiMesh* mesh);
    ModelData::Material ProcessMaterial(aiMaterial* mat, const std::string& basePath);
};